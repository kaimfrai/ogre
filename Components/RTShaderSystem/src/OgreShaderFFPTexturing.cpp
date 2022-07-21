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
module;

#include <cstddef>

module Ogre.Components.RTShaderSystem;

import :ShaderFFPRenderState;
import :ShaderFFPTexturing;
import :ShaderFunction;
import :ShaderFunctionAtom;
import :ShaderGenerator;
import :ShaderParameter;
import :ShaderPrecompiledHeaders;
import :ShaderPrerequisites;
import :ShaderProgram;
import :ShaderProgramSet;
import :ShaderScriptTranslator;
import :ShaderSubRenderState;

import Ogre.Core;

import <map>;
import <memory>;
import <string>;
import <utility>;

namespace Ogre::RTShader {

/************************************************************************/
/*                                                                      */
/************************************************************************/
#define _INT_VALUE(f) (*(int*)(&(f)))

const String c_ParamTexelEx("texel_");

//-----------------------------------------------------------------------
FFPTexturing::FFPTexturing()  
= default;

//-----------------------------------------------------------------------
auto FFPTexturing::getType() const noexcept -> std::string_view
{
    return Type;
}

//-----------------------------------------------------------------------
auto FFPTexturing::getExecutionOrder() const noexcept -> FFPShaderStage
{       
    return FFPShaderStage::TEXTURING;
}

//-----------------------------------------------------------------------
auto FFPTexturing::resolveParameters(ProgramSet* programSet) -> bool
{
    for (auto & i : mTextureUnitParamsList)
    {
        TextureUnitParams* curParams = &i;

        if (false == resolveUniformParams(curParams, programSet))
            return false;


        if (false == resolveFunctionsParams(curParams, programSet))
            return false;
    }
    

    return true;
}

//-----------------------------------------------------------------------
auto FFPTexturing::resolveUniformParams(TextureUnitParams* textureUnitParams, ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    
    // Resolve texture sampler parameter.       
    textureUnitParams->mTextureSampler = psProgram->resolveParameter(textureUnitParams->mTextureSamplerType, "gTextureSampler", textureUnitParams->mTextureSamplerIndex);

    // Resolve texture matrix parameter.
    if (needsTextureMatrix(textureUnitParams->mTextureUnitState))
    {               
        textureUnitParams->mTextureMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::TEXTURE_MATRIX, textureUnitParams->mTextureSamplerIndex);
    }

    using enum TexCoordCalcMethod;
    switch (textureUnitParams->mTexCoordCalcMethod)
    {
    case NONE:
        break;

    // Resolve World + View matrices.
    case ENVIRONMENT_MAP:
    case ENVIRONMENT_MAP_PLANAR:
    case ENVIRONMENT_MAP_NORMAL:
        //TODO: change the following 'mWorldITMatrix' member to 'mWorldViewITMatrix'
        mWorldITMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::INVERSE_TRANSPOSE_WORLDVIEW_MATRIX);
        mViewMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::VIEW_MATRIX);
        mWorldMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::WORLD_MATRIX);
        break;

    case ENVIRONMENT_MAP_REFLECTION:
        mWorldMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::WORLD_MATRIX);
        mWorldITMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::INVERSE_TRANSPOSE_WORLD_MATRIX);
        mViewMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::VIEW_MATRIX);
        break;

    case PROJECTIVE_TEXTURE:
        mWorldMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::WORLD_MATRIX);
        textureUnitParams->mTextureViewProjImageMatrix = vsProgram->resolveParameter(
            GpuProgramParameters::AutoConstantType::TEXTURE_VIEWPROJ_MATRIX, textureUnitParams->mTextureSamplerIndex);
        break;
    }

    return true;
}



//-----------------------------------------------------------------------
auto FFPTexturing::resolveFunctionsParams(TextureUnitParams* textureUnitParams, ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* vsMain   = vsProgram->getEntryPointFunction();
    Function* psMain   = psProgram->getEntryPointFunction();
    Parameter::Content texCoordContent = Parameter::Content::UNKNOWN;

    using enum TexCoordCalcMethod;
    switch (textureUnitParams->mTexCoordCalcMethod)
    {
        case NONE:
            // Resolve explicit vs input texture coordinates.
            
            if(mIsPointSprite)
                break;

            if (textureUnitParams->mTextureMatrix.get() == nullptr)
                texCoordContent = Parameter::Content(std::to_underlying(Parameter::Content::TEXTURE_COORDINATE0) + textureUnitParams->mTextureUnitState->getTextureCoordSet());

            // assume already resolved
            if(vsMain->getOutputParameter(texCoordContent, textureUnitParams->mVSInTextureCoordinateType))
                break;

            textureUnitParams->mVSInputTexCoord = vsMain->resolveInputParameter(
                Parameter::Content(std::to_underlying(Parameter::Content::TEXTURE_COORDINATE0) +
                                   textureUnitParams->mTextureUnitState->getTextureCoordSet()),
                textureUnitParams->mVSInTextureCoordinateType);
            break;

        case ENVIRONMENT_MAP:
        case ENVIRONMENT_MAP_PLANAR:
        case ENVIRONMENT_MAP_NORMAL:
            // Resolve vertex normal.
            mVSInputPos = vsMain->resolveInputParameter(Parameter::Content::POSITION_OBJECT_SPACE);
            mVSInputNormal = vsMain->resolveInputParameter(Parameter::Content::NORMAL_OBJECT_SPACE);
            break;  

        case ENVIRONMENT_MAP_REFLECTION:

            // Resolve vertex normal.
            mVSInputNormal = vsMain->resolveInputParameter(Parameter::Content::NORMAL_OBJECT_SPACE);
            // Resolve vertex position.
            mVSInputPos = vsMain->resolveInputParameter(Parameter::Content::POSITION_OBJECT_SPACE);
            break;

        case PROJECTIVE_TEXTURE:
            // Resolve vertex position.
            mVSInputPos = vsMain->resolveInputParameter(Parameter::Content::POSITION_OBJECT_SPACE);
            break;
    }

    if(mIsPointSprite)
    {
        textureUnitParams->mPSInputTexCoord = psMain->resolveInputParameter(Parameter::Content::POINTSPRITE_COORDINATE);
    }
    else
    {
        // Resolve vs output texture coordinates.
        textureUnitParams->mVSOutputTexCoord =
            vsMain->resolveOutputParameter(texCoordContent, textureUnitParams->mVSOutTextureCoordinateType);

        // Resolve ps input texture coordinates.
        textureUnitParams->mPSInputTexCoord = psMain->resolveInputParameter(textureUnitParams->mVSOutputTexCoord);
    }

    mPSDiffuse = psMain->getInputParameter(Parameter::Content::COLOR_DIFFUSE);
    if (mPSDiffuse.get() == nullptr)
    {
        mPSDiffuse = psMain->getLocalParameter(Parameter::Content::COLOR_DIFFUSE);
    }

    mPSSpecular = psMain->getInputParameter(Parameter::Content::COLOR_SPECULAR);
    if (mPSSpecular.get() == nullptr)
    {
        mPSSpecular = psMain->getLocalParameter(Parameter::Content::COLOR_SPECULAR);
    }

    mPSOutDiffuse = psMain->resolveOutputParameter(Parameter::Content::COLOR_DIFFUSE);

    if (!mPSDiffuse || !mPSSpecular)
    {
        OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR,
                    "Not all parameters could be constructed for the sub-render state.");
    }
    return true;
}

//-----------------------------------------------------------------------
auto FFPTexturing::resolveDependencies(ProgramSet* programSet) -> bool
{
    //! [deps_resolve]
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);

    vsProgram->addDependency(FFP_LIB_COMMON);
    vsProgram->addDependency(FFP_LIB_TEXTURING);    
    psProgram->addDependency(FFP_LIB_COMMON);
    psProgram->addDependency(FFP_LIB_TEXTURING);
    //! [deps_resolve]
    return true;
}

//-----------------------------------------------------------------------
auto FFPTexturing::addFunctionInvocations(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* vsMain   = vsProgram->getEntryPointFunction();
    Function* psMain   = psProgram->getEntryPointFunction();

    for (auto & i : mTextureUnitParamsList)
    {
        TextureUnitParams* curParams = &i;

        if (false == addVSFunctionInvocations(curParams, vsMain))
            return false;

        if (false == addPSFunctionInvocations(curParams, psMain))
            return false;
    }

    return true;
}

//-----------------------------------------------------------------------
auto FFPTexturing::addVSFunctionInvocations(TextureUnitParams* textureUnitParams, Function* vsMain) -> bool
{
    if(mIsPointSprite)
        return true;
    
    auto stage = vsMain->getStage(std::to_underlying(FFPVertexShaderStage::TEXTURING));

    using enum TexCoordCalcMethod;
    switch (textureUnitParams->mTexCoordCalcMethod)
    {
    case NONE:
        if(textureUnitParams->mVSInputTexCoord)
        stage.assign(textureUnitParams->mVSInputTexCoord, textureUnitParams->mVSOutputTexCoord);
        break;
    case ENVIRONMENT_MAP:
    case ENVIRONMENT_MAP_PLANAR:
        stage.callFunction(FFP_FUNC_GENERATE_TEXCOORD_ENV_SPHERE,
                           {In(mWorldMatrix), In(mViewMatrix), In(mWorldITMatrix), In(mVSInputPos), In(mVSInputNormal),
                            Out(textureUnitParams->mVSOutputTexCoord)});
        break;
    case ENVIRONMENT_MAP_REFLECTION:
        stage.callFunction(FFP_FUNC_GENERATE_TEXCOORD_ENV_REFLECT,
                           {In(mWorldMatrix), In(mWorldITMatrix), In(mViewMatrix), In(mVSInputNormal), In(mVSInputPos),
                            Out(textureUnitParams->mVSOutputTexCoord)});
        break;
    case ENVIRONMENT_MAP_NORMAL:
        stage.callFunction(
            FFP_FUNC_GENERATE_TEXCOORD_ENV_NORMAL,
            {In(mWorldITMatrix), In(mViewMatrix), In(mVSInputNormal), Out(textureUnitParams->mVSOutputTexCoord)});
        break;
    case PROJECTIVE_TEXTURE:
        stage.callFunction(FFP_FUNC_GENERATE_TEXCOORD_PROJECTION,
                           {In(mWorldMatrix), In(textureUnitParams->mTextureViewProjImageMatrix), In(mVSInputPos),
                            Out(textureUnitParams->mVSOutputTexCoord)});
        break;
    default:
        return false;
    }

    if (textureUnitParams->mTextureMatrix)
    {
        stage.callFunction(FFP_FUNC_TRANSFORM_TEXCOORD, textureUnitParams->mTextureMatrix,
                           textureUnitParams->mVSOutputTexCoord, textureUnitParams->mVSOutputTexCoord);
    }

    return true;
}
//-----------------------------------------------------------------------
auto FFPTexturing::addPSFunctionInvocations(TextureUnitParams* textureUnitParams, Function* psMain) -> bool
{
    const LayerBlendModeEx& colourBlend = textureUnitParams->mTextureUnitState->getColourBlendMode();
    const LayerBlendModeEx& alphaBlend  = textureUnitParams->mTextureUnitState->getAlphaBlendMode();
    ParameterPtr source1;
    ParameterPtr source2;
    int groupOrder = std::to_underlying(FFPFragmentShaderStage::TEXTURING);
    
            
    // Add texture sampling code.
    ParameterPtr texel = psMain->resolveLocalParameter(GpuConstantType::FLOAT4, ::std::format("{}{}", c_ParamTexelEx, textureUnitParams->mTextureSamplerIndex));

    // Build colour argument for source1.
    source1 = getPSArgument(texel, colourBlend.source1, colourBlend.colourArg1, colourBlend.alphaArg1, false);

    // Build colour argument for source2.
    source2 = getPSArgument(texel, colourBlend.source2, colourBlend.colourArg2, colourBlend.alphaArg2, false);

    if(source1 == texel || source2 == texel || colourBlend.operation == LayerBlendOperationEx::BLEND_TEXTURE_ALPHA)
        addPSSampleTexelInvocation(textureUnitParams, psMain, texel, std::to_underlying(FFPFragmentShaderStage::SAMPLING));

    bool needDifferentAlphaBlend = false;
    if (alphaBlend.operation != colourBlend.operation ||
        alphaBlend.source1 != colourBlend.source1 ||
        alphaBlend.source2 != colourBlend.source2 ||
        colourBlend.source1 == LayerBlendSource::MANUAL ||
        colourBlend.source2 == LayerBlendSource::MANUAL ||
        alphaBlend.source1 == LayerBlendSource::MANUAL ||
        alphaBlend.source2 == LayerBlendSource::MANUAL)
        needDifferentAlphaBlend = true;

    if(mLateAddBlend && colourBlend.operation == LayerBlendOperationEx::ADD)
    {
        groupOrder = std::to_underlying(FFPFragmentShaderStage::COLOUR_END) + 50 + 1; // after PBR lighting
    }

    // Build colours blend
    addPSBlendInvocations(psMain, source1, source2, texel, 
        textureUnitParams->mTextureSamplerIndex,
        colourBlend, groupOrder,
        needDifferentAlphaBlend ? Operand::OpMask::XYZ : Operand::OpMask::ALL);

    // Case we need different alpha channel code.
    if (needDifferentAlphaBlend)
    {
        // Build alpha argument for source1.
        source1 = getPSArgument(texel, alphaBlend.source1, alphaBlend.colourArg1, alphaBlend.alphaArg1, true);

        // Build alpha argument for source2.
        source2 = getPSArgument(texel, alphaBlend.source2, alphaBlend.colourArg2, alphaBlend.alphaArg2, true);

        if(source1 == texel || source2 == texel || alphaBlend.operation == LayerBlendOperationEx::BLEND_TEXTURE_ALPHA)
            addPSSampleTexelInvocation(textureUnitParams, psMain, texel, std::to_underlying(FFPFragmentShaderStage::SAMPLING));

        // Build alpha blend
        addPSBlendInvocations(psMain, source1, source2, texel,
                              textureUnitParams->mTextureSamplerIndex, alphaBlend, groupOrder,
                              Operand::OpMask::W);
    }
    
    

    return true;
}

//-----------------------------------------------------------------------
void FFPTexturing::addPSSampleTexelInvocation(TextureUnitParams* textureUnitParams, Function* psMain, 
                                              const ParameterPtr& texel, int groupOrder)
{
    auto stage = psMain->getStage(groupOrder);

    if (textureUnitParams->mTexCoordCalcMethod != TexCoordCalcMethod::PROJECTIVE_TEXTURE)
    {
        stage.sampleTexture(textureUnitParams->mTextureSampler, textureUnitParams->mPSInputTexCoord, texel);
        return;
    }

    stage.callFunction(FFP_FUNC_SAMPLE_TEXTURE_PROJ, textureUnitParams->mTextureSampler,
                       textureUnitParams->mPSInputTexCoord, texel);
}

//-----------------------------------------------------------------------
auto FFPTexturing::getPSArgument(ParameterPtr texel, LayerBlendSource blendSrc, const ColourValue& colourValue,
                                         Real alphaValue, bool isAlphaArgument) const -> ParameterPtr
{
    using enum LayerBlendSource;
    switch(blendSrc)
    {
    case CURRENT:
        return mPSOutDiffuse;
    case TEXTURE:
        return texel;
    case DIFFUSE:
        return mPSDiffuse;
    case SPECULAR:
        return mPSSpecular;
    case MANUAL:
        if (isAlphaArgument)
        {
            return ParameterFactory::createConstParam(Vector4::Fill(alphaValue));
        }

        return ParameterFactory::createConstParam(Vector4{(Real)colourValue.r, (Real)colourValue.g,
                                                         (Real)colourValue.b, (Real)colourValue.a});
    }

    return {};
}

//-----------------------------------------------------------------------
void FFPTexturing::addPSBlendInvocations(Function* psMain, 
                                          ParameterPtr arg1,
                                          ParameterPtr arg2,
                                          ParameterPtr texel,
                                          int samplerIndex,
                                          const LayerBlendModeEx& blendMode,
                                          const int groupOrder, 
                                          Operand::OpMask mask)
{
    auto stage = psMain->getStage(groupOrder);
    using enum LayerBlendOperationEx;
    switch(blendMode.operation)
    {
    case SOURCE1:
        stage.assign(In(arg1).mask(mask), Out(mPSOutDiffuse).mask(mask));
        break;
    case SOURCE2:
        stage.assign(In(arg2).mask(mask), Out(mPSOutDiffuse).mask(mask));
        break;
    case MODULATE:
        stage.mul(In(arg1).mask(mask), In(arg2).mask(mask), Out(mPSOutDiffuse).mask(mask));
        break;
    case MODULATE_X2:
        stage.callFunction(FFP_FUNC_MODULATEX2, In(arg1).mask(mask), In(arg2).mask(mask),
                           Out(mPSOutDiffuse).mask(mask));
        break;
    case MODULATE_X4:
        stage.callFunction(FFP_FUNC_MODULATEX4, In(arg1).mask(mask), In(arg2).mask(mask),
                           Out(mPSOutDiffuse).mask(mask));
        break;
    case ADD:
        stage.add(In(arg1).mask(mask), In(arg2).mask(mask), Out(mPSOutDiffuse).mask(mask));
        break;
    case ADD_SIGNED:
        stage.callFunction(FFP_FUNC_ADDSIGNED, In(arg1).mask(mask), In(arg2).mask(mask),
                           Out(mPSOutDiffuse).mask(mask));
        break;
    case ADD_SMOOTH:
        stage.callFunction(FFP_FUNC_ADDSMOOTH, In(arg1).mask(mask), In(arg2).mask(mask),
                           Out(mPSOutDiffuse).mask(mask));
        break;
    case SUBTRACT:
        stage.sub(In(arg1).mask(mask), In(arg2).mask(mask), Out(mPSOutDiffuse).mask(mask));
        break;
    case BLEND_DIFFUSE_ALPHA:
        stage.callFunction(FFP_FUNC_LERP, {In(arg2).mask(mask), In(arg1).mask(mask), In(mPSDiffuse).w(),
                                           Out(mPSOutDiffuse).mask(mask)});
        break;
    case BLEND_TEXTURE_ALPHA:
        stage.callFunction(FFP_FUNC_LERP, {In(arg2).mask(mask), In(arg1).mask(mask), In(texel).w(),
                                           Out(mPSOutDiffuse).mask(mask)});
        break;
    case BLEND_CURRENT_ALPHA:
        stage.callFunction(FFP_FUNC_LERP, {In(arg2).mask(mask), In(arg1).mask(mask),
                                           In(mPSOutDiffuse).w(),
                                           Out(mPSOutDiffuse).mask(mask)});
        break;
    case BLEND_MANUAL:
        stage.callFunction(FFP_FUNC_LERP, {In(arg2).mask(mask), In(arg1).mask(mask),
                                           In(ParameterFactory::createConstParam(blendMode.factor)),
                                           Out(mPSOutDiffuse).mask(mask)});
        break;
    case DOTPRODUCT:
        stage.callFunction(FFP_FUNC_DOTPRODUCT, In(arg2).mask(mask), In(arg1).mask(mask),
                           Out(mPSOutDiffuse).mask(mask));
        break;
    case BLEND_DIFFUSE_COLOUR:
        stage.callFunction(FFP_FUNC_LERP, {In(arg2).mask(mask), In(arg1).mask(mask),
                                           In(mPSDiffuse).mask(mask), Out(mPSOutDiffuse).mask(mask)});
        break;
    }
}

//-----------------------------------------------------------------------
auto FFPTexturing::getTexCalcMethod(TextureUnitState* textureUnitState) -> TexCoordCalcMethod
{
    TexCoordCalcMethod                      texCoordCalcMethod = TexCoordCalcMethod::NONE;  
    const TextureUnitState::EffectMap&      effectMap = textureUnitState->getEffects(); 
    
    for (auto const& effi : effectMap)
    {
        using enum TextureUnitState::TextureEffectType;
        switch (effi.second.type)
        {
        case ENVIRONMENT_MAP:
        {
            auto subtype = static_cast<TextureUnitState::EnvMapType>(effi.second.subtype);
            if (subtype == TextureUnitState::EnvMapType::CURVED)
            {
                texCoordCalcMethod = TexCoordCalcMethod::ENVIRONMENT_MAP;               
            }
            else if (subtype == TextureUnitState::EnvMapType::PLANAR)
            {
                texCoordCalcMethod = TexCoordCalcMethod::ENVIRONMENT_MAP_PLANAR;                
            }
            else if (subtype == TextureUnitState::EnvMapType::REFLECTION)
            {
                texCoordCalcMethod = TexCoordCalcMethod::ENVIRONMENT_MAP_REFLECTION;                
            }
            else if (subtype == TextureUnitState::EnvMapType::NORMAL)
            {
                texCoordCalcMethod = TexCoordCalcMethod::ENVIRONMENT_MAP_NORMAL;                
            }
            break;
        }
        case UVSCROLL:
        case USCROLL:
        case VSCROLL:
        case ROTATE:
        case TRANSFORM:
            break;
        case PROJECTIVE_TEXTURE:
            texCoordCalcMethod = TexCoordCalcMethod::PROJECTIVE_TEXTURE;
            break;
        }
    }

    return texCoordCalcMethod;
}

//-----------------------------------------------------------------------
auto FFPTexturing::needsTextureMatrix(TextureUnitState* textureUnitState) -> bool
{
    const TextureUnitState::EffectMap&      effectMap = textureUnitState->getEffects(); 

    for (auto const& effi : effectMap)
    {
        using enum TextureUnitState::TextureEffectType;
        switch (effi.second.type)
        {
    
        case UVSCROLL:
        case USCROLL:
        case VSCROLL:
        case ROTATE:
        case TRANSFORM:
        case ENVIRONMENT_MAP:
        case PROJECTIVE_TEXTURE:
            return true;        
        }
    }

    const Ogre::Matrix4 matTexture = textureUnitState->getTextureTransform();

    // Resolve texture matrix parameter.
    if (matTexture != Matrix4::IDENTITY)
        return true;

    return false;
}


auto FFPTexturing::setParameter(std::string_view name, std::string_view value) noexcept -> bool
{
    if(name == "late_add_blend")
    {
        StringConverter::parse(value, mLateAddBlend);
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------
void FFPTexturing::copyFrom(const SubRenderState& rhs)
{
    const auto& rhsTexture = static_cast<const FFPTexturing&>(rhs);

    mLateAddBlend = rhsTexture.mLateAddBlend;
    setTextureUnitCount(rhsTexture.getTextureUnitCount());

    for (unsigned int i=0; i < rhsTexture.getTextureUnitCount(); ++i)
    {
        setTextureUnit(i, rhsTexture.mTextureUnitParamsList[i].mTextureUnitState);
    }       
}

//-----------------------------------------------------------------------
auto FFPTexturing::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept -> bool
{
    mIsPointSprite = srcPass->getPointSpritesEnabled();

    if (auto rs = Root::getSingleton().getRenderSystem())
    {
        if (mIsPointSprite && !rs->getCapabilities()->hasCapability(Capabilities::POINT_SPRITES))
            return false;
    }

    setTextureUnitCount(srcPass->getTextureUnitStates().size());

    // Build texture stage sub states.
    for (unsigned short i=0; i < srcPass->getNumTextureUnitStates(); ++i)
    {       
        TextureUnitState* texUnitState = srcPass->getTextureUnitState(i);
        setTextureUnit(i, texUnitState);
    }   

    return true;
}

//-----------------------------------------------------------------------
void FFPTexturing::setTextureUnitCount(size_t count)
{
    mTextureUnitParamsList.resize(count);

    for (unsigned int i=0; i < count; ++i)
    {
        TextureUnitParams& curParams = mTextureUnitParamsList[i];

        curParams.mTextureUnitState             = nullptr;
        curParams.mTextureSamplerIndex          = 0;              
        curParams.mTextureSamplerType           = GpuConstantType::SAMPLER2D;        
        curParams.mVSInTextureCoordinateType    = GpuConstantType::FLOAT2;   
        curParams.mVSOutTextureCoordinateType   = GpuConstantType::FLOAT2;       
    }
}

//-----------------------------------------------------------------------
void FFPTexturing::setTextureUnit(unsigned short index, TextureUnitState* textureUnitState)
{
    OgreAssert(index < mTextureUnitParamsList.size(), "FFPTexturing unit index out of bounds");

    TextureUnitParams& curParams = mTextureUnitParamsList[index];


    curParams.mTextureSamplerIndex = index;
    curParams.mTextureUnitState    = textureUnitState;

    bool isGLES2 = ShaderGenerator::getSingleton().getTargetLanguage() == "glsles";

    using enum TextureType;
    switch (curParams.mTextureUnitState->getTextureType())
    {
    case _1D:
        curParams.mTextureSamplerType = GpuConstantType::SAMPLER1D;
        curParams.mVSInTextureCoordinateType = GpuConstantType::FLOAT1;
        if(!isGLES2) // no 1D texture support
            break;
        [[fallthrough]];
    case _2D:
        curParams.mTextureSamplerType = GpuConstantType::SAMPLER2D;
        curParams.mVSInTextureCoordinateType = GpuConstantType::FLOAT2;
        break;
    case EXTERNAL_OES:
        curParams.mTextureSamplerType = GpuConstantType::SAMPLER_EXTERNAL_OES;
        curParams.mVSInTextureCoordinateType = GpuConstantType::FLOAT2;
        break;
    case _2D_ARRAY:
        curParams.mTextureSamplerType = GpuConstantType::SAMPLER2DARRAY;
        curParams.mVSInTextureCoordinateType = GpuConstantType::FLOAT3;
        break;
    case _3D:
        curParams.mTextureSamplerType = GpuConstantType::SAMPLER3D;
        curParams.mVSInTextureCoordinateType = GpuConstantType::FLOAT3;
        break;
    case CUBE_MAP:
        curParams.mTextureSamplerType = GpuConstantType::SAMPLERCUBE;
        curParams.mVSInTextureCoordinateType = GpuConstantType::FLOAT3;
        break;
    }   

     curParams.mVSOutTextureCoordinateType = curParams.mVSInTextureCoordinateType;
     curParams.mTexCoordCalcMethod = getTexCalcMethod(curParams.mTextureUnitState);

    // let TexCalcMethod override texture type, as it might be wrong for
    // content_type shadow & content_type compositor
    if (curParams.mTexCoordCalcMethod == TexCoordCalcMethod::ENVIRONMENT_MAP_REFLECTION)
    {
        curParams.mVSOutTextureCoordinateType = GpuConstantType::FLOAT3;
        curParams.mTextureSamplerType = GpuConstantType::SAMPLERCUBE;
    }

     if (curParams.mTexCoordCalcMethod == TexCoordCalcMethod::PROJECTIVE_TEXTURE)
         curParams.mVSOutTextureCoordinateType = GpuConstantType::FLOAT3;    
}

//-----------------------------------------------------------------------
auto FFPTexturingFactory::getType() const noexcept -> std::string_view
{
    return FFPTexturing::Type;
}

//-----------------------------------------------------------------------
auto FFPTexturingFactory::createInstance(ScriptCompiler* compiler, 
                                                 PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) noexcept -> SubRenderState*
{
    if (prop->name == "texturing_stage")
    {
        if(prop->values.size() == 1)
        {
            String value;

            if(false == SGScriptTranslator::getString(prop->values.front(), &value))
            {
                compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                return nullptr;
            }

            auto inst = createOrRetrieveInstance(translator);

            if (value == "ffp")
                return inst;

            if (value == "late_add_blend")
                inst->setParameter(value, "true");

            return inst;
        }       
    }

    return nullptr;
}

//-----------------------------------------------------------------------
void FFPTexturingFactory::writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, 
                                     Pass* srcPass, Pass* dstPass)
{
    ser->writeAttribute(4, "texturing_stage");
    ser->writeValue("ffp");
}

//-----------------------------------------------------------------------
auto FFPTexturingFactory::createInstanceImpl() -> SubRenderState*
{
    return new FFPTexturing;
}


}
