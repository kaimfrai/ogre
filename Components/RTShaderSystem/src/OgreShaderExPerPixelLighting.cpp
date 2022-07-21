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

import :ShaderExPerPixelLighting;
import :ShaderFFPLighting;
import :ShaderFFPRenderState;
import :ShaderFunction;
import :ShaderFunctionAtom;
import :ShaderParameter;
import :ShaderPrecompiledHeaders;
import :ShaderPrerequisites;
import :ShaderProgram;
import :ShaderProgramSet;
import :ShaderScriptTranslator;
import :ShaderSubRenderState;

import Ogre.Core;

import <list>;
import <memory>;
import <string>;
import <vector>;

namespace Ogre {
    class Pass;
}  // namespace Ogre

namespace Ogre::RTShader {

/************************************************************************/
/*                                                                      */
/************************************************************************/
//-----------------------------------------------------------------------
auto PerPixelLighting::getType() const noexcept -> std::string_view
{
    return Type;
}

auto PerPixelLighting::setParameter(std::string_view name, std::string_view value) noexcept -> bool
{
	if(name == "two_sided")
	{
		return StringConverter::parse(value, mTwoSidedLighting);
	}

	return FFPLighting::setParameter(name, value);
}

//-----------------------------------------------------------------------
auto PerPixelLighting::resolveParameters(ProgramSet* programSet) -> bool
{
    if (false == resolveGlobalParameters(programSet))
        return false;
    
    if (false == resolvePerLightParameters(programSet))
        return false;
    
    return true;
}

//-----------------------------------------------------------------------
auto PerPixelLighting::resolveGlobalParameters(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* vsMain = vsProgram->getEntryPointFunction();
    Function* psMain = psProgram->getEntryPointFunction();

    // Resolve world view IT matrix.
    mWorldViewITMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::NORMAL_MATRIX);

    // Get surface ambient colour if need to.
    if ((mTrackVertexColourType & TrackVertexColourEnum::AMBIENT) == TrackVertexColourEnum{})
    {       
        mDerivedAmbientLightColour = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::DERIVED_AMBIENT_LIGHT_COLOUR);
    }
    else
    {
        mLightAmbientColour = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::AMBIENT_LIGHT_COLOUR);
    }

    // Get surface emissive colour if need to.
    if ((mTrackVertexColourType & TrackVertexColourEnum::EMISSIVE) == TrackVertexColourEnum{})
    {
        mSurfaceEmissiveColour = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::SURFACE_EMISSIVE_COLOUR);
    }

    // Get derived scene colour.
    mDerivedSceneColour = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::DERIVED_SCENE_COLOUR);

    // Get surface shininess.
    mSurfaceShininess = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::SURFACE_SHININESS);

    mViewNormal = psMain->getLocalParameter(Parameter::Content::NORMAL_VIEW_SPACE);

    if(!mViewNormal)
    {
        // Resolve input vertex shader normal.
        mVSInNormal = vsMain->resolveInputParameter(Parameter::Content::NORMAL_OBJECT_SPACE);

        // Resolve output vertex shader normal.
        mVSOutNormal = vsMain->resolveOutputParameter(Parameter::Content::NORMAL_VIEW_SPACE);

        // Resolve input pixel shader normal.
        mViewNormal = psMain->resolveInputParameter(mVSOutNormal);
    }

    mInDiffuse = psMain->getInputParameter(Parameter::Content::COLOR_DIFFUSE);
    if (mInDiffuse.get() == nullptr)
    {
        mInDiffuse = psMain->getLocalParameter(Parameter::Content::COLOR_DIFFUSE);
    }

    OgreAssert(mInDiffuse, "mInDiffuse is NULL");

    mOutDiffuse = psMain->resolveOutputParameter(Parameter::Content::COLOR_DIFFUSE);

    if (mSpecularEnable)
    {
        mOutSpecular = psMain->resolveLocalParameter(Parameter::Content::COLOR_SPECULAR);

        mVSInPosition = vsMain->getLocalParameter(Parameter::Content::POSITION_OBJECT_SPACE);
        if(!mVSInPosition)
            mVSInPosition = vsMain->resolveInputParameter(Parameter::Content::POSITION_OBJECT_SPACE);

        mVSOutViewPos = vsMain->resolveOutputParameter(Parameter::Content::POSITION_VIEW_SPACE);

        mViewPos = psMain->resolveInputParameter(mVSOutViewPos);

        mWorldViewMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::WORLDVIEW_MATRIX);
    }

    return true;
}

//-----------------------------------------------------------------------
auto PerPixelLighting::resolvePerLightParameters(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* vsMain = vsProgram->getEntryPointFunction();
    Function* psMain = psProgram->getEntryPointFunction();

    bool needViewPos = false;

    // Resolve per light parameters.
    for (unsigned int i=0; i < mLightParamsList.size(); ++i)
    {       
        switch (mLightParamsList[i].mType)
        {
            using enum Light::LightTypes;
        case DIRECTIONAL:
            mLightParamsList[i].mDirection = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_DIRECTION_VIEW_SPACE, i);
            mLightParamsList[i].mPSInDirection = mLightParamsList[i].mDirection;
            needViewPos = mSpecularEnable || needViewPos;
            break;

        case POINT:
            mLightParamsList[i].mPosition = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_POSITION_VIEW_SPACE, i);
            mLightParamsList[i].mAttenuatParams = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_ATTENUATION, i);
            
            needViewPos = true;
            break;

        case SPOTLIGHT:
            mLightParamsList[i].mPosition = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_POSITION_VIEW_SPACE, i);
            mLightParamsList[i].mDirection = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_DIRECTION_VIEW_SPACE, i);
            mLightParamsList[i].mPSInDirection = mLightParamsList[i].mDirection;
            mLightParamsList[i].mAttenuatParams = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_ATTENUATION, i);
            mLightParamsList[i].mSpotParams = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::SPOTLIGHT_PARAMS, i);

            needViewPos = true;
            break;
        }

        // Resolve diffuse colour.
        if ((mTrackVertexColourType & TrackVertexColourEnum::DIFFUSE) == TrackVertexColourEnum{})
        {
            mLightParamsList[i].mDiffuseColour = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::DERIVED_LIGHT_DIFFUSE_COLOUR, i);
        }
        else
        {
            mLightParamsList[i].mDiffuseColour = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_DIFFUSE_COLOUR_POWER_SCALED, i);
        }   

        if (mSpecularEnable)
        {
            // Resolve specular colour.
            if ((mTrackVertexColourType & TrackVertexColourEnum::SPECULAR) == TrackVertexColourEnum{})
            {
                mLightParamsList[i].mSpecularColour = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::DERIVED_LIGHT_SPECULAR_COLOUR, i);
            }
            else
            {
                mLightParamsList[i].mSpecularColour = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_SPECULAR_COLOUR_POWER_SCALED, i);
            }   
        }       
    }

    if (needViewPos)
    {
        mWorldViewMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::WORLDVIEW_MATRIX);
        if(!mVSInPosition)
            mVSInPosition = vsMain->resolveInputParameter(Parameter::Content::POSITION_OBJECT_SPACE);
        mVSOutViewPos = vsMain->resolveOutputParameter(Parameter::Content::POSITION_VIEW_SPACE);

        mViewPos = psMain->resolveInputParameter(mVSOutViewPos);
    }

    if(mTwoSidedLighting)
    {
        mFrontFacing = psMain->resolveInputParameter(Parameter::Content::FRONT_FACING);
        mTargetFlipped = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::RENDER_TARGET_FLIPPING);
    }

    return true;
}

//-----------------------------------------------------------------------
auto PerPixelLighting::resolveDependencies(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);

    vsProgram->addDependency(FFP_LIB_TRANSFORM);
    vsProgram->addDependency(SGX_LIB_PERPIXELLIGHTING);

    psProgram->addDependency(SGX_LIB_PERPIXELLIGHTING);

    if(mNormalisedEnable)
        psProgram->addPreprocessorDefines("NORMALISED");

    return true;
}

//-----------------------------------------------------------------------
auto PerPixelLighting::addFunctionInvocations(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM); 
    Function* vsMain = vsProgram->getEntryPointFunction();  
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* psMain = psProgram->getEntryPointFunction();  

    // Add the global illumination functions.
    addVSInvocation(vsMain->getStage(std::to_underlying(FFPVertexShaderStage::LIGHTING)));

    auto stage = psMain->getStage(std::to_underlying(FFPFragmentShaderStage::COLOUR_BEGIN) + 1);
    // Add the global illumination functions.
    addPSGlobalIlluminationInvocation(stage);

    if(mFrontFacing)
        stage.callFunction("SGX_Flip_Backface_Normal", mFrontFacing, mTargetFlipped, mViewNormal);

    // Add per light functions.
    for (const auto& lp : mLightParamsList)
    {
        addIlluminationInvocation(&lp, stage);
    }

    // Assign back temporary variables
    stage.assign(mOutDiffuse, mInDiffuse);

    return true;
}

//-----------------------------------------------------------------------
void PerPixelLighting::addVSInvocation(const FunctionStageRef& stage)
{
    // Transform normal in view space.
    if(!mLightParamsList.empty() && mVSInNormal)
        stage.callFunction(FFP_FUNC_TRANSFORM, mWorldViewITMatrix, mVSInNormal, mVSOutNormal);

    // Transform view space position if need to.
    if (mVSOutViewPos)
    {
        stage.callFunction(FFP_FUNC_TRANSFORM, mWorldViewMatrix, mVSInPosition, mVSOutViewPos);
    }
}


//-----------------------------------------------------------------------
void PerPixelLighting::addPSGlobalIlluminationInvocation(const FunctionStageRef& stage)
{
    if ((mTrackVertexColourType & TrackVertexColourEnum::AMBIENT) == TrackVertexColourEnum{} &&
        (mTrackVertexColourType & TrackVertexColourEnum::EMISSIVE) == TrackVertexColourEnum{})
    {
        stage.assign(mDerivedSceneColour, mOutDiffuse);
    }
    else
    {
        if (!!(mTrackVertexColourType & TrackVertexColourEnum::AMBIENT))
        {
            stage.mul(mLightAmbientColour, mInDiffuse, mOutDiffuse);
        }
        else
        {
            stage.assign(In(mDerivedAmbientLightColour).xyz(), Out(mOutDiffuse).xyz());
        }

        if (!!(mTrackVertexColourType & TrackVertexColourEnum::EMISSIVE))
        {
            stage.add(mInDiffuse, mOutDiffuse, mOutDiffuse);
        }
        else
        {
            stage.add(mSurfaceEmissiveColour, mOutDiffuse, mOutDiffuse);
        }       
    }
}

//-----------------------------------------------------------------------
auto PerPixelLightingFactory::getType() const noexcept -> std::string_view
{
    return PerPixelLighting::Type;
}

//-----------------------------------------------------------------------
auto PerPixelLightingFactory::createInstance(ScriptCompiler* compiler, 
                                                        PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) noexcept -> SubRenderState*
{
    if (prop->name != "lighting_stage" || prop->values.empty())
        return nullptr;

    auto it = prop->values.begin();
    String val;

    if(!SGScriptTranslator::getString(*it++, &val))
    {
        compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
        return nullptr;
    }

    if (val != "per_pixel")
        return nullptr;

    auto ret = createOrRetrieveInstance(translator);

    // process the flags
    while(it != prop->values.end())
    {
        if (!SGScriptTranslator::getString(*it++, &val) || !ret->setParameter(val, "true"))
        {
            compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line, val);
        }
    }

    return ret;
}

//-----------------------------------------------------------------------
void PerPixelLightingFactory::writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, 
                                            Pass* srcPass, Pass* dstPass)
{
    ser->writeAttribute(4, "lighting_stage");
    ser->writeValue("per_pixel");
}

//-----------------------------------------------------------------------
auto PerPixelLightingFactory::createInstanceImpl() -> SubRenderState*
{
    return new PerPixelLighting;
}

}
