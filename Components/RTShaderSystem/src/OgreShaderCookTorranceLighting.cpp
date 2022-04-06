// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
#include <cstddef>
#include <list>
#include <memory>
#include <string>

#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgreMaterialSerializer.hpp"
#include "OgrePass.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreShaderCookTorranceLighting.hpp"
#include "OgreShaderFFPRenderState.hpp"
#include "OgreShaderFunction.hpp"
#include "OgreShaderFunctionAtom.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrecompiledHeaders.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderProgram.hpp"
#include "OgreShaderProgramSet.hpp"
#include "OgreShaderRenderState.hpp"
#include "OgreShaderScriptTranslator.hpp"
#include "OgreShaderSubRenderState.hpp"
#include "OgreVector.hpp"

namespace Ogre
{
namespace RTShader
{

/************************************************************************/
/*                                                                      */
/************************************************************************/
String CookTorranceLighting::Type = "CookTorranceLighting";

//-----------------------------------------------------------------------
CookTorranceLighting::CookTorranceLighting()  = default;

//-----------------------------------------------------------------------
auto CookTorranceLighting::getType() const -> const String& { return Type; }
//-----------------------------------------------------------------------
auto CookTorranceLighting::createCpuSubPrograms(ProgramSet* programSet) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GPT_VERTEX_PROGRAM);
    Function* vsMain = vsProgram->getEntryPointFunction();
    Program* psProgram = programSet->getCpuProgram(GPT_FRAGMENT_PROGRAM);
    Function* psMain = psProgram->getEntryPointFunction();

    vsProgram->addDependency(FFP_LIB_TRANSFORM);

    psProgram->addDependency(FFP_LIB_TRANSFORM);
    psProgram->addDependency(FFP_LIB_TEXTURING);
    psProgram->addDependency("SGXLib_CookTorrance");

    // Resolve texture coordinates.
    auto vsInTexcoord = vsMain->resolveInputParameter(Parameter::SPC_TEXTURE_COORDINATE0, GCT_FLOAT2);
    auto vsOutTexcoord = vsMain->resolveOutputParameter(Parameter::SPC_TEXTURE_COORDINATE0, GCT_FLOAT2);
    auto psInTexcoord = psMain->resolveInputParameter(vsOutTexcoord);

    // resolve view position
    auto vsInPosition = vsMain->getLocalParameter(Parameter::SPC_POSITION_OBJECT_SPACE);
    if (!vsInPosition)
        vsInPosition = vsMain->resolveInputParameter(Parameter::SPC_POSITION_OBJECT_SPACE);
    auto vsOutViewPos = vsMain->resolveOutputParameter(Parameter::SPC_POSITION_VIEW_SPACE);
    auto viewPos = psMain->resolveInputParameter(vsOutViewPos);
    auto worldViewMatrix = vsProgram->resolveParameter(GpuProgramParameters::ACT_WORLDVIEW_MATRIX);

    // Resolve normal.
    auto viewNormal = psMain->getLocalParameter(Parameter::SPC_NORMAL_VIEW_SPACE);
    ParameterPtr vsInNormal, vsOutNormal;

    if (!viewNormal)
    {
        // Resolve input vertex shader normal.
        vsInNormal = vsMain->resolveInputParameter(Parameter::SPC_NORMAL_OBJECT_SPACE);

        // Resolve output vertex shader normal.
        vsOutNormal = vsMain->resolveOutputParameter(Parameter::SPC_NORMAL_VIEW_SPACE);

        // Resolve input pixel shader normal.
        viewNormal = psMain->resolveInputParameter(vsOutNormal);
    }

    // resolve light params
    auto outDiffuse = psMain->resolveOutputParameter(Parameter::SPC_COLOR_DIFFUSE);
    auto outSpecular = psMain->resolveLocalParameter(Parameter::SPC_COLOR_SPECULAR);

    // insert after texturing
    auto vstage = vsMain->getStage(FFP_PS_COLOUR_BEGIN + 1);
    auto fstage = psMain->getStage(FFP_PS_COLOUR_END + 50);

    // Forward texture coordinates
    vstage.assign(vsInTexcoord, vsOutTexcoord);
    vstage.callFunction(FFP_FUNC_TRANSFORM, worldViewMatrix, vsInPosition, vsOutViewPos);

    // transform normal in VS
    if (vsOutNormal)
    {
        auto worldViewITMatrix = vsProgram->resolveParameter(GpuProgramParameters::ACT_NORMAL_MATRIX);
        vstage.callFunction(FFP_FUNC_TRANSFORM, worldViewITMatrix, vsInNormal, vsOutNormal);
    }

    // add the lighting computation
    In mrparams(ParameterPtr(nullptr));
    if(!mMetalRoughnessMapName.empty())
    {
        auto metalRoughnessSampler =
            psProgram->resolveParameter(GCT_SAMPLER2D, "metalRoughnessSampler", mMRMapSamplerIndex);
        auto mrSample = psMain->resolveLocalParameter(GCT_FLOAT4, "mrSample");
        // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
        // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
        fstage.sampleTexture(metalRoughnessSampler, psInTexcoord, mrSample);
        mrparams = In(mrSample).mask(Operand::OPM_YZ);
    }
    else
    {
        auto specular = psProgram->resolveParameter(GpuProgramParameters::ACT_SURFACE_SPECULAR_COLOUR);
        mrparams = In(specular).xy();
    }

    auto litResult = psMain->resolveLocalParameter(GCT_FLOAT3, "litResult");
    fstage.assign(Vector3(0), litResult);

    for (int i = 0; i < mLightCount; i++)
    {
        auto lightPos = psProgram->resolveParameter(GpuProgramParameters::ACT_LIGHT_POSITION_VIEW_SPACE, i);
        auto lightDiffuse = psProgram->resolveParameter(GpuProgramParameters::ACT_LIGHT_DIFFUSE_COLOUR, i);
        auto pointParams = psProgram->resolveParameter(GpuProgramParameters::ACT_LIGHT_ATTENUATION, i);
        auto spotParams = psProgram->resolveParameter(GpuProgramParameters::ACT_SPOTLIGHT_PARAMS, i);
        auto lightDirView = psProgram->resolveParameter(GpuProgramParameters::ACT_LIGHT_DIRECTION_VIEW_SPACE, i);

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
auto CookTorranceLighting::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) -> bool
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

auto CookTorranceLighting::setParameter(const String& name, const String& value) -> bool
{
    if (name == "texture")
    {
        mMetalRoughnessMapName = value;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------
auto CookTorranceLightingFactory::getType() const -> const String& { return CookTorranceLighting::Type; }

//-----------------------------------------------------------------------
auto CookTorranceLightingFactory::createInstance(ScriptCompiler* compiler, PropertyAbstractNode* prop,
                                                            Pass* pass, SGScriptTranslator* translator) -> SubRenderState*
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

} // namespace RTShader
} // namespace Ogre
