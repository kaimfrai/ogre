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
export module Ogre.RenderSystems.GLSupport:GLSL.ProgramCommon;

export import :GLSL.ShaderCommon;

export import Ogre.Core;

export import <array>;
export import <vector>;

export
namespace Ogre
{
struct GpuConstantDefinition;

/// Structure used to keep track of named uniforms in the linked program object
struct GLUniformReference
{
    /// GL location handle
    int mLocation;
    /// Which type of program params will this value come from?
    GpuProgramType mSourceProgType;
    /// The constant definition it relates to
    const GpuConstantDefinition* mConstantDef;
};
using GLUniformReferenceList = std::vector<GLUniformReference>;
using GLUniformReferenceIterator = GLUniformReferenceList::iterator;

using GLShaderList = std::array<GLSLShaderCommon *, std::to_underlying(GpuProgramType::COUNT)>;

class GLSLProgramCommon
{
public:
    explicit GLSLProgramCommon(const GLShaderList& shaders);
    virtual ~GLSLProgramCommon() = default;

    /// Get the GL Handle for the program object
    [[nodiscard]] auto getGLProgramHandle() const noexcept -> uint { return mGLProgramHandle; }

    /** Makes a program object active by making sure it is linked and then putting it in use.
     */
    virtual void activate() = 0;

    /// query if the program is using the given shader
    auto isUsingShader(GLSLShaderCommon* shader) const -> bool { return mShaders[std::to_underlying(shader->getType())] == shader; }

    /** Updates program object uniforms using data from GpuProgramParameters.
        Normally called by GLSLShader::bindParameters() just before rendering occurs.
    */
    virtual void updateUniforms(GpuProgramParametersPtr params, GpuParamVariability mask, GpuProgramType fromProgType) = 0;

    /** Get the fixed attribute bindings normally used by GL for a semantic. */
    static auto getFixedAttributeIndex(VertexElementSemantic semantic, uint index) -> int32;

    /**
     * use alternate vertex attribute layout using only 8 vertex attributes
     *
     * For "Vivante GC1000" and "VideoCore IV" (notably in Raspberry Pi) on GLES2
     */
    static void useTightAttributeLayout();
protected:
    /// Container of uniform references that are active in the program object
    GLUniformReferenceList mGLUniformReferences;

    /// Linked shaders
    GLShaderList mShaders;

    /// Flag to indicate that uniform references have already been built
    bool mUniformRefsBuilt{false};
    /// GL handle for the program object
    uint mGLProgramHandle{0};
    /// Flag indicating that the program or pipeline object has been successfully linked
    int mLinked{false};

    /// Compiles and links the vertex and fragment programs
    virtual void compileAndLink() = 0;

    auto getCombinedHash() noexcept -> uint32;
    auto getCombinedName() -> String;

    /// Name / attribute list
    struct CustomAttribute
    {
        const char* name;
        int32 attrib;
        VertexElementSemantic semantic;
    };
    static CustomAttribute msCustomAttributes[17];
};

} /* namespace Ogre */
