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

module Ogre.Core;

import :Bitwise;
import :Common;
import :Exception;
import :HardwarePixelBuffer;
import :Image;
import :Log;
import :LogManager;
import :RenderSystem;
import :RenderSystemCapabilities;
import :ResourceGroupManager;
import :ResourceManager;
import :Root;
import :String;
import :Texture;
import :TextureManager;

import <algorithm>;
import <atomic>;
import <memory>;

namespace Ogre {
    const char* Texture::CUBEMAP_SUFFIXES[] = {"_rt", "_lf", "_up", "_dn", "_fr", "_bk"};
    //--------------------------------------------------------------------------
    Texture::Texture(ResourceManager* creator, std::string_view name, 
        ResourceHandle handle, std::string_view group, bool isManual, 
        ManualResourceLoader* loader)
        : Resource(creator, name, handle, group, isManual, loader)
            
    {
        if (createParamDictionary("Texture"))
        {
            // Define the parameters that have to be present to load
            // from a generic source; actually there are none, since when
            // predeclaring, you use a texture file which includes all the
            // information required.
        }

        // Set some defaults for default load path
        if (TextureManager::getSingletonPtr())
        {
            TextureManager& tmgr = TextureManager::getSingleton();
            setNumMipmaps(tmgr.getDefaultNumMipmaps());
            setDesiredBitDepths(tmgr.getPreferredIntegerBitDepth(), tmgr.getPreferredFloatBitDepth());
        }

        
    }
    //--------------------------------------------------------------------------
    void Texture::loadRawData( DataStreamPtr& stream, 
        ushort uWidth, ushort uHeight, PixelFormat eFormat)
    {
        Image img;
        img.loadRawData(stream, uWidth, uHeight, 1, eFormat);
        loadImage(img);
    }
    //--------------------------------------------------------------------------    
    void Texture::loadImage( const Image &img )
    {
        OgreAssert(img.getSize(), "cannot load empty image");
        LoadingState old = mLoadingState.load();

        // Scope lock for actual loading
        try
        {
            _loadImages({&img});
        }
        catch (...)
        {
            // Reset loading in-progress flag in case failed for some reason
            mLoadingState.store(old);
            // Re-throw
            throw;
        }

        // Notify manager
        if(getCreator())
            getCreator()->_notifyResourceLoaded(this);

        // No deferred loading events since this method is not called in background
    }
    //--------------------------------------------------------------------------
    void Texture::setFormat(PixelFormat pf)
    {
        mFormat = pf;
        mDesiredFormat = pf;
    }
    //--------------------------------------------------------------------------
    auto Texture::hasAlpha() const -> bool
    {
        return PixelUtil::hasAlpha(mFormat);
    }
    //--------------------------------------------------------------------------
    void Texture::setDesiredIntegerBitDepth(ushort bits)
    {
        mDesiredIntegerBitDepth = bits;
    }
    //--------------------------------------------------------------------------
    auto Texture::getDesiredIntegerBitDepth() const noexcept -> ushort
    {
        return mDesiredIntegerBitDepth;
    }
    //--------------------------------------------------------------------------
    void Texture::setDesiredFloatBitDepth(ushort bits)
    {
        mDesiredFloatBitDepth = bits;
    }
    //--------------------------------------------------------------------------
    auto Texture::getDesiredFloatBitDepth() const noexcept -> ushort
    {
        return mDesiredFloatBitDepth;
    }
    //--------------------------------------------------------------------------
    void Texture::setDesiredBitDepths(ushort integerBits, ushort floatBits)
    {
        mDesiredIntegerBitDepth = integerBits;
        mDesiredFloatBitDepth = floatBits;
    }
    //--------------------------------------------------------------------------
    void Texture::setTreatLuminanceAsAlpha(bool asAlpha)
    {
        mTreatLuminanceAsAlpha = asAlpha;
    }
    //--------------------------------------------------------------------------
    auto Texture::calculateSize() const -> size_t
    {
        return getNumFaces() * PixelUtil::getMemorySize(mWidth, mHeight, mDepth, mFormat);
    }
    //--------------------------------------------------------------------------
    auto Texture::getNumFaces() const noexcept -> uint32
    {
        return getTextureType() == TextureType::CUBE_MAP ? 6 : 1;
    }
    //--------------------------------------------------------------------------
    void Texture::_loadImages( const ConstImagePtrList& images )
    {
        OgreAssert(!images.empty(), "Cannot load empty vector of images");

        // Set desired texture size and properties from images[0]
        mSrcWidth = mWidth = images[0]->getWidth();
        mSrcHeight = mHeight = images[0]->getHeight();
        mSrcDepth = mDepth = images[0]->getDepth();
        mSrcFormat = images[0]->getFormat();

        if(!mLayerNames.empty() && mTextureType != TextureType::CUBE_MAP)
            mDepth = uint32(mLayerNames.size());

        if(mTreatLuminanceAsAlpha && mSrcFormat == PixelFormat::L8)
            mDesiredFormat = PixelFormat::A8;

        if (mDesiredFormat != PixelFormat::UNKNOWN)
        {
            // If have desired format, use it
            mFormat = mDesiredFormat;
        }
        else
        {
            // Get the format according with desired bit depth
            mFormat = PixelUtil::getFormatForBitDepths(mSrcFormat, mDesiredIntegerBitDepth, mDesiredFloatBitDepth);
        }

        // The custom mipmaps in the image clamp the request
        auto imageMips = images[0]->getNumMipmaps();

        if(imageMips > TextureMipmap{})
        {
            mNumMipmaps = mNumRequestedMipmaps = std::min(mNumRequestedMipmaps, imageMips);
            // Disable flag for auto mip generation
            mUsage &= ~TextureUsage::AUTOMIPMAP;
        }

        // Create the texture
        createInternalResources();
        // Check if we're loading one image with multiple faces
        // or a vector of images representing the faces
        uint32 faces;
        bool multiImage; // Load from multiple images?
        if(images.size() > 1)
        {
            faces = uint32(images.size());
            multiImage = true;
        }
        else
        {
            faces = images[0]->getNumFaces();
            multiImage = false;
        }
        
        // Check whether number of faces in images exceeds number of faces
        // in this texture. If so, clamp it.
        if(faces > getNumFaces())
            faces = getNumFaces();
        
        if (TextureManager::getSingleton().getVerbose()) {
            // Say what we're doing
            Log::Stream str = LogManager::getSingleton().stream();
            str << "Texture '" << mName << "': Loading " << faces << " faces"
                << "(" << PixelUtil::getFormatName(images[0]->getFormat()) << ","
                << images[0]->getWidth() << "x" << images[0]->getHeight() << "x"
                << images[0]->getDepth() << ")";
            if (!(mMipmapsHardwareGenerated && mNumMipmaps == TextureMipmap{}))
            {
                str << " with " << mNumMipmaps;
                if(!!(mUsage & TextureUsage::AUTOMIPMAP))
                {
                    if (mMipmapsHardwareGenerated)
                        str << " hardware";

                    str << " generated mipmaps";
                }
                else
                {
                    str << " custom mipmaps";
                }
                if(multiImage)
                    str << " from multiple Images.";
                else
                    str << " from Image.";
            }

            // Print data about first destination surface
            const auto& buf = getBuffer(0, TextureMipmap{});
            str << " Internal format is " << PixelUtil::getFormatName(buf->getFormat()) << ","
                << buf->getWidth() << "x" << buf->getHeight() << "x" << buf->getDepth() << ".";
        }
        
        // Main loading loop
        // imageMips == 0 if the image has no custom mipmaps, otherwise contains the number of custom mips
        for(uint32 mip = 0; mip <= std::to_underlying(std::min(mNumMipmaps, imageMips)); ++mip)
        {
            for(uint32 i = 0; i < std::max(faces, uint32(images.size())); ++i)
            {
                PixelBox src;
                size_t face = (mDepth == 1) ? i : 0; // depth = 1, then cubemap face else 3d/ array layer

                auto buffer = getBuffer(face, static_cast<TextureMipmap>(mip));
                Box dst{0, 0, buffer->getWidth(), buffer->getHeight(), 0, buffer->getDepth()};

                if(multiImage)
                {
                    // Load from multiple images
                    src = images[i]->getPixelBox(0, static_cast<TextureMipmap>(mip));
                    // set dst layer
                    if(mDepth > 1)
                    {
                        dst.front = i;
                        dst.back = i + 1;
                    }
                }
                else
                {
                    // Load from faces of images[0]
                    src = images[0]->getPixelBox(i, static_cast<TextureMipmap>(mip));
                }

                if(mGamma != 1.0f) {
                    // Apply gamma correction
                    // Do not overwrite original image but do gamma correction in temporary buffer
                    Image tmp(src.format, src.getWidth(), getHeight(), src.getDepth());
                    PixelBox corrected = tmp.getPixelBox();
                    PixelUtil::bulkPixelConversion(src, corrected);

                    Image::applyGamma(corrected.data, mGamma, tmp.getSize(), tmp.getBPP());

                    // Destination: entire texture. blitFromMemory does the scaling to
                    // a power of two for us when needed
                    buffer->blitFromMemory(corrected, dst);
                }
                else 
                {
                    // Destination: entire texture. blitFromMemory does the scaling to
                    // a power of two for us when needed
                    buffer->blitFromMemory(src, dst);
                }
                
            }
        }
        // Update size (the final size, not including temp space)
        mSize = getNumFaces() * PixelUtil::getMemorySize(mWidth, mHeight, mDepth, mFormat);

    }
    //-----------------------------------------------------------------------------
    void Texture::createInternalResources()
    {
        if (!mInternalResourcesCreated)
        {
            createInternalResourcesImpl();
            mInternalResourcesCreated = true;

            // this is also public API, so update state accordingly
            if(!isLoading())
            {
                if(mIsManual && mLoader)
                    mLoader->loadResource(this);

                mLoadingState.store(LoadingState::LOADED);
                _fireLoadingComplete(false);
            }
        }
    }
    //-----------------------------------------------------------------------------
    void Texture::freeInternalResources()
    {
        if (mInternalResourcesCreated)
        {
            mSurfaceList.clear();
            freeInternalResourcesImpl();
            mInternalResourcesCreated = false;

            // this is also public API, so update state accordingly
            if(mLoadingState.load() != LoadingState::UNLOADING)
            {
                mLoadingState.store(LoadingState::UNLOADED);
                _fireUnloadingComplete();
            }
        }
    }
    //-----------------------------------------------------------------------------
    void Texture::unloadImpl()
    {
        freeInternalResources();
    }
    //-----------------------------------------------------------------------------   
    void Texture::copyToTexture( TexturePtr& target )
    {
        OgreAssert(target->getNumFaces() == getNumFaces(), "Texture types must match");
        auto numMips = std::min(getNumMipmaps(), target->getNumMipmaps());
        if(!!(mUsage & TextureUsage::AUTOMIPMAP) || !!(target->getUsage()&TextureUsage::AUTOMIPMAP))
            numMips = {};
        for(unsigned int face=0; face<getNumFaces(); face++)
        {
            for(unsigned int mip=0; mip<=std::to_underlying(numMips); mip++)
            {
                target->getBuffer(face, static_cast<TextureMipmap>(mip))->blit(getBuffer(face, static_cast<TextureMipmap>(mip)));
            }
        }
    }
    //---------------------------------------------------------------------
    auto Texture::getSourceFileType() const -> String
    {
        if (mName.empty())
            return "";

        String::size_type pos = mName.find_last_of('.');
        if (pos != String::npos && pos < (mName.length() - 1))
        {
            String ext = mName.substr(pos + 1);
            StringUtil::toLowerCase(ext);
            return ext;
        }

        // No extension
        auto dstream = ResourceGroupManager::getSingleton().openResource(
            mName, mGroup, nullptr, false);

        if (!dstream && getTextureType() == TextureType::CUBE_MAP)
        {
            // try again with one of the faces (non-dds)
            dstream = ResourceGroupManager::getSingleton().openResource(::std::format("{}_rt", mName), mGroup, nullptr, false);
        }

        return String{ dstream ? Image::getFileExtFromMagic(dstream) : BLANKSTRING };

    }
    auto Texture::getBuffer(size_t face, TextureMipmap mipmap) -> const HardwarePixelBufferSharedPtr&
    {
        OgreAssert(face < getNumFaces(), "out of range");
        OgreAssert(mipmap <= mNumMipmaps, "out of range");

        size_t idx = face * (std::to_underlying(mNumMipmaps) + 1) + std::to_underlying(mipmap);
        assert(idx < mSurfaceList.size());
        return mSurfaceList[idx];
    }

    //---------------------------------------------------------------------
    void Texture::convertToImage(Image& destImage, bool includeMipMaps)
    {
        auto numMips = static_cast<TextureMipmap>(includeMipMaps? std::to_underlying(getNumMipmaps()) + 1 : 1);
        destImage.create(getFormat(), getWidth(), getHeight(), getDepth(), getNumFaces(), numMips);

        for (uint32 face = 0; face < getNumFaces(); ++face)
        {
            for (uint32 mip = 0; mip < std::to_underlying(numMips); ++mip)
            {
                getBuffer(face, static_cast<TextureMipmap>(mip))->blitToMemory(destImage.getPixelBox(face, static_cast<TextureMipmap>(mip)));
            }
        }
    }

    //--------------------------------------------------------------------------
    void Texture::getCustomAttribute(std::string_view , void*)
    {
    }

    void Texture::readImage(LoadedImages& imgs, std::string_view name, std::string_view ext, bool haveNPOT)
    {
        DataStreamPtr dstream = ResourceGroupManager::getSingleton().openResource(name, mGroup, this);

        imgs.push_back(Image());
        Image& img = imgs.back();
        img.load(dstream, ext);

        if( haveNPOT )
            return;

        // Scale to nearest power of 2
        uint32 w = Bitwise::firstPO2From(img.getWidth());
        uint32 h = Bitwise::firstPO2From(img.getHeight());
        if((img.getWidth() != w) || (img.getHeight() != h))
            img.resize(w, h);
    }

    void Texture::prepareImpl()
    {
        if (!!(mUsage & TextureUsage::RENDERTARGET))
            return;

        const RenderSystemCapabilities* renderCaps =
            Root::getSingleton().getRenderSystem()->getCapabilities();

        bool haveNPOT = renderCaps->hasCapability(Capabilities::NON_POWER_OF_2_TEXTURES) ||
                        (renderCaps->getNonPOW2TexturesLimited() && mNumMipmaps == TextureMipmap{});

        std::string_view baseName, ext;
        StringUtil::splitBaseFilename(mName, baseName, ext);

        LoadedImages loadedImages;

        try
        {
            if(mLayerNames.empty())
            {
                readImage(loadedImages, mName, ext, haveNPOT);

                // If this is a volumetric texture set the texture type flag accordingly.
                // If this is a cube map, set the texture type flag accordingly.
                if (loadedImages[0].hasFlag(ImageFlags::CUBEMAP))
                    mTextureType = TextureType::CUBE_MAP;
                // If this is a volumetric texture set the texture type flag accordingly.
                if (loadedImages[0].getDepth() > 1 && mTextureType != TextureType::_2D_ARRAY)
                    mTextureType = TextureType::_3D;
            }
        }
        catch(const FileNotFoundException&)
        {
            if(mTextureType == TextureType::CUBE_MAP)
            {
                mLayerNames.resize(6);
                for (size_t i = 0; i < 6; i++)
                    mLayerNames[i] = std::format("{}{}.{}", baseName, CUBEMAP_SUFFIXES[i], ext);
            }
            else if (mTextureType == TextureType::_2D_ARRAY)
            { // ignore
            }
            else
                throw; // rethrow
        }

        // read sub-images
        for(std::string_view name : mLayerNames)
        {
            StringUtil::splitBaseFilename(name, baseName, ext);
            readImage(loadedImages, name, ext, haveNPOT);
        }

        // If compressed and 0 custom mipmap, disable auto mip generation and
        // disable software mipmap creation.
        // Not supported by GLES.
        if (PixelUtil::isCompressed(loadedImages[0].getFormat()) &&
            !renderCaps->hasCapability(Capabilities::AUTOMIPMAP_COMPRESSED) && loadedImages[0].getNumMipmaps() == TextureMipmap{})
        {
            mNumMipmaps = mNumRequestedMipmaps = TextureMipmap{};
            // Disable flag for auto mip generation
            mUsage &= ~TextureUsage::AUTOMIPMAP;
        }

        // avoid copying Image data
        std::swap(mLoadedImages, loadedImages);
    }

    void Texture::unprepareImpl()
    {
        mLoadedImages.clear();
    }

    void Texture::loadImpl()
    {
        if (!!(mUsage & TextureUsage::RENDERTARGET))
        {
            createInternalResources();
            return;
        }

        LoadedImages loadedImages;
        // Now the only copy is on the stack and will be cleaned in case of
        // exceptions being thrown from _loadImages
        std::swap(loadedImages, mLoadedImages);

        // Call internal _loadImages, not loadImage since that's external and
        // will determine load status etc again
        ConstImagePtrList imagePtrs;

        for (auto & loadedImage : loadedImages)
        {
            imagePtrs.push_back(&loadedImage);
        }

        _loadImages(imagePtrs);
    }
}
