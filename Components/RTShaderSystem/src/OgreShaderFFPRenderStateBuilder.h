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
#ifndef _ShaderFFPRenderStateBuilder_
#define _ShaderFFPRenderStateBuilder_

#include "OgreShaderGenerator.h"

namespace Ogre {
    namespace RTShader {
        class TargetRenderState;
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

/** Fixed Function Pipeline render state builder.
This class builds RenderState from a given pass that represents the fixed function pipeline
that the source pass describes.
*/
class FFPRenderStateBuilder
{
// Interface.
public:

    /** 
    Build render state from the given pass that emulates the fixed function pipeline behaviour. 
    @param sgPass The shader generator pass representation. Contains both source and destination pass.
    @param renderState The target render state that will hold the given pass FFP representation.
    */
    static void buildRenderState(ShaderGenerator::SGPass* sgPass, TargetRenderState* renderState);
};


/** @} */
/** @} */

}
}

#endif
