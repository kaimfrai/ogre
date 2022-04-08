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
module Ogre.Components.RTShaderSystem:ShaderFFPFog;

import :ShaderPrerequisites;
import :ShaderSubRenderState;

import Ogre.Core;

namespace Ogre::RTShader {
    class ProgramSet;
    class RenderState;
    class SGScriptTranslator;
}  // namespace RTShader

namespace Ogre::RTShader {

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** Fog sub render state implementation of the Fixed Function Pipeline.
@see http://msdn.microsoft.com/en-us/library/bb173398.aspx
Derives from SubRenderState class.
*/
class FFPFog : public SubRenderState
{
public:

    // Fog calculation mode enum.
    enum CalcMode
    {
        CM_PER_VERTEX   = 1,        // Per vertex fog calculations. (Default).
        CM_PER_PIXEL    = 2         // Per pixel fog calculations.
    };

// Interface.
public:

    /** Class default constructor */
    FFPFog();

    /** 
    @see SubRenderState::getType.
    */
    auto getType() const -> const String& override;

    /** 
    @see SubRenderState::getType.
    */
    auto getExecutionOrder() const -> int override;

    /** 
    @see SubRenderState::copyFrom.
    */
    void copyFrom(const SubRenderState& rhs) override;

    /** 
    @see SubRenderState::preAddToRenderState.
    */
    auto preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) -> bool override;

    /** 
    Set the fog calculation mode. Either per vertex or per pixel.
    @param calcMode The calculation mode to set.
    */
    void setCalcMode(CalcMode calcMode) { mCalcMode = calcMode; }

    auto setParameter(const String& name, const String& value) -> bool override;

    /** 
    Return the current calculation mode.
    */
    auto getCalcMode() const -> CalcMode { return mCalcMode; }

    static String Type;

// Protected methods
protected:

    /** 
    @see SubRenderState::resolveParameters.
    */
    auto resolveParameters(ProgramSet* programSet) -> bool override;

    /** 
    @see SubRenderState::resolveDependencies.
    */
    auto resolveDependencies(ProgramSet* programSet) -> bool override;

    /** 
    @see SubRenderState::addFunctionInvocations.
    */
    auto addFunctionInvocations(ProgramSet* programSet) -> bool override;

// Attributes.
protected:  
    // Fog calculation mode.
    CalcMode mCalcMode;
    // Fog formula. 
    FogMode mFogMode;

    // Fog colour parameter.    
    UniformParameterPtr mFogColour;
    // Fog parameters program parameter.    
    UniformParameterPtr mFogParams;
    // Vertex shader output position parameter.
    ParameterPtr mVSOutPos;
    // Vertex shader output fog colour parameter.
    ParameterPtr mVSOutFogFactor;
    // Pixel shader input fog factor.
    ParameterPtr mPSInFogFactor;
    // Vertex shader output depth.
    ParameterPtr mVSOutDepth;
    // Pixel shader input depth.
    ParameterPtr mPSInDepth;
    // Pixel shader output diffuse colour.
    ParameterPtr mPSOutDiffuse;
};


/** 
A factory that enables creation of FFPFog instances.
@remarks Sub class of SubRenderStateFactory
*/
class FFPFogFactory : public SubRenderStateFactory
{
public:

    /** 
    @see SubRenderStateFactory::getType.
    */
    [[nodiscard]]
    auto getType() const -> const String& override;

    /** 
    @see SubRenderStateFactory::createInstance.
    */
    auto createInstance(ScriptCompiler* compiler, PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) -> SubRenderState* override;

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
