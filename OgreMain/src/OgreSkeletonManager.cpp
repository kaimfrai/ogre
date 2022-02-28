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
#include <assert.h>
#include <memory>

#include "OgreStableHeaders.h"
#include "OgreSkeletonManager.h"
#include "OgreMemoryAllocatorConfig.h"
#include "OgreSharedPtr.h"
#include "OgreSkeleton.h"

namespace Ogre
{
    //-----------------------------------------------------------------------
    template<> SkeletonManager* Singleton<SkeletonManager>::msSingleton = 0;
    SkeletonManager* SkeletonManager::getSingletonPtr(void)
    {
        return msSingleton;
    }
    SkeletonManager& SkeletonManager::getSingleton(void)
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
    SkeletonPtr SkeletonManager::getByName(const String& name, const String& groupName) const
    {
        return static_pointer_cast<Skeleton>(getResourceByName(name, groupName));
    }
    //-----------------------------------------------------------------------
    SkeletonPtr SkeletonManager::create (const String& name, const String& group,
                                    bool isManual, ManualResourceLoader* loader,
                                    const NameValuePairList* createParams)
    {
        return static_pointer_cast<Skeleton>(createResource(name,group,isManual,loader,createParams));
    }
    //-----------------------------------------------------------------------
    SkeletonManager::~SkeletonManager()
    {
        ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    }
    //-----------------------------------------------------------------------
    Resource* SkeletonManager::createImpl(const String& name, ResourceHandle handle, 
        const String& group, bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* createParams)
    {
        return OGRE_NEW Skeleton(this, name, handle, group, isManual, loader);
    }



}
