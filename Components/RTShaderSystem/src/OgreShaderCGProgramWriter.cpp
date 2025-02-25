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
module Ogre.Components.RTShaderSystem;

import :ShaderCGProgramWriter;
import :ShaderFunction;
import :ShaderFunctionAtom;
import :ShaderParameter;
import :ShaderPrerequisites;
import :ShaderProgram;
import :ShaderProgramWriter;

import Ogre.Core;

import <format>;
import <memory>;
import <ostream>;
import <string>;
import <vector>;

namespace Ogre::RTShader {

std::string_view const constinit CGProgramWriter::TargetLanguage = "cg";

//-----------------------------------------------------------------------
CGProgramWriter::CGProgramWriter()
{
    initializeStringMaps();
}

//-----------------------------------------------------------------------
CGProgramWriter::~CGProgramWriter()
= default;

//-----------------------------------------------------------------------
void CGProgramWriter::initializeStringMaps()
{
    mGpuConstTypeMap[GpuConstantType::FLOAT1] = "float";
    mGpuConstTypeMap[GpuConstantType::FLOAT2] = "float2";
    mGpuConstTypeMap[GpuConstantType::FLOAT3] = "float3";
    mGpuConstTypeMap[GpuConstantType::FLOAT4] = "float4";
    mGpuConstTypeMap[GpuConstantType::SAMPLER1D] = "sampler1D";
    mGpuConstTypeMap[GpuConstantType::SAMPLER2D] = "sampler2D";
    mGpuConstTypeMap[GpuConstantType::SAMPLER3D] = "sampler3D";
    mGpuConstTypeMap[GpuConstantType::SAMPLERCUBE] = "samplerCUBE";
    mGpuConstTypeMap[GpuConstantType::MATRIX_2X2] = "float2x2";
    mGpuConstTypeMap[GpuConstantType::MATRIX_2X3] = "float2x3";
    mGpuConstTypeMap[GpuConstantType::MATRIX_2X4] = "float2x4";
    mGpuConstTypeMap[GpuConstantType::MATRIX_3X2] = "float3x2";
    mGpuConstTypeMap[GpuConstantType::MATRIX_3X3] = "float3x3";
    mGpuConstTypeMap[GpuConstantType::MATRIX_3X4] = "float3x4";
    mGpuConstTypeMap[GpuConstantType::MATRIX_4X2] = "float4x2";
    mGpuConstTypeMap[GpuConstantType::MATRIX_4X3] = "float4x3";
    mGpuConstTypeMap[GpuConstantType::MATRIX_4X4] = "float4x4";
    mGpuConstTypeMap[GpuConstantType::INT1] = "int";
    mGpuConstTypeMap[GpuConstantType::INT2] = "int2";
    mGpuConstTypeMap[GpuConstantType::INT3] = "int3";
    mGpuConstTypeMap[GpuConstantType::INT4] = "int4";
    mGpuConstTypeMap[GpuConstantType::UINT1] = "uint";
    mGpuConstTypeMap[GpuConstantType::UINT2] = "uint2";
    mGpuConstTypeMap[GpuConstantType::UINT3] = "uint3";
    mGpuConstTypeMap[GpuConstantType::UINT4] = "uint4";

    mParamSemanticMap[Parameter::Semantic::FRONT_FACING] = "VFACE";
}

//-----------------------------------------------------------------------
void CGProgramWriter::writeSourceCode(std::ostream& os, Program* program)
{
    // Generate source code header.
    writeProgramTitle(os, program);
    os << std::endl;

    // Generate dependencies.
    writeProgramDependencies(os, program);
    os << std::endl;

    // Generate global variable code.
    writeUniformParametersTitle(os, program);
    os << std::endl;

    for (const auto& param : program->getParameters())
    {
        param->isSampler() ? writeSamplerParameter(os, param) : writeParameter(os, param);
        os << ";" << std::endl;             
    }
    os << std::endl;


    Function* curFunction = program->getMain();

    writeFunctionTitle(os, curFunction);
    writeFunctionDeclaration(os, curFunction);

    os << "{" << std::endl;

    // Write local parameters.
    const ShaderParameterList& localParams = curFunction->getLocalParameters();

    for (auto const& itParam : localParams)
    {
        os << "\t";
        writeParameter(os, itParam);
        os << ";" << std::endl;
    }

    const FunctionAtomInstanceList& atomInstances = curFunction->getAtomInstances();
    for (auto const& itAtom : atomInstances)
    {
        writeAtomInstance(os, itAtom);
    }


    os << "}" << std::endl;
    os << std::endl;
}


//-----------------------------------------------------------------------
void CGProgramWriter::writeProgramDependencies(std::ostream& os, Program* program)
{
    os << "//-----------------------------------------------------------------------------" << std::endl;
    os << "//                         PROGRAM DEPENDENCIES" << std::endl;
    os << "//-----------------------------------------------------------------------------" << std::endl;

    os << "#include <OgreUnifiedShader.h>" << std::endl;

    const auto& rgm = ResourceGroupManager::getSingleton();

    for (unsigned int i=0; i < program->getDependencyCount(); ++i)
    {
        String curDependency = ::std::format("{}.cg", program->getDependency(i));
        if (!rgm.resourceExistsInAnyGroup(curDependency))
            curDependency = ::std::format("{}.glsl", program->getDependency(i)); // fall back to glsl extension

        os << "#include \"" << curDependency << '\"' << std::endl;
    }
}

//-----------------------------------------------------------------------
void CGProgramWriter::writeFunctionParameter(std::ostream& os, ParameterPtr parameter)
{
    writeParameter(os, parameter);
    os << " : ";
    writeParameterSemantic(os, parameter);
}

//-----------------------------------------------------------------------
void CGProgramWriter::writeFunctionDeclaration(std::ostream& os, Function* function)
{
    const ShaderParameterList& inParams  = function->getInputParameters();
    const ShaderParameterList& outParams = function->getOutputParameters();


    os << "void main(\n";

    // Write input parameters.
    for (auto const& it : inParams)
    {                   
        os << "\t in ";
        writeFunctionParameter(os, it);
        os << ",\n";
    }


    // Write output parameters.
    for (auto const& it : outParams)
    {
        os << "\t out ";
        writeFunctionParameter(os, it);
        os << ",\n";
    }

    os.seekp(-2, std::ios_base::end);

    os << "\n)\n";
}

//-----------------------------------------------------------------------
void CGProgramWriter::writeAtomInstance(std::ostream& os, FunctionAtom* atom)
{
    os << std::endl << "\t";
    atom->writeSourceCode(os, getTargetLanguage());
    os << std::endl;
}

}
