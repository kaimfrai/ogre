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
module;

#include <cassert>

module Ogre.RenderSystems.GL;

import :HardwarePixelBuffer;
import :PBRenderTexture;

import Ogre.Core;
import Ogre.RenderSystems.GLSupport;

import <string>;

namespace Ogre {
class GLContext;
class RenderTexture;

//-----------------------------------------------------------------------------  
    GLPBRenderTexture::GLPBRenderTexture(GLPBRTTManager *manager, std::string_view name, 
        const GLSurfaceDesc &target, bool writeGamma, uint fsaa):
        GLRenderTexture(name, target, writeGamma, fsaa),
        mManager(manager)
    {
        mPBFormat = PixelUtil::getComponentType(target.buffer->getFormat());
        
        mManager->requestPBuffer(mPBFormat, mWidth, mHeight);
    }
    GLPBRenderTexture::~GLPBRenderTexture()
    {
        // Release PBuffer
        mManager->releasePBuffer(mPBFormat);
    }
    void GLPBRenderTexture::getCustomAttribute(std::string_view name, void* pData)
    {
        if( name == GLRenderTexture::CustomAttributeString_TARGET )
        {
            GLSurfaceDesc &target = *static_cast<GLSurfaceDesc*>(pData);
            target.buffer = static_cast<GLHardwarePixelBufferCommon*>(mBuffer);
            target.zoffset = mZOffset;
        }
        else if (name == GLRenderTexture::CustomAttributeString_GLCONTEXT )
        {
            // Get PBuffer for our internal format
            *static_cast<GLContext**>(pData) = getContext();
        }
    }

    auto GLPBRenderTexture::getContext() const noexcept -> GLContext*
    {
        return mManager->getContextFor(mPBFormat, mWidth, mHeight);
    }

//-----------------------------------------------------------------------------  
    GLPBRTTManager::GLPBRTTManager(GLNativeSupport *support, RenderTarget *mainwindow):
        mSupport(support),
        mMainWindow(mainwindow)
        
    {
        mMainContext = dynamic_cast<GLRenderTarget*>(mMainWindow)->getContext();
    }  
    GLPBRTTManager::~GLPBRTTManager()
    {
        // Delete remaining PBuffers
        for(auto & mPBuffer : mPBuffers)
        {
            delete mPBuffer.pb;
        }
    }

    auto GLPBRTTManager::createRenderTexture(std::string_view name, 
        const GLSurfaceDesc &target, bool writeGamma, uint fsaa) -> RenderTexture *
    {
        return new GLPBRenderTexture(this, name, target, writeGamma, fsaa);
    }
    
    auto GLPBRTTManager::checkFormat(PixelFormat format) -> bool 
    { 
        return true; 
    }

    void GLPBRTTManager::bind(RenderTarget *target)
    {
        // Nothing to do here
        // Binding of context is done by GL subsystem, as contexts are also used for RenderWindows
    }

    void GLPBRTTManager::unbind(RenderTarget *target)
    { 
        // Copy on unbind
        GLSurfaceDesc surface;
        surface.buffer = nullptr;
        target->getCustomAttribute(GLRenderTexture::CustomAttributeString_TARGET, &surface);
        if(surface.buffer)
            static_cast<GLTextureBuffer*>(surface.buffer)->copyFromFramebuffer(surface.zoffset);
    }
    
    void GLPBRTTManager::requestPBuffer(PixelComponentType ctype, uint32 width, uint32 height)
    {
        //Check size
        if(mPBuffers[std::to_underlying(ctype)].pb)
        {
            if(mPBuffers[std::to_underlying(ctype)].pb->getWidth()<width || mPBuffers[std::to_underlying(ctype)].pb->getHeight()<height)
            {
                // If the current PBuffer is too small, destroy it and create a new one
                delete mPBuffers[std::to_underlying(ctype)].pb;
                mPBuffers[std::to_underlying(ctype)].pb = nullptr;
            }
        }
        if(!mPBuffers[std::to_underlying(ctype)].pb)
        {
            // Create pbuffer via rendersystem
            mPBuffers[std::to_underlying(ctype)].pb = mSupport->createPBuffer(ctype, width, height);
        }
        
        ++mPBuffers[std::to_underlying(ctype)].refcount;
    }
    
    void GLPBRTTManager::releasePBuffer(PixelComponentType ctype)
    {
        --mPBuffers[std::to_underlying(ctype)].refcount;
        if(mPBuffers[std::to_underlying(ctype)].refcount == 0)
        {
            delete mPBuffers[std::to_underlying(ctype)].pb;
            mPBuffers[std::to_underlying(ctype)].pb = nullptr;
        }
    }
    
    auto GLPBRTTManager::getContextFor(PixelComponentType ctype, uint32 width, uint32 height) -> GLContext *
    {
        // Faster to return main context if the RTT is smaller than the window size
        // and ctype is PixelComponentType::BYTE. This must be checked every time because the window might have been resized
        if(ctype == PixelComponentType::BYTE)
        {
            if(width <= mMainWindow->getWidth() && height <= mMainWindow->getHeight())
                return mMainContext;
        }
        assert(mPBuffers[std::to_underlying(ctype)].pb);
        return mPBuffers[std::to_underlying(ctype)].pb->getContext();
    }
//---------------------------------------------------------------------------------------------

}
