// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
module;

#include <cstddef>

module Ogre.Components.RTShaderSystem;

import :ShaderCookTorranceLighting;
import :ShaderFFPRenderState;
import :ShaderFunction;
import :ShaderFunctionAtom;
import :ShaderParameter;
import :ShaderPrecompiledHeaders;
import :ShaderPrerequisites;
import :ShaderProgram;
import :ShaderProgramSet;
import :ShaderRenderState;
import :ShaderScriptTranslator;
import :ShaderSubRenderState;

import Ogre.Core;

import <list>;
import <memory>;
import <string>;

namespace Ogre::RTShader
{

/************************************************************************/
/*                                                                      */
/************************************************************************/
std::string_view const constinit CookTorranceLighting::Type = "CookTorranceLighting";

//-----------------------------------------------------------------------
CookTorranceLighting::CookTorranceLighting()  = default;

//-----------------------------------------------------------------------
auto CookTorranceLighting::getType() const noexcept -> std::string_view { return Type; }
//-----------------------------------------------------------------------
auto CookTorranceLighting::createCpuSubPrograms(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Function* vsMain = vsProgram->getEntryPointFunction();
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* psMain = psProgram->getEntryPointFunction();

    vsProgram->addDependency(FFP_LIB_TRANSFORM);

    psProgram->addDependency(FFP_LIB_TRANSFORM);
    psProgram->addDependency(FFP_LIB_TEXTURING);
    psProgram->addDependency("SGXLib_CookTorrance");

    // Resolve texture coordinates.
    auto vsInTexcoord = vsMain->resolveInputParameter(Parameter::Content::TEXTURE_COORDINATE0, GpuConstantType::FLOAT2);
    auto vsOutTexcoord = vsMain->resolveOutputParameter(Parameter::Content::TEXTURE_COORDINATE0, GpuConstantType::FLOAT2);
    auto psInTexcoord = psMain->resolveInputParameter(vsOutTexcoord);

    // resolve view position
    auto vsInPosition = vsMain->getLocalParameter(Parameter::Content::POSITION_OBJECT_SPACE);
    if (!vsInPosition)
        vsInPosition = vsMain->resolveInputParameter(Parameter::Content::POSITION_OBJECT_SPACE);
    auto vsOutViewPos = vsMain->resolveOutputParameter(Parameter::Content::POSITION_VIEW_SPACE);
    auto viewPos = psMain->resolveInputParameter(vsOutViewPos);
    auto worldViewMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::WORLDVIEW_MATRIX);

    // Resolve normal.
    auto viewNormal = psMain->getLocalParameter(Parameter::Content::NORMAL_VIEW_SPACE);
    ParameterPtr vsInNormal, vsOutNormal;

    if (!viewNormal)
    {
        // Resolve input vertex shader normal.
        vsInNormal = vsMain->resolveInputParameter(Parameter::Content::NORMAL_OBJECT_SPACE);

        // Resolve output vertex shader normal.
        vsOutNormal = vsMain->resolveOutputParameter(Parameter::Content::NORMAL_VIEW_SPACE);

        // Resolve input pixel shader normal.
        viewNormal = psMain->resolveInputParameter(vsOutNormal);
    }

    // resolve light params
    auto outDiffuse = psMain->resolveOutputParameter(Parameter::Content::COLOR_DIFFUSE);
    auto outSpecular = psMain->resolveLocalParameter(Parameter::Content::COLOR_SPECULAR);

    // insert after texturing
    auto vstage = vsMain->getStage(std::to_underlying(FFPFragmentShaderStage::COLOUR_BEGIN) + 1);
    auto fstage = psMain->getStage(std::to_underlying(FFPFragmentShaderStage::COLOUR_END) + 50);

    // Forward texture coordinates
    vstage.assign(vsInTexcoord, vsOutTexcoord);
    vstage.callFunction(FFP_FUNC_TRANSFORM, worldViewMatrix, vsInPosition, vsOutViewPos);

    // transform normal in VS
    if (vsOutNormal)
    {
        auto worldViewITMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::NORMAL_MATRIX);
        vstage.callFunction(FFP_FUNC_TRANSFORM, worldViewITMatrix, vsInNormal, vsOutNormal);
    }

    // add the lighting computation
    In mrparams(ParameterPtr(nullptr));
    if(!mMetalRoughnessMapName.empty())
    {
        auto metalRoughnessSampler =
            psProgram->resolveParameter(GpuConstantType::SAMPLER2D, "metalRoughnessSampler", mMRMapSamplerIndex);
        auto mrSample = psMain->resolveLocalParameter(GpuConstantType::FLOAT4, "mrSample");
        // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
        // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
        fstage.sampleTexture(metalRoughnessSampler, psInTexcoord, mrSample);
        mrparams = In(mrSample).mask(Operand::OpMask::YZ);
    }
    else
    {
        auto specular = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::SURFACE_SPECULAR_COLOUR);
        mrparams = In(specular).xy();
    }

    auto litResult = psMain->resolveLocalParameter(GpuConstantType::FLOAT3, "litResult");
    fstage.assign(Vector3{}, litResult);

    for (int i = 0; i < mLightCount; i++)
    {
        auto lightPos = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_POSITION_VIEW_SPACE, i);
        auto lightDiffuse = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_DIFFUSE_COLOUR, i);
        auto pointParams = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_ATTENUATION, i);
        auto spotParams = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::SPOTLIGHT_PARAMS, i);
        auto lightDirView = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::LIGHT_DIRECTION_VIEW_SPACE, i);

        fstage.callFunction("PBR_Light", {In(viewNormal), In(viewPos), In(lightPos), In(lightDiffuse).xyz(),
                                          In(pointParams), In(lightDirView), In(spotParams), In(outDiffuse).xyz(),
                                          mrparams, InOut(litResult)});
    }

    fstage.assign(litResult, Out(outDiffuse).xyz());

    return true;
}

//-----------------------------------------------------------------------
void CookTorranceLighting::copyFrom(const SubRenderState& rhs)
{
    const auto& rhsLighting = static_cast<const CookTorranceLighting&>(rhs);
    mMetalRoughnessMapName = rhsLighting.mMetalRoughnessMapName;
    mLightCount = rhsLighting.mLightCount;
}

//-----------------------------------------------------------------------
auto CookTorranceLighting::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept -> bool
{
    if (!srcPass->getLightingEnabled())
        return false;

    auto lightsPerType = renderState->getLightCount();
    mLightCount = lightsPerType[0] + lightsPerType[1] + lightsPerType[2];

    if(mMetalRoughnessMapName.empty())
        return true;

    dstPass->createTextureUnitState(mMetalRoughnessMapName);
    mMRMapSamplerIndex = dstPass->getNumTextureUnitStates() - 1;

    return true;
}

auto CookTorranceLighting::setParameter(std::string_view name, std::string_view value) noexcept -> bool
{
    if (name == "texture")
    {
        mMetalRoughnessMapName = value;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------
auto CookTorranceLightingFactory::getType() const noexcept -> std::string_view { return CookTorranceLighting::Type; }

//-----------------------------------------------------------------------
auto CookTorranceLightingFactory::createInstance(ScriptCompiler* compiler, PropertyAbstractNode* prop,
                                                            Pass* pass, SGScriptTranslator* translator) noexcept -> SubRenderState*
{
    if (prop->name == "lighting_stage" && prop->values.size() >= 1)
    {
        String strValue;
        auto it = prop->values.begin();

        // Read light model type.
        if (!SGScriptTranslator::getString(*it++, &strValue))
        {
            compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
            return nullptr;
        }

        if(strValue != "metal_roughness")
            return nullptr;

        auto subRenderState = createOrRetrieveInstance(translator);

        if(prop->values.size() == 1)
            return subRenderState;

        if (!SGScriptTranslator::getString(*it++, &strValue) || strValue != "texture")
        {
            compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
            return subRenderState;
        }

        if (false == SGScriptTranslator::getString(*it, &strValue))
        {
            compiler->addError(ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
            return subRenderState;
        }

        subRenderState->setParameter("texture", strValue);
        return subRenderState;
    }

    return nullptr;
}

//-----------------------------------------------------------------------
void CookTorranceLightingFactory::writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, Pass* srcPass,
                                                Pass* dstPass)
{
    auto ctSubRenderState = static_cast<CookTorranceLighting*>(subRenderState);

    ser->writeAttribute(4, "lighting_stage");
    ser->writeValue("metal_roughness");
    if(ctSubRenderState->getMetalRoughnessMapName().empty())
        return;
    ser->writeValue("texture");
    ser->writeValue(ctSubRenderState->getMetalRoughnessMapName());
}

//-----------------------------------------------------------------------
auto CookTorranceLightingFactory::createInstanceImpl() -> SubRenderState* { return new CookTorranceLighting; }

} // namespace Ogre
