// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_COOKTORRANCELIGHTING_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_COOKTORRANCELIGHTING_H

#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderFFPRenderState.hpp"
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

class CookTorranceLighting : public SubRenderState
{
public:
    CookTorranceLighting();

    auto getType() const -> const String& override;

    auto getExecutionOrder() const -> int { return FFP_LIGHTING; }

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
    int mLightCount;
    uint8 mMRMapSamplerIndex;
};

class CookTorranceLightingFactory : public SubRenderStateFactory
{
public:
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
