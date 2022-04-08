// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
module Ogre.Components.RTShaderSystem:ShaderExWBOIT;

import :ShaderSubRenderState;

import Ogre.Core;

namespace Ogre::RTShader {
    class ProgramSet;
    class RenderState;
    class SGScriptTranslator;
}  // namespace RTShader

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
    auto getType() const -> const String& override;
    auto getExecutionOrder() const -> int override;
    auto preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) -> bool override;
    auto createCpuSubPrograms(ProgramSet* programSet) -> bool override;
    void copyFrom(const SubRenderState& rhs) override {}

    static String Type;
};

/**
A factory that enables creation of GBuffer instances.
@remarks Sub class of SubRenderStateFactory
*/
class WBOITFactory : public SubRenderStateFactory
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
