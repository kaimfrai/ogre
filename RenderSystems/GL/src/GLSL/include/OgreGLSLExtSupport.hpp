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
module;

#include "glad/glad.h"

module Ogre.RenderSystems.GL:GLSL.SLExtSupport;

import Ogre.Core;

//
// OpenGL Shading Language entry points
//
namespace Ogre::GLSL {

    /** Check for GL errors and report them in the Ogre Log.
    */
    void reportGLSLError(GLenum glErr, std::string_view ogreMethod, std::string_view errorTextPrefix, const uint obj, const bool forceInfoLog = false, const bool forceException = false);

    /** if there is a message in GL info log then post it in the Ogre Log
    @param msg the info log message string is appended to this string
    @param obj the GL object that is used to retrieve the info log
    */
    auto logObjectInfo(std::string_view msg, uint obj) -> String;

    /// just return the info without logging it
    auto getObjectInfo(GLuint obj) -> String;

    } // namespace Ogre
