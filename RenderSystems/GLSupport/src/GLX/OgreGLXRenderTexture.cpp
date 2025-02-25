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

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <climits>

module Ogre.RenderSystems.GLSupport;

import :GLX.Context;
import :GLX.GLSupport;
import :GLX.RenderTexture;

import Ogre.Core;

import <format>;
import <string>;

enum {
GLX_RGBA_FLOAT_ATI_BIT = 0x00000100,
GLX_RGBA_FLOAT_BIT =     0x00000004
};

namespace Ogre
{
    //-------------------------------------------------------------------------------------------------//
    GLXPBuffer::GLXPBuffer(GLXGLSupport* glsupport, PixelComponentType format, size_t width, size_t height):
        GLPBuffer(format, width, height),  mGLSupport(glsupport)
    {
        Display *glDisplay = mGLSupport->getGLDisplay();
        ::GLXDrawable glxDrawable = 0;
        ::GLXFBConfig fbConfig = nullptr;
        
        int bits = 0;
        
        using enum PixelComponentType;
        switch (mFormat)
        {
        case BYTE:
            bits = 8; 
            break;
            
        case SHORT:
            bits = 16; 
            break;
            
        case FLOAT16:
            bits = 16; 
            break;
            
        case FLOAT32:
            bits = 32; 
            break;
            
        default: 
            break;
        }
        
        int renderAttrib = GLX_RENDER_TYPE;
        int renderValue  = GLX_RGBA_BIT;
        
        if (mFormat == PixelComponentType::FLOAT16 || mFormat == PixelComponentType::FLOAT32)
        {
            if (glsupport->checkExtension("GLX_NV_float_buffer"))
            {
                renderAttrib = GLX_FLOAT_COMPONENTS_NV;
                renderValue  = GL_TRUE;
            }
            
            if (glsupport->checkExtension("GLX_ATI_pixel_format_float"))
            {
                renderAttrib = GLX_RENDER_TYPE;
                renderValue  = GLX_RGBA_FLOAT_ATI_BIT;
            }
            
            if (glsupport->checkExtension("GLX_ARB_fbconfig_float"))
            {
                renderAttrib = GLX_RENDER_TYPE;
                renderValue  = GLX_RGBA_FLOAT_BIT;
            }
            
            if (renderAttrib == GLX_RENDER_TYPE && renderValue == GLX_RGBA_BIT)
            {
                OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "No support for Floating point PBuffers",  "GLRenderTexture::createPBuffer");
            }
        }
        
        int minAttribs[] = {
            GLX_DRAWABLE_TYPE, GLX_PBUFFER,
            renderAttrib,     renderValue,
            GLX_DOUBLEBUFFER,  0,
            None
        };
        
        int maxAttribs[] = {
            GLX_RED_SIZE,     bits,
            GLX_GREEN_SIZE, bits,
            GLX_BLUE_SIZE,   bits,
            GLX_ALPHA_SIZE, bits,
            GLX_STENCIL_SIZE,  INT_MAX,
            None
        };
        
        int pBufferAttribs[] = {
            GLX_PBUFFER_WIDTH,    (int)mWidth,
            GLX_PBUFFER_HEIGHT,  (int)mHeight,
            GLX_PRESERVED_CONTENTS, GL_TRUE,
            None
        };
        
        fbConfig = mGLSupport->selectFBConfig(minAttribs, maxAttribs);
        
        glxDrawable = glXCreatePbuffer(glDisplay, fbConfig, pBufferAttribs);
        
        if (! fbConfig || ! glxDrawable) 
        {
            OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, "Unable to create Pbuffer", "GLXPBuffer::GLXPBuffer");
        }
        
        GLint fbConfigID;
        GLuint iWidth, iHeight;
        
        glXGetFBConfigAttrib(glDisplay, fbConfig, GLX_FBCONFIG_ID, &fbConfigID);
        glXQueryDrawable(glDisplay, glxDrawable, GLX_WIDTH, &iWidth);
        glXQueryDrawable(glDisplay, glxDrawable, GLX_HEIGHT, &iHeight);
        
        mWidth = iWidth;  
        mHeight = iHeight;
        LogManager::getSingleton().logMessage(LogMessageLevel::Normal, ::std::format("GLXPBuffer::create used final dimensions {} x {}", mWidth, mHeight));
        LogManager::getSingleton().logMessage(::std::format("GLXPBuffer::create used FBConfigID {}", fbConfigID));
        
        mContext = ::std::make_unique<GLXContext>(mGLSupport, fbConfig, glxDrawable);
    }
    
    //-------------------------------------------------------------------------------------------------//
    GLXPBuffer::~GLXPBuffer()
    {
        glXDestroyPbuffer(mGLSupport->getGLDisplay(), mContext->mDrawable);
        
        LogManager::getSingleton().logMessage(LogMessageLevel::Normal, "GLXPBuffer::PBuffer destroyed");
    }
    
    //-------------------------------------------------------------------------------------------------//
    auto GLXPBuffer::getContext() const noexcept -> GLContext *
    {
        return mContext.get();
    }
}
