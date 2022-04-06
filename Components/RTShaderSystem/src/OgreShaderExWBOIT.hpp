// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_WBOIT_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_WBOIT_H

#include "OgrePrerequisites.hpp"
#include "OgreShaderSubRenderState.hpp"

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

namespace Ogre
{
namespace RTShader
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

} // namespace RTShader
} // namespace Ogre

#endif
