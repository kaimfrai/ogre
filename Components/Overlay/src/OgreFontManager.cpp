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
-------------------------------------------------------------------------*/
module;

#include <cassert>

module Ogre.Components.Overlay;

import :FontManager;

import Ogre.Core;

import <memory>;

namespace Ogre
{
    //---------------------------------------------------------------------
    template<> FontManager * Singleton< FontManager >::msSingleton = nullptr;
    auto FontManager::getSingletonPtr() noexcept -> FontManager*
    {
        return msSingleton;
    }
    auto FontManager::getSingleton() noexcept -> FontManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //---------------------------------------------------------------------
    FontManager::FontManager() : ResourceManager()
    {
        // Loading order
        mLoadOrder = 200.0f;
        // Scripting is supported by this manager
        mScriptPatterns.push_back("*.fontdef");
        // Register scripting with resource group manager
        ResourceGroupManager::getSingleton()._registerScriptLoader(this);

        // Resource type
        mResourceType = "Font";

        // Register with resource group manager
        ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
    }
    //---------------------------------------------------------------------
    FontManager::~FontManager()
    {
        // Unregister with resource group manager
        ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
        // Unegister scripting with resource group manager
        ResourceGroupManager::getSingleton()._unregisterScriptLoader(this);

    }
    //---------------------------------------------------------------------
    auto FontManager::createImpl(std::string_view name, ResourceHandle handle, 
        std::string_view group, bool isManual, ManualResourceLoader* loader,
        const NameValuePairList* params) -> Resource*
    {
        return new Font(this, name, handle, group, isManual, loader);
    }
    //-----------------------------------------------------------------------
    auto FontManager::getByName(std::string_view name, std::string_view groupName) const -> FontPtr
    {
        return static_pointer_cast<Font>(getResourceByName(name, groupName));
    }
    //---------------------------------------------------------------------
    auto FontManager::create (std::string_view name, std::string_view group,
                                    bool isManual, ManualResourceLoader* loader,
                                    const NameValuePairList* createParams) -> FontPtr
    {
        return static_pointer_cast<Font>(createResource(name,group,isManual,loader,createParams));
    }
}
