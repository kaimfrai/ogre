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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_FFPCOLOUR_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_FFPCOLOUR_H

#include "OgrePrerequisites.hpp"
#include "OgreShaderPrerequisites.hpp"
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

namespace Ogre {
namespace RTShader {

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** Colour sub render state implementation of the Fixed Function Pipeline.
Derives from SubRenderState class.
*/
class FFPColour : public SubRenderState
{
public:

    // Parameter stage flags of the colour component.
    enum StageFlags
    {
        SF_VS_INPUT_DIFFUSE     = 1 << 1,
        SF_VS_OUTPUT_DIFFUSE    = 1 << 2,
        SF_VS_OUTPUT_SPECULAR   = 1 << 3,
        SF_PS_INPUT_DIFFUSE     = 1 << 4,
        SF_PS_INPUT_SPECULAR    = 1 << 5
    };

// Interface.
public:

    /** Class default constructor */
    FFPColour();


    /** 
    @see SubRenderState::getType.
    */
    virtual auto getType() const -> const String&;

    /** 
    @see SubRenderState::getType.
    */
    virtual auto getExecutionOrder() const -> int;

    /** 
    @see SubRenderState::copyFrom.
    */
    virtual void copyFrom(const SubRenderState& rhs);

    /** 
    @see SubRenderState::preAddToRenderState.
    */
    virtual auto preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) -> bool;

    /**
    Set the resolve stage flags that this sub render state will produce.
    I.E - If one want to specify that the vertex shader program needs to get a diffuse component
    and the pixel shader should output diffuse component he should pass SF_VS_INPUT_DIFFUSE.
    @param flags The stage flag to set.
    */
    void setResolveStageFlags(unsigned int flags) { mResolveStageFlags = flags; }

    /**
    Get the current resolve stage flags.
    */
    auto getResolveStageFlags() const -> unsigned int            { return mResolveStageFlags; }

    /**
    Add the given mask to resolve stage flags that this sub render state will produce.
    @param mask The mask to add to current flag set.
    */
    void addResolveStageMask(unsigned int mask)  { mResolveStageFlags |= mask; }

    /**
    Remove the given mask from the resolve stage flags that this sub render state will produce.
    @param mask The mask to remove from current flag set.
    */
    void removeResolveStageMask(unsigned int mask)  { mResolveStageFlags &= ~mask; }

    static String Type;

// Protected methods
protected:  
    virtual auto resolveParameters(ProgramSet* programSet) -> bool; 
    virtual auto resolveDependencies(ProgramSet* programSet) -> bool;
    virtual auto addFunctionInvocations(ProgramSet* programSet) -> bool;

// Attributes.
protected:
    // Vertex shader input diffuse component.
    ParameterPtr mVSInputDiffuse;
    // Vertex shader output diffuse component.
    ParameterPtr mVSOutputDiffuse;
    // Vertex shader input specular component.
    ParameterPtr mVSOutputSpecular;
    // Pixel shader input diffuse component.
    ParameterPtr mPSInputDiffuse;
    // Pixel shader input specular component.
    ParameterPtr mPSInputSpecular;
    // Pixel shader output diffuse component.
    ParameterPtr mPSOutputDiffuse;
    // Stage flags that defines resolve parameters definitions.
    unsigned int mResolveStageFlags;
};


/** 
A factory that enables creation of FFPColour instances.
@remarks Sub class of SubRenderStateFactory
*/
class FFPColourFactory : public SubRenderStateFactory
{
public:

    /** 
    @see SubRenderStateFactory::getType.
    */
    [[nodiscard]]
    virtual auto getType() const -> const String&;

    /** 
    @see SubRenderStateFactory::createInstance.
    */
    virtual auto createInstance(ScriptCompiler* compiler, PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) -> SubRenderState*;

    /** 
    @see SubRenderStateFactory::writeInstance.
    */
    virtual void writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, Pass* srcPass, Pass* dstPass);

    
protected:

    /** 
    @see SubRenderStateFactory::createInstanceImpl.
    */
    virtual auto createInstanceImpl() -> SubRenderState*;


};

/** @} */
/** @} */

}
}

#endif
