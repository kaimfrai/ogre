/*-------------------------------------------------------------------------
This source file is a part of OGRE
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
THE SOFTWARE

You may alternatively use this source under the terms of a specific version of
the OGRE Unrestricted License provided you have obtained such a license from
Torus Knot Software Ltd.
-------------------------------------------------------------------------*/
module;

#include <cassert>
#include <cstddef>

module Ogre.Core;

import :HardwareBuffer;
import :HardwarePixelBuffer;
import :PixelFormat;
import :Prerequisites;
import :ResourceGroupManager;
import :SceneManager;
import :ShadowTextureManager;
import :SharedPtr;
import :Singleton;
import :StringConverter;
import :Texture;
import :TextureManager;

import <set>;
import <string>;
import <vector>;

namespace Ogre
{
    //-----------------------------------------------------------------------
    auto operator== ( const ShadowTextureConfig& lhs, const ShadowTextureConfig& rhs ) noexcept -> bool
    {
        if ( lhs.width != rhs.width ||
            lhs.height != rhs.height ||
            lhs.format != rhs.format )
        {
            return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------
    auto ShadowTextureManager::getSingletonPtr() noexcept -> ShadowTextureManager*
    {
        return msSingleton;
    }
    auto ShadowTextureManager::getSingleton() noexcept -> ShadowTextureManager&
    {
        assert( msSingleton );  return ( *msSingleton );
    }
    //---------------------------------------------------------------------
    ShadowTextureManager::ShadowTextureManager()
         
    = default;
    //---------------------------------------------------------------------
    ShadowTextureManager::~ShadowTextureManager()
    {
        clear();
    }
    //---------------------------------------------------------------------
    void ShadowTextureManager::getShadowTextures(ShadowTextureConfigList& configList,
        ShadowTextureList& listToPopulate)
    {
        listToPopulate.clear();

        std::set<Texture*> usedTextures;

        for (ShadowTextureConfig& config : configList)
        {
            bool found = false;
            for (auto & tex : mTextureList)
            {
                // Skip if already used this one
                if (usedTextures.find(tex.get()) != usedTextures.end())
                    continue;

                if (config.width == tex->getWidth() && config.height == tex->getHeight()
                    && config.format == tex->getFormat() && config.fsaa == tex->getFSAA())
                {
                    // Ok, a match
                    listToPopulate.push_back(tex);
                    usedTextures.insert(tex.get());
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                // Create a new texture
                static std::string_view const constexpr baseName = "Ogre/ShadowTexture";
                String targName = std::format("{}{}", baseName, mCount++);
                TexturePtr shadowTex = TextureManager::getSingleton().createManual(
                    targName, 
                    ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, 
                    TextureType::_2D, config.width, config.height, TextureMipmap{}, config.format,
                    TextureUsage::RENDERTARGET, nullptr, false, config.fsaa);
                // Ensure texture loaded
                shadowTex->load();

                // update with actual format, if the requested format is not supported
                config.format = shadowTex->getFormat();
                listToPopulate.push_back(shadowTex);
                usedTextures.insert(shadowTex.get());
                mTextureList.push_back(shadowTex);
            }
        }

    }
    //---------------------------------------------------------------------
    auto ShadowTextureManager::getNullShadowTexture(PixelFormat format) -> TexturePtr
    {
        for (auto & tex : mNullTextureList)
        {
            if (format == tex->getFormat())
            {
                // Ok, a match
                return tex;
            }
        }

        // not found, create a new one
        // A 1x1 texture of the correct format, not a render target
        static std::string_view const constexpr baseName = "Ogre/ShadowTextureNull";
        String targName = std::format("{}{}", baseName, mCount++);
        TexturePtr shadowTex = TextureManager::getSingleton().createManual(
            targName, 
            ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, 
            TextureType::_2D, 1, 1, TextureMipmap{}, format, TextureUsage::STATIC_WRITE_ONLY);
        mNullTextureList.push_back(shadowTex);

        // lock & populate the texture based on format
        if(PixelUtil::isDepth(format))
            return shadowTex;

        HardwareBufferLockGuard shadowTexLock(shadowTex->getBuffer(), HardwareBuffer::LockOptions::DISCARD);
        const PixelBox& box = shadowTex->getBuffer()->getCurrentLock();

        // set high-values across all bytes of the format 
        PixelUtil::packColour( 1.0f, 1.0f, 1.0f, 1.0f, shadowTex->getFormat(), box.data );

        return shadowTex;
    
    }
    //---------------------------------------------------------------------
    void ShadowTextureManager::clearUnused()
    {
        for (auto i = mTextureList.begin(); i != mTextureList.end(); )
        {
            // Unreferenced if only this reference and the resource system
            // Any cached shadow textures should be re-bound each frame dropping
            // any old references
            if ((*i).use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS + 1)
            {
                TextureManager::getSingleton().remove((*i)->getHandle());
                i = mTextureList.erase(i);
            }
            else
            {
                ++i;
            }
        }
        for (auto i = mNullTextureList.begin(); i != mNullTextureList.end(); )
        {
            // Unreferenced if only this reference and the resource system
            // Any cached shadow textures should be re-bound each frame dropping
            // any old references
            if ((*i).use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS + 1)
            {
                TextureManager::getSingleton().remove((*i)->getHandle());
                i = mNullTextureList.erase(i);
            }
            else
            {
                ++i;
            }
        }

    }
    //---------------------------------------------------------------------
    void ShadowTextureManager::clear()
    {
        for (auto & i : mTextureList)
        {
            TextureManager::getSingleton().remove(i->getHandle());
        }
        mTextureList.clear();

    }
    //---------------------------------------------------------------------

}
