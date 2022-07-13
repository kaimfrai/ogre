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

#include <algorithm>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "OgreException.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderSystemCapabilities.hpp"
#include "OgreRoot.hpp"
#include "OgreShaderFunction.hpp"
#include "OgreShaderFunctionAtom.hpp"
#include "OgreShaderGLSLProgramWriter.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderProgram.hpp"
#include "OgreShaderProgramWriter.hpp"

namespace Ogre::RTShader {

String GLSLProgramWriter::TargetLanguage = "glsl";

//-----------------------------------------------------------------------
GLSLProgramWriter::GLSLProgramWriter()  
{
    auto* rs = Root::getSingleton().getRenderSystem();
    mGLSLVersion = rs ? rs->getNativeShadingLanguageVersion() : 120;

    if(rs && rs->getCapabilities()->isShaderProfileSupported("spirv"))
    {
        mGLSLVersion = 460;
        mIsVulkan = true;
    }

    initializeStringMaps();
}

//-----------------------------------------------------------------------
GLSLProgramWriter::~GLSLProgramWriter()
= default;

//-----------------------------------------------------------------------
void GLSLProgramWriter::initializeStringMaps()
{
    // basic glsl types
    mGpuConstTypeMap[GpuConstantType::FLOAT1] = "float";
    mGpuConstTypeMap[GpuConstantType::FLOAT2] = "vec2";
    mGpuConstTypeMap[GpuConstantType::FLOAT3] = "vec3";
    mGpuConstTypeMap[GpuConstantType::FLOAT4] = "vec4";
    mGpuConstTypeMap[GpuConstantType::SAMPLER1D] = "sampler1D";
    mGpuConstTypeMap[GpuConstantType::SAMPLER2D] = "sampler2D";
    mGpuConstTypeMap[GpuConstantType::SAMPLER2DARRAY] = "sampler2DArray";
    mGpuConstTypeMap[GpuConstantType::SAMPLER3D] = "sampler3D";
    mGpuConstTypeMap[GpuConstantType::SAMPLERCUBE] = "samplerCube";
    mGpuConstTypeMap[GpuConstantType::SAMPLER1DSHADOW] = "sampler1DShadow";
    mGpuConstTypeMap[GpuConstantType::SAMPLER2DSHADOW] = "sampler2DShadow";
    mGpuConstTypeMap[GpuConstantType::SAMPLER_EXTERNAL_OES] = "samplerExternalOES";
    mGpuConstTypeMap[GpuConstantType::MATRIX_2X2] = "mat2";
    mGpuConstTypeMap[GpuConstantType::MATRIX_2X3] = "mat2x3";
    mGpuConstTypeMap[GpuConstantType::MATRIX_2X4] = "mat2x4";
    mGpuConstTypeMap[GpuConstantType::MATRIX_3X2] = "mat3x2";
    mGpuConstTypeMap[GpuConstantType::MATRIX_3X3] = "mat3";
    mGpuConstTypeMap[GpuConstantType::MATRIX_3X4] = "mat3x4";
    mGpuConstTypeMap[GpuConstantType::MATRIX_4X2] = "mat4x2";
    mGpuConstTypeMap[GpuConstantType::MATRIX_4X3] = "mat4x3";
    mGpuConstTypeMap[GpuConstantType::MATRIX_4X4] = "mat4";
    mGpuConstTypeMap[GpuConstantType::INT1] = "int";
    mGpuConstTypeMap[GpuConstantType::INT2] = "ivec2";
    mGpuConstTypeMap[GpuConstantType::INT3] = "ivec3";
    mGpuConstTypeMap[GpuConstantType::INT4] = "ivec4";
    mGpuConstTypeMap[GpuConstantType::UINT1] = "uint";
    mGpuConstTypeMap[GpuConstantType::UINT2] = "uvec2";
    mGpuConstTypeMap[GpuConstantType::UINT3] = "uvec3";
    mGpuConstTypeMap[GpuConstantType::UINT4] = "uvec4";

    // Custom vertex attributes defined http://www.ogre3d.org/docs/manual/manual_21.html
    mContentToPerVertexAttributes[Parameter::Content::POSITION_OBJECT_SPACE] = "vertex";
    mContentToPerVertexAttributes[Parameter::Content::NORMAL_OBJECT_SPACE] = "normal";
    mContentToPerVertexAttributes[Parameter::Content::TANGENT_OBJECT_SPACE] = "tangent";
    mContentToPerVertexAttributes[Parameter::Content::BINORMAL_OBJECT_SPACE] = "binormal";
    mContentToPerVertexAttributes[Parameter::Content::BLEND_INDICES] = "blendIndices";
    mContentToPerVertexAttributes[Parameter::Content::BLEND_WEIGHTS] = "blendWeights";

    mContentToPerVertexAttributes[Parameter::Content::TEXTURE_COORDINATE0] = "uv0";
    mContentToPerVertexAttributes[Parameter::Content::TEXTURE_COORDINATE1] = "uv1";
    mContentToPerVertexAttributes[Parameter::Content::TEXTURE_COORDINATE2] = "uv2";
    mContentToPerVertexAttributes[Parameter::Content::TEXTURE_COORDINATE3] = "uv3";
    mContentToPerVertexAttributes[Parameter::Content::TEXTURE_COORDINATE4] = "uv4";
    mContentToPerVertexAttributes[Parameter::Content::TEXTURE_COORDINATE5] = "uv5";
    mContentToPerVertexAttributes[Parameter::Content::TEXTURE_COORDINATE6] = "uv6";
    mContentToPerVertexAttributes[Parameter::Content::TEXTURE_COORDINATE7] = "uv7";

    mContentToPerVertexAttributes[Parameter::Content::COLOR_DIFFUSE] = "colour";
    mContentToPerVertexAttributes[Parameter::Content::COLOR_SPECULAR] = "secondary_colour";
}

//-----------------------------------------------------------------------
void GLSLProgramWriter::writeSourceCode(std::ostream& os, Program* program)
{
    // Write the current version (this force the driver to more fulfill the glsl standard)
    os << "#version "<< mGLSLVersion << std::endl;

    // Generate dependencies.
    writeProgramDependencies(os, program);
    os << std::endl;

    writeMainSourceCode(os, program);
}

void GLSLProgramWriter::writeUniformBlock(std::ostream& os, std::string_view name, int binding,
                                          const UniformParameterList& uniforms)
{
    os << "layout(binding = " << binding << ", row_major) uniform " << name << " {";

    for (auto uparam : uniforms)
    {
        if(uparam->getType() == GpuConstantType::MATRIX_3X4 || uparam->getType() == GpuConstantType::MATRIX_2X4)
            os << "layout(column_major) ";
        writeParameter(os, uparam);
        os << ";\n";
    }

    os << "\n};\n";
}

void GLSLProgramWriter::writeMainSourceCode(std::ostream& os, Program* program)
{
    GpuProgramType gpuType = program->getType();
    if(gpuType == GpuProgramType::GEOMETRY_PROGRAM)
    {
        OGRE_EXCEPT( ExceptionCodes::NOT_IMPLEMENTED,
            "Geometry Program not supported in GLSL writer ",
            "GLSLProgramWriter::writeSourceCode" );
    }

    const UniformParameterList& parameterList = program->getParameters();
    
    // Generate global variable code.
    writeUniformParametersTitle(os, program);
    os << std::endl;

    auto* rs = Root::getSingleton().getRenderSystem();
    auto hasSSO = rs ? rs->getCapabilities()->hasCapability(Capabilities::SEPARATE_SHADER_OBJECTS) : false;

    // Write the uniforms
    UniformParameterList uniforms;
    for (auto param : parameterList)
    {
        if(!param->isSampler())
        {
            uniforms.push_back(param);
            continue;
        }
        writeSamplerParameter(os, param);
        os << ";" << std::endl;
    }
    if (mIsVulkan && !uniforms.empty())
    {
        writeUniformBlock(os, "OgreUniforms", std::to_underlying(gpuType), uniforms);
        uniforms.clear();
    }

    int uniformLoc = 0;
    for (auto uparam : uniforms)
    {
        if(mGLSLVersion >= 430 && hasSSO)
        {
            os << "layout(location = " << uniformLoc << ") ";
            auto esize = GpuConstantDefinition::getElementSize(uparam->getType(), true) / 4;
            uniformLoc += esize * std::max<int>(uparam->getSize(), 1);
        }

        os << "uniform\t";
        writeParameter(os, uparam);
        os << ";\n";
    }
    os << std::endl;            

    Function* curFunction = program->getMain();
    const ShaderParameterList& inParams = curFunction->getInputParameters();

    writeFunctionTitle(os, curFunction);

    // Write inout params and fill mInputToGLStatesMap
    writeInputParameters(os, curFunction, gpuType);
    writeOutParameters(os, curFunction, gpuType);

    // The function name must always main.
    os << "void main(void) {" << std::endl;

    // Write local parameters.
    for (const ShaderParameterList& localParams = curFunction->getLocalParameters();
         auto const& itParam : localParams)
    {
        os << "\t";
        writeParameter(os, itParam);
        os << ";" << std::endl;
    }
    os << std::endl;

    for (const auto& pFuncInvoc : curFunction->getAtomInstances())
    {
        for (auto& operand : pFuncInvoc->getOperandList())
        {
            const ParameterPtr& param = operand.getParameter();
            Operand::OpSemantic opSemantic = operand.getSemantic();

            bool isInputParam =
                std::ranges::find(inParams, param) != inParams.end();

            if (opSemantic == Operand::OpSemantic::OUT || opSemantic == Operand::OpSemantic::INOUT)
            {
                // Check if we write to an input variable because they are only readable
                // Well, actually "attribute" were writable in GLSL < 120, but we dont care here
                bool doLocalRename = isInputParam;

                // If its not a varying param check if a uniform is written
                if (!doLocalRename)
                {
                    doLocalRename = std::ranges::find(parameterList,
                                                param) != parameterList.end();
                }

                // now we check if we already declared a redirector var
                if(doLocalRename && mLocalRenames.find(param->getName()) == mLocalRenames.end())
                {
                    // Declare the copy variable and assign the original
                    String newVar = ::std::format("local_{}", param->getName());
                    os << "\t" << mGpuConstTypeMap[param->getType()] << " " << newVar << " = " << param->getName() << ";" << std::endl;

                    // From now on we replace it automatic
                    param->_rename(newVar, true);
                    mLocalRenames.insert(newVar);
                }
            }

            // Now that every texcoord is a vec4 (passed as vertex attributes) we
            // have to swizzle them according the desired type.
            if (gpuType == GpuProgramType::VERTEX_PROGRAM && isInputParam &&
                param->getSemantic() == Parameter::Semantic::TEXTURE_COORDINATES)
            {
                operand.setMaskToParamType();
            }
        }

        os << "\t";
        pFuncInvoc->writeSourceCode(os, getTargetLanguage());
        os << std::endl;
    }
    os << "}" << std::endl;
    os << std::endl;
}

//-----------------------------------------------------------------------
void GLSLProgramWriter::writeProgramDependencies(std::ostream& os, Program* program)
{
    os << "//-----------------------------------------------------------------------------" << std::endl;
    os << "//                         PROGRAM DEPENDENCIES" << std::endl;
    os << "//-----------------------------------------------------------------------------" << std::endl;
    os << "#define USE_OGRE_FROM_FUTURE" << std::endl;
    os << "#include <OgreUnifiedShader.h>" << std::endl;

    for (unsigned int i=0; i < program->getDependencyCount(); ++i)
    {
        os << "#include \"" << program->getDependency(i) << ".glsl\"" << std::endl;
    }
}
//-----------------------------------------------------------------------
void GLSLProgramWriter::writeInputParameters(std::ostream& os, Function* function, GpuProgramType gpuType)
{
    const ShaderParameterList& inParams = function->getInputParameters();

    for (int psInLocation = 0;
         ParameterPtr pParam : inParams)
    {       
        Parameter::Content paramContent = pParam->getContent();
        std::string_view paramName = pParam->getName();

        if (gpuType == GpuProgramType::FRAGMENT_PROGRAM)
        {
            if(paramContent == Parameter::Content::POINTSPRITE_COORDINATE)
            {
                pParam->_rename("gl_PointCoord");
                continue;
            }
            else if(paramContent == Parameter::Content::POSITION_PROJECTIVE_SPACE)
            {
                pParam->_rename("gl_FragCoord");
                continue;
            }
            else if(paramContent == Parameter::Content::FRONT_FACING)
            {
                pParam->_rename("gl_FrontFacing");
                continue;
            }

            os << "IN(";
            os << mGpuConstTypeMap[pParam->getType()];
            os << "\t"; 
            os << paramName;
            os << ", " << psInLocation++ << ")\n";
        }
        else if (gpuType == GpuProgramType::VERTEX_PROGRAM && 
                 mContentToPerVertexAttributes.find(paramContent) != mContentToPerVertexAttributes.end())
        {
            // Due the fact that glsl does not have register like cg we have to rename the params
            // according there content.
            pParam->_rename(mContentToPerVertexAttributes[paramContent]);

            os << "IN(";
            // all uv texcoords passed by ogre are at least vec4
            if ((paramContent == Parameter::Content::TEXTURE_COORDINATE0 ||
                 paramContent == Parameter::Content::TEXTURE_COORDINATE1 ||
                 paramContent == Parameter::Content::TEXTURE_COORDINATE2 ||
                 paramContent == Parameter::Content::TEXTURE_COORDINATE3 ||
                 paramContent == Parameter::Content::TEXTURE_COORDINATE4 ||
                 paramContent == Parameter::Content::TEXTURE_COORDINATE5 ||
                 paramContent == Parameter::Content::TEXTURE_COORDINATE6 ||
                 paramContent == Parameter::Content::TEXTURE_COORDINATE7) &&
                (pParam->getType() < GpuConstantType::FLOAT4))
            {
                os << "vec4";
            }
            else
            {
                // the gl rendersystems only pass float attributes
                GpuConstantType type = pParam->getType();
                if(!mIsVulkan && !GpuConstantDefinition::isFloat(type))
                    type = GpuConstantType(type & ~static_cast<GpuConstantType>(GpuConstantDefinition::getBaseType(type)));
                os << mGpuConstTypeMap[type];
            }
            os << "\t"; 
            os << mContentToPerVertexAttributes[paramContent] << ", ";
            writeParameterSemantic(os, pParam);  // maps to location
            os << ")\n";
        }
        else
        {
            os << "uniform \t ";
            os << mGpuConstTypeMap[pParam->getType()];
            os << "\t"; 
            os << paramName;
            os << ";" << std::endl; 
        }                           
    }
}

//-----------------------------------------------------------------------
void GLSLProgramWriter::writeOutParameters(std::ostream& os, Function* function, GpuProgramType gpuType)
{
    const ShaderParameterList& outParams = function->getOutputParameters();

    for (int vsOutLocation = 0;
         ParameterPtr pParam : outParams)
    {
        if(gpuType == GpuProgramType::VERTEX_PROGRAM)
        {
            // GLSL vertex program has to write always gl_Position (but this is also deprecated after version 130)
            if(pParam->getContent() == Parameter::Content::POSITION_PROJECTIVE_SPACE)
            {
                pParam->_rename("gl_Position");
            }
            else if(pParam->getContent() == Parameter::Content::POINTSPRITE_SIZE)
            {
                pParam->_rename("gl_PointSize");
            }
            else
            {
                os << "OUT(";

                // In the vertex and fragment program the variable names must match.
                // Unfortunately now the input params are prefixed with an 'i' and output params with 'o'.
                // Thats why we rename the params which are used in function atoms
                std::string paramName{ pParam->getName() };
                paramName[0] = 'i';
                pParam->_rename(paramName);

                writeParameter(os, pParam);
                os << ", " << vsOutLocation++ << ")\n";
            }
        }
        else if(gpuType == GpuProgramType::FRAGMENT_PROGRAM &&
                pParam->getSemantic() == Parameter::Semantic::COLOR)
        {                   
            if(pParam->getIndex() == 0)
            {
                // handled by UnifiedShader
                pParam->_rename("gl_FragColor");
                continue;
            }

            os << "OUT(vec4\t" << pParam->getName() << ", " << pParam->getIndex() << ")\n";
        }
    }
    
    if(gpuType == GpuProgramType::VERTEX_PROGRAM && !mIsGLSLES) // TODO: also use for GLSLES?
    {
        // Special case where gl_Position needs to be redeclared
        if (mGLSLVersion >= 150 && Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(
                                       Capabilities::GLSL_SSO_REDECLARE))
        {
            os << "out gl_PerVertex\n{\nvec4 gl_Position;\nfloat gl_PointSize;\nfloat gl_ClipDistance[];\n};\n" << std::endl;
        }
    }
}
//-----------------------------------------------------------------------
}
