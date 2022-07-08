/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreException.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreLogManager.hpp"
#include "OgreMaterial.hpp"
#include "OgrePass.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderFFPColour.hpp"
#include "OgreShaderFFPLighting.hpp"
#include "OgreShaderFFPRenderState.hpp"
#include "OgreShaderGenerator.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderProgram.hpp"
#include "OgreShaderProgramManager.hpp"
#include "OgreShaderProgramSet.hpp"
#include "OgreShaderRenderState.hpp"
#include "OgreShaderSubRenderState.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreString.hpp"
#include "OgreStringVector.hpp"
#include "OgreTechnique.hpp"
#include "OgreVector.hpp"

namespace Ogre {
class AutoParamDataSource;
class Renderable;

namespace RTShader {

const char* TargetRenderState::UserKey = "TargetRenderState";

//-----------------------------------------------------------------------
RenderState::RenderState()
{
    mLightCountAutoUpdate    = true;    
    mLightCount[0]           = 0;
    mLightCount[1]           = 0;
    mLightCount[2]           = 0;   
}

//-----------------------------------------------------------------------
RenderState::~RenderState()
{
    reset();
}

//-----------------------------------------------------------------------
void RenderState::reset()
{
    for (auto & it : mSubRenderStateList)
    {
        ShaderGenerator::getSingleton().destroySubRenderState(it);
    }
    mSubRenderStateList.clear();
}

//-----------------------------------------------------------------------
void RenderState::setLightCount(const Vector3i& lightCount)
{
    mLightCount = lightCount;
}

//-----------------------------------------------------------------------
auto RenderState::getLightCount() const noexcept -> const Vector3i&
{
    return mLightCount;
}

//-----------------------------------------------------------------------
void RenderState::addTemplateSubRenderState(SubRenderState* subRenderState)
{
    bool addSubRenderState = true;

    // Go over the current sub render state.
    for (auto & it : mSubRenderStateList)
    {
        // Case the same instance already exists -> do not add to list.
        if (it == subRenderState)
        {
            addSubRenderState = false;
            break;
        }

        // Case it is different sub render state instance with the same type, use the new sub render state
        // instead of the previous sub render state. This scenario is usually caused by material inheritance, so we use the derived material sub render state
        // and destroy the base sub render state.
        else if (it->getType() == subRenderState->getType())
        {
            removeSubRenderState(it);
            break;
        }
    }

    // Add only if this sub render state instance is not on the list.
    if (addSubRenderState)
    {
        mSubRenderStateList.push_back(subRenderState);  
    }   
}

//-----------------------------------------------------------------------
void RenderState::removeSubRenderState(SubRenderState* subRenderState)
{
    auto it = std::ranges::find(mSubRenderStateList, subRenderState);
    if(it == mSubRenderStateList.end()) return;

    mSubRenderStateList.erase(it);
    ShaderGenerator::getSingleton().destroySubRenderState(subRenderState);
}

//-----------------------------------------------------------------------
TargetRenderState::TargetRenderState()  = default;

TargetRenderState::~TargetRenderState()
{
    if(mParent)
        releasePrograms(mParent);
}

//-----------------------------------------------------------------------
void TargetRenderState::addSubRenderStateInstance(SubRenderState* subRenderState)
{
    mSubRenderStateList.push_back(subRenderState);
    mSubRenderStateSortValid = false;
}

//-----------------------------------------------------------------------------
void TargetRenderState::bindUniformParameters(Program* pCpuProgram, const GpuProgramParametersSharedPtr& passParams)
{
    // samplers are bound via registers in HLSL & Cg
    bool samplersBound = ShaderGenerator::getSingleton().getTargetLanguage()[0] != 'g';

    // Bind each uniform parameter to its GPU parameter.
    for (const auto& param : pCpuProgram->getParameters())
    {
        if((samplersBound && param->isSampler()) || !param->isUsed()) continue;

        param->bind(passParams);
        param->setUsed(false); // reset for shader regen
    }
}

void TargetRenderState::acquirePrograms(Pass* pass)
{
    createCpuPrograms();
    ProgramManager::getSingleton().createGpuPrograms(mProgramSet.get());

    bool hasError = false;
    bool logProgramNames = !ShaderGenerator::getSingleton().getShaderCachePath().empty();
    const char* matName = pass->getParent()->getParent()->getName().data();

    for(auto type : {GPT_VERTEX_PROGRAM, GPT_FRAGMENT_PROGRAM})
    {
        auto prog = mProgramSet->getGpuProgram(type);
        hasError = hasError || prog->hasCompileError();
        if (logProgramNames)
        {
            LogManager::getSingleton().logMessage(std::format(
                "RTSS: using {} for Pass {} of '{}'", prog->getName(), pass->getIndex(), matName));
        }

        // Bind the created GPU programs to the target pass.
        pass->setGpuProgram(type, prog);
        // Bind uniform parameters to pass parameters.
        bindUniformParameters(mProgramSet->getCpuProgram(type), pass->getGpuProgramParameters(type));
    }

    if (hasError)
    {
        LogManager::getSingleton().logError(
            std::format("RTSS: failed to create GpuPrograms for Pass {} of '{}'", pass->getIndex(), matName));
    }

    mParent = pass;
}


void TargetRenderState::releasePrograms(Pass* pass)
{
    if(!mProgramSet)
        return;

    pass->setGpuProgram(GPT_VERTEX_PROGRAM, GpuProgramPtr());
    pass->setGpuProgram(GPT_FRAGMENT_PROGRAM, GpuProgramPtr());

    ProgramManager::getSingleton().releasePrograms(mProgramSet.get());

    mProgramSet.reset();
}

/// we cannot resolve this at preAddToRenderState time as addition order is arbitrary
static void fixupFFPLighting(TargetRenderState* renderState)
{
    const SubRenderStateList& subRenderStateList = renderState->getSubRenderStates();
    auto it = std::ranges::find_if(subRenderStateList,
                           [](const SubRenderState* e) { return e->getType() == FFPLighting::Type; });

    if (it == subRenderStateList.end())
        return;

    auto ffpLighting = static_cast<FFPLighting*>(*it);

    it = std::ranges::find_if(subRenderStateList,
                      [](const SubRenderState* e) { return e->getType() == FFPColour::Type; });

    OgreAssert(it != subRenderStateList.end(), "FFPColour required");

    auto ffpColour = static_cast<FFPColour*>(*it);
    ffpColour->addResolveStageMask(FFPColour::SF_VS_OUTPUT_DIFFUSE);

    if(ffpLighting->getSpecularEnable())
        ffpColour->addResolveStageMask(FFPColour::SF_VS_OUTPUT_SPECULAR);
}

//-----------------------------------------------------------------------
void TargetRenderState::createCpuPrograms()
{
    sortSubRenderStates();

    fixupFFPLighting(this);

    ProgramSet* programSet = createProgramSet();
    programSet->setCpuProgram(std::unique_ptr<Program>(new Program(GPT_VERTEX_PROGRAM)));
    programSet->setCpuProgram(std::unique_ptr<Program>(new Program(GPT_FRAGMENT_PROGRAM)));

    for (auto srcSubRenderState : mSubRenderStateList)
    {
        if (!srcSubRenderState->createCpuSubPrograms(programSet))
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        ::std::format("Could not generate sub render program of type: {}", srcSubRenderState->getType()));
        }
    }
}

//-----------------------------------------------------------------------
auto TargetRenderState::createProgramSet() -> ProgramSet*
{
    mProgramSet = std::make_unique<ProgramSet>();

    return mProgramSet.get();
}

//-----------------------------------------------------------------------
void TargetRenderState::updateGpuProgramsParams(Renderable* rend, const Pass* pass, const AutoParamDataSource* source,
                                                const LightList* pLightList)
{
    for (auto curSubRenderState : mSubRenderStateList)
    {
        curSubRenderState->updateGpuProgramsParams(rend, pass, source, pLightList);     
    }
}

void TargetRenderState::link(const StringVector& srsTypes, Pass* srcPass, Pass* dstPass)
{
    for (const auto& srsType : srsTypes)
    {
        auto srs = ShaderGenerator::getSingleton().createSubRenderState(srsType);

        if (srs->preAddToRenderState(this, srcPass, dstPass))
        {
            addSubRenderStateInstance(srs);
        }
        else
        {
            ShaderGenerator::getSingleton().destroySubRenderState(srs);
        }
    }
}

//-----------------------------------------------------------------------
void TargetRenderState::link(const RenderState& templateRS, Pass* srcPass, Pass* dstPass)
{
    for (auto srcSubRenderState : templateRS.getSubRenderStates())
    {
        auto it = mSubRenderStateList.end();
        switch (srcSubRenderState->getExecutionOrder())
        {
        case FFP_TRANSFORM:
        case FFP_COLOUR:
        case FFP_LIGHTING:
        case FFP_TEXTURING:
        case FFP_FOG:
            // Check if this FFP stage already exists.
            it = std::ranges::find_if(mSubRenderStateList,
                              [srcSubRenderState](const SubRenderState* e) {
                                  return srcSubRenderState->getExecutionOrder() == e->getExecutionOrder();
                              });
        default:
            break;
        }

        if(it != mSubRenderStateList.end())
            continue;

        // Check if this type of sub render state already exists.
        it = std::ranges::find_if(mSubRenderStateList,
                          [srcSubRenderState](const SubRenderState* e) {
                              return srcSubRenderState->getType() == e->getType();
                          });

        if(it != mSubRenderStateList.end())
            continue;

        // Case custom sub render state not exits -> add it to custom list.
        auto newSubRenderState = ShaderGenerator::getSingleton().createSubRenderState(srcSubRenderState->getType());
        *newSubRenderState = *srcSubRenderState;

        if (newSubRenderState->preAddToRenderState(this, srcPass, dstPass))
        {
            addSubRenderStateInstance(newSubRenderState);
        }
        else
        {
            ShaderGenerator::getSingleton().destroySubRenderState(newSubRenderState);
        }
    }
}

namespace {
    struct CmpSubRenderStates {
        auto operator()(const SubRenderState* a, const SubRenderState* b) const -> bool
        {
            return a->getExecutionOrder() < b->getExecutionOrder();
        }
    };
}

//-----------------------------------------------------------------------
void TargetRenderState::sortSubRenderStates()
{
    if (mSubRenderStateSortValid == false)
    {
        std::ranges::sort(mSubRenderStateList, CmpSubRenderStates());
        mSubRenderStateSortValid = true;
    }
}

}
}
