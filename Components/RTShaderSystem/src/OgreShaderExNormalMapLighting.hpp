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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_EXNORMALMAPLIGHTING_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_EXNORMALMAPLIGHTING_H

#include "OgrePrerequisites.hpp"
#include "OgreShaderFFPRenderState.hpp"
#include "OgreShaderSubRenderState.hpp"
#include "OgreTextureUnitState.hpp"

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

namespace Ogre::RTShader {

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** Normal Map Lighting extension sub render state implementation.
Derives from SubRenderState class.
*/
class NormalMapLighting : public SubRenderState
{

// Interface.
public:
    /** Class default constructor */    
    NormalMapLighting();

    /** 
    @see SubRenderState::getType.
    */
    auto getType() const noexcept -> std::string_view override;

    auto getExecutionOrder() const noexcept -> int override { return FFP_LIGHTING - 1; }

    /** 
    @see SubRenderState::copyFrom.
    */
    void copyFrom(const SubRenderState& rhs) override;


    /** 
    @see SubRenderState::preAddToRenderState.
    */
    auto preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept -> bool override;

    /** 
    Set the index of the input vertex shader texture coordinate set 
    */
    void setTexCoordIndex(unsigned int index) { mVSTexCoordSetIndex = index;}

    /** 
    Return the index of the input vertex shader texture coordinate set.
    */
    auto getTexCoordIndex() const noexcept -> unsigned int { return mVSTexCoordSetIndex; }

    // Type of this render state.
    static String Type;

    enum NormalMapSpace
    {
        NMS_OBJECT = 1,
        NMS_TANGENT = 2,
        NMS_PARALLAX = 6
    };

    /** 
    Set the normal map space.
    @param normalMapSpace The normal map space.
    */
    void setNormalMapSpace(NormalMapSpace normalMapSpace) { mNormalMapSpace = normalMapSpace; }

    /** Return the normal map space. */
    auto getNormalMapSpace() const noexcept -> NormalMapSpace { return mNormalMapSpace; }

    /** 
    Return the normal map texture name.
    */
    auto getNormalMapTextureName() const noexcept -> std::string_view { return mNormalMapTextureName; }

    auto setParameter(std::string_view name, std::string_view value) noexcept -> bool override;

// Protected methods
protected:
    auto createCpuSubPrograms(ProgramSet* programSet) -> bool override;

// Attributes.
protected:  
    // The normal map texture name.
    String mNormalMapTextureName;
    // Normal map texture sampler index.
    unsigned short mNormalMapSamplerIndex;
    // Vertex shader input texture coordinate set index.
    unsigned int mVSTexCoordSetIndex;
    // The normal map sampler
    SamplerPtr mNormalMapSampler;
    // The normal map space.
    NormalMapSpace mNormalMapSpace;
};


/** 
A factory that enables creation of NormalMapLighting instances.
@remarks Sub class of SubRenderStateFactory
*/
class NormalMapLightingFactory : public SubRenderStateFactory
{
public:

    /** 
    @see SubRenderStateFactory::getType.
    */
    [[nodiscard]] auto getType() const noexcept -> std::string_view override;

    /** 
    @see SubRenderStateFactory::createInstance.
    */
    auto createInstance(ScriptCompiler* compiler, PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) noexcept -> SubRenderState* override;

    /** 
    @see SubRenderStateFactory::writeInstance.
    */
    void writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, Pass* srcPass, Pass* dstPass) override;

    
protected:

    /** 
    @see SubRenderStateFactory::createInstanceImpl.
    */
    auto createInstanceImpl() -> SubRenderState* override;


};

/** @} */
/** @} */

}

#endif
