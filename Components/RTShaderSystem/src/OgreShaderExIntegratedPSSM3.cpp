/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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
module;

#include <cstddef>

module Ogre.Components.RTShaderSystem;

import :ShaderExIntegratedPSSM3;
import :ShaderFFPRenderState;
import :ShaderFunction;
import :ShaderFunctionAtom;
import :ShaderGenerator;
import :ShaderParameter;
import :ShaderPrecompiledHeaders;
import :ShaderPrerequisites;
import :ShaderProgram;
import :ShaderProgramSet;
import :ShaderRenderState;
import :ShaderScriptTranslator;

import Ogre.Core;

import <algorithm>;
import <list>;
import <memory>;
import <string>;
import <vector>;

#define SGX_LIB_INTEGRATEDPSSM                      "SGXLib_IntegratedPSSM"
#define SGX_FUNC_COMPUTE_SHADOW_COLOUR3             "SGX_ComputeShadowFactor_PSSM3"
#define SGX_FUNC_APPLYSHADOWFACTOR_DIFFUSE          "SGX_ApplyShadowFactor_Diffuse"
namespace Ogre::RTShader {

/************************************************************************/
/*                                                                      */
/************************************************************************/
std::string_view const constinit IntegratedPSSM3::Type = "SGX_IntegratedPSSM3";

//-----------------------------------------------------------------------
IntegratedPSSM3::IntegratedPSSM3()
{
    mPCFxSamples = 2;
    mUseTextureCompare = false;
    mUseColourShadows = false;
    mDebug = false;
    mIsD3D9 = false;
    mShadowTextureParamsList.resize(1); // normal single texture depth shadowmapping
}

//-----------------------------------------------------------------------
auto IntegratedPSSM3::getType() const noexcept -> std::string_view
{
    return Type;
}


//-----------------------------------------------------------------------
auto IntegratedPSSM3::getExecutionOrder() const noexcept -> FFPShaderStage
{
    return FFPShaderStage::TEXTURING + 1;
}

//-----------------------------------------------------------------------
void IntegratedPSSM3::updateGpuProgramsParams(Renderable* rend, const Pass* pass,
                                             const AutoParamDataSource* source, 
                                             const LightList* pLightList)
{
    Vector4 vSplitPoints;

    for(size_t i = 0; i < mShadowTextureParamsList.size() - 1; i++)
    {
        vSplitPoints[i] = mShadowTextureParamsList[i].mMaxRange;
    }
    vSplitPoints[3] = mShadowTextureParamsList.back().mMaxRange;

    const Matrix4& proj = source->getProjectionMatrix();

    for(int i = 0; i < 4; i++)
    {
        auto tmp = proj * Vector4{0, 0, -vSplitPoints[i], 1};
        vSplitPoints[i] = tmp[2] / tmp[3];
    }


    mPSSplitPoints->setGpuParameter(vSplitPoints);

}

//-----------------------------------------------------------------------
void IntegratedPSSM3::copyFrom(const SubRenderState& rhs)
{
    const auto& rhsPssm= static_cast<const IntegratedPSSM3&>(rhs);

    mPCFxSamples = rhsPssm.mPCFxSamples;
    mUseTextureCompare = rhsPssm.mUseTextureCompare;
    mUseColourShadows = rhsPssm.mUseColourShadows;
    mDebug = rhsPssm.mDebug;
    mShadowTextureParamsList.resize(rhsPssm.mShadowTextureParamsList.size());

    for(auto itSrc = rhsPssm.mShadowTextureParamsList.begin();
        auto& itDst : mShadowTextureParamsList)
    {
        itDst.mMaxRange = itSrc->mMaxRange;
        ++itSrc;
    }
}

//-----------------------------------------------------------------------
auto IntegratedPSSM3::preAddToRenderState(const RenderState* renderState, 
                                         Pass* srcPass, Pass* dstPass) noexcept -> bool
{
    if (!srcPass->getParent()->getParent()->getReceiveShadows() ||
        renderState->getLightCount().isZeroLength())
        return false;

    mIsD3D9 = ShaderGenerator::getSingleton().getTargetLanguage() == "hlsl" &&
              !GpuProgramManager::getSingleton().isSyntaxSupported("vs_4_0_level_9_1");

    PixelFormat shadowTexFormat = PixelFormat::UNKNOWN;
    const auto& configs = ShaderGenerator::getSingleton().getActiveSceneManager()->getShadowTextureConfigList();
    if (!configs.empty())
        shadowTexFormat = configs[0].format; // assume first texture is representative
    mUseTextureCompare = PixelUtil::isDepth(shadowTexFormat) && !mIsD3D9;
    mUseColourShadows = PixelUtil::getComponentType(shadowTexFormat) == PixelComponentType::BYTE; // use colour shadowmaps for byte textures

    for(auto& it : mShadowTextureParamsList)
    {
        TextureUnitState* curShadowTexture = dstPass->createTextureUnitState();
            
        curShadowTexture->setContentType(TextureUnitState::ContentType::SHADOW);
        curShadowTexture->setTextureAddressingMode(TextureAddressingMode::BORDER);
        curShadowTexture->setTextureBorderColour(ColourValue::White);
        if(mUseTextureCompare)
        {
            curShadowTexture->setTextureCompareEnabled(true);
            curShadowTexture->setTextureCompareFunction(CompareFunction::LESS_EQUAL);
        }
        it.mTextureSamplerIndex = dstPass->getNumTextureUnitStates() - 1;
    }

    

    return true;
}

//-----------------------------------------------------------------------
void IntegratedPSSM3::setSplitPoints(const SplitPointList& newSplitPoints)
{
    OgreAssert(newSplitPoints.size() <= 5, "at most 5 split points are supported");

    mShadowTextureParamsList.resize(newSplitPoints.size() - 1);

    for (size_t i = 1; i < newSplitPoints.size(); ++i)
    {
        mShadowTextureParamsList[i - 1].mMaxRange = newSplitPoints[i];
    }
}

auto IntegratedPSSM3::setParameter(std::string_view name, std::string_view value) noexcept -> bool
{
    if(name == "debug")
    {
        return StringConverter::parse(value, mDebug);
    }
    else if (name == "filter")
    {
        if(value == "pcf4")
            mPCFxSamples = 2;
        else if(value == "pcf16")
            mPCFxSamples = 4;
        else
            return false;
    }

    return false;
}

//-----------------------------------------------------------------------
auto IntegratedPSSM3::resolveParameters(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* vsMain = vsProgram->getEntryPointFunction();
    Function* psMain = psProgram->getEntryPointFunction();
    
    // Get input position parameter.
    mVSInPos = vsMain->getLocalParameter(Parameter::Content::POSITION_OBJECT_SPACE);
    if(!mVSInPos)
        mVSInPos = vsMain->getInputParameter(Parameter::Content::POSITION_OBJECT_SPACE);
    
    // Get output position parameter.
    mVSOutPos = vsMain->getOutputParameter(Parameter::Content::POSITION_PROJECTIVE_SPACE);

    if (mIsD3D9)
    {
        mVSOutPos = vsMain->resolveOutputParameter(Parameter::Content::UNKNOWN, GpuConstantType::FLOAT4);
    }

    // Resolve input depth parameter.
    mPSInDepth = psMain->resolveInputParameter(mVSOutPos);
    
    // Get in/local diffuse parameter.
    mPSDiffuse = psMain->getInputParameter(Parameter::Content::COLOR_DIFFUSE);
    if (mPSDiffuse.get() == nullptr)   
    {
        mPSDiffuse = psMain->getLocalParameter(Parameter::Content::COLOR_DIFFUSE);
    }
    
    // Resolve output diffuse parameter.
    mPSOutDiffuse = psMain->resolveOutputParameter(Parameter::Content::COLOR_DIFFUSE);
    
    // Get in/local specular parameter.
    mPSSpecualr = psMain->getInputParameter(Parameter::Content::COLOR_SPECULAR);
    if (mPSSpecualr.get() == nullptr)  
    {
        mPSSpecualr = psMain->getLocalParameter(Parameter::Content::COLOR_SPECULAR);
    }
    
    // Resolve computed local shadow colour parameter.
    mPSLocalShadowFactor = psMain->resolveLocalParameter(GpuConstantType::FLOAT1, "lShadowFactor");

    // Resolve computed local shadow colour parameter.
    mPSSplitPoints = psProgram->resolveParameter(GpuConstantType::FLOAT4, "pssm_split_points");

    // Get derived scene colour.
    mPSDerivedSceneColour = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::DERIVED_SCENE_COLOUR);
    
    for(int lightIndex = 0;
        auto& it : mShadowTextureParamsList)
    {
        it.mWorldViewProjMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::TEXTURE_WORLDVIEWPROJ_MATRIX, lightIndex);

        it.mVSOutLightPosition = vsMain->resolveOutputParameter(Parameter::Content(std::to_underlying(Parameter::Content::POSITION_LIGHT_SPACE0) + lightIndex));
        it.mPSInLightPosition = psMain->resolveInputParameter(it.mVSOutLightPosition);
        auto stype = mUseTextureCompare ? GpuConstantType::SAMPLER2DSHADOW : GpuConstantType::SAMPLER2D;
        it.mTextureSampler = psProgram->resolveParameter(stype, "shadow_map", it.mTextureSamplerIndex);
        it.mInvTextureSize = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::INVERSE_TEXTURE_SIZE,
                                                          it.mTextureSamplerIndex);

        ++lightIndex;
    }

    if (!(mVSInPos.get()) || !(mVSOutPos.get()) || !(mPSDiffuse.get()) || !(mPSSpecualr.get()))
    {
        OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, "Not all parameters could be constructed for the sub-render state.");
    }

    return true;
}

//-----------------------------------------------------------------------
auto IntegratedPSSM3::resolveDependencies(ProgramSet* programSet) -> bool
{
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    psProgram->addDependency(SGX_LIB_INTEGRATEDPSSM);

    psProgram->addPreprocessorDefines(std::format("PROJ_SPACE_SPLITS,PSSM_NUM_SPLITS={},PCF_XSAMPLES={:.1f}",
                                                         mShadowTextureParamsList.size(), mPCFxSamples));

    if(mDebug)
        psProgram->addPreprocessorDefines("DEBUG_PSSM");

    if(mUseTextureCompare)
        psProgram->addPreprocessorDefines("PSSM_SAMPLE_CMP");

    if(mUseColourShadows)
        psProgram->addPreprocessorDefines("PSSM_SAMPLE_COLOUR");

    return true;
}

//-----------------------------------------------------------------------
auto IntegratedPSSM3::addFunctionInvocations(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM); 
    Function* vsMain = vsProgram->getEntryPointFunction();  
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);

    // Add vertex shader invocations.
    if (false == addVSInvocation(vsMain, std::to_underlying(FFPVertexShaderStage::TEXTURING) + 1))
        return false;

    // Add pixel shader invocations.
    if (false == addPSInvocation(psProgram, std::to_underlying(FFPFragmentShaderStage::COLOUR_BEGIN) + 2))
        return false;

    return true;
}

//-----------------------------------------------------------------------
auto IntegratedPSSM3::addVSInvocation(Function* vsMain, const int groupOrder) -> bool
{
    auto stage = vsMain->getStage(groupOrder);

    if(mIsD3D9)
    {
        auto vsOutPos = vsMain->resolveOutputParameter(Parameter::Content::POSITION_PROJECTIVE_SPACE);
        stage.assign(vsOutPos, mVSOutPos);
    }

    // Compute world space position.    
    for(auto const& it : mShadowTextureParamsList)
    {
        stage.callFunction(FFP_FUNC_TRANSFORM, it.mWorldViewProjMatrix, mVSInPos, it.mVSOutLightPosition);
    }

    return true;
}

//-----------------------------------------------------------------------
auto IntegratedPSSM3::addPSInvocation(Program* psProgram, const int groupOrder) -> bool
{
    Function* psMain = psProgram->getEntryPointFunction();
    auto stage = psMain->getStage(groupOrder);

    if(mShadowTextureParamsList.size() < 2)
    {
        ShadowTextureParams& splitParams0 = mShadowTextureParamsList[0];
        stage.callFunction("SGX_ShadowPCF4",
                           {In(splitParams0.mTextureSampler), In(splitParams0.mPSInLightPosition),
                            In(splitParams0.mInvTextureSize).xy(), Out(mPSLocalShadowFactor)});
    }
    else
    {
        auto fdepth = psMain->resolveLocalParameter(GpuConstantType::FLOAT1, "fdepth");
        if(mIsD3D9)
            stage.div(In(mPSInDepth).z(), In(mPSInDepth).w(), fdepth);
        else
            stage.assign(In(mPSInDepth).z(), fdepth);
        std::vector<Operand> params = {In(fdepth), In(mPSSplitPoints)};

        for(auto& texp : mShadowTextureParamsList)
        {
            params.push_back(In(texp.mPSInLightPosition));
            params.push_back(In(texp.mTextureSampler));
            params.push_back(In(texp.mInvTextureSize).xy());
        }

        params.push_back(Out(mPSLocalShadowFactor));

        // Compute shadow factor.
        stage.callFunction(SGX_FUNC_COMPUTE_SHADOW_COLOUR3, params);
    }

    // Apply shadow factor on diffuse colour.
    stage.callFunction(SGX_FUNC_APPLYSHADOWFACTOR_DIFFUSE,
                       {In(mPSDerivedSceneColour), In(mPSDiffuse), In(mPSLocalShadowFactor), Out(mPSDiffuse)});

    // Apply shadow factor on specular colour.
    stage.mul(mPSLocalShadowFactor, mPSSpecualr, mPSSpecualr);

    // Assign the local diffuse to output diffuse.
    stage.assign(mPSDiffuse, mPSOutDiffuse);

    return true;
}



//-----------------------------------------------------------------------
auto IntegratedPSSM3Factory::getType() const noexcept -> std::string_view
{
    return IntegratedPSSM3::Type;
}

//-----------------------------------------------------------------------
auto IntegratedPSSM3Factory::createInstance(ScriptCompiler* compiler, 
                                                      PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) noexcept -> SubRenderState*
{
    if (prop->name == "integrated_pssm4")
    {       
        if (prop->values.size() != 4)
        {
            compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
        }
        else
        {
            IntegratedPSSM3::SplitPointList splitPointList; 

            for(auto const& it : prop->values)
            {
                Real curSplitValue;
                
                if (false == SGScriptTranslator::getReal(it, &curSplitValue))
                {
                    compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                    break;
                }

                splitPointList.push_back(curSplitValue);
            }

            if (splitPointList.size() == 4)
            {
                SubRenderState* subRenderState = createOrRetrieveInstance(translator);
                auto* pssmSubRenderState = static_cast<IntegratedPSSM3*>(subRenderState);

                pssmSubRenderState->setSplitPoints(splitPointList);

                return pssmSubRenderState;
            }
        }       
    }

    return nullptr;
}

//-----------------------------------------------------------------------
auto IntegratedPSSM3Factory::createInstanceImpl() -> SubRenderState*
{
    return new IntegratedPSSM3;
}

}
