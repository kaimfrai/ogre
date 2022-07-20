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

#include <cstddef>

module Ogre.RenderSystems.GL;

import :GpuProgram;

import Ogre.Core;

import <format>;
import <map>;
import <string>;
import <utility>;

namespace Ogre {
class ResourceManager;

auto GLArbGpuProgram::getProgramType() const -> GLenum
{
    using enum GpuProgramType;
    switch (mType)
    {
        case VERTEX_PROGRAM:
        default:
            return GL_VERTEX_PROGRAM_ARB;
        case GEOMETRY_PROGRAM:
            return GL_GEOMETRY_PROGRAM_NV;
        case FRAGMENT_PROGRAM:
            return GL_FRAGMENT_PROGRAM_ARB;
    }
}

GLGpuProgram::GLGpuProgram(ResourceManager* creator, std::string_view name,
    ResourceHandle handle, std::string_view group, bool isManual, 
    ManualResourceLoader* loader) 
    : GpuProgram(creator, name, handle, group, isManual, loader)
{
    if (createParamDictionary("GLGpuProgram"))
    {
        setupBaseParamDictionary();
    }
}

GLGpuProgram::~GLGpuProgram()
{
    // have to call this here reather than in Resource destructor
    // since calling virtual methods in base destructors causes crash
    unload(); 
}

auto GLGpuProgramBase::isAttributeValid(VertexElementSemantic semantic, uint index) -> bool
{
    using enum VertexElementSemantic;
    // default implementation
    switch(semantic)
    {
        case POSITION:
        case NORMAL:
        case DIFFUSE:
        case SPECULAR:
        case TEXTURE_COORDINATES:
            return false;
        case BLEND_WEIGHTS:
        case BLEND_INDICES:
        case BINORMAL:
        case TANGENT:
            return true; // with default binding
    }

    return false;
}

GLArbGpuProgram::GLArbGpuProgram(ResourceManager* creator, std::string_view name, 
    ResourceHandle handle, std::string_view group, bool isManual, 
    ManualResourceLoader* loader) 
    : GLGpuProgram(creator, name, handle, group, isManual, loader)
{
    glGenProgramsARB(1, &mProgramID);
}

GLArbGpuProgram::~GLArbGpuProgram()
{
    // have to call this here reather than in Resource destructor
    // since calling virtual methods in base destructors causes crash
    unload(); 
}

void GLArbGpuProgram::bindProgram()
{
    glEnable(getProgramType());
    glBindProgramARB(getProgramType(), mProgramID);
}

void GLArbGpuProgram::unbindProgram()
{
    glBindProgramARB(getProgramType(), 0);
    glDisable(getProgramType());
}

void GLArbGpuProgram::bindProgramParameters(GpuProgramParametersSharedPtr params, Ogre::GpuParamVariability mask)
{
    GLenum type = getProgramType();
    
    // only supports float constants
    GpuLogicalBufferStructPtr floatStruct = params->getLogicalBufferStruct();

    for (auto & i : floatStruct->map)
    {
        if (!!(i.second.variability & mask))
        {
            auto logicalIndex = static_cast<GLuint>(i.first);
            const float* pFloat = params->getFloatPointer(i.second.physicalIndex);
            // Iterate over the params, set in 4-float chunks (low-level)
            for (size_t j = 0; j < i.second.currentSize; j+=4)
            {
                glProgramLocalParameter4fvARB(type, logicalIndex, pFloat);
                pFloat += 4;
                ++logicalIndex;
            }
        }
    }
}

void GLArbGpuProgram::unloadImpl()
{
    glDeleteProgramsARB(1, &mProgramID);
}

void GLArbGpuProgram::loadFromSource()
{
    if (GL_INVALID_OPERATION == glGetError()) {
        LogManager::getSingleton().logMessage(::std::format("Invalid Operation before loading program {}", mName), LogMessageLevel::Critical);

    }
    glBindProgramARB(getProgramType(), mProgramID);
    glProgramStringARB(getProgramType(), GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)mSource.length(), mSource.c_str());

    if (GL_INVALID_OPERATION == glGetError())
    {
        GLint errPos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
        const char* errStr = (const char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
        OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, ::std::format("'{}' ", mName ) + errStr);
    }
    glBindProgramARB(getProgramType(), 0);
}

    
}
