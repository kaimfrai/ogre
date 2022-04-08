// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
module Ogre.Components.RTShaderSystem:ShaderCookTorranceLighting;

import :ShaderFFPRenderState;
import :ShaderSubRenderState;

import Ogre.Core;

namespace Ogre::RTShader
{
    class ProgramSet;
    class RenderState;
    class SGScriptTranslator;
/** \addtogroup Optional
 *  @{
 */
/** \addtogroup RTShader
 *  @{
 */

class CookTorranceLighting : public SubRenderState
{
public:
    CookTorranceLighting();

    auto getType() const -> const String& override;

    auto getExecutionOrder() const -> int override { return FFP_LIGHTING; }

    void copyFrom(const SubRenderState& rhs) override;

    auto preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) -> bool override;

    // Type of this render state.
    static String Type;

    /**
    Return the metallic-roughness map texture name.
    */
    auto getMetalRoughnessMapName() const -> const String& { return mMetalRoughnessMapName; }

    auto setParameter(const String& name, const String& value) -> bool override;

    auto createCpuSubPrograms(ProgramSet* programSet) -> bool override;

private:
    String mMetalRoughnessMapName;
    int mLightCount{0};
    uint8 mMRMapSamplerIndex{0};
};

class CookTorranceLightingFactory : public SubRenderStateFactory
{
public:
    [[nodiscard]]
    auto getType() const -> const String& override;

    auto createInstance(ScriptCompiler* compiler, PropertyAbstractNode* prop, Pass* pass,
                                   SGScriptTranslator* translator) -> SubRenderState* override;
    void writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, Pass* srcPass, Pass* dstPass) override;

protected:
    auto createInstanceImpl() -> SubRenderState* override;
};

/** @} */
/** @} */

} // namespace Ogre
