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

    const String& getType() const noexcept override;

    int getExecutionOrder() const noexcept override { return FFP_LIGHTING; }

    void copyFrom(const SubRenderState& rhs) override;

    bool preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept override;

    // Type of this render state.
    static String Type;

    /**
    Return the metallic-roughness map texture name.
    */
    const String& getMetalRoughnessMapName() const noexcept { return mMetalRoughnessMapName; }

    bool setParameter(const String& name, const String& value) noexcept override;

    bool createCpuSubPrograms(ProgramSet* programSet) override;

private:
    String mMetalRoughnessMapName;
    int mLightCount{0};
    uint8 mMRMapSamplerIndex{0};
};

class CookTorranceLightingFactory : public SubRenderStateFactory
{
public:
    [[nodiscard]] const String& getType() const noexcept override;

    SubRenderState* createInstance(ScriptCompiler* compiler, PropertyAbstractNode* prop, Pass* pass,
                                   SGScriptTranslator* translator) noexcept override;
    void writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, Pass* srcPass, Pass* dstPass) override;

protected:
    SubRenderState* createInstanceImpl() override;
};

/** @} */
/** @} */

} // namespace RTShader
} // namespace Ogre

#endif
