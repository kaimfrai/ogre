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
export module Ogre.Components.RTShaderSystem:ShaderExIntegratedPSSM3;

export import :ShaderPrerequisites;
export import :ShaderSubRenderState;

export import Ogre.Core;

export import <algorithm>;
export import <vector>;

export
namespace Ogre::RTShader {
    struct Function;
    struct Program;

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** Integrated PSSM shadow receiver with 3 splits sub render state implementation.
Derives from SubRenderState class.
*/
class IntegratedPSSM3 : public SubRenderState
{

    // Interface.
public:
    using SplitPointList = std::vector<Real>;

    /** Class default constructor */    
    IntegratedPSSM3();

    /** 
    @see SubRenderState::getType.
    */
    auto getType() const noexcept -> std::string_view override;

    /** 
    @see SubRenderState::getType.
    */
    auto getExecutionOrder() const noexcept -> FFPShaderStage override;

    /** 
    @see SubRenderState::updateGpuProgramsParams.
    */
    void updateGpuProgramsParams(Renderable* rend, const Pass* pass, const AutoParamDataSource* source, const LightList* pLightList) override;

    /** 
    @see SubRenderState::copyFrom.
    */
    void copyFrom(const SubRenderState& rhs) override;


    /** 
    @see SubRenderState::preAddToRenderState.
    */
    auto preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept -> bool override;


    
    /** Manually configure a new splitting scheme.
    @param newSplitPoints A list which is splitCount + 1 entries long, containing the
    split points. The first value is the near point, the last value is the
    far point, and each value in between is both a far point of the previous
    split, and a near point for the next one.
    */
    void setSplitPoints(const SplitPointList& newSplitPoints);

    void setDebug(bool enable) { mDebug = enable; }

    auto setParameter(std::string_view name, std::string_view value) noexcept -> bool override;

    static std::string_view const constexpr Type = "SGX_IntegratedPSSM3";

    // Protected types:
protected:

    // Shadow texture parameters.
    struct ShadowTextureParams
    {                   
        // The max range of this shadow texture in terms of PSSM (far plane of viewing camera).
        Real mMaxRange;
        // The shadow map sampler index.
        unsigned int mTextureSamplerIndex;
        // The shadow map sampler.          
        UniformParameterPtr mTextureSampler;
        // The inverse texture 
        UniformParameterPtr mInvTextureSize;
        // The source light view projection matrix combined with world matrix.      
        UniformParameterPtr mWorldViewProjMatrix;
        // The vertex shader output position in light space.
        ParameterPtr mVSOutLightPosition;
        // The pixel shader input position in light space.
        ParameterPtr mPSInLightPosition;

    };

    using ShadowTextureParamsList = std::vector<ShadowTextureParams>;
    using ShadowTextureParamsIterator = ShadowTextureParamsList::iterator;
    using ShadowTextureParamsConstIterator = ShadowTextureParamsList::const_iterator;

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

    /** 
    Internal method that adds related vertex shader functions invocations.
    */
    auto addVSInvocation(Function* vsMain, const int groupOrder) -> bool;

    /** 
    Internal method that adds related pixel shader functions invocations.
    */
    auto addPSInvocation(Program* psProgram, const int groupOrder) -> bool;





    // Attributes.
protected:      
    // Shadow texture parameter list.   
    ShadowTextureParamsList mShadowTextureParamsList;
    // Split points parameter.
    UniformParameterPtr mPSSplitPoints;
    // Vertex shader input position parameter.  
    ParameterPtr mVSInPos;
    // Vertex shader output position (clip space) parameter.
    ParameterPtr mVSOutPos;
    // Vertex shader output depth (clip space) parameter.
    ParameterPtr mVSOutDepth;
    // Pixel shader input depth (clip space) parameter.
    ParameterPtr mPSInDepth;
    // Pixel shader local computed shadow colour parameter.
    ParameterPtr mPSLocalShadowFactor;
    // Pixel shader in/local diffuse colour parameter.
    ParameterPtr mPSDiffuse;
    // Pixel shader output diffuse colour parameter.
    ParameterPtr mPSOutDiffuse;
    // Pixel shader in/local specular colour parameter.
    ParameterPtr mPSSpecualr;
    // Derived scene colour (ambient term).
    UniformParameterPtr mPSDerivedSceneColour;

    float mPCFxSamples;
    bool mUseTextureCompare;
    bool mUseColourShadows;
    bool mDebug;
    bool mIsD3D9;
};


/** 
A factory that enables creation of IntegratedPSSM3 instances.
@remarks Sub class of SubRenderStateFactory
*/
class IntegratedPSSM3Factory : public SubRenderStateFactory
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


protected:

    /** 
    @see SubRenderStateFactory::createInstanceImpl.
    */
    auto createInstanceImpl() -> SubRenderState* override;


};

/** @} */
/** @} */

}
