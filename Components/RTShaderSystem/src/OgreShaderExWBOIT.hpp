// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
module Ogre.Components.RTShaderSystem:ShaderExWBOIT;

import :ShaderSubRenderState;

import Ogre.Core;

namespace Ogre {
    class MaterialSerializer;
    class Pass;
    class PropertyAbstractNode;
    class ScriptCompiler;
    namespace RTShader {
        class ProgramSet;
        class RenderState;
        class SGScriptTranslator;
    }  // namespace RTShader
}  // namespace Ogre

namespace Ogre::RTShader
{

/** \addtogroup Optional
 *  @{
 */
/** \addtogroup RTShader
 *  @{
 */

/** Transform sub render state implementation of writing to WBOIT buffers
 */
class WBOIT : public SubRenderState
{
public:
    auto getType() const noexcept -> std::string_view override;
    auto getExecutionOrder() const noexcept -> FFPShaderStage override;
    auto preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept -> bool override;
    auto createCpuSubPrograms(ProgramSet* programSet) -> bool override;
    void copyFrom(const SubRenderState& rhs) override {}

    static std::string_view const Type;
};

/**
A factory that enables creation of GBuffer instances.
@remarks Sub class of SubRenderStateFactory
*/
class WBOITFactory : public SubRenderStateFactory
{
public:
    [[nodiscard]] auto getType() const noexcept -> std::string_view override;

    auto createInstance(ScriptCompiler* compiler, PropertyAbstractNode* prop, Pass* pass,
                                           SGScriptTranslator* translator) noexcept -> SubRenderState* override;

    void writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, Pass* srcPass, Pass* dstPass) override;

protected:
    auto createInstanceImpl() -> SubRenderState* override;
};

/** @} */
/** @} */

} // namespace Ogre
