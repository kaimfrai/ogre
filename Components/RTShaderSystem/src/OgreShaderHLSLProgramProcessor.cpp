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

import :ShaderHLSLProgramProcessor;
import :ShaderProgram;
import :ShaderProgramProcessor;
import :ShaderProgramSet;

import Ogre.Core;

import <initializer_list>;

namespace Ogre::RTShader {
std::string_view const constinit HLSLProgramProcessor::TargetLanguage = "hlsl";

//-----------------------------------------------------------------------------
HLSLProgramProcessor::HLSLProgramProcessor()
= default;

//-----------------------------------------------------------------------------
HLSLProgramProcessor::~HLSLProgramProcessor()
= default;

//-----------------------------------------------------------------------------
auto HLSLProgramProcessor::preCreateGpuPrograms( ProgramSet* programSet ) -> bool
{
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    Function* vsMain   = vsProgram->getEntryPointFunction();
    Function* fsMain   = psProgram->getEntryPointFunction();    
    bool success;

    // Compact vertex shader outputs.
    success = ProgramProcessor::compactVsOutputs(vsMain, fsMain);
    if (success == false)   
        return false;   

    return true;
}

//-----------------------------------------------------------------------------
auto HLSLProgramProcessor::postCreateGpuPrograms( ProgramSet* programSet ) -> bool
{
    // Bind vertex auto parameters.
    for(auto type : {GpuProgramType::VERTEX_PROGRAM, GpuProgramType::FRAGMENT_PROGRAM})
        bindAutoParameters(programSet->getCpuProgram(type), programSet->getGpuProgram(type));

    return true;
}

}
