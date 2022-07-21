/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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
export module Ogre.Components.RTShaderSystem:ShaderExLayeredBlending;

export import :ShaderFFPTexturing;
export import :ShaderFunctionAtom;
export import :ShaderPrerequisites;
export import :ShaderSubRenderState;

export import Ogre.Core;

export import <memory>;
export import <vector>;

export
namespace Ogre::RTShader {

/** Texturing sub render state implementation of layered blending.
Derives from FFPTexturing class which derives from SubRenderState class.
*/
class LayeredBlending : public FFPTexturing
{
public:
    enum class BlendMode
    {
        Invalid = -1,
        FFPBlend,
        BlendNormal,
        BlendLighten,
        BlendDarken,
        BlendMultiply,
        BlendAverage,
        BlendAdd,
        BlendSubtract,
        BlendDifference,
        BlendNegation,
        BlendExclusion,
        BlendScreen,
        BlendOverlay,
        BlendSoftLight,
        BlendHardLight,
        BlendColorDodge,
        BlendColorBurn,
        BlendLinearDodge,
        BlendLinearBurn,
        BlendLinearLight,
        BlendVividLight,
        BlendPinLight,
        BlendHardMix,
        BlendReflect,
        BlendGlow,
        BlendPhoenix,
        BlendSaturation,
        BlendColor,
        BlendLuminosity,
        MaxBlendModes
    };

    enum class SourceModifier
    {
        Invalid = -1,
        None,
        Source1Modulate,
        Source2Modulate,
        Source1InvModulate,
        Source2InvModulate,
        MaxSourceModifiers
    };

    struct TextureBlend
    {
        //The blend mode to use
        BlendMode blendMode{BlendMode::Invalid};
        //The source modification to use
        SourceModifier sourceModifier{SourceModifier::Invalid};
        // The number of the custom param controlling the source modification
        int customNum{0};
        //The parameter controlling the source modification
        ParameterPtr modControlParam;
    };


    /** Class default constructor */
    LayeredBlending();

    /** 
    @see SubRenderState::getType.
    */
    auto getType                 () const noexcept -> std::string_view override;


    /** 
    Set the blend mode of the given texture unit layer with the previous layer.
    @param index The texture unit texture. Textures units (index-1) and (index) will be blended.
    @param mode The blend mode to apply.
    */
    void setBlendMode(unsigned short index, BlendMode mode);

    /** 
    Return the blend mode of the given texture unit index.
    */
    auto getBlendMode(unsigned short index) const -> BlendMode;

    

    /** 
    Set the source modifier parameters for a given texture unit
    @param index Texture blend index
    @param modType The source modification type to use
    @param customNum The custom parameter number used to control the modification
    */
    void setSourceModifier(unsigned short index, SourceModifier modType, int customNum);

    /** 
    Returns the source modifier parameters for a given texture unit
    @return True if a valid modifier exist for the given texture unit
    @param index Texture blend index
    @param modType The source modification type to use
    @param customNum The custom parameter number used to control the modification
    */
    auto getSourceModifier(unsigned short index, SourceModifier& modType, int& customNum) const -> bool;

    /** 
    @see SubRenderState::copyFrom.
    */
    void copyFrom(const SubRenderState& rhs) override;

    static std::string_view const Type;

// Protected methods
protected:
    
    /** 
    @see SubRenderState::resolveParameters.
    */
    auto resolveParameters(ProgramSet* programSet) -> bool override;

    /** 
    @see SubRenderState::resolveDependencies.
    */
    auto resolveDependencies(Ogre::RTShader::ProgramSet* programSet) -> bool override;


    void addPSBlendInvocations(Function* psMain, 
                                       ParameterPtr arg1,
                                       ParameterPtr arg2,
                                       ParameterPtr texel,
                                       int samplerIndex,
                                       const LayerBlendModeEx& blendMode,
                                       const int groupOrder, 
                                       Operand::OpMask targetChannels) override;
    /** 
    Adds the function invocation to the pixel shader which will modify
    the blend sources according to the source modification parameters.
    */
    void addPSModifierInvocation(Function* psMain, 
                                 int samplerIndex, 
                                 ParameterPtr arg1,
                                 ParameterPtr arg2,
                                 const int groupOrder, 
                                 Operand::OpMask targetChannels);

    // Attributes.
protected:
    std::vector<TextureBlend> mTextureBlends;

};



/** 
A factory that enables creation of LayeredBlending instances.
@remarks Sub class of SubRenderStateFactory
*/
class LayeredBlendingFactory : public SubRenderStateFactory
{
public:

    /** 
    @see SubRenderStateFactory::getType.
    */
    [[nodiscard]] auto getType() const noexcept -> std::string_view override;

    /** 
    @see SubRenderStateFactory::createInstance.
    */
    auto createInstance(ScriptCompiler* compiler, PropertyAbstractNode* prop, TextureUnitState* texState, SGScriptTranslator* translator) noexcept -> SubRenderState* override;

    /** 
    @see SubRenderStateFactory::writeInstance.
    */
    void writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, const TextureUnitState* srcTextureUnit, const TextureUnitState* dstTextureUnit) override;

    
protected:

    /** 
    @see SubRenderStateFactory::createInstanceImpl.
    */
    auto createInstanceImpl() -> SubRenderState* override;

    /** 
    @Converts string to Enum
    */
    auto stringToBlendMode(std::string_view strValue) -> LayeredBlending::BlendMode;
    /** 
    @Converts Enum to string
    */
    auto blendModeToString(LayeredBlending::BlendMode blendMode) -> String;

    /** 
    @Converts string to Enum
    */
    auto stringToSourceModifier(std::string_view strValue) -> LayeredBlending::SourceModifier;
    
    /** 
    @Converts Enum to string
    */
    auto sourceModifierToString(LayeredBlending::SourceModifier modifier) -> String;

    /** 
    Returns the LayeredBlending sub-rener state previously created for this material/pass.
    if no such sub-render state exists creates a new one
    @param translator compiler
    */
    auto createOrRetrieveSubRenderState(SGScriptTranslator* translator) -> LayeredBlending*;
};

} // namespace Ogre
