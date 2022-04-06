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
#ifndef OGRE_RENDERSYSTEMS_GL_GLSL_LINKPROGRAMMANAGER_H
#define OGRE_RENDERSYSTEMS_GL_GLSL_LINKPROGRAMMANAGER_H

#include "OgreGLSLProgramCommon.hpp"
#include "OgreGLSLProgramManagerCommon.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSingleton.hpp"

namespace Ogre {

    namespace GLSL {
class GLSLLinkProgram;
class GLSLProgram;

    class GLSLLinkProgramManager : public Singleton<GLSLLinkProgramManager>, public GLSLProgramManagerCommon
    {

    private:
        GLSLLinkProgram* mActiveLinkProgram;

        /// Find where the data for a specific uniform should come from, populate
        static auto completeParamSource(const String& paramName,
            const GpuConstantDefinitionMap* vertexConstantDefs, 
            const GpuConstantDefinitionMap* geometryConstantDefs,
            const GpuConstantDefinitionMap* fragmentConstantDefs,
            GLUniformReference& refToUpdate) -> bool;

    public:

        GLSLLinkProgramManager();
        ~GLSLLinkProgramManager() override;

        /**
            Get the program object that links the two active shader objects together
            if a program object was not already created and linked a new one is created and linked
        */
        auto getActiveLinkProgram() -> GLSLLinkProgram*;

        /** Set the active fragment shader for the next rendering state.
            The active program object will be cleared.
            Normally called from the GLSLGpuProgram::bindProgram and unbindProgram methods
        */
        void setActiveShader(GpuProgramType type, GLSLProgram* gpuProgram);

        /** Populate a list of uniforms based on a program object.
        @param programObject Handle to the program object to query
        @param vertexConstantDefs Definition of the constants extracted from the
            vertex program, used to match up physical buffer indexes with program
            uniforms. May be null if there is no vertex program.
        @param geometryConstantDefs Definition of the constants extracted from the
            geometry program, used to match up physical buffer indexes with program
            uniforms. May be null if there is no geometry program.
        @param fragmentConstantDefs Definition of the constants extracted from the
            fragment program, used to match up physical buffer indexes with program
            uniforms. May be null if there is no fragment program.
        @param list The list to populate (will not be cleared before adding, clear
        it yourself before calling this if that's what you want).
        */
        static void extractUniforms(uint programObject,
            const GpuConstantDefinitionMap* vertexConstantDefs, 
            const GpuConstantDefinitionMap* geometryConstantDefs,
            const GpuConstantDefinitionMap* fragmentConstantDefs,
            GLUniformReferenceList& list);

        static auto getSingleton() -> GLSLLinkProgramManager&;
        static auto getSingletonPtr() -> GLSLLinkProgramManager*;

    };

    }
}

#endif // OGRE_RENDERSYSTEMS_GL_GLSL_LINKPROGRAMMANAGER_H
