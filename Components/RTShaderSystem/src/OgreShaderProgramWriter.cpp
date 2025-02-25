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
module Ogre.Components.RTShaderSystem;

import :ShaderParameter;
import :ShaderPrerequisites;
import :ShaderProgram;
import :ShaderProgramWriter;

import Ogre.Core;

import <memory>;
import <ostream>;
import <string>;

namespace Ogre::RTShader {
//-----------------------------------------------------------------------
void ProgramWriter::writeProgramTitle(std::ostream& os, Program* program)
{
    os << "//-----------------------------------------------------------------------------" << std::endl;
    os << "// Program Type: " << to_string(program->getType()) << std::endl;
    os << "// Language: " <<  getTargetLanguage() << std::endl;
    os << "// Created by Ogre RT Shader Generator. All rights reserved." << std::endl;
    os << "//-----------------------------------------------------------------------------" << std::endl;
}

//-----------------------------------------------------------------------
void ProgramWriter::writeUniformParametersTitle(std::ostream& os, Program* program)
{
    os << "//-----------------------------------------------------------------------------" << std::endl;
    os << "//                         GLOBAL PARAMETERS" << std::endl;
    os << "//-----------------------------------------------------------------------------" << std::endl;
}
//-----------------------------------------------------------------------
void ProgramWriter::writeFunctionTitle(std::ostream& os, Function* function)
{
    os << "//-----------------------------------------------------------------------------" << std::endl;
    os << "//                         MAIN" << std::endl;
    os << "//-----------------------------------------------------------------------------" << std::endl;
}

ProgramWriter::ProgramWriter()
{
    mParamSemanticMap[Parameter::Semantic::POSITION] = "POSITION";
    mParamSemanticMap[Parameter::Semantic::BLEND_WEIGHTS] = "BLENDWEIGHT";
    mParamSemanticMap[Parameter::Semantic::BLEND_INDICES] = "BLENDINDICES";
    mParamSemanticMap[Parameter::Semantic::NORMAL] = "NORMAL";
    mParamSemanticMap[Parameter::Semantic::COLOR] = "COLOR";
    mParamSemanticMap[Parameter::Semantic::TEXTURE_COORDINATES] = "TEXCOORD";
    mParamSemanticMap[Parameter::Semantic::BINORMAL] = "BINORMAL";
    mParamSemanticMap[Parameter::Semantic::TANGENT] = "TANGENT";
}

ProgramWriter::~ProgramWriter() = default;

void ProgramWriter::writeParameter(std::ostream& os, const ParameterPtr& parameter)
{
    os << mGpuConstTypeMap[parameter->getType()] << '\t' << parameter->getName();
    if (parameter->isArray())
        os << '[' << parameter->getSize() << ']';
}

void ProgramWriter::writeSamplerParameter(std::ostream& os, const UniformParameterPtr& parameter)
{
    using enum GpuConstantType;
    if (parameter->getType() == SAMPLER_EXTERNAL_OES)
    {
        os << "uniform\t";
        writeParameter(os, parameter);
        return;
    }

    switch(parameter->getType())
    {
    case SAMPLER1D:
        os << "SAMPLER1D(";
        break;
    case SAMPLER2D:
        os << "SAMPLER2D(";
        break;
    case SAMPLER3D:
        os << "SAMPLER3D(";
        break;
    case SAMPLERCUBE:
        os << "SAMPLERCUBE(";
        break;
    case SAMPLER2DSHADOW:
        os << "SAMPLER2DSHADOW(";
        break;
    case SAMPLER2DARRAY:
        os << "SAMPLER2DARRAY(";
        break;
    default:
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "unsupported sampler type");
    }
    os << parameter->getName() << ", " << parameter->getIndex() << ")";
}

void ProgramWriter::writeParameterSemantic(std::ostream& os, const ParameterPtr& parameter)
{
    OgreAssertDbg(parameter->getSemantic() != Parameter::Semantic::UNKNOWN, "invalid semantic");
    os << mParamSemanticMap[parameter->getSemantic()];

    if (parameter->getSemantic() == Parameter::Semantic::TEXTURE_COORDINATES ||
        (parameter->getSemantic() == Parameter::Semantic::COLOR && parameter->getIndex() > 0))
    {
        os << parameter->getIndex();
    }
}

}
