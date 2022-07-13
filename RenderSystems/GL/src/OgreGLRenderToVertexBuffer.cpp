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

#include "OgreGLRenderToVertexBuffer.hpp"

#include <cassert>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreException.hpp"
#include "OgreGLHardwareBuffer.hpp"
#include "OgreGLPrerequisites.hpp"
#include "OgreGLRenderSystem.hpp"
#include "OgreGLSLLinkProgram.hpp"
#include "OgreGLSLLinkProgramManager.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgreHardwareBuffer.hpp"
#include "OgreHardwareBufferManager.hpp"
#include "OgreLog.hpp"
#include "OgreLogManager.hpp"
#include "OgreMaterial.hpp"
#include "OgreMatrix4.hpp"
#include "OgrePass.hpp"
#include "OgreRenderOperation.hpp"
#include "OgreRenderable.hpp"
#include "OgreRoot.hpp"
#include "OgreSceneManager.hpp"
#include "OgreStringConverter.hpp"
#include "OgreTechnique.hpp"
#include "OgreVertexIndexData.hpp"

namespace Ogre {
//-----------------------------------------------------------------------------
    static auto getR2VBPrimitiveType(RenderOperation::OperationType operationType) -> GLint
    {
        switch (operationType)
        {
        case RenderOperation::OperationType::POINT_LIST:
            return GL_POINTS;
        case RenderOperation::OperationType::LINE_LIST:
            return GL_LINES;
        case RenderOperation::OperationType::TRIANGLE_LIST:
            return GL_TRIANGLES;
        default:
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "GL RenderToVertexBuffer"
                "can only output point lists, line lists, or triangle lists",
                "OgreGLRenderToVertexBuffer::getR2VBPrimitiveType");
        }
    }
//-----------------------------------------------------------------------------
    static auto getVertexCountPerPrimitive(RenderOperation::OperationType operationType) -> GLint
    {
        //We can only get points, lines or triangles since they are the only
        //legal R2VB output primitive types
        switch (operationType)
        {
        case RenderOperation::OperationType::POINT_LIST:
            return 1;
        case RenderOperation::OperationType::LINE_LIST:
            return 2;
        default:
        case RenderOperation::OperationType::TRIANGLE_LIST:
            return 3;
        }
    }
//-----------------------------------------------------------------------------
    static void checkGLError(bool logError, bool throwException,
        std::string_view sectionName = BLANKSTRING)
    {
        String msg;
        bool foundError = false;

        // get all the GL errors
        GLenum glErr = glGetError();
        while (glErr != GL_NO_ERROR)
        {
            msg += glErrorToString(glErr);
            glErr = glGetError();
            foundError = true;  
        }

        if (foundError && (logError || throwException))
        {
            String fullErrorMessage = ::std::format("GL Error : {} in {}", msg, sectionName);
            if (logError)
            {
                LogManager::getSingleton().getDefaultLog()->logMessage(fullErrorMessage, LogMessageLevel::Critical);
            }
            if (throwException)
            {
                OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, 
                    fullErrorMessage, "OgreGLRenderToVertexBuffer");
            }
        }
    }
//-----------------------------------------------------------------------------
    GLRenderToVertexBuffer::GLRenderToVertexBuffer()  
    {
        mVertexBuffers[0].reset();
        mVertexBuffers[1].reset();

         // create query objects
        glGenQueries(1, &mPrimitivesDrawnQuery);
    }
//-----------------------------------------------------------------------------
    GLRenderToVertexBuffer::~GLRenderToVertexBuffer()
    {
        glDeleteQueries(1, &mPrimitivesDrawnQuery);
    }
//-----------------------------------------------------------------------------
    void GLRenderToVertexBuffer::getRenderOperation(RenderOperation& op)
    {
        op.operationType = mOperationType;
        op.useIndexes = false;
        op.vertexData = mVertexData.get();
    }
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
    void GLRenderToVertexBuffer::update(SceneManager* sceneMgr)
    {
        checkGLError(true, false, "start of GLRenderToVertexBuffer::update");

        size_t bufSize = mVertexData->vertexDeclaration->getVertexSize(0) * mMaxVertexCount;
        if (!mVertexBuffers[0] || mVertexBuffers[0]->getSizeInBytes() != bufSize)
        {
            //Buffers don't match. Need to reallocate.
            mResetRequested = true;
        }
        
        //Single pass only for now
        Ogre::Pass* r2vbPass = mMaterial->getBestTechnique()->getPass(0);
        //Set pass before binding buffers to activate the GPU programs
        sceneMgr->_setPass(r2vbPass);
        
        checkGLError(true, false);

        bindVerticesOutput(r2vbPass);

        r2vbPass->_updateAutoParams(sceneMgr->_getAutoParamDataSource(), GpuParamVariability::GLOBAL);

        RenderOperation renderOp;
        size_t targetBufferIndex;
        if (mResetRequested || mResetsEveryUpdate)
        {
            //Use source data to render to first buffer
            mSourceRenderable->getRenderOperation(renderOp);
            targetBufferIndex = 0;
        }
        else
        {
            //Use current front buffer to render to back buffer
            this->getRenderOperation(renderOp);
            targetBufferIndex = 1 - mFrontBufferIndex;
        }

        if (!mVertexBuffers[targetBufferIndex] || 
            mVertexBuffers[targetBufferIndex]->getSizeInBytes() != bufSize)
        {
            reallocateBuffer(targetBufferIndex);
        }

        auto* vertexBuffer = mVertexBuffers[targetBufferIndex]->_getImpl<GLHardwareBuffer>();
        GLuint bufferId = vertexBuffer->getGLBufferId();

        //Bind the target buffer
        glBindBufferOffsetNV(GL_TRANSFORM_FEEDBACK_BUFFER_NV, 0, bufferId, 0);

        glBeginTransformFeedbackNV(getR2VBPrimitiveType(mOperationType));

        glEnable(GL_RASTERIZER_DISCARD_NV);    // disable rasterization

        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_NV, mPrimitivesDrawnQuery);

        auto* targetRenderSystem = static_cast<GLRenderSystem*>(Root::getSingleton().getRenderSystem());
        //Draw the object
        targetRenderSystem->setWorldMatrix(Matrix4::IDENTITY);
        targetRenderSystem->setViewMatrix(Matrix4::IDENTITY);
        targetRenderSystem->setProjectionMatrix(Matrix4::IDENTITY);
        if (r2vbPass->hasVertexProgram())
        {
            targetRenderSystem->bindGpuProgramParameters(GpuProgramType::VERTEX_PROGRAM, 
                r2vbPass->getVertexProgramParameters(), GpuParamVariability::ALL);
        }
        if (r2vbPass->hasGeometryProgram())
        {
            targetRenderSystem->bindGpuProgramParameters(GpuProgramType::GEOMETRY_PROGRAM,
                r2vbPass->getGeometryProgramParameters(), GpuParamVariability::ALL);
        }
        targetRenderSystem->_render(renderOp);
        
        //Finish the query
        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_NV);
        glDisable(GL_RASTERIZER_DISCARD_NV);
        glEndTransformFeedbackNV();

        //read back query results
        GLuint primitivesWritten;
        glGetQueryObjectuiv(mPrimitivesDrawnQuery, GL_QUERY_RESULT, &primitivesWritten);
        mVertexData->vertexCount = primitivesWritten * getVertexCountPerPrimitive(mOperationType);

        checkGLError(true, true, "GLRenderToVertexBuffer::update");

        //Switch the vertex binding if necessary
        if (targetBufferIndex != mFrontBufferIndex)
        {
            mVertexData->vertexBufferBinding->unsetAllBindings();
            mVertexData->vertexBufferBinding->setBinding(0, mVertexBuffers[targetBufferIndex]);
            mFrontBufferIndex = targetBufferIndex;
        }

        glDisable(GL_RASTERIZER_DISCARD_NV);    // enable rasterization

        //Clear the reset flag
        mResetRequested = false;
    }
//-----------------------------------------------------------------------------
    void GLRenderToVertexBuffer::reallocateBuffer(size_t index)
    {
        assert(index == 0 || index == 1);
        if (mVertexBuffers[index])
        {
            mVertexBuffers[index].reset();
        }
        
        mVertexBuffers[index] = HardwareBufferManager::getSingleton().createVertexBuffer(
            mVertexData->vertexDeclaration->getVertexSize(0), mMaxVertexCount, 
            HardwareBuffer::STATIC_WRITE_ONLY
            );
    }
//-----------------------------------------------------------------------------
    auto GLRenderToVertexBuffer::getSemanticVaryingName(VertexElementSemantic semantic, unsigned short index) -> String
    {
        switch (semantic)
        {
        case VertexElementSemantic::POSITION:
            return "gl_Position";
        case VertexElementSemantic::TEXTURE_COORDINATES:
            return ::std::format("gl_TexCoord[{}]", index);
        case VertexElementSemantic::DIFFUSE:
            return "gl_FrontColor";
        case VertexElementSemantic::SPECULAR:
            return "gl_FrontSecondaryColor";
        //TODO : Implement more?
        default:
            OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, 
                "Unsupported vertex element semantic in render to vertex buffer",
                "OgreGLRenderToVertexBuffer::getSemanticVaryingName");
        }
    }
//-----------------------------------------------------------------------------
    auto GLRenderToVertexBuffer::getGLSemanticType(VertexElementSemantic semantic) -> GLint
    {
        switch (semantic)
        {
        case VertexElementSemantic::POSITION:
            return GL_POSITION;
        case VertexElementSemantic::TEXTURE_COORDINATES:
            return GL_TEXTURE_COORD_NV;
        case VertexElementSemantic::DIFFUSE:
            return GL_PRIMARY_COLOR;
        case VertexElementSemantic::SPECULAR:
            return GL_SECONDARY_COLOR_NV;
        //TODO : Implement more?
        default:
            OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, 
                "Unsupported vertex element semantic in render to vertex buffer",
                "OgreGLRenderToVertexBuffer::getGLSemanticType");
            
        }
    }
//-----------------------------------------------------------------------------
    void GLRenderToVertexBuffer::bindVerticesOutput(Pass* pass)
    {
        VertexDeclaration* declaration = mVertexData->vertexDeclaration;
        bool useVaryingAttributes = false;
        
        //Check if we are FixedFunc/ASM shaders (Static attributes) or GLSL (Varying attributes)
        //We assume that there isn't a mix of GLSL and ASM as this is illegal
        GpuProgram* sampleProgram = nullptr;
        if (pass->hasVertexProgram())
        {
            sampleProgram = pass->getVertexProgram().get();
        }
        else if (pass->hasGeometryProgram())
        {
            sampleProgram = pass->getGeometryProgram().get();
        }
        if ((sampleProgram != nullptr) && (sampleProgram->getLanguage() == "glsl"))
        {
            useVaryingAttributes = true;
        }

        if (useVaryingAttributes)
        {
            //Have GLSL shaders, using varying attributes
            GLSL::GLSLLinkProgram* linkProgram = GLSL::GLSLLinkProgramManager::getSingleton().getActiveLinkProgram();
            uint linkProgramId = linkProgram->getGLHandle();
            
            std::vector<GLint> locations;
            for (unsigned short e=0; e < declaration->getElementCount(); e++)
            {
                const VertexElement* element =declaration->getElement(e);
                String varyingName = getSemanticVaryingName(element->getSemantic(), element->getIndex());
                GLint location = glGetVaryingLocationNV(linkProgramId, varyingName.c_str());
                if (location < 0)
                {
                    OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, 
                        ::std::format("GLSL link program does not output {}"
                        " so it cannot fill the requested vertex buffer",
                        varyingName),
                        "OgreGLRenderToVertexBuffer::bindVerticesOutput");
                }
                locations.push_back(location);
            }
            glTransformFeedbackVaryingsNV(
                linkProgramId, static_cast<GLsizei>(locations.size()), 
                &locations[0], GL_INTERLEAVED_ATTRIBS_NV);
        }
        else
        {
            //Either fixed function or assembly (CG = assembly) shaders
            std::vector<GLint> attribs;
            for (unsigned short e=0; e < declaration->getElementCount(); e++)
            {
                const VertexElement* element = declaration->getElement(e);
                //Type
                attribs.push_back(getGLSemanticType(element->getSemantic()));
                //Number of components
                attribs.push_back(VertexElement::getTypeCount(element->getType()));
                //Index
                attribs.push_back(element->getIndex());
            }
            
            glTransformFeedbackAttribsNV(
                static_cast<GLuint>(declaration->getElementCount()), 
                &attribs[0], GL_INTERLEAVED_ATTRIBS_NV);
        }

        checkGLError(true, true, "GLRenderToVertexBuffer::bindVerticesOutput");
    }
}
