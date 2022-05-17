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
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "OgreColourValue.hpp"
#include "OgreCommon.hpp"
#include "OgreException.hpp"
#include "OgreImage.hpp"
#include "OgrePixelFormat.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderSystemCapabilities.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreResourceManager.hpp"
#include "OgreRoot.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreSingleton.hpp"
#include "OgreTexture.hpp"
#include "OgreTextureManager.hpp"
#include "OgreTextureUnitState.hpp"

namespace Ogre {

    //-----------------------------------------------------------------------
    template<> TextureManager* Singleton<TextureManager>::msSingleton = 0;
    auto TextureManager::getSingletonPtr() -> TextureManager*
    {
        return msSingleton;
    }
    auto TextureManager::getSingleton() -> TextureManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    TextureManager::TextureManager()
         : mPreferredIntegerBitDepth(0)
         , mPreferredFloatBitDepth(0)
         , mDefaultNumMipmaps(MIP_UNLIMITED)
    {
        mResourceType = "Texture";
        mLoadOrder = 75.0f;

        // Subclasses should register (when this is fully constructed)
    }
    //-----------------------------------------------------------------------
    TextureManager::~TextureManager()
    {
        // subclasses should unregister with resource group manager

    }
    auto TextureManager::createSampler(const String& name) -> SamplerPtr
    {
        SamplerPtr ret = _createSamplerImpl();
        if(!name.empty())
        {
            if (mNamedSamplers.find(name) != mNamedSamplers.end())
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Sampler '" + name + "' already exists");
            mNamedSamplers[name] = ret;
        }
        return ret;
    }

    /// retrieve an named sampler
    auto TextureManager::getSampler(const String& name) const -> const SamplerPtr&
    {
        static SamplerPtr nullPtr;
        auto it = mNamedSamplers.find(name);
        if(it == mNamedSamplers.end())
            return nullPtr;
        return it->second;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::getByName(const String& name, const String& groupName) const -> TexturePtr
    {
        return static_pointer_cast<Texture>(getResourceByName(name, groupName));
    }
    //-----------------------------------------------------------------------
    auto TextureManager::create (const String& name, const String& group,
                                    bool isManual, ManualResourceLoader* loader,
                                    const NameValuePairList* createParams) -> TexturePtr
    {
        return static_pointer_cast<Texture>(createResource(name,group,isManual,loader,createParams));
    }
    //-----------------------------------------------------------------------
    auto TextureManager::createOrRetrieve(
            const String &name, const String& group, bool isManual, ManualResourceLoader* loader,
            const NameValuePairList* createParams, TextureType texType, int numMipmaps, Real gamma,
            bool isAlpha, PixelFormat desiredFormat, bool hwGamma) -> TextureManager::ResourceCreateOrRetrieveResult
    {
        ResourceCreateOrRetrieveResult res =
            Ogre::ResourceManager::createOrRetrieve(name, group, isManual, loader, createParams);
        // Was it created?
        if(res.second)
        {
            TexturePtr tex = static_pointer_cast<Texture>(res.first);
            tex->setTextureType(texType);
            tex->setNumMipmaps((numMipmaps == MIP_DEFAULT)? mDefaultNumMipmaps :
                static_cast<uint32>(numMipmaps));
            tex->setGamma(gamma);
            tex->setTreatLuminanceAsAlpha(isAlpha);
            tex->setFormat(desiredFormat);
            tex->setHardwareGammaEnabled(hwGamma);
        }
        return res;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::prepare(const String &name, const String& group, TextureType texType,
                                       int numMipmaps, Real gamma, bool isAlpha,
                                       PixelFormat desiredFormat, bool hwGamma) -> TexturePtr
    {
        ResourceCreateOrRetrieveResult res =
            createOrRetrieve(name,group,false,0,0,texType,numMipmaps,gamma,isAlpha,desiredFormat,hwGamma);
        TexturePtr tex = static_pointer_cast<Texture>(res.first);
        tex->prepare();
        return tex;
    }

    auto TextureManager::load(const String& name, const String& group, TextureType texType,
                                    int numMipmaps, Real gamma, PixelFormat desiredFormat, bool hwGamma) -> TexturePtr
    {
        auto res = createOrRetrieve(name, group, false, 0, 0, texType, numMipmaps, gamma, false,
                                    desiredFormat, hwGamma);
        TexturePtr tex = static_pointer_cast<Texture>(res.first);
        tex->load();
        return tex;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::loadImage( const String &name, const String& group,
        const Image &img, TextureType texType, int numMipmaps, Real gamma, bool isAlpha, 
        PixelFormat desiredFormat, bool hwGamma) -> TexturePtr
    {
        TexturePtr tex = create(name, group, true);

        tex->setTextureType(texType);
        tex->setNumMipmaps((numMipmaps == MIP_DEFAULT)? mDefaultNumMipmaps :
            static_cast<uint32>(numMipmaps));
        tex->setGamma(gamma);
        tex->setTreatLuminanceAsAlpha(isAlpha);
        tex->setFormat(desiredFormat);
        tex->setHardwareGammaEnabled(hwGamma);
        tex->loadImage(img);

        return tex;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::loadRawData(const String &name, const String& group,
        DataStreamPtr& stream, ushort uWidth, ushort uHeight, 
        PixelFormat format, TextureType texType, 
        int numMipmaps, Real gamma, bool hwGamma) -> TexturePtr
    {
        TexturePtr tex = create(name, group, true);

        tex->setTextureType(texType);
        tex->setNumMipmaps((numMipmaps == MIP_DEFAULT)? mDefaultNumMipmaps :
            static_cast<uint32>(numMipmaps));
        tex->setGamma(gamma);
        tex->setHardwareGammaEnabled(hwGamma);
        tex->loadRawData(stream, uWidth, uHeight, format);
        
        return tex;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::createManual(const String & name, const String& group,
        TextureType texType, uint width, uint height, uint depth, int numMipmaps,
        PixelFormat format, int usage, ManualResourceLoader* loader, bool hwGamma, 
        uint fsaa, const String& fsaaHint) -> TexturePtr
    {
        TexturePtr ret;

        OgreAssert(width && height && depth, "total size of texture must not be zero");

        // Check for texture support
        const auto caps = Root::getSingleton().getRenderSystem()->getCapabilities();
        if (((texType == TEX_TYPE_3D) && !caps->hasCapability(RSC_TEXTURE_3D)) ||
            ((texType == TEX_TYPE_2D_ARRAY) && !caps->hasCapability(RSC_TEXTURE_2D_ARRAY)))
            return ret;

        ret = create(name, group, true, loader);

        if(!ret)
            return ret;

        ret->setTextureType(texType);
        ret->setWidth(width);
        ret->setHeight(height);
        ret->setDepth(depth);
        ret->setNumMipmaps((numMipmaps == MIP_DEFAULT)? mDefaultNumMipmaps :
            static_cast<uint32>(numMipmaps));
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
            for (ResourceMap::iterator it = mResources.begin(); it != mResources.end(); ++it)
            {
                Texture* texture = static_cast<Texture*>(it->second.get());
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
    auto TextureManager::getPreferredIntegerBitDepth() const -> ushort
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
            for (ResourceMap::iterator it = mResources.begin(); it != mResources.end(); ++it)
            {
                Texture* texture = static_cast<Texture*>(it->second.get());
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
    auto TextureManager::getPreferredFloatBitDepth() const -> ushort
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
            for (ResourceMap::iterator it = mResources.begin(); it != mResources.end(); ++it)
            {
                Texture* texture = static_cast<Texture*>(it->second.get());
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
    void TextureManager::setDefaultNumMipmaps( uint32 num )
    {
        mDefaultNumMipmaps = num;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::isFormatSupported(TextureType ttype, PixelFormat format, int usage) -> bool
    {
        return getNativeFormat(ttype, format, usage) == format;
    }
    //-----------------------------------------------------------------------
    auto TextureManager::isEquivalentFormatSupported(TextureType ttype, PixelFormat format, int usage) -> bool
    {
        PixelFormat supportedFormat = getNativeFormat(ttype, format, usage);

        // Assume that same or greater number of bits means quality not degraded
        return PixelUtil::getNumElemBits(supportedFormat) >= PixelUtil::getNumElemBits(format);
        
    }

    auto TextureManager::isHardwareFilteringSupported(TextureType ttype, PixelFormat format,
                                                      int usage, bool preciseFormatOnly) -> bool
    {
        if (format == PF_UNKNOWN)
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
        Image pixels(PF_R5G6B5, 8, 8);

        // Yellow/black stripes
        const ColourValue black(0, 0, 0), yellow(1, 1, 0);
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

    auto TextureManager::getDefaultSampler() -> const SamplerPtr&
    {
        if(!mDefaultSampler)
            mDefaultSampler = createSampler();

        return mDefaultSampler;
    }
}
