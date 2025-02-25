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
export module Ogre.Components.RTShaderSystem:ShaderPrerequisites;

export import Ogre.Core;

export
namespace Ogre 
{

/// Forward declarations from Ogre namespace.
using RTShaderSystemAlloc = GeneralAllocatedObject;

namespace RTShader 
{

/// Forward declarations from RTShader namespace.
class Parameter;
class UniformParameter;

using ParameterPtr = shared_ptr<Parameter>;
using UniformParameterPtr = shared_ptr<UniformParameter>;
using ShaderParameterList = std::vector<ParameterPtr>;

// Vertex shader output parameters compact policy.
enum class VSOutputCompactPolicy
{   
    LOW       = 0,        // VS Outputs will be compacted just in case the maximum slot count exceeded.
    MEDIUM    = 1,        // VS Outputs will be compacted always without parameter splits.
    HIGH      = 2         // VS Outputs will be compacted always including parameter splits.
};

enum class SkinningType
{
    LINEAR,
    DUAL_QUATERNION
};


}
}
