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

#include "OgreGLTexture.hpp"

#include <memory>
#include <vector>

#include "OgreException.hpp"
#include "OgreGLHardwarePixelBuffer.hpp"
#include "OgreGLPixelFormat.hpp"
#include "OgreGLRenderSystem.hpp"
#include "OgreGLStateCacheManager.hpp"
#include "OgreHardwarePixelBuffer.hpp"
#include "OgrePixelFormat.hpp"
#include "OgrePlatform.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreTexture.hpp"
#include "OgreTextureManager.hpp"

namespace Ogre {
class ResourceManager;



    GLTexture::GLTexture(ResourceManager* creator, std::string_view name, 
        ResourceHandle handle, std::string_view group, bool isManual, 
        ManualResourceLoader* loader, GLRenderSystem* renderSystem) 
        : GLTextureCommon(creator, name, handle, group, isManual, loader),
          mRenderSystem(renderSystem)
    {
    }


    GLTexture::~GLTexture()
    {
        // have to call this here rather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        if (isLoaded())
        {
            unload(); 
        }
        else
        {
            freeInternalResources();
        }
    }

    auto GLTexture::getGLTextureTarget() const -> GLenum
    {
        switch(mTextureType)
        {
            case TextureType::_1D:
                return GL_TEXTURE_1D;
            case TextureType::_2D:
                return GL_TEXTURE_2D;
            case TextureType::_3D:
                return GL_TEXTURE_3D;
            case TextureType::CUBE_MAP:
                return GL_TEXTURE_CUBE_MAP;
            case TextureType::_2D_ARRAY:
                return GL_TEXTURE_2D_ARRAY_EXT;
            default:
                return 0;
        };
    }

    //* Creation / loading methods ********************************************
    void GLTexture::createInternalResourcesImpl()
    {
        OgreAssert(mTextureType != TextureType::EXTERNAL_OES,
                   "TextureType::EXTERNAL_OES is not available for openGL");

        // Convert to nearest power-of-two size if required
        mWidth = GLPixelUtil::optionalPO2(mWidth);      
        mHeight = GLPixelUtil::optionalPO2(mHeight);
        mDepth = GLPixelUtil::optionalPO2(mDepth);
        

        // Adjust format if required
        mFormat = TextureManager::getSingleton().getNativeFormat(mTextureType, mFormat, mUsage);
        
        // Check requested number of mipmaps
        auto maxMips = getMaxMipmaps();
        mNumMipmaps = mNumRequestedMipmaps;
        if(mNumMipmaps>maxMips)
            mNumMipmaps = maxMips;

        // Check if we can do HW mipmap generation
        mMipmapsHardwareGenerated = true;
        
        // Generate texture name
        glGenTextures( 1, &mTextureID );
        
        // Set texture type
        mRenderSystem->_getStateCacheManager()->bindGLTexture( getGLTextureTarget(), mTextureID );
        
        // This needs to be set otherwise the texture doesn't get rendered
        mRenderSystem->_getStateCacheManager()->setTexParameteri(getGLTextureTarget(), GL_TEXTURE_MAX_LEVEL,
                                                                 static_cast<GLint>(mNumMipmaps));

        if (!!(mUsage & TextureUsage::AUTOMIPMAP) && !!mNumRequestedMipmaps)
        {
            mRenderSystem->_getStateCacheManager()->setTexParameteri( getGLTextureTarget(), GL_GENERATE_MIPMAP, GL_TRUE );
        }
        
        // Allocate internal buffer so that glTexSubImageXD can be used
        // Internal format
        GLenum internalformat = GLPixelUtil::getGLInternalFormat(mFormat, mHwGamma);
        uint32 width = mWidth;
        uint32 height = mHeight;
        uint32 depth = mDepth;

        GLenum format = GLPixelUtil::getGLOriginFormat(mFormat);
        GLenum datatype = GLPixelUtil::getGLOriginDataType(mFormat);

        if(PixelUtil::isCompressed(mFormat))
        {
            // Compressed formats
            auto size = static_cast<GLsizei>(PixelUtil::getMemorySize(mWidth, mHeight, mDepth, mFormat));
            // Provide temporary buffer filled with zeroes as glCompressedTexImageXD does not
            // accept a 0 pointer like normal glTexImageXD
            // Run through this process for every mipmap to pregenerate mipmap piramid
            std::vector<uint8> tmpdata(size);
            
            for(uint32 mip=0; mip<=std::to_underlying(mNumMipmaps); mip++)
            {
                size = static_cast<GLsizei>(PixelUtil::getMemorySize(width, height, depth, mFormat));
                switch(mTextureType)
                {
                    case TextureType::_1D:
                        glCompressedTexImage1DARB(GL_TEXTURE_1D, mip, internalformat, 
                            width, 0, 
                            size, &tmpdata[0]);
                        break;
                    case TextureType::_2D:
                        glCompressedTexImage2DARB(GL_TEXTURE_2D, mip, internalformat,
                            width, height, 0, 
                            size, &tmpdata[0]);
                        break;
                    case TextureType::_2D_ARRAY:
                    case TextureType::_3D:
                        glCompressedTexImage3DARB(getGLTextureTarget(), mip, internalformat,
                            width, height, depth, 0, 
                            size, &tmpdata[0]);
                        break;
                    case TextureType::CUBE_MAP:
                        for(int face=0; face<6; face++) {
                            glCompressedTexImage2DARB(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mip, internalformat,
                                width, height, 0, 
                                size, &tmpdata[0]);
                        }
                        break;
                    case TextureType::EXTERNAL_OES:
                        OGRE_EXCEPT(
                            ExceptionCodes::RENDERINGAPI_ERROR,
                            "Attempt to create mipmaps for unsupported TextureType::EXTERNAL_OES, should never happen",
                            "GLTexture::createInternalResourcesImpl"
                        );
                        break;
                };
                if(width>1)
                    width = width/2;
                if(height>1)
                    height = height/2;
                if(depth>1 && mTextureType != TextureType::_2D_ARRAY)
                    depth = depth/2;
            }
        }
        else
        {
            // Run through this process to pregenerate mipmap pyramid
            for(uint32 mip=0; mip<=std::to_underlying(mNumMipmaps); mip++)
            {
                // Normal formats
                switch(mTextureType)
                {
                    case TextureType::_1D:
                        glTexImage1D(GL_TEXTURE_1D, mip, internalformat,
                            width, 0, 
                            format, datatype, nullptr);
    
                        break;
                    case TextureType::_2D:
                        glTexImage2D(GL_TEXTURE_2D, mip, internalformat,
                            width, height, 0, 
                            format, datatype, nullptr);
                        break;
                    case TextureType::_2D_ARRAY:
                    case TextureType::_3D:
                        glTexImage3D(getGLTextureTarget(), mip, internalformat,
                            width, height, depth, 0, 
                            format, datatype, nullptr);
                        break;
                    case TextureType::CUBE_MAP:
                        for(int face=0; face<6; face++) {
                            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mip, internalformat,
                                width, height, 0, 
                                format, datatype, nullptr);
                        }
                        break;
                    case TextureType::EXTERNAL_OES:
                        OGRE_EXCEPT(
                            ExceptionCodes::RENDERINGAPI_ERROR,
                            "Attempt to create mipmaps for unsupported TextureType::EXTERNAL_OES, should never happen",
                            "GLTexture::createInternalResourcesImpl"
                        );
                        break;
                };
                if(width>1)
                    width = width/2;
                if(height>1)
                    height = height/2;
                if(depth>1 && mTextureType != TextureType::_2D_ARRAY)
                    depth = depth/2;
            }
        }
        _createSurfaceList();
        // Get final internal format
        mFormat = getBuffer(0, TextureMipmap{})->getFormat();
    }

    //*************************************************************************
    
    void GLTexture::freeInternalResourcesImpl()
    {
        if (GLStateCacheManager* stateCacheManager = mRenderSystem->_getStateCacheManager())
        {
            glDeleteTextures(1, &mTextureID);
            stateCacheManager->invalidateStateForTexture(mTextureID);
        }
    }
    
    //---------------------------------------------------------------------------------------------
    void GLTexture::_createSurfaceList()
    {
        mSurfaceList.clear();
        
        uint32 depth = mDepth;

        // For all faces and mipmaps, store surfaces as HardwarePixelBufferSharedPtr
        for(GLint face=0; face<static_cast<GLint>(getNumFaces()); face++)
        {
            uint32 width = mWidth;
            uint32 height = mHeight;

            for(uint32 mip=0; mip<=std::to_underlying(getNumMipmaps()); mip++)
            {
                auto buf = std::make_shared<GLTextureBuffer>(mRenderSystem, this, face, mip, width, height, depth);
                mSurfaceList.push_back(buf);
                
                if (width > 1)
                    width = width / 2;
                if (height > 1)
                    height = height / 2;
                if (depth > 1 && mTextureType != TextureType::_2D_ARRAY)
                    depth = depth / 2;
            }
        }
    }
}

