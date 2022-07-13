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
#include <cstddef>
#include <list>
#include <memory>
#include <string>

#include "OgreException.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramManager.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgreMaterial.hpp"
#include "OgreMaterialSerializer.hpp"
#include "OgrePass.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreShaderExGBuffer.hpp"
#include "OgreShaderFFPRenderState.hpp"
#include "OgreShaderFunction.hpp"
#include "OgreShaderFunctionAtom.hpp"
#include "OgreShaderGenerator.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrecompiledHeaders.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderProgram.hpp"
#include "OgreShaderProgramSet.hpp"
#include "OgreShaderScriptTranslator.hpp"
#include "OgreTechnique.hpp"

namespace Ogre::RTShader {
        class RenderState;
    }  // namespace Ogre

namespace Ogre::RTShader
{

/************************************************************************/
/*                                                                      */
/************************************************************************/
String GBuffer::Type = "GBuffer";

//-----------------------------------------------------------------------
auto GBuffer::getType() const noexcept -> std::string_view { return Type; }

//-----------------------------------------------------------------------
auto GBuffer::getExecutionOrder() const noexcept -> FFPShaderStage { return FFPShaderStage::LIGHTING; }

auto GBuffer::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept -> bool
{
    srcPass->getParent()->getParent()->setReceiveShadows(false);
    return true;
}

//-----------------------------------------------------------------------
auto GBuffer::createCpuSubPrograms(ProgramSet* programSet) -> bool
{
    Function* psMain = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM)->getMain();

    for(size_t i = 0; i < mOutBuffers.size(); i++)
    {
        auto out =
            psMain->resolveOutputParameter(i == 0 ? Parameter::Content::COLOR_DIFFUSE : Parameter::Content::COLOR_SPECULAR);

        switch(mOutBuffers[i])
        {
        case TargetLayout::DEPTH:
            addDepthInvocations(programSet, out);
            break;
        case TargetLayout::NORMAL_VIEWDEPTH:
            addViewPosInvocations(programSet, out, true);
            [[fallthrough]];
        case TargetLayout::NORMAL:
            addNormalInvocations(programSet, out);
            break;
        case TargetLayout::VIEWPOS:
            addViewPosInvocations(programSet, out, false);
            break;
        case TargetLayout::DIFFUSE_SPECULAR:
            addDiffuseSpecularInvocations(programSet, out);
            break;
        default:
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "unsupported TargetLayout");
        }
    }

    return true;
}

void GBuffer::addViewPosInvocations(ProgramSet* programSet, const ParameterPtr& out, bool depthOnly)
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* vsMain = vsProgram->getMain();
    Function* psMain = psProgram->getMain();

    // vertex shader
    auto vstage = vsMain->getStage(std::to_underlying(FFPVertexShaderStage::POST_PROCESS));
    auto vsInPosition = vsMain->resolveInputParameter(Parameter::Content::POSITION_OBJECT_SPACE);
    auto vsOutPos = vsMain->resolveOutputParameter(Parameter::Content::POSITION_VIEW_SPACE);
    auto worldViewMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::WORLDVIEW_MATRIX);
    vstage.callFunction(FFP_FUNC_TRANSFORM, worldViewMatrix, vsInPosition, vsOutPos);

    // fragment shader
    auto fstage = psMain->getStage(std::to_underlying(FFPFragmentShaderStage::COLOUR_END));
    auto viewPos = psMain->resolveInputParameter(vsOutPos);

    if(depthOnly)
    {
        auto far = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::FAR_CLIP_DISTANCE);
        fstage.callFunction("FFP_Length", viewPos, Out(out).w());
        fstage.div(In(out).w(), far, Out(out).w());
        return;
    }

    fstage.assign(viewPos, Out(out).xyz());
    fstage.assign(0, Out(out).w());
}

void GBuffer::addDepthInvocations(ProgramSet* programSet, const ParameterPtr& out)
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* vsMain = vsProgram->getMain();
    Function* psMain = psProgram->getMain();

    // vertex shader
    auto vsOutPos = vsMain->resolveOutputParameter(Parameter::Content::POSITION_PROJECTIVE_SPACE);

    bool isD3D9 = ShaderGenerator::getSingleton().getTargetLanguage() == "hlsl" &&
                  !GpuProgramManager::getSingleton().isSyntaxSupported("vs_4_0_level_9_1");

    if (isD3D9)
    {
        auto vstage = vsMain->getStage(std::to_underlying(FFPVertexShaderStage::POST_PROCESS));
        auto vsPos = vsMain->resolveOutputParameter(Parameter::Content::UNKNOWN, GpuConstantType::FLOAT4);
        vstage.assign(vsOutPos, vsPos);
        std::swap(vsOutPos, vsPos);
    }

    // fragment shader
    auto fstage = psMain->getStage(std::to_underlying(FFPFragmentShaderStage::COLOUR_END));
    auto viewPos = psMain->resolveInputParameter(vsOutPos);

    fstage.assign(In(viewPos).z(), Out(out).x());

    if (isD3D9)
    {
        fstage.div(In(out).x(), In(viewPos).w(), Out(out).x());
    }
}

void GBuffer::addNormalInvocations(ProgramSet* programSet, const ParameterPtr& out)
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Function* vsMain = vsProgram->getMain();
    Function* psMain = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM)->getMain();

    auto fstage = psMain->getStage(std::to_underlying(FFPFragmentShaderStage::COLOUR_END));
    auto viewNormal = psMain->getLocalParameter(Parameter::Content::NORMAL_VIEW_SPACE);
    if(!viewNormal)
    {
        // compute vertex shader normal
        auto vstage = vsMain->getStage(std::to_underlying(FFPVertexShaderStage::LIGHTING));
        auto vsInNormal = vsMain->resolveInputParameter(Parameter::Content::NORMAL_OBJECT_SPACE);
        auto vsOutNormal = vsMain->resolveOutputParameter(Parameter::Content::NORMAL_VIEW_SPACE);
        auto worldViewITMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::NORMAL_MATRIX);
        vstage.callFunction(FFP_FUNC_TRANSFORM, worldViewITMatrix, vsInNormal, vsOutNormal);
        vstage.callFunction(FFP_FUNC_NORMALIZE, vsOutNormal);

        // pass through
        viewNormal = psMain->resolveInputParameter(vsOutNormal);
    }
    fstage.assign(viewNormal, Out(out).xyz());
}

void GBuffer::addDiffuseSpecularInvocations(ProgramSet* programSet, const ParameterPtr& out)
{
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* psMain = psProgram->getMain();

    // set diffuse - TODO vertex color tracking
    auto diffuse = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::SURFACE_DIFFUSE_COLOUR);
    psMain->getStage(std::to_underlying(FFPFragmentShaderStage::COLOUR_BEGIN) + 1).assign(diffuse, out);

    // set shininess
    auto surfaceShininess = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::SURFACE_SHININESS);
    psMain->getStage(std::to_underlying(FFPFragmentShaderStage::COLOUR_END)).assign(surfaceShininess, Out(out).w());
}

//-----------------------------------------------------------------------
void GBuffer::copyFrom(const SubRenderState& rhs)
{
    const auto& rhsTransform = static_cast<const GBuffer&>(rhs);
    mOutBuffers = rhsTransform.mOutBuffers;
}

//-----------------------------------------------------------------------
auto GBufferFactory::getType() const noexcept -> std::string_view { return GBuffer::Type; }

static auto translate(std::string_view val) -> GBuffer::TargetLayout
{
    if(val == "depth")
        return GBuffer::TargetLayout::DEPTH;
    if(val == "normal")
        return GBuffer::TargetLayout::NORMAL;
    if(val == "viewpos")
        return GBuffer::TargetLayout::VIEWPOS;
    if(val == "normal_viewdepth")
        return GBuffer::TargetLayout::NORMAL_VIEWDEPTH;
    return GBuffer::TargetLayout::DIFFUSE_SPECULAR;
}

//-----------------------------------------------------------------------
auto GBufferFactory::createInstance(ScriptCompiler* compiler, PropertyAbstractNode* prop, Pass* pass,
                                               SGScriptTranslator* translator) noexcept -> SubRenderState*
{
    if (prop->name != "lighting_stage" || prop->values.size() < 2)
        return nullptr;

    auto it = prop->values.begin();
    String val;

    if(!SGScriptTranslator::getString(*it++, &val))
    {
        compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
        return nullptr;
    }

    if (val != "gbuffer")
        return nullptr;

    GBuffer::TargetBuffers targets;

    if(!SGScriptTranslator::getString(*it++, &val))
    {
        compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
        return nullptr;
    }
    targets.push_back(translate(val));

    if(it != prop->values.end())
    {
        if(!SGScriptTranslator::getString(*it++, &val))
        {
            compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
            return nullptr;
        }

        targets.push_back(translate(val));
    }

    auto ret = static_cast<GBuffer*>(createOrRetrieveInstance(translator));
    ret->setOutBuffers(targets);
    return ret;
}

//-----------------------------------------------------------------------
void GBufferFactory::writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, Pass* srcPass,
                                   Pass* dstPass)
{
    ser->writeAttribute(4, "lighting_stage");
    ser->writeValue("gbuffer");
    ser->writeValue("depth");
}

//-----------------------------------------------------------------------
auto GBufferFactory::createInstanceImpl() -> SubRenderState* { return new GBuffer; }

} // namespace Ogre
