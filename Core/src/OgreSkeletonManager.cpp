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

import :SharedPtr;
import :Skeleton;
import :SkeletonManager;

import <memory>;

namespace Ogre
{
    //-----------------------------------------------------------------------
    auto SkeletonManager::getSingletonPtr() noexcept -> SkeletonManager*
    {
        return msSingleton;
    }
    auto SkeletonManager::getSingleton() noexcept -> SkeletonManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    SkeletonManager::SkeletonManager()
    {
        mLoadOrder = 300.0f;
        mResourceType = "Skeleton";

        ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
    }
    //-----------------------------------------------------------------------
    auto SkeletonManager::getByName(std::string_view name, std::string_view groupName) const -> SkeletonPtr
    {
        return static_pointer_cast<Skeleton>(getResourceByName(name, groupName));
    }
    //-----------------------------------------------------------------------
    auto SkeletonManager::create (std::string_view name, std::string_view group,
                                    bool isManual, ManualResourceLoader* loader,
                                    const NameValuePairList* createParams) -> SkeletonPtr
    {
        return static_pointer_cast<Skeleton>(createResource(name,group,isManual,loader,createParams));
    }
    //-----------------------------------------------------------------------
    SkeletonManager::~SkeletonManager()
    {
        ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    }
    //-----------------------------------------------------------------------
    auto SkeletonManager::createImpl(std::string_view name, ResourceHandle handle, 
        std::string_view group, bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* createParams) -> Resource*
    {
        return new Skeleton(this, name, handle, group, isManual, loader);
    }



}
