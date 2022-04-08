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
module Ogre.Components.RTShaderSystem:ShaderExPerPixelLighting;

import :ShaderFFPLighting;
import :ShaderPrerequisites;
import :ShaderSubRenderState;

import Ogre.Core;

namespace Ogre::RTShader {
    class FunctionStageRef;
    class ProgramSet;
    class SGScriptTranslator;
}  // namespace RTShader

namespace Ogre::RTShader {

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** Per pixel Lighting extension sub render state implementation.
Derives from SubRenderState class.
*/
class PerPixelLighting : public FFPLighting
{

// Interface.
public:
    /** 
    @see SubRenderState::getType.
    */
    auto getType() const -> const String& override;

    auto setParameter(const String& name, const String& value) -> bool override;

    static String Type;

// Protected methods
protected:
    /** 
    @see SubRenderState::resolveParameters.
    */
    auto resolveParameters(ProgramSet* programSet) -> bool override;

    /** Resolve global lighting parameters */
    virtual auto resolveGlobalParameters(ProgramSet* programSet) -> bool;

    /** Resolve per light parameters */
    virtual auto resolvePerLightParameters(ProgramSet* programSet) -> bool;

    /** 
    @see SubRenderState::resolveDependencies.
    */
    auto resolveDependencies(ProgramSet* programSet) -> bool override;

    /** 
    @see SubRenderState::addFunctionInvocations.
    */
    auto addFunctionInvocations(ProgramSet* programSet) -> bool override;
    

    /** 
    Internal method that adds related vertex shader functions invocations.
    */
    void addVSInvocation(const FunctionStageRef& stage);

    
    /** 
    Internal method that adds global illumination component functions invocations.
    */
    void addPSGlobalIlluminationInvocation(const FunctionStageRef& stage);

// Attributes.
protected:  
    // Vertex shader output view position (position in camera space) parameter.
    ParameterPtr mVSOutViewPos;
    // Vertex shader output normal.
    ParameterPtr mVSOutNormal;
    ParameterPtr mFrontFacing;
    ParameterPtr mTargetFlipped;
};


/** 
A factory that enables creation of PerPixelLighting instances.
@remarks Sub class of SubRenderStateFactory
*/
class PerPixelLightingFactory : public SubRenderStateFactory
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
