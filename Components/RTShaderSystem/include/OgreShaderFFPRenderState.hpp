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
export module Ogre.Components.RTShaderSystem:ShaderFFPRenderState;

export import :ShaderPrerequisites;

export
namespace Ogre::RTShader {

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

// Fixed Function vertex shader stages.
enum class FFPVertexShaderStage : uint32
{
    PRE_PROCESS                  = 0,
    TRANSFORM                    = 100,
    COLOUR                       = 200,
    LIGHTING                     = 300,
    TEXTURING                    = 400,
    FOG                          = 500,
    POST_PROCESS                 = 2000
};

// Fixed Function fragment shader stages.
enum class FFPFragmentShaderStage : uint32
{
    PRE_PROCESS                  = 0,
    COLOUR_BEGIN                 = 100,
    SAMPLING                     = 150,
    TEXTURING                    = 200,
    COLOUR_END                   = 300,
    FOG                          = 400,
    POST_PROCESS                 = 500,
	ALPHA_TEST					= 1000
};

// Fixed Function generic stages.
enum class FFPShaderStage : int
{
    PRE_PROCESS                     = 0,
    TRANSFORM                       = 100,
    COLOUR                          = 200,
    LIGHTING                        = 300,
    TEXTURING                       = 400,
    FOG                             = 500,
    POST_PROCESS                    = 600,
	ALPHA_TEST						= 1000
};

auto constexpr operator + (FFPShaderStage stage, int offset) -> FFPShaderStage
{
    return static_cast<FFPShaderStage>
    (   std::to_underlying(stage)
    +   offset
    );
}

auto constexpr operator - (FFPShaderStage stage, int offset) -> FFPShaderStage
{
    return static_cast<FFPShaderStage>
    (   std::to_underlying(stage)
    -   offset
    );
}

char const constexpr inline FFP_LIB_COMMON[] =
    "FFPLib_Common";
char const constexpr inline FFP_LIB_TEXTURING[] =
    "FFPLib_Texturing";
char const constexpr inline FFP_LIB_FOG[] =
    "FFPLib_Fog";

/** @} */
/** @} */

}
