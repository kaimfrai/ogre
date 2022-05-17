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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_FFPTRANSFORM_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_FFPTRANSFORM_H

#include "OgrePrerequisites.hpp"
#include "OgreShaderParameter.hpp"
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

/** Transform sub render state implementation of the Fixed Function Pipeline.
@see http://msdn.microsoft.com/en-us/library/bb206269.aspx
Derives from SubRenderState class.
*/
class FFPTransform : public SubRenderState
{

// Interface.
public:
    
    /** 
    @see SubRenderState::getType.
    */
    virtual auto getType() const -> const String&;

    /** 
    @see SubRenderState::getExecutionOrder.
    */
    virtual auto getExecutionOrder() const -> int;

    /** 
    @see SubRenderState::copyFrom.
    */
    virtual void copyFrom(const SubRenderState& rhs);

    /** 
    @see SubRenderState::createCpuSubPrograms.
    */
    virtual auto createCpuSubPrograms(ProgramSet* programSet) -> bool;

    auto preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) -> bool;

    void setInstancingParams(bool enabled, int texCoordIndex)
    {
        mInstanced = enabled;
        mTexCoordIndex = Parameter::Content(Parameter::SPC_TEXTURE_COORDINATE0 + texCoordIndex);
    }

    static String Type;
protected:
    Parameter::Content mTexCoordIndex = Parameter::SPC_TEXTURE_COORDINATE0;
    bool mSetPointSize;
    bool mInstanced = false;
    bool mDoLightCalculations;
};


/** 
A factory that enables creation of FFPTransform instances.
@remarks Sub class of SubRenderStateFactory
*/
class FFPTransformFactory : public SubRenderStateFactory
{
public:

    /** 
    @see SubRenderStateFactory::getType.
    */
    [[nodiscard]] virtual auto getType() const -> const String&;

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
