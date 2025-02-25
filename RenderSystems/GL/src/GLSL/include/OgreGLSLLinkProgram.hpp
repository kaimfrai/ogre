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

module Ogre.RenderSystems.GL:GLSL.SLLinkProgram;

import Ogre.Core;
import Ogre.RenderSystems.GLSupport;

import <memory>;
import <set>;

namespace Ogre {

    namespace GLSL {

    /** C++ encapsulation of GLSL Program Object

    */

    class GLSLLinkProgram : public GLSLProgramCommon
    {
    private:
        ::std::unique_ptr<GLUniformCache> mUniformCache;

        /// Build uniform references from active named uniforms
        void buildGLUniformReferences();
        /// Extract attributes
        void extractAttributes();

        using AttributeSet = std::set<GLuint>;
        /// Custom attribute bindings
        AttributeSet mValidAttributes;

        /// Compiles and links the the vertex and fragment programs
        void compileAndLink() override;
        /// Get the the binary data of a program from the microcode cache
        void getMicrocodeFromCache(uint32 id);
    public:
        /// Constructor should only be used by GLSLLinkProgramManager
        explicit GLSLLinkProgram(const GLShaderList& shaders);
        ~GLSLLinkProgram() override;

        /** Makes a program object active by making sure it is linked and then putting it in use.

        */
        void activate() override;

        auto isAttributeValid(VertexElementSemantic semantic, uint index) -> bool;
        
        /** Updates program object uniforms using data from GpuProgramParameters.
        normally called by GLSLGpuProgram::bindParameters() just before rendering occurs.
        */
        void updateUniforms(GpuProgramParametersSharedPtr params, GpuParamVariability mask, GpuProgramType fromProgType) override;

        /// Get the GL Handle for the program object
        [[nodiscard]] auto getGLHandle() const noexcept -> uint { return mGLProgramHandle; }
    };

    }
}
