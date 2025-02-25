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
module Ogre.Components.RTShaderSystem:ShaderExHardwareSkinningTechnique;

import :ShaderFunctionAtom;
import :ShaderPrerequisites;

import Ogre.Core;

namespace Ogre::RTShader {
class ProgramSet;

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** Implement a sub render state which performs hardware skinning.
Meaning, this sub render states adds calculations which multiply
the points and normals by their assigned bone matricies.
*/
class HardwareSkinningTechnique : public RTShaderSystemAlloc
{
// Interface.
public:
    /** Class default constructor */
    HardwareSkinningTechnique();

    virtual ~HardwareSkinningTechnique();

    /**
    @see SubRenderState::copyFrom.
    */
    virtual void copyFrom(const HardwareSkinningTechnique* hardSkin);

    /**
    @see HardwareSkinning::setHardwareSkinningParam.
    */
    void setHardwareSkinningParam(ushort boneCount, ushort weightCount, bool correctAntipodalityHandling = false, bool scalingShearingSupport = false);

    /**
    Returns the number of bones in the model assigned to the material.
    @see setHardwareSkinningParam()
    */
    auto getBoneCount() noexcept -> ushort;

    /**
    Returns the number of weights/bones affecting a vertex.
    @see setHardwareSkinningParam()
    */
    auto getWeightCount() noexcept -> ushort;

    /**
    Only applicable for dual quaternion skinning.
    @see setHardwareSkinningParam()
    */
    auto hasCorrectAntipodalityHandling() -> bool;

    /**
    Only applicable for dual quaternion skinning.
    @see setHardwareSkinningParam()
    */
    auto hasScalingShearingSupport() -> bool;

    /**
    */
    void setDoBoneCalculations(bool doBoneCalculations);

    void setDoLightCalculations(bool val) { mDoLightCalculations = val; }

    /**
    @see SubRenderState::resolveParameters.
    */
    virtual auto resolveParameters(ProgramSet* programSet) -> bool = 0;

    /**
    @see SubRenderState::resolveDependencies.
    */
    virtual auto resolveDependencies(ProgramSet* programSet) -> bool = 0;

    /**
    @see SubRenderState::addFunctionInvocations.
    */
    virtual auto addFunctionInvocations(ProgramSet* programSet) -> bool = 0;

protected:
    /** Translates an index number to a mask value */
    auto indexToMask (int index) -> Operand::OpMask;

// Attributes.
protected:
    ushort mBoneCount{0};
    ushort mWeightCount{0};

    bool mCorrectAntipodalityHandling{false};
    bool mScalingShearingSupport{false};

    bool mDoBoneCalculations{false};
    bool mDoLightCalculations;
    
    ParameterPtr mParamInPosition;
    ParameterPtr mParamInNormal;
    //ParameterPtr mParamInBiNormal;
    //ParameterPtr mParamInTangent;
    ParameterPtr mParamInIndices;
    ParameterPtr mParamInWeights;
    UniformParameterPtr mParamInWorldMatrices;
    UniformParameterPtr mParamInInvWorldMatrix;
    UniformParameterPtr mParamInViewProjMatrix;
    UniformParameterPtr mParamInWorldMatrix;
    UniformParameterPtr mParamInWorldViewProjMatrix;

    ParameterPtr mParamTempFloat4;
    ParameterPtr mParamTempFloat3;
    ParameterPtr mParamLocalPositionWorld;
    ParameterPtr mParamLocalNormalWorld;
    //ParameterPtr mParamLocalTangentWorld;
    //ParameterPtr mParamLocalBinormalWorld;
    ParameterPtr mParamOutPositionProj;
};

}
