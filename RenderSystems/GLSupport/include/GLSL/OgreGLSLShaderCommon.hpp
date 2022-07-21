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
export module Ogre.RenderSystems.GLSupport:GLSL.ShaderCommon;

export import :GLUniformCache;

export import Ogre.Core;

export import <vector>;

export
namespace Ogre {
    /** Specialisation of HighLevelGpuProgram to provide support for OpenGL
        Shader Language (GLSL).

        GLSL has no target assembler or entry point specification like DirectX 9 HLSL.
        Vertex and Fragment shaders only have one entry point called "main".
        When a shader is compiled, microcode is generated but can not be accessed by
        the application.
        GLSL also does not provide assembler low level output after compiling. Therefore the GLSLShader will also stand in
        for the low level implementation.
        The GLSLProgram class will create a shader object and compile the source but will not
        create a program object.  It's up to the GLSLProgramManager to request a program object to link the shader object to.

    @note
        GLSL supports multiple modular shader objects that can be attached to one program
        object to form a single shader.  This is supported through the "attach" material script
        command.  All the modules to be attached are listed on the same line as the attach command
        separated by white space.

    */
    class GLSLShaderCommon : public HighLevelGpuProgram
    {
    public:
        /// Command object for attaching another GLSL Program 
        class CmdAttach : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view shaderNames) override;
        };
        /// Command object for setting matrix packing in column-major order
        class CmdColumnMajorMatrices : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };

        GLSLShaderCommon(ResourceManager* creator,
            std::string_view name, ResourceHandle handle,
            std::string_view group, bool isManual, ManualResourceLoader* loader);

        virtual void attachToProgramObject(const uint programObject) = 0;
        virtual void detachFromProgramObject(const uint programObject) = 0;

        auto getAttachedShaderNames() const noexcept -> String { return mAttachedShaderNames; }

        /// Overridden
        /** Attach another GLSL Shader to this one. */
        void attachChildShader(std::string_view name);

        /** Sets whether matrix packing in column-major order. */ 
        void setColumnMajorMatrices(bool columnMajor) { mColumnMajorMatrices = columnMajor; }
        /** Gets whether matrix packed in column-major order. */
        auto getColumnMajorMatrices() const noexcept -> bool { return mColumnMajorMatrices; }

        /// Only used for separable programs.
        virtual auto linkSeparable() noexcept -> bool { return false; }

        /// reset link status of separable program
        void resetLinked() { mLinked = 0; }

        /// Get the OGRE assigned shader ID.
        auto getShaderID() const noexcept -> uint { return mShaderID; }

        /// If we are using program pipelines, the OpenGL program handle
        auto getGLProgramHandle() const noexcept -> uint { return mGLProgramHandle; }

        /// Get the uniform cache for this shader
        auto getUniformCache() noexcept -> GLUniformCache*{return &mUniformCache;}

        /// GLSL does not provide access to the low level code of the shader, so use this shader for binding as well
        auto _getBindingDelegate() noexcept -> GpuProgram* override { return this; }
    protected:
        /// GLSL does not provide access to the low level implementation of the shader, so this method s a no-op
        void createLowLevelImpl() override {}

        static CmdAttach msCmdAttach;
        static CmdColumnMajorMatrices msCmdColumnMajorMatrices;

        auto getResourceLogName() const -> String;

        void prepareImpl() override;

        /// Attached Shader names
        String mAttachedShaderNames;
        /// Container of attached programs
        using GLSLProgramContainer = std::vector<GLSLShaderCommon *>;
        using GLSLProgramContainerIterator = GLSLProgramContainer::iterator;
        GLSLProgramContainer mAttachedGLSLPrograms;
        /// Matrix in column major pack format?
        bool mColumnMajorMatrices{true};

        /** Flag indicating that the shader has been successfully
            linked.
            Only used for separable programs. */
        int mLinked{0};

        /// OGRE assigned shader ID.
        uint mShaderID;

        /// GL handle for shader object.
        uint mGLShaderHandle{0};
        /// GL handle for program object the shader is bound to.
        uint mGLProgramHandle{0};

        int mShaderVersion{100};

        /// Pointer to the uniform cache for this shader
        GLUniformCache    mUniformCache;

        /// Keep track of the number of shaders created.
        static uint mShaderCount;
    };
}
