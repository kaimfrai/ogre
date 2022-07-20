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

import :ShaderExNormalMapLighting;
import :ShaderFFPRenderState;
import :ShaderFunction;
import :ShaderFunctionAtom;
import :ShaderParameter;
import :ShaderPrecompiledHeaders;
import :ShaderProgram;
import :ShaderProgramSet;
import :ShaderScriptTranslator;

import Ogre.Core;

import <list>;
import <memory>;
import <string>;

namespace Ogre::RTShader {
class RenderState;
}  // namespace Ogre

#define SGX_LIB_NORMALMAP                           "SGXLib_NormalMap"
#define SGX_FUNC_CONSTRUCT_TBNMATRIX                "SGX_ConstructTBNMatrix"
#define SGX_FUNC_FETCHNORMAL                        "SGX_FetchNormal"
namespace Ogre::RTShader {

/************************************************************************/
/*                                                                      */
/************************************************************************/
std::string_view const constinit NormalMapLighting::Type = "NormalMap";

//-----------------------------------------------------------------------
NormalMapLighting::NormalMapLighting()
{
    mNormalMapSamplerIndex          = 0;
    mVSTexCoordSetIndex             = 0;
    mNormalMapSpace                 = NormalMapSpace::TANGENT;
    mNormalMapSampler = TextureManager::getSingleton().createSampler();
    mNormalMapSampler->setMipmapBias(-1.0);
}

//-----------------------------------------------------------------------
auto NormalMapLighting::getType() const noexcept -> std::string_view
{
    return Type;
}
//-----------------------------------------------------------------------
auto NormalMapLighting::createCpuSubPrograms(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Function* vsMain = vsProgram->getEntryPointFunction();
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* psMain = psProgram->getEntryPointFunction();

    vsProgram->addDependency(FFP_LIB_TRANSFORM);

    psProgram->addDependency(FFP_LIB_TRANSFORM);
    psProgram->addDependency(FFP_LIB_TEXTURING);
    psProgram->addDependency(SGX_LIB_NORMALMAP);

    // Resolve texture coordinates.
    auto vsInTexcoord = vsMain->resolveInputParameter(
        Parameter::Content(std::to_underlying(Parameter::Content::TEXTURE_COORDINATE0) + mVSTexCoordSetIndex), GpuConstantType::FLOAT2);
    auto vsOutTexcoord = vsMain->resolveOutputParameter(
        Parameter::Content(std::to_underlying(Parameter::Content::TEXTURE_COORDINATE0) + mVSTexCoordSetIndex), GpuConstantType::FLOAT2);
    auto psInTexcoord = psMain->resolveInputParameter(vsOutTexcoord);

    // Resolve normal.
    auto vsInNormal = vsMain->resolveInputParameter(Parameter::Content::NORMAL_OBJECT_SPACE);
    auto vsOutNormal = vsMain->resolveOutputParameter(Parameter::Content::NORMAL_VIEW_SPACE);
    auto viewNormal = psMain->resolveInputParameter(vsOutNormal);
    auto newViewNormal = psMain->resolveLocalParameter(Parameter::Content::NORMAL_VIEW_SPACE);

    // insert before lighting stage
    auto vstage = vsMain->getStage(std::to_underlying(FFPFragmentShaderStage::COLOUR_BEGIN) + 1);
    auto fstage = psMain->getStage(std::to_underlying(FFPFragmentShaderStage::COLOUR_BEGIN) + 1);

    // Output texture coordinates.
    vstage.assign(vsInTexcoord, vsOutTexcoord);

    // Add the normal fetch function invocation
    auto normalMapSampler = psProgram->resolveParameter(GpuConstantType::SAMPLER2D, "gNormalMapSampler", mNormalMapSamplerIndex);
    fstage.callFunction(SGX_FUNC_FETCHNORMAL, normalMapSampler, psInTexcoord, newViewNormal);

    if (!!(mNormalMapSpace & NormalMapSpace::TANGENT))
    {
        auto vsInTangent = vsMain->resolveInputParameter(Parameter::Content::TANGENT_OBJECT_SPACE);
        auto vsOutTangent = vsMain->resolveOutputParameter(Parameter::Content::TANGENT_OBJECT_SPACE);
        auto psInTangent = psMain->resolveInputParameter(vsOutTangent);

        // transform normal & tangent
        auto normalMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::NORMAL_MATRIX);
        vstage.callFunction(FFP_FUNC_TRANSFORM, normalMatrix, vsInNormal, vsOutNormal);
        vstage.callFunction(FFP_FUNC_TRANSFORM, normalMatrix, vsInTangent, vsOutTangent);

        // Construct TBN matrix.
        auto TBNMatrix = psMain->resolveLocalParameter(GpuConstantType::MATRIX_3X3, "lMatTBN");
        fstage.callFunction(SGX_FUNC_CONSTRUCT_TBNMATRIX, viewNormal, psInTangent, TBNMatrix);
        // transform normal
        fstage.callFunction(FFP_FUNC_TRANSFORM, TBNMatrix, newViewNormal, newViewNormal);
    }
    else if (!!(mNormalMapSpace & NormalMapSpace::OBJECT))
    {
        // transform normal in FS
        auto normalMatrix = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::NORMAL_MATRIX);
        fstage.callFunction(FFP_FUNC_TRANSFORM, normalMatrix, newViewNormal, newViewNormal);
    }

    if (mNormalMapSpace == NormalMapSpace::PARALLAX)
    {
        // assuming: lighting stage computed this
        auto vsOutViewPos = vsMain->resolveOutputParameter(Parameter::Content::POSITION_VIEW_SPACE);
        auto viewPos = psMain->resolveInputParameter(vsOutViewPos);

        // TODO: user specificed scale and bias
        fstage.callFunction("SGX_Generate_Parallax_Texcoord", {In(normalMapSampler), In(psInTexcoord), In(viewPos),
                                                              In(Vector2{0.04, -0.02}), Out(psInTexcoord)});

        // overwrite texcoord0 unconditionally, only one texcoord set is supported with parallax mapping
        // we are before FFPFragmentShaderStage::TEXTURING, so the new value will be used
        auto texcoord0 = psMain->resolveInputParameter(Parameter::Content::TEXTURE_COORDINATE0, GpuConstantType::FLOAT2);
        fstage.assign(psInTexcoord, texcoord0);
    }

    return true;
}

//-----------------------------------------------------------------------
void NormalMapLighting::copyFrom(const SubRenderState& rhs)
{
    const auto& rhsLighting = static_cast<const NormalMapLighting&>(rhs);

    mNormalMapSpace = rhsLighting.mNormalMapSpace;
    mNormalMapTextureName = rhsLighting.mNormalMapTextureName;
    mNormalMapSampler = rhsLighting.mNormalMapSampler;
}

//-----------------------------------------------------------------------
auto NormalMapLighting::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept -> bool
{
    TextureUnitState* normalMapTexture = dstPass->createTextureUnitState();

    normalMapTexture->setTextureName(mNormalMapTextureName);
    normalMapTexture->setSampler(mNormalMapSampler);
    mNormalMapSamplerIndex = dstPass->getNumTextureUnitStates() - 1;

    return true;
}

auto NormalMapLighting::setParameter(std::string_view name, std::string_view value) noexcept -> bool
{
	if(name == "normalmap_space")
	{
        // Normal map defines normals in tangent space.
        if (value == "tangent_space")
        {
            setNormalMapSpace(NormalMapSpace::TANGENT);
            return true;
        }
        // Normal map defines normals in object space.
        if (value == "object_space")
        {
            setNormalMapSpace(NormalMapSpace::OBJECT);
            return true;
        }
        if (value == "parallax")
        {
            setNormalMapSpace(NormalMapSpace::PARALLAX);
            return true;
        }
		return false;
	}

	if(name == "texture")
	{
		mNormalMapTextureName = value;
		return true;
	}

	if(name == "texcoord_index")
	{
		setTexCoordIndex(StringConverter::parseInt(value));
		return true;
	}

    if(name == "sampler")
    {
        auto sampler = TextureManager::getSingleton().getSampler(value);
        if(!sampler)
            return false;
        mNormalMapSampler = sampler;
        return true;
    }

	return false;
}

//-----------------------------------------------------------------------
auto NormalMapLightingFactory::getType() const noexcept -> std::string_view
{
    return NormalMapLighting::Type;
}

//-----------------------------------------------------------------------
auto NormalMapLightingFactory::createInstance(ScriptCompiler* compiler, 
                                                        PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) noexcept -> SubRenderState*
{
    if (prop->name == "lighting_stage")
    {
        if(prop->values.size() >= 2)
        {
            String strValue;
            auto it = prop->values.begin();
            
            // Read light model type.
            if(false == SGScriptTranslator::getString(*it, &strValue))
            {
                compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                return nullptr;
            }

            // Case light model type is normal map
            if (strValue == "normal_map")
            {
                ++it;
                if (false == SGScriptTranslator::getString(*it, &strValue))
                {
                    compiler->addError(ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
                    return nullptr;
                }

                
                SubRenderState* subRenderState = createOrRetrieveInstance(translator);
                auto* normalMapSubRenderState = static_cast<NormalMapLighting*>(subRenderState);
                
                normalMapSubRenderState->setParameter("texture", strValue);

                
                // Read normal map space type.
                if (prop->values.size() >= 3)
                {                   
                    ++it;
                    if (false == SGScriptTranslator::getString(*it, &strValue))
                    {
                        compiler->addError(ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
                        return nullptr;
                    }

                    if(!normalMapSubRenderState->setParameter("normalmap_space", strValue))
                    {
                        compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                        return nullptr;
                    }
                }

                // Read texture coordinate index.
                if (prop->values.size() >= 4)
                {   
                    unsigned int textureCoordinateIndex = 0;

                    ++it;
                    if (SGScriptTranslator::getUInt(*it, &textureCoordinateIndex))
                    {
                        normalMapSubRenderState->setTexCoordIndex(textureCoordinateIndex);
                    }
                }

                // Read texture filtering format.
                if (prop->values.size() >= 5)
                {
                    ++it;
                    if (false == SGScriptTranslator::getString(*it, &strValue))
                    {
                        compiler->addError(ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
                        return nullptr;
                    }

                    // sampler reference
                    if(!normalMapSubRenderState->setParameter("sampler", strValue))
                    {
                        compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                        return nullptr;
                    }
                }
                                
                return subRenderState;                              
            }
        }
    }

    return nullptr;
}

//-----------------------------------------------------------------------
void NormalMapLightingFactory::writeInstance(MaterialSerializer* ser, 
                                             SubRenderState* subRenderState, 
                                             Pass* srcPass, Pass* dstPass)
{
    auto* normalMapSubRenderState = static_cast<NormalMapLighting*>(subRenderState);

    ser->writeAttribute(4, "lighting_stage");
    ser->writeValue("normal_map");
    ser->writeValue(normalMapSubRenderState->getNormalMapTextureName());    
    
    if (normalMapSubRenderState->getNormalMapSpace() == NormalMapLighting::NormalMapSpace::TANGENT)
    {
        ser->writeValue("tangent_space");
    }
    else if (normalMapSubRenderState->getNormalMapSpace() == NormalMapLighting::NormalMapSpace::OBJECT)
    {
        ser->writeValue("object_space");
    }

    ser->writeValue(StringConverter::toString(normalMapSubRenderState->getTexCoordIndex()));
}

//-----------------------------------------------------------------------
auto NormalMapLightingFactory::createInstanceImpl() -> SubRenderState*
{
    return new NormalMapLighting;
}

}
