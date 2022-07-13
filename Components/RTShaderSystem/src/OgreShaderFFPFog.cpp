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
#include <string>

#include "OgreCommon.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgreMaterialSerializer.hpp"
#include "OgrePass.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSceneManager.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreShaderFFPFog.hpp"
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

namespace Ogre::RTShader {
        class RenderState;
    }  // namespace Ogre

namespace Ogre::RTShader {

/************************************************************************/
/*                                                                      */
/************************************************************************/
String FFPFog::Type = "FFP_Fog";

//-----------------------------------------------------------------------
FFPFog::FFPFog()
{
    mFogMode                = FogMode::NONE;
    mCalcMode               = CalcMode::PER_VERTEX;
}

//-----------------------------------------------------------------------
auto FFPFog::getType() const noexcept -> std::string_view
{
    return Type;
}

//-----------------------------------------------------------------------
auto FFPFog::getExecutionOrder() const noexcept -> FFPShaderStage
{
    return FFPShaderStage::FOG;
}

//-----------------------------------------------------------------------
auto FFPFog::resolveParameters(ProgramSet* programSet) -> bool
{
    if (mFogMode == FogMode::NONE)
        return true;

    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* vsMain = vsProgram->getEntryPointFunction();
    Function* psMain = psProgram->getEntryPointFunction();
    
    // Resolve vertex shader output position.
    mVSOutPos = vsMain->resolveOutputParameter(Parameter::Content::POSITION_PROJECTIVE_SPACE);
    
    // Resolve fog colour.
    mFogColour = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::FOG_COLOUR);
    
    // Resolve pixel shader output diffuse color.
    mPSOutDiffuse = psMain->resolveOutputParameter(Parameter::Content::COLOR_DIFFUSE);
    
    // Per pixel fog.
    if (mCalcMode == CalcMode::PER_PIXEL)
    {
        // Resolve fog params.      
        mFogParams = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::FOG_PARAMS);
        
        // Resolve vertex shader output depth.      
        mVSOutDepth = vsMain->resolveOutputParameter(Parameter::Content::DEPTH_VIEW_SPACE);
        
        // Resolve pixel shader input depth.
        mPSInDepth = psMain->resolveInputParameter(mVSOutDepth);
    }
    // Per vertex fog.
    else
    {       
        // Resolve fog params.      
        mFogParams = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::FOG_PARAMS);
        
        // Resolve vertex shader output fog factor.
        mVSOutFogFactor = vsMain->resolveOutputParameter(Parameter::Content::UNKNOWN, GpuConstantType::FLOAT1);

        // Resolve pixel shader input fog factor.
        mPSInFogFactor = psMain->resolveInputParameter(mVSOutFogFactor);
    }

    return true;
}

//-----------------------------------------------------------------------
auto FFPFog::resolveDependencies(ProgramSet* programSet) -> bool
{
    if (mFogMode == FogMode::NONE)
        return true;

    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);

    vsProgram->addDependency(FFP_LIB_FOG);
    psProgram->addDependency(FFP_LIB_COMMON);

    // Per pixel fog.
    if (mCalcMode == CalcMode::PER_PIXEL)
    {
        psProgram->addDependency(FFP_LIB_FOG);

    }   

    return true;
}

//-----------------------------------------------------------------------
auto FFPFog::addFunctionInvocations(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* vsMain = vsProgram->getEntryPointFunction();
    Function* psMain = psProgram->getEntryPointFunction();

    const char* fogfunc = nullptr;
    
    // Per pixel fog.
    if (mCalcMode == CalcMode::PER_PIXEL)
    {
        vsMain->getStage(std::to_underlying(FFPVertexShaderStage::FOG)).assign(In(mVSOutPos).w(), mVSOutDepth);

        switch (mFogMode)
        {
            using enum FogMode;
        case LINEAR:
            fogfunc = FFP_FUNC_PIXELFOG_LINEAR;
            break;
        case EXP:
            fogfunc = FFP_FUNC_PIXELFOG_EXP;
            break;
        case EXP2:
            fogfunc = FFP_FUNC_PIXELFOG_EXP2;
            break;
        case NONE:
            return true;
        }
        psMain->getStage(std::to_underlying(FFPFragmentShaderStage::FOG))
            .callFunction(fogfunc,
                          {In(mPSInDepth), In(mFogParams), In(mFogColour), In(mPSOutDiffuse), Out(mPSOutDiffuse)});
    }

    // Per vertex fog.
    else
    {
        using enum FogMode;
        switch (mFogMode)
        {
        case LINEAR:
            fogfunc = FFP_FUNC_VERTEXFOG_LINEAR;
            break;
        case EXP:
            fogfunc = FFP_FUNC_VERTEXFOG_EXP;
            break;
        case EXP2:
            fogfunc = FFP_FUNC_VERTEXFOG_EXP2;
            break;
        case NONE:
            return true;
        }

        //! [func_invoc]
        auto vsFogStage = vsMain->getStage(std::to_underlying(FFPVertexShaderStage::FOG));
        vsFogStage.callFunction(fogfunc, mVSOutPos, mFogParams, mVSOutFogFactor);
        //! [func_invoc]
        psMain->getStage(std::to_underlying(FFPVertexShaderStage::FOG))
            .callFunction(FFP_FUNC_LERP, {In(mFogColour), In(mPSOutDiffuse), In(mPSInFogFactor), Out(mPSOutDiffuse)});
    }



    return true;
}

//-----------------------------------------------------------------------
void FFPFog::copyFrom(const SubRenderState& rhs)
{
    const auto& rhsFog = static_cast<const FFPFog&>(rhs);

    mFogMode            = rhsFog.mFogMode;

    setCalcMode(rhsFog.mCalcMode);
}

//-----------------------------------------------------------------------
auto FFPFog::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept -> bool
{
    if (srcPass->getFogOverride())
    {
        mFogMode         = srcPass->getFogMode();
    }
    else if(SceneManager* sceneMgr = ShaderGenerator::getSingleton().getActiveSceneManager())
    {
        mFogMode         = sceneMgr->getFogMode();
    }

    return mFogMode != FogMode::NONE;
}

//-----------------------------------------------------------------------
auto FFPFog::setParameter(std::string_view name, std::string_view value) noexcept -> bool
{
	if(name == "calc_mode")
	{
        CalcMode cm = value == "per_vertex" ? CalcMode::PER_VERTEX : CalcMode::PER_PIXEL;
		setCalcMode(cm);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------
auto FFPFogFactory::getType() const noexcept -> std::string_view
{
    return FFPFog::Type;
}

//-----------------------------------------------------------------------
auto FFPFogFactory::createInstance(ScriptCompiler* compiler, 
                                                    PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) noexcept -> SubRenderState*
{
    if (prop->name == "fog_stage")
    {
        if(prop->values.size() >= 1)
        {
            String strValue;

            if(false == SGScriptTranslator::getString(prop->values.front(), &strValue))
            {
                compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                return nullptr;
            }

            if (strValue == "ffp")
            {
                SubRenderState* subRenderState = createOrRetrieveInstance(translator);
                auto* fogSubRenderState = static_cast<FFPFog*>(subRenderState);
                auto it = prop->values.begin();

                if(prop->values.size() >= 2)
                {
                    ++it;
                    if (false == SGScriptTranslator::getString(*it, &strValue))
                    {
                        compiler->addError(ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
                        return nullptr;
                    }

                    fogSubRenderState->setParameter("calc_mode", strValue);
                }
                
                return subRenderState;
            }
        }       
    }

    return nullptr;
}

//-----------------------------------------------------------------------
void FFPFogFactory::writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, 
                                        Pass* srcPass, Pass* dstPass)
{
    ser->writeAttribute(4, "fog_stage");
    ser->writeValue("ffp");

    auto* fogSubRenderState = static_cast<FFPFog*>(subRenderState);


    if (fogSubRenderState->getCalcMode() == FFPFog::CalcMode::PER_VERTEX)
    {
        ser->writeValue("per_vertex");
    }
    else if (fogSubRenderState->getCalcMode() == FFPFog::CalcMode::PER_PIXEL)
    {
        ser->writeValue("per_pixel");
    }
}

//-----------------------------------------------------------------------
auto FFPFogFactory::createInstanceImpl() -> SubRenderState*
{
    return new FFPFog;
}

}
