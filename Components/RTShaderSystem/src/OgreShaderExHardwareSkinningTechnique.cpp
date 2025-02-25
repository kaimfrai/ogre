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
module Ogre.Components.RTShaderSystem;

import :ShaderExHardwareSkinningTechnique;
import :ShaderFunctionAtom;

import Ogre.Core;

import <algorithm>;

#define HS_DATA_BIND_NAME "HS_SRS_DATA"
namespace Ogre::RTShader {

/************************************************************************/
/*                                                                      */
/************************************************************************/
HardwareSkinningTechnique::HardwareSkinningTechnique() 
    
= default;

//-----------------------------------------------------------------------
HardwareSkinningTechnique::~HardwareSkinningTechnique()
= default;

//-----------------------------------------------------------------------
void HardwareSkinningTechnique::setHardwareSkinningParam(ushort boneCount, ushort weightCount, bool correctAntipodalityHandling, bool scalingShearingSupport)
{
    mBoneCount = std::min<ushort>(boneCount, 256);
    mWeightCount = std::min<ushort>(weightCount, 4);
    mCorrectAntipodalityHandling = correctAntipodalityHandling;
    mScalingShearingSupport = scalingShearingSupport;
}

//-----------------------------------------------------------------------
void HardwareSkinningTechnique::setDoBoneCalculations(bool doBoneCalculations)
{
    mDoBoneCalculations = doBoneCalculations;
}

//-----------------------------------------------------------------------
auto HardwareSkinningTechnique::getBoneCount() noexcept -> ushort
{
    return mBoneCount;
}

//-----------------------------------------------------------------------
auto HardwareSkinningTechnique::getWeightCount() noexcept -> ushort
{
    return mWeightCount;
}

//-----------------------------------------------------------------------
auto HardwareSkinningTechnique::hasCorrectAntipodalityHandling() -> bool
{
    return mCorrectAntipodalityHandling;
}

//-----------------------------------------------------------------------
auto HardwareSkinningTechnique::hasScalingShearingSupport() -> bool
{
    return mScalingShearingSupport;
}

//-----------------------------------------------------------------------
auto HardwareSkinningTechnique::indexToMask(int index) -> Operand::OpMask
{
    switch(index)
    {
    case 0: return Operand::OpMask::X;
    case 1: return Operand::OpMask::Y;
    case 2: return Operand::OpMask::Z;
    case 3: return Operand::OpMask::W;
    default: OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Illegal value", "HardwareSkinningTechnique::indexToMask");
    }
}

//-----------------------------------------------------------------------
void HardwareSkinningTechnique::copyFrom(const HardwareSkinningTechnique* hardSkin)
{
    mWeightCount = hardSkin->mWeightCount;
    mBoneCount = hardSkin->mBoneCount;
    mDoBoneCalculations = hardSkin->mDoBoneCalculations;
    mCorrectAntipodalityHandling = hardSkin->mCorrectAntipodalityHandling;
    mScalingShearingSupport = hardSkin->mScalingShearingSupport;
}

}
