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

#include <cassert>

module Ogre.Components.RTShaderSystem;

import :ShaderGLSLESProgramWriter;
import :ShaderGLSLProgramWriter;
import :ShaderHLSLProgramWriter;
import :ShaderProgramWriter;
import :ShaderProgramWriterManager;

import Ogre.Core;

import <map>;
import <memory>;
import <string>;
import <utility>;

namespace Ogre {

//-----------------------------------------------------------------------
template<> 
RTShader::ProgramWriterManager* Singleton<RTShader::ProgramWriterManager>::msSingleton = nullptr;

namespace RTShader {
//-----------------------------------------------------------------------
auto ProgramWriterManager::getSingletonPtr() noexcept -> ProgramWriterManager*
{
    return msSingleton;
}
//-----------------------------------------------------------------------
auto ProgramWriterManager::getSingleton() noexcept -> ProgramWriterManager&
{  
    assert( msSingleton );  
    return ( *msSingleton );  
}
//-----------------------------------------------------------------------
ProgramWriterManager::ProgramWriterManager()
{
    // Add standard shader writer factories
    addProgramWriter("glsl", ::std::make_unique<GLSLProgramWriter>());
    addProgramWriter("hlsl", ::std::make_unique<HLSLProgramWriter>());

    addProgramWriter("glslang", ::std::make_unique<GLSLProgramWriter>());
    addProgramWriter("glsles", ::std::make_unique<GLSLESProgramWriter>());
}
//-----------------------------------------------------------------------
void ProgramWriterManager::addProgramWriter(std::string_view lang, ::std::unique_ptr<ProgramWriter> writer)
{
    mProgramWriters[lang] = ::std::move(writer);
}
//-----------------------------------------------------------------------
auto ProgramWriterManager::isLanguageSupported(std::string_view lang) -> bool
{
    return mProgramWriters.find(lang) != mProgramWriters.end();
}
}
}
