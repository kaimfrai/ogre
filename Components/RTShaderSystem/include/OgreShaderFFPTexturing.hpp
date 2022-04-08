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
export module Ogre.Components.RTShaderSystem:ShaderFFPTexturing;

export import :ShaderFunctionAtom;
export import :ShaderPrerequisites;
export import :ShaderSubRenderState;

export import Ogre.Core;

export import <cstddef>;
export import <vector>;

export
namespace Ogre::RTShader {
    class Function;
    class ProgramSet;
    class RenderState;
    class SGScriptTranslator;
}  // namespace RTShader

export
namespace Ogre::RTShader {


/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** Texturing sub render state implementation of the Fixed Function Pipeline.
Implements texture coordinate processing:
@see http://msdn.microsoft.com/en-us/library/bb206247.aspx
Implements texture blending operation:
@see http://msdn.microsoft.com/en-us/library/bb206241.aspx
Derives from SubRenderState class.
*/
class FFPTexturing : public SubRenderState
{

// Interface.
public:

    /** Class default constructor */
    FFPTexturing();

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
    
    static String Type;

// Protected types:
protected:
    
    // Per texture unit parameters.
    struct TextureUnitParams
    {
        // Texture unit state.
        TextureUnitState* mTextureUnitState;
        // Texture sampler index.
        unsigned short mTextureSamplerIndex;
        // Texture sampler index.
        GpuConstantType mTextureSamplerType;
        // Vertex shader input texture coordinate type.
        GpuConstantType mVSInTextureCoordinateType;
        // Vertex shader output texture coordinates type.       
        GpuConstantType mVSOutTextureCoordinateType;
        // Texture coordinates calculation method.
        TexCoordCalcMethod mTexCoordCalcMethod;
        // Texture matrix parameter.
        UniformParameterPtr mTextureMatrix;
        // Texture View Projection Image space matrix parameter.
        UniformParameterPtr mTextureViewProjImageMatrix;
        // Texture sampler parameter.
        UniformParameterPtr mTextureSampler;
        // Vertex shader input texture coordinates parameter.
        ParameterPtr mVSInputTexCoord;
        // Vertex shader output texture coordinates parameter.
        ParameterPtr mVSOutputTexCoord;
        // Pixel shader input texture coordinates parameter.
        ParameterPtr mPSInputTexCoord;
    };

    using TextureUnitParamsList = std::vector<TextureUnitParams>;
    using TextureUnitParamsIterator = TextureUnitParamsList::iterator;
    using TextureUnitParamsConstIterator = TextureUnitParamsList::const_iterator;

// Protected methods
protected:

    /** 
    Set the number of texture units this texturing sub state has to handle.
    @param count The number of texture unit states.
    */
    void setTextureUnitCount(size_t count);

    /** 
    Return the number of texture units this sub state handle. 
    */
    auto getTextureUnitCount() const -> size_t { return mTextureUnitParamsList.size(); }

    /** 
    Set texture unit of a given stage index.
    @param index The stage index of the given texture unit state.
    @param textureUnitState The texture unit state to bound the the stage index.
    */
    void setTextureUnit(unsigned short index, TextureUnitState* textureUnitState);

    /** 
    @see SubRenderState::resolveParameters.
    */
    auto resolveParameters(ProgramSet* programSet) -> bool override;

    /** 
    Internal method that resolves uniform parameters of the given texture unit parameters.
    */
    auto resolveUniformParams(TextureUnitParams* textureUnitParams, ProgramSet* programSet) -> bool;

    /** 
    Internal method that resolves functions parameters of the given texture unit parameters.
    */
    auto resolveFunctionsParams(TextureUnitParams* textureUnitParams, ProgramSet* programSet) -> bool;

    /** 
    @see SubRenderState::resolveDependencies.
    */
    auto resolveDependencies(ProgramSet* programSet) -> bool override;

    /** 
    @see SubRenderState::addFunctionInvocations.
    */
    auto addFunctionInvocations(ProgramSet* programSet) -> bool override;


    /** 
    Internal method that adds vertex shader functions invocations.
    */
    auto addVSFunctionInvocations(TextureUnitParams* textureUnitParams, Function* vsMain) -> bool;

    /** 
    Internal method that adds pixel shader functions invocations.
    */
    auto addPSFunctionInvocations(TextureUnitParams* textureUnitParams, Function* psMain) -> bool;

    /** 
    Adds the fragment shader code which samples the texel color in the texture
    */
    virtual void addPSSampleTexelInvocation(TextureUnitParams* textureUnitParams, Function* psMain, 
        const ParameterPtr& texel, int groupOrder);

    auto getPSArgument(ParameterPtr texel, LayerBlendSource blendSrc, const ColourValue& colourValue,
                               Real alphaValue, bool isAlphaArgument) const -> ParameterPtr;

    virtual void addPSBlendInvocations(Function* psMain, ParameterPtr arg1, ParameterPtr arg2,
                ParameterPtr texel,int samplerIndex, const LayerBlendModeEx& blendMode,
                const int groupOrder, Operand::OpMask targetChannels);
    
    /** 
    Determines the texture coordinates calculation method of the given texture unit state.
    */
    auto getTexCalcMethod(TextureUnitState* textureUnitState) -> TexCoordCalcMethod;

    /** 
    Determines if the given texture unit state need to use texture transformation matrix.
    */
    auto needsTextureMatrix(TextureUnitState* textureUnitState) -> bool;

    auto setParameter(const String& name, const String& value) -> bool override;

// Attributes.
protected:
    // Texture units list.      
    TextureUnitParamsList mTextureUnitParamsList;
    // World matrix parameter.
    UniformParameterPtr mWorldMatrix;
    // World inverse transpose matrix parameter.
    UniformParameterPtr mWorldITMatrix;
    // View matrix parameter.           
    UniformParameterPtr mViewMatrix;
    // Vertex shader input normal parameter.
    ParameterPtr mVSInputNormal;
    // Vertex shader input position parameter.      
    ParameterPtr mVSInputPos;
    // Pixel shader output colour.
    ParameterPtr mPSOutDiffuse;
    // Pixel shader diffuse colour.
    ParameterPtr mPSDiffuse;
    // Pixel shader specular colour.
    ParameterPtr mPSSpecular;

    bool mIsPointSprite{false};
    bool mLateAddBlend{false};
};


/** 
A factory that enables creation of FFPTexturing instances.
@remarks Sub class of SubRenderStateFactory
*/
class FFPTexturingFactory : public SubRenderStateFactory
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
