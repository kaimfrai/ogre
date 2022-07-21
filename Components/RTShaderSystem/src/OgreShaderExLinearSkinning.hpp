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
module Ogre.Components.RTShaderSystem:ShaderExLinearSkinning;

import :ShaderExHardwareSkinningTechnique;
import :ShaderPrerequisites;


namespace Ogre::RTShader {
    class Function;

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
class LinearSkinning : public HardwareSkinningTechnique
{
// Interface.
public:
    /** Class default constructor */
    LinearSkinning();

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

protected:
    /** Adds functions to calculate position data in world, object and projective space */
    void addPositionCalculations(Function* vsMain);

    /** Adds the weight of a given position for a given index */
    void addIndexedPositionWeight(Function* vsMain, int index);

    /** Adds the calculations for calculating a normal related element */
    void addNormalRelatedCalculations(Function* vsMain,
                        ParameterPtr& pNormalRelatedParam,
                        ParameterPtr& pNormalWorldRelatedParam);

    /** Adds the weight of a given normal related parameter for a given index */
    void addIndexedNormalRelatedWeight(Function* vsMain, ParameterPtr& pNormalRelatedParam,
                        ParameterPtr& pNormalWorldRelatedParam,
                        int index);
};

}
