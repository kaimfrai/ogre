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
module Ogre.Components.RTShaderSystem:ShaderFFPColour;

import :ShaderPrerequisites;
import :ShaderSubRenderState;

import Ogre.Core;

namespace Ogre::RTShader {

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
    enum class StageFlags : unsigned int
    {
        VS_INPUT_DIFFUSE     = 1 << 1,
        VS_OUTPUT_DIFFUSE    = 1 << 2,
        VS_OUTPUT_SPECULAR   = 1 << 3,
        PS_INPUT_DIFFUSE     = 1 << 4,
        PS_INPUT_SPECULAR    = 1 << 5
    };

    friend auto constexpr operator compl(StageFlags flags) -> StageFlags
    {   return static_cast<StageFlags>(compl std::to_underlying(flags));    }

    friend auto constexpr operator not(StageFlags flags) -> bool
    {   return not std::to_underlying(flags);    }

    friend auto constexpr operator bitor(StageFlags left, StageFlags right) -> StageFlags
    {
        return static_cast<StageFlags>
        (   std::to_underlying(left)
        bitor
            std::to_underlying(right)
        );
    }

    friend auto constexpr operator |=(StageFlags& left, StageFlags right) -> StageFlags&
    {
        return left = left bitor right;
    }

    friend auto constexpr operator bitand(StageFlags left, StageFlags right) -> StageFlags
    {
        return static_cast<StageFlags>
        (   std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }

    friend auto constexpr operator &=(StageFlags& left, StageFlags right) -> StageFlags&
    {
        return left = left bitand right;
    }

// Interface.
public:

    /** Class default constructor */
    FFPColour();


    /** 
    @see SubRenderState::getType.
    */
    auto getType() const noexcept -> std::string_view override;

    /** 
    @see SubRenderState::getType.
    */
    auto getExecutionOrder() const noexcept -> FFPShaderStage override;

    /** 
    @see SubRenderState::copyFrom.
    */
    void copyFrom(const SubRenderState& rhs) override;

    /** 
    @see SubRenderState::preAddToRenderState.
    */
    auto preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept -> bool override;

    /**
    Set the resolve stage flags that this sub render state will produce.
    I.E - If one want to specify that the vertex shader program needs to get a diffuse component
    and the pixel shader should output diffuse component he should pass StageFlags::VS_INPUT_DIFFUSE.
    @param flags The stage flag to set.
    */
    void setResolveStageFlags(StageFlags flags) { mResolveStageFlags = flags; }

    /**
    Get the current resolve stage flags.
    */
    auto getResolveStageFlags() const noexcept -> StageFlags { return mResolveStageFlags; }

    /**
    Add the given mask to resolve stage flags that this sub render state will produce.
    @param mask The mask to add to current flag set.
    */
    void addResolveStageMask(StageFlags mask)  { mResolveStageFlags |= mask; }

    /**
    Remove the given mask from the resolve stage flags that this sub render state will produce.
    @param mask The mask to remove from current flag set.
    */
    void removeResolveStageMask(StageFlags mask)  { mResolveStageFlags &= ~mask; }

    static std::string_view const Type;

// Protected methods
protected:  
    auto resolveParameters(ProgramSet* programSet) -> bool override; 
    auto resolveDependencies(ProgramSet* programSet) -> bool override;
    auto addFunctionInvocations(ProgramSet* programSet) -> bool override;

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
    StageFlags mResolveStageFlags;
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
