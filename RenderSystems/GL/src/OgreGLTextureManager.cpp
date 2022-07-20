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

#include "glad/glad.h"

module Ogre.RenderSystems.GL;

import :PixelFormat;
import :RenderSystem;
import :Texture;
import :TextureManager;

import Ogre.Core;
import Ogre.RenderSystems.GLSupport;

namespace Ogre {
    //-----------------------------------------------------------------------------
    GLTextureManager::GLTextureManager(GLRenderSystem* renderSystem)
        : TextureManager(), mRenderSystem(renderSystem)
    {
        // register with group manager
        ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
    }
    //-----------------------------------------------------------------------------
    GLTextureManager::~GLTextureManager()
    {
        // unregister with group manager
        ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    }
    //-----------------------------------------------------------------------------
    auto GLTextureManager::createImpl(std::string_view name, ResourceHandle handle, 
        std::string_view group, bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* createParams) -> Resource*
    {
        return new GLTexture(this, name, handle, group, isManual, loader, mRenderSystem);
    }

    //-----------------------------------------------------------------------------
    auto GLTextureManager::getNativeFormat(TextureType ttype, PixelFormat format, HardwareBufferUsage usage) -> PixelFormat
    {
        // Adjust requested parameters to capabilities
        const RenderSystemCapabilities *caps = Root::getSingleton().getRenderSystem()->getCapabilities();

        // Check compressed texture support
        // if a compressed format not supported, revert to PixelFormat::A8R8G8B8
        if(PixelUtil::isCompressed(format) &&
            !caps->hasCapability( Capabilities::TEXTURE_COMPRESSION_DXT ))
        {
            return PixelFormat::BYTE_RGBA;
        }
        // if floating point textures not supported, revert to PixelFormat::A8R8G8B8
        if(PixelUtil::isFloatingPoint(format) &&
            !caps->hasCapability( Capabilities::TEXTURE_FLOAT ))
        {
            return PixelFormat::BYTE_RGBA;
        }
        
        // Check if this is a valid rendertarget format
        if( !!(usage & TextureUsage::RENDERTARGET) )
        {
            /// Get closest supported alternative
            /// If mFormat is supported it's returned
            return GLRTTManager::getSingleton().getSupportedAlternative(format);
        }

        if(GLPixelUtil::getGLInternalFormat(format) == GL_NONE)
        {
            return PixelFormat::BYTE_RGBA;
        }

        // Supported
        return format;

        
    }
    //-----------------------------------------------------------------------------
    auto GLTextureManager::isHardwareFilteringSupported(TextureType ttype, PixelFormat format, HardwareBufferUsage usage,
            bool preciseFormatOnly) -> bool
    {
        // precise format check
        if (!TextureManager::isHardwareFilteringSupported(ttype, format, usage, preciseFormatOnly))
            return false;

        // Assume non-floating point is supported always
        if (!PixelUtil::isFloatingPoint(getNativeFormat(ttype, format, usage)))
            return true;

        // check for floating point extension
        // note false positives on old harware: 
        // https://www.khronos.org/opengl/wiki/Floating_point_and_mipmapping_and_filtering
        return mRenderSystem->checkExtension("GL_ARB_texture_float");
    }

}
