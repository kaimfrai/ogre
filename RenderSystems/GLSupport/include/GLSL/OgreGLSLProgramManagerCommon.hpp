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
export module Ogre.RenderSystems.GLSupport:GLSL.ProgramManagerCommon;

export import :GLSL.ProgramCommon;

export import Ogre.Core;

export import <map>;
export import <string>;

export
namespace Ogre {
    /** Ogre assumes that there are separate programs to deal with but
        GLSL has one program object that represents the active shader
        objects during a rendering state.  GLSL shader objects are
        compiled separately and then attached to a program object and
        then the program object is linked.  Since Ogre can only handle
        one program being active in a pass, the GLSL Link Program
        Manager does the same.  The GLSL Link program manager acts as
        a state machine and activates a program object based on the
        active programs.  Previously created program objects are
        stored along with a unique key in a hash_map for quick
        retrieval the next time the program object is required.
    */
    class GLSLProgramManagerCommon
    {
    protected:
        using StringToEnumMap = std::map<std::string_view, GpuConstantType>;
        StringToEnumMap mTypeEnumMap;

        /** Parse an individual uniform from a GLSL source file and
            store it in a GpuNamedConstant. */
        void parseGLSLUniform(std::string_view line, GpuNamedConstants& defs, std::string_view filename);

        using ProgramMap = std::map<uint32, ::std::unique_ptr<GLSLProgramCommon>>;
        using ProgramIterator = ProgramMap::iterator;

        /// container holding previously created program objects
        ProgramMap mPrograms;

        /// Active shader objects defining the active program object.
        GLShaderList mActiveShader;
    public:
        GLSLProgramManagerCommon();
        virtual ~GLSLProgramManagerCommon() = default;

        /** Populate a list of uniforms based on GLSL source and store
            them in GpuNamedConstants.  
            @param src Reference to the source code.
            @param constantDefs The defs to populate (will
            not be cleared before adding, clear it yourself before
            calling this if that's what you want).  
            @param filename The file name this came from, for logging errors.
        */
        void extractUniformsFromGLSL(
            std::string_view src, GpuNamedConstants& constantDefs, std::string_view filename);

        /// Destroy all programs which referencing this shader
        void destroyAllByShader(GLSLShaderCommon* shader);
    };

}
