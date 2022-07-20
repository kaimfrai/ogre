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

import :ColourValue;
import :Common;
import :Exception;
import :Image;
import :PixelFormat;
import :Platform;
import :Prerequisites;
import :RenderSystem;
import :RenderSystemCapabilities;
import :ResourceGroupManager;
import :ResourceManager;
import :Root;
import :SharedPtr;
import :Singleton;
import :Texture;
import :TextureManager;
import :TextureUnitState;

import <map>;
import <memory>;
import <string>;
import <unordered_map>;
import <utility>;

namespace Ogre {

    //-----------------------------------------------------------------------
    auto TextureManager::getSingletonPtr() noexcept -> TextureManager*
    {
        return msSingleton;
    }
    auto TextureManager::getSingleton() noexcept -> TextureManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    TextureManager::TextureManager()
          
    {
        mResourceType = "Texture";
        mLoadOrder = 75.0f;

        // Subclasses should register (when this is fully constructed)
    }
    auto TextureManager::createSampler(std::string_view name) -> SamplerPtr
    {
        SamplerPtr ret = _createSamplerImpl();
        if(!name.empty())
        {
            if (mNamedSamplers.find(name) != mNamedSamplers.end())
                OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, ::std::format("Sampler '{}' already exists", name ));
            mNamedSamplers[std::string{ name }] = ret;
        }
        return ret;
    }

    /// retrieve an named sampler
    auto TextureManager::getSampler(std::string_view name) const -> const SamplerPtr&
    {
        static SamplerPtr nullPtr;
        auto it = mNamedSamplers.find(name);
        if(it == mNamedSamplers.end())
            return nullPtr;
        return it->second;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::getByName(std::string_view name, std::string_view groupName) const -> TexturePtr
    {
        return static_pointer_cast<Texture>(getResourceByName(name, groupName));
    }
    //-----------------------------------------------------------------------
    auto TextureManager::create (std::string_view name, std::string_view group,
                                    bool isManual, ManualResourceLoader* loader,
                                    const NameValuePairList* createParams) -> TexturePtr
    {
        return static_pointer_cast<Texture>(createResource(name,group,isManual,loader,createParams));
    }
    //-----------------------------------------------------------------------
    auto TextureManager::createOrRetrieve(
            std::string_view name, std::string_view group, bool isManual, ManualResourceLoader* loader,
            const NameValuePairList* createParams, TextureType texType, TextureMipmap numMipmaps, Real gamma,
            bool isAlpha, PixelFormat desiredFormat, bool hwGamma) -> TextureManager::ResourceCreateOrRetrieveResult
    {
        ResourceCreateOrRetrieveResult res =
            Ogre::ResourceManager::createOrRetrieve(name, group, isManual, loader, createParams);
        // Was it created?
        if(res.second)
        {
            TexturePtr tex = static_pointer_cast<Texture>(res.first);
            tex->setTextureType(texType);
            tex->setNumMipmaps((numMipmaps == TextureMipmap::DEFAULT)? mDefaultNumMipmaps :
                numMipmaps);
            tex->setGamma(gamma);
            tex->setTreatLuminanceAsAlpha(isAlpha);
            tex->setFormat(desiredFormat);
            tex->setHardwareGammaEnabled(hwGamma);
        }
        return res;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::prepare(std::string_view name, std::string_view group, TextureType texType,
                                       TextureMipmap numMipmaps, Real gamma, bool isAlpha,
                                       PixelFormat desiredFormat, bool hwGamma) -> TexturePtr
    {
        ResourceCreateOrRetrieveResult res =
            createOrRetrieve(name,group,false,nullptr,nullptr,texType,numMipmaps,gamma,isAlpha,desiredFormat,hwGamma);
        TexturePtr tex = static_pointer_cast<Texture>(res.first);
        tex->prepare();
        return tex;
    }

    auto TextureManager::load(std::string_view name, std::string_view group, TextureType texType,
                                    TextureMipmap numMipmaps, Real gamma, PixelFormat desiredFormat, bool hwGamma) -> TexturePtr
    {
        auto res = createOrRetrieve(name, group, false, nullptr, nullptr, texType, numMipmaps, gamma, false,
                                    desiredFormat, hwGamma);
        TexturePtr tex = static_pointer_cast<Texture>(res.first);
        tex->load();
        return tex;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::loadImage( std::string_view name, std::string_view group,
        const Image &img, TextureType texType, TextureMipmap numMipmaps, Real gamma, bool isAlpha,
        PixelFormat desiredFormat, bool hwGamma) -> TexturePtr
    {
        TexturePtr tex = create(name, group, true);

        tex->setTextureType(texType);
        tex->setNumMipmaps((numMipmaps == TextureMipmap::DEFAULT)? mDefaultNumMipmaps :
            numMipmaps);
        tex->setGamma(gamma);
        tex->setTreatLuminanceAsAlpha(isAlpha);
        tex->setFormat(desiredFormat);
        tex->setHardwareGammaEnabled(hwGamma);
        tex->loadImage(img);

        return tex;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::loadRawData(std::string_view name, std::string_view group,
        DataStreamPtr& stream, ushort uWidth, ushort uHeight, 
        PixelFormat format, TextureType texType, 
        TextureMipmap numMipmaps, Real gamma, bool hwGamma) -> TexturePtr
    {
        TexturePtr tex = create(name, group, true);

        tex->setTextureType(texType);
        tex->setNumMipmaps((numMipmaps == TextureMipmap::DEFAULT)? mDefaultNumMipmaps :
            numMipmaps);
        tex->setGamma(gamma);
        tex->setHardwareGammaEnabled(hwGamma);
        tex->loadRawData(stream, uWidth, uHeight, format);
        
        return tex;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::createManual(std::string_view name, std::string_view group,
        TextureType texType, uint width, uint height, uint depth, TextureMipmap numMipmaps,
        PixelFormat format, HardwareBufferUsage usage, ManualResourceLoader* loader, bool hwGamma,
        uint fsaa, std::string_view fsaaHint) -> TexturePtr
    {
        TexturePtr ret;

        OgreAssert(width && height && depth, "total size of texture must not be zero");

        // Check for texture support
        const auto caps = Root::getSingleton().getRenderSystem()->getCapabilities();
        if (((texType == TextureType::_3D) && !caps->hasCapability(Capabilities::TEXTURE_3D)) ||
            ((texType == TextureType::_2D_ARRAY) && !caps->hasCapability(Capabilities::TEXTURE_2D_ARRAY)))
            return ret;

        ret = create(name, group, true, loader);

        if(!ret)
            return ret;

        ret->setTextureType(texType);
        ret->setWidth(width);
        ret->setHeight(height);
        ret->setDepth(depth);
        ret->setNumMipmaps((numMipmaps == TextureMipmap::DEFAULT)? mDefaultNumMipmaps :
            numMipmaps);
        ret->setFormat(format);
        ret->setUsage(usage);
        ret->setHardwareGammaEnabled(hwGamma);
        ret->setFSAA(fsaa, fsaaHint);
        ret->createInternalResources();
        return ret;
    }
    //-----------------------------------------------------------------------
    void TextureManager::setPreferredIntegerBitDepth(ushort bits, bool reloadTextures)
    {
        mPreferredIntegerBitDepth = bits;

        if (reloadTextures)
        {
            // Iterate through all textures
            for (auto & mResource : mResources)
            {
                auto* texture = static_cast<Texture*>(mResource.second.get());
                // Reload loaded and reloadable texture only
                if (texture->isLoaded() && texture->isReloadable())
                {
                    texture->unload();
                    texture->setDesiredIntegerBitDepth(bits);
                    texture->load();
                }
                else
                {
                    texture->setDesiredIntegerBitDepth(bits);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    auto TextureManager::getPreferredIntegerBitDepth() const noexcept -> ushort
    {
        return mPreferredIntegerBitDepth;
    }
    //-----------------------------------------------------------------------
    void TextureManager::setPreferredFloatBitDepth(ushort bits, bool reloadTextures)
    {
        mPreferredFloatBitDepth = bits;

        if (reloadTextures)
        {
            // Iterate through all textures
            for (auto & mResource : mResources)
            {
                auto* texture = static_cast<Texture*>(mResource.second.get());
                // Reload loaded and reloadable texture only
                if (texture->isLoaded() && texture->isReloadable())
                {
                    texture->unload();
                    texture->setDesiredFloatBitDepth(bits);
                    texture->load();
                }
                else
                {
                    texture->setDesiredFloatBitDepth(bits);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    auto TextureManager::getPreferredFloatBitDepth() const noexcept -> ushort
    {
        return mPreferredFloatBitDepth;
    }
    //-----------------------------------------------------------------------
    void TextureManager::setPreferredBitDepths(ushort integerBits, ushort floatBits, bool reloadTextures)
    {
        mPreferredIntegerBitDepth = integerBits;
        mPreferredFloatBitDepth = floatBits;

        if (reloadTextures)
        {
            // Iterate through all textures
            for (auto & mResource : mResources)
            {
                auto* texture = static_cast<Texture*>(mResource.second.get());
                // Reload loaded and reloadable texture only
                if (texture->isLoaded() && texture->isReloadable())
                {
                    texture->unload();
                    texture->setDesiredBitDepths(integerBits, floatBits);
                    texture->load();
                }
                else
                {
                    texture->setDesiredBitDepths(integerBits, floatBits);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void TextureManager::setDefaultNumMipmaps( TextureMipmap num )
    {
        mDefaultNumMipmaps = num;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::isFormatSupported(TextureType ttype, PixelFormat format, HardwareBufferUsage usage) -> bool
    {
        return getNativeFormat(ttype, format, usage) == format;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::isEquivalentFormatSupported(TextureType ttype, PixelFormat format, HardwareBufferUsage usage) -> bool
    {
        PixelFormat supportedFormat = getNativeFormat(ttype, format, usage);

        // Assume that same or greater number of bits means quality not degraded
        return PixelUtil::getNumElemBits(supportedFormat) >= PixelUtil::getNumElemBits(format);
        
    }

    auto TextureManager::isHardwareFilteringSupported(TextureType ttype, PixelFormat format,
                                                      HardwareBufferUsage usage, bool preciseFormatOnly) -> bool
    {
        if (format == PixelFormat::UNKNOWN)
            return false;

        // Check native format
        if (preciseFormatOnly && !isFormatSupported(ttype, format, usage))
            return false;

        return true;
    }

    auto TextureManager::_getWarningTexture() -> const TexturePtr&
    {
        if(mWarningTexture)
            return mWarningTexture;

        // Generate warning texture
        Image pixels(PixelFormat::R5G6B5, 8, 8);

        // Yellow/black stripes
        const ColourValue black{0, 0, 0}, yellow{1, 1, 0};
        for (uint32 y = 0; y < pixels.getHeight(); ++y)
        {
            for (uint32 x = 0; x < pixels.getWidth(); ++x)
            {
                pixels.setColourAt((((x + y) % 8) < 4) ? black : yellow, x, y, 0);
            }
        }

        mWarningTexture = loadImage("Warning", RGN_INTERNAL, pixels);

        return mWarningTexture;
    }

    auto TextureManager::getDefaultSampler() noexcept -> const SamplerPtr&
    {
        if(!mDefaultSampler)
            mDefaultSampler = createSampler();

        return mDefaultSampler;
    }
}
