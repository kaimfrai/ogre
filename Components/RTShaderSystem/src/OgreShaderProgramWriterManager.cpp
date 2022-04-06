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

#include <cassert>
#include <map>
#include <string>
#include <utility>

#include "OgrePrerequisites.hpp"
#include "OgreShaderGLSLESProgramWriter.hpp"
#include "OgreShaderGLSLProgramWriter.hpp"
#include "OgreShaderHLSLProgramWriter.hpp"
#include "OgreShaderProgramWriter.hpp"
#include "OgreShaderProgramWriterManager.hpp"
#include "OgreSingleton.hpp"

namespace Ogre {

//-----------------------------------------------------------------------
template<> 
RTShader::ProgramWriterManager* Singleton<RTShader::ProgramWriterManager>::msSingleton = nullptr;

namespace RTShader {
//-----------------------------------------------------------------------
auto ProgramWriterManager::getSingletonPtr() -> ProgramWriterManager*
{
    return msSingleton;
}
//-----------------------------------------------------------------------
auto ProgramWriterManager::getSingleton() -> ProgramWriterManager&
{  
    assert( msSingleton );  
    return ( *msSingleton );  
}
//-----------------------------------------------------------------------
ProgramWriterManager::ProgramWriterManager()
{
    // Add standard shader writer factories
    addProgramWriter("glsl", new GLSLProgramWriter());
    addProgramWriter("hlsl", new HLSLProgramWriter());

    addProgramWriter("glslang", new GLSLProgramWriter());
    addProgramWriter("glsles", new GLSLESProgramWriter());
}
//-----------------------------------------------------------------------
ProgramWriterManager::~ProgramWriterManager()
{
    for(auto& it : mProgramWriters)
    {
        delete it.second;
    }
}
//-----------------------------------------------------------------------
void ProgramWriterManager::addProgramWriter(const String& lang, ProgramWriter* writer)
{
    mProgramWriters[lang] = writer;
}
//-----------------------------------------------------------------------
auto ProgramWriterManager::isLanguageSupported(const String& lang) -> bool
{
    return mProgramWriters.find(lang) != mProgramWriters.end();
}
}
}
