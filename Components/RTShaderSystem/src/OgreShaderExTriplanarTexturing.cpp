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

#include "OgreCommon.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgrePass.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreShaderExTriplanarTexturing.hpp"
#include "OgreShaderFFPRenderState.hpp"
#include "OgreShaderFunction.hpp"
#include "OgreShaderFunctionAtom.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderProgram.hpp"
#include "OgreShaderProgramSet.hpp"
#include "OgreShaderScriptTranslator.hpp"
#include "OgreTextureUnitState.hpp"
#include "OgreVector.hpp"

namespace Ogre {
    class AutoParamDataSource;
    class Renderable;

    namespace RTShader {
        class RenderState;
    }  // namespace RTShader
}  // namespace Ogre

#define SGX_FUNC_TRIPLANAR_TEXTURING "SGX_TriplanarTexturing"

namespace Ogre::RTShader {

    String TriplanarTexturing::type = "SGX_TriplanarTexturing";

    //-----------------------------------------------------------------------

	auto TriplanarTexturing::resolveParameters(ProgramSet* programSet) -> bool
	{
		Program* vsProgram = programSet->getCpuProgram(GPT_VERTEX_PROGRAM);
		Program* psProgram = programSet->getCpuProgram(GPT_FRAGMENT_PROGRAM);
		Function* vsMain = vsProgram->getEntryPointFunction();
		Function* psMain = psProgram->getEntryPointFunction();

		// Resolve input vertex shader normal.
		mVSInNormal = vsMain->resolveInputParameter(Parameter::SPC_NORMAL_OBJECT_SPACE);

		// Resolve output vertex shader normal.
		mVSOutNormal = vsMain->resolveOutputParameter(Parameter::SPC_NORMAL_OBJECT_SPACE);

		// Resolve pixel shader output diffuse color.
		mPSInDiffuse = psMain->resolveOutputParameter(Parameter::SPC_COLOR_DIFFUSE);

		// Resolve input pixel shader normal.
		mPSInNormal = psMain->resolveInputParameter(mVSOutNormal);

		// Resolve input vertex shader normal.
		mVSInPosition = vsMain->resolveInputParameter(Parameter::SPC_POSITION_OBJECT_SPACE);

		mVSOutPosition = vsMain->resolveOutputParameter(Parameter::SPS_TEXTURE_COORDINATES, -1, Parameter::SPC_POSITION_OBJECT_SPACE, GCT_FLOAT4);
		mPSInPosition = psMain->resolveInputParameter(mVSOutPosition);

		mSamplerFromX = psProgram->resolveParameter(GCT_SAMPLER2D, "tp_sampler_from_x", mTextureSamplerIndexFromX);
		if (mSamplerFromX.get() == nullptr)
			return false;

		mSamplerFromY = psProgram->resolveParameter(GCT_SAMPLER2D, "tp_sampler_from_y", mTextureSamplerIndexFromY);
		if (mSamplerFromY.get() == nullptr)
			return false;

		mSamplerFromZ = psProgram->resolveParameter(GCT_SAMPLER2D, "tp_sampler_from_z", mTextureSamplerIndexFromZ);
		if (mSamplerFromZ.get() == nullptr)
			return false;

        mPSOutDiffuse = psMain->resolveOutputParameter(Parameter::SPC_COLOR_DIFFUSE);
        if (mPSOutDiffuse.get() == nullptr)    
            return false;
    
        mPSTPParams = psProgram->resolveParameter(GCT_FLOAT3, "gTPParams");
        if (mPSTPParams.get() == nullptr)
            return false;
        return true;
    }

    //-----------------------------------------------------------------------
    auto TriplanarTexturing::resolveDependencies(ProgramSet* programSet) -> bool
    {
        Program* psProgram = programSet->getCpuProgram(GPT_FRAGMENT_PROGRAM);
        Program* vsProgram = programSet->getCpuProgram(GPT_VERTEX_PROGRAM);
		psProgram->addDependency(FFP_LIB_TEXTURING);
        psProgram->addDependency("SGXLib_TriplanarTexturing");
        vsProgram->addDependency(FFP_LIB_COMMON);
        return true;
    }

    //-----------------------------------------------------------------------
	auto TriplanarTexturing::addFunctionInvocations(ProgramSet* programSet) -> bool
	{
        Program* psProgram = programSet->getCpuProgram(GPT_FRAGMENT_PROGRAM);
        Function* psMain = psProgram->getEntryPointFunction();
        Program* vsProgram = programSet->getCpuProgram(GPT_VERTEX_PROGRAM);
        Function* vsMain = vsProgram->getEntryPointFunction();

        auto vsStage = vsMain->getStage(FFP_PS_TEXTURING);
        vsStage.assign(mVSInNormal, mVSOutNormal);
        vsStage.assign(mVSInPosition, mVSOutPosition);

        psMain->getStage(FFP_PS_TEXTURING)
            .callFunction("SGX_TriplanarTexturing",
                          {In(mPSInDiffuse), In(mPSInNormal), In(mPSInPosition), In(mSamplerFromX),
                           In(mSamplerFromY), In(mSamplerFromZ), In(mPSTPParams), Out(mPSOutDiffuse)});

        return true;
    }

    //-----------------------------------------------------------------------
    auto TriplanarTexturing::getType() const noexcept -> std::string_view
    {
        return type;
    }

    //-----------------------------------------------------------------------
    auto TriplanarTexturing::getExecutionOrder() const noexcept -> int
    {
        return FFP_TEXTURING;
    }

    //-----------------------------------------------------------------------
    auto TriplanarTexturing::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass ) noexcept -> bool
    {
        TextureUnitState* textureUnit;
    
        // Create the mapping textures
        textureUnit = dstPass->createTextureUnitState();
        textureUnit->setTextureName(mTextureNameFromX);     
        mTextureSamplerIndexFromX = dstPass->getNumTextureUnitStates() - 1;
    
        textureUnit = dstPass->createTextureUnitState();
        textureUnit->setTextureName(mTextureNameFromY);     
        mTextureSamplerIndexFromY = dstPass->getNumTextureUnitStates() - 1;
    
        textureUnit = dstPass->createTextureUnitState();
        textureUnit->setTextureName(mTextureNameFromZ);     
        mTextureSamplerIndexFromZ = dstPass->getNumTextureUnitStates() - 1;
        return true;
    }

    //-----------------------------------------------------------------------
    void TriplanarTexturing::copyFrom(const SubRenderState& rhs)
    {
        const auto& rhsTP = static_cast<const TriplanarTexturing&>(rhs);
    
        mPSOutDiffuse = rhsTP.mPSOutDiffuse;
        mPSInDiffuse = rhsTP.mPSInDiffuse;

        mVSInPosition = rhsTP.mVSInPosition;
        mVSOutPosition = rhsTP.mVSOutPosition;

        mVSOutNormal = rhsTP.mVSOutNormal;
        mVSInNormal = rhsTP.mVSInNormal;
        mPSInNormal = rhsTP.mPSInNormal;

        mVSOutPosition = rhsTP.mVSOutPosition;
        mVSInPosition = rhsTP.mVSInPosition;
        mPSInPosition = rhsTP.mPSInPosition;

        mSamplerFromX = rhsTP.mSamplerFromX;
        mSamplerFromY = rhsTP.mSamplerFromY;
        mSamplerFromZ = rhsTP.mSamplerFromZ;

        mPSTPParams = rhsTP.mPSTPParams;
        mParameters = rhsTP.mParameters;
        mTextureNameFromX = rhsTP.mTextureNameFromX;
        mTextureNameFromY = rhsTP.mTextureNameFromY;
        mTextureNameFromZ = rhsTP.mTextureNameFromZ;
    }

    //-----------------------------------------------------------------------
    void TriplanarTexturing::updateGpuProgramsParams(Renderable* rend, const Pass* pass, const AutoParamDataSource* source,
                                         const LightList* pLightList)
    {
        mPSTPParams->setGpuParameter(mParameters);
    }

    //-----------------------------------------------------------------------
    void TriplanarTexturing::setParameters(const Vector3 &parameters)
    {
        mParameters = parameters;
    }

    //-----------------------------------------------------------------------
    void TriplanarTexturing::setTextureNames(std::string_view textureNameFromX, std::string_view textureNameFromY, std::string_view textureNameFromZ)
    {
        mTextureNameFromX = textureNameFromX;
        mTextureNameFromY = textureNameFromY;
        mTextureNameFromZ = textureNameFromZ;
    }

    //-----------------------------------------------------------------------
    auto TriplanarTexturingFactory::getType() const noexcept -> std::string_view
    {
        return TriplanarTexturing::type;
    }

    //-----------------------------------------------------------------------
    auto TriplanarTexturingFactory::createInstance(ScriptCompiler* compiler, 
                                                       PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) noexcept -> SubRenderState*
    {
        if (prop->name == "triplanarTexturing")
        {
            if (prop->values.size() == 6)
            {
                SubRenderState* subRenderState = createOrRetrieveInstance(translator);
                auto* tpSubRenderState = static_cast<TriplanarTexturing*>(subRenderState);
                
                auto it = prop->values.begin();
                float parameters[3];
                if (false == SGScriptTranslator::getFloat(*it, parameters))
                {
                    compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                    return nullptr;
                }
                ++it;
                if (false == SGScriptTranslator::getFloat(*it, parameters + 1))
                {
                    compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                    return nullptr;
                }
                ++it;
                if (false == SGScriptTranslator::getFloat(*it, parameters + 2))
                {
                    compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                    return nullptr;
                }
                Vector3 vParameters(parameters[0], parameters[1], parameters[2]);
                tpSubRenderState->setParameters(vParameters);

                String textureNameFromX, textureNameFromY, textureNameFromZ;
                ++it;
                if (false == SGScriptTranslator::getString(*it, &textureNameFromX))
                {
                    compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                    return nullptr;
                }
                ++it;
                if (false == SGScriptTranslator::getString(*it, &textureNameFromY))
                {
                    compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                    return nullptr;
                }
                ++it;
                if (false == SGScriptTranslator::getString(*it, &textureNameFromZ))
                {
                    compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                    return nullptr;
                }
                tpSubRenderState->setTextureNames(textureNameFromX, textureNameFromY, textureNameFromZ);

                return subRenderState;
            }
            else
            {
                compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
            }
        }
        return nullptr;
    }

    //-----------------------------------------------------------------------
    auto TriplanarTexturingFactory::createInstanceImpl() -> SubRenderState*
    {
        return new TriplanarTexturing;
    }


}
