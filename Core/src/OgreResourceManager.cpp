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
#include <limits>
#include <utility>

#include "OgreException.hpp"
#include "OgreResourceManager.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreStringConverter.hpp"

namespace Ogre {

    //-----------------------------------------------------------------------
    ResourceManager::ResourceManager()
        : mNextHandle(1), mMemoryUsage(0) 
    {
        // Init memory limit & usage
        mMemoryBudget = std::numeric_limits<unsigned long>::max();
    }
    //-----------------------------------------------------------------------
    ResourceManager::~ResourceManager()
    {
        destroyAllResourcePools();
        removeAll();
    }
    void ResourceManager::parseScript(DataStreamPtr& stream, const String& groupName)
    {
        ScriptCompilerManager::getSingleton().parseScript(stream, groupName);
    }
    //-----------------------------------------------------------------------
    ResourcePtr ResourceManager::createResource(const String& name, const String& group,
        bool isManual, ManualResourceLoader* loader, const NameValuePairList* params)
    {
        OgreAssert(!name.empty(), "resource name must not be empty");

        // Call creation implementation
        ResourcePtr ret = ResourcePtr(
            createImpl(name, getNextHandle(), group, isManual, loader, params));
        if (params)
            ret->setParameterList(*params);

        addImpl(ret);
        // Tell resource group manager
        if(ret)
            ResourceGroupManager::getSingleton()._notifyResourceCreated(ret);
        return ret;

    }
    //-----------------------------------------------------------------------
    ResourceManager::ResourceCreateOrRetrieveResult 
    ResourceManager::createOrRetrieve(
        const String& name, const String& group, 
        bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* params)
    {
        ResourcePtr res = getResourceByName(name, group);
        bool created = false;
        if (!res)
        {
            created = true;
            res = createResource(name, group, isManual, loader, params);
        }

        return { res, created };
    }
    //-----------------------------------------------------------------------
    ResourcePtr ResourceManager::prepare(const String& name, 
        const String& group, bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* loadParams, bool backgroundThread)
    {
        ResourcePtr r = createOrRetrieve(name,group,isManual,loader,loadParams).first;
        // ensure prepared
        r->prepare(backgroundThread);
        return r;
    }
    //-----------------------------------------------------------------------
    ResourcePtr ResourceManager::load(const String& name, 
        const String& group, bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* loadParams, bool backgroundThread)
    {
        ResourcePtr r = createOrRetrieve(name,group,isManual,loader,loadParams).first;
        // ensure loaded
        r->load(backgroundThread);

        return r;
    }
    //-----------------------------------------------------------------------
    void ResourceManager::addImpl( ResourcePtr& res )
    {
            std::pair<ResourceMap::iterator, bool> result;
        if(ResourceGroupManager::getSingleton().isResourceGroupInGlobalPool(res->getGroup()))
        {
            result = mResources.emplace(res->getName(), res);
        }
        else
        {
            // we will create the group if it doesn't exists in our list
            auto resgroup = mResourcesWithGroup.emplace(res->getGroup(), ResourceMap()).first;
            result = resgroup->second.emplace(res->getName(), res);
        }

        // Attempt to resolve the collision
        ResourceLoadingListener* listener = ResourceGroupManager::getSingleton().getLoadingListener();
        if (!result.second && listener)
        {
            if(listener->resourceCollision(res.get(), this) == false)
            {
                // explicitly use previous instance and destroy current
                res.reset();
                return;
            }

            // Try to do the addition again, no seconds attempts to resolve collisions are allowed
            if(ResourceGroupManager::getSingleton().isResourceGroupInGlobalPool(res->getGroup()))
            {
                result = mResources.emplace(res->getName(), res);
            }
            else
            {
                auto resgroup = mResourcesWithGroup.emplace(res->getGroup(), ResourceMap()).first;
                result = resgroup->second.emplace(res->getName(), res);
            }
        }

        if (!result.second)
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, getResourceType()+" with the name " + res->getName() +
                " already exists.", "ResourceManager::add");
        }

        // Insert the handle
        std::pair<ResourceHandleMap::iterator, bool> resultHandle = mResourcesByHandle.emplace(res->getHandle(), res);
        if (!resultHandle.second)
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, getResourceType()+" with the handle " +
                StringConverter::toString((long) (res->getHandle())) +
                " already exists.", "ResourceManager::add");
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::removeImpl(const ResourcePtr& res )
    {
        OgreAssert(res, "attempting to remove nullptr");

        if(ResourceGroupManager::getSingleton().isResourceGroupInGlobalPool(res->getGroup()))
        {
            auto nameIt = mResources.find(res->getName());
            if (nameIt != mResources.end())
            {
                mResources.erase(nameIt);
            }
        }
        else
        {
            auto groupIt = mResourcesWithGroup.find(res->getGroup());
            if (groupIt != mResourcesWithGroup.end())
            {
                auto nameIt = groupIt->second.find(res->getName());
                if (nameIt != groupIt->second.end())
                {
                    groupIt->second.erase(nameIt);
                }

                if (groupIt->second.empty())
                {
                    mResourcesWithGroup.erase(groupIt);
                }
            }
        }

        auto handleIt = mResourcesByHandle.find(res->getHandle());
        if (handleIt != mResourcesByHandle.end())
        {
            mResourcesByHandle.erase(handleIt);
        }
        // Tell resource group manager
        ResourceGroupManager::getSingleton()._notifyResourceRemoved(res);
    }
    //-----------------------------------------------------------------------
    void ResourceManager::setMemoryBudget( size_t bytes)
    {
        // Update limit & check usage
        mMemoryBudget = bytes;
        checkUsage();
    }
    //-----------------------------------------------------------------------
    size_t ResourceManager::getMemoryBudget() const
    {
        return mMemoryBudget;
    }
    //-----------------------------------------------------------------------
    void ResourceManager::unload(const String& name, const String& group)
    {
        ResourcePtr res = getResourceByName(name, group);

        if (!res)
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        ::std::format("attempting to unload unknown resource: {} in group ", name ) + group);

        if (res)
        {
            res->unload();
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::unload(ResourceHandle handle)
    {
        ResourcePtr res = getByHandle(handle);

        OgreAssert(res, "attempting to unload unknown resource");

        if (res)
        {
            res->unload();
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::unloadAll(Resource::LoadingFlags flags)
    {
        bool reloadableOnly = (flags & Resource::LF_INCLUDE_NON_RELOADABLE) == 0;
        bool unreferencedOnly = (flags & Resource::LF_ONLY_UNREFERENCED) != 0;

        for (auto & mResource : mResources)
        {
            // A use count of 3 means that only RGM and RM have references
            // RGM has one (this one) and RM has 2 (by name and by handle)
            if (!unreferencedOnly || mResource.second.use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
            {
                Resource* res = mResource.second.get();
                if (!reloadableOnly || res->isReloadable())
                {
                    res->unload();
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::reloadAll(Resource::LoadingFlags flags)
    {
        bool reloadableOnly = (flags & Resource::LF_INCLUDE_NON_RELOADABLE) == 0;
        bool unreferencedOnly = (flags & Resource::LF_ONLY_UNREFERENCED) != 0;

        for (auto & mResource : mResources)
        {
            // A use count of 3 means that only RGM and RM have references
            // RGM has one (this one) and RM has 2 (by name and by handle)
            if (!unreferencedOnly || mResource.second.use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
            {
                Resource* res = mResource.second.get();
                if (!reloadableOnly || res->isReloadable())
                {
                    res->reload(flags);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::remove(const ResourcePtr& res)
    {
        removeImpl(res);
    }
    //-----------------------------------------------------------------------
    void ResourceManager::remove(const String& name, const String& group)
    {
        ResourcePtr res = getResourceByName(name, group);

        if (!res)
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        ::std::format("attempting to remove unknown resource: {} in group ", name ) + group);

        if (res)
        {
            removeImpl(res);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::remove(ResourceHandle handle)
    {
        ResourcePtr res = getByHandle(handle);

        OgreAssert(res, "attempting to remove unknown resource");

        if (res)
        {
            removeImpl(res);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::removeAll()
    {
        mResources.clear();
        mResourcesWithGroup.clear();
        mResourcesByHandle.clear();
        // Notify resource group manager
        ResourceGroupManager::getSingleton()._notifyAllResourcesRemoved(this);
    }
    //-----------------------------------------------------------------------
    void ResourceManager::removeUnreferencedResources(bool reloadableOnly)
    {
        for (auto i = mResources.begin(); i != mResources.end(); )
        {
            // A use count of 3 means that only RGM and RM have references
            // RGM has one (this one) and RM has 2 (by name and by handle)
            if (i->second.use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
            {
                Resource* res = (i++)->second.get();
                if (!reloadableOnly || res->isReloadable())
                {
                    remove( res->getHandle() );
                }
            }
            else
            {
                ++i;
            }
        }
    }
    //-----------------------------------------------------------------------
    ResourcePtr ResourceManager::getResourceByName(const String& name, const String& groupName) const
    {
        // resource should be in global pool
        bool isGlobal = ResourceGroupManager::getSingleton().isResourceGroupInGlobalPool(groupName);

        if(isGlobal)
        {
            auto it = mResources.find(name);
            if( it != mResources.end())
            {
                return it->second;
            }
        }

        // look in all grouped pools
        if (groupName == ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)
        {
            for (auto const& iter : mResourcesWithGroup)
            {
                auto resMapIt = iter.second.find(name);

                if( resMapIt != iter.second.end())
                {
                    return resMapIt->second;
                }
            }
        }
        else if (!isGlobal)
        {
            // look in the grouped pool
            auto itGroup = mResourcesWithGroup.find(groupName);
            if( itGroup != mResourcesWithGroup.end())
            {
                auto it = itGroup->second.find(name);

                if( it != itGroup->second.end())
                {
                    return it->second;
                }
            }
        }
    
        return {};
    }
    //-----------------------------------------------------------------------
    ResourcePtr ResourceManager::getByHandle(ResourceHandle handle) const
    {
        auto it = mResourcesByHandle.find(handle);
        return it == mResourcesByHandle.end() ? ResourcePtr() : it->second;
    }
    //-----------------------------------------------------------------------
    ResourceHandle ResourceManager::getNextHandle()
    {
        // This is an atomic operation and hence needs no locking
        return mNextHandle++;
    }
    //-----------------------------------------------------------------------
    void ResourceManager::checkUsage()
    {
        if (getMemoryUsage() > mMemoryBudget)
        {
            // unload unreferenced resources until we are within our budget again
            for (auto const& i : mResources)
            {
                if (getMemoryUsage() <= mMemoryBudget)
                    break;

                // A use count of 3 means that only RGM and RM have references
                // RGM has one (this one) and RM has 2 (by name and by handle)
                if (i.second.use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
                {
                    Resource* res = i.second.get();
                    if (res->isReloadable())
                    {
                        res->unload();
                    }
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceManager::_notifyResourceTouched(Resource* res)
    {
        // TODO
    }
    //-----------------------------------------------------------------------
    void ResourceManager::_notifyResourceLoaded(Resource* res)
    {
        mMemoryUsage += res->getSize();
        checkUsage();
    }
    //-----------------------------------------------------------------------
    void ResourceManager::_notifyResourceUnloaded(Resource* res)
    {
        mMemoryUsage -= res->getSize();
    }
    //---------------------------------------------------------------------
    ResourceManager::ResourcePool* ResourceManager::getResourcePool(const String& name)
    {
        auto i = mResourcePoolMap.find(name);
        if (i == mResourcePoolMap.end())
        {
            i = mResourcePoolMap.insert(ResourcePoolMap::value_type(name, 
                new ResourcePool(name))).first;
        }
        return i->second;

    }
    //---------------------------------------------------------------------
    void ResourceManager::destroyResourcePool(ResourcePool* pool)
    {
        OgreAssert(pool, "Cannot destroy a null ResourcePool");

        auto i = mResourcePoolMap.find(pool->getName());
        if (i != mResourcePoolMap.end())
            mResourcePoolMap.erase(i);

        delete pool;
        
    }
    //---------------------------------------------------------------------
    void ResourceManager::destroyResourcePool(const String& name)
    {
        auto i = mResourcePoolMap.find(name);
        if (i != mResourcePoolMap.end())
        {
            delete i->second;
            mResourcePoolMap.erase(i);
        }

    }
    //---------------------------------------------------------------------
    void ResourceManager::destroyAllResourcePools()
    {
        for (auto & i : mResourcePoolMap)
            delete i.second;

        mResourcePoolMap.clear();
    }
    //-----------------------------------------------------------------------
    //---------------------------------------------------------------------
    ResourceManager::ResourcePool::ResourcePool(String  name)
        : mName(std::move(name))
    {

    }
    //---------------------------------------------------------------------
    ResourceManager::ResourcePool::~ResourcePool()
    {
        clear();
    }
    //---------------------------------------------------------------------
    const String& ResourceManager::ResourcePool::getName() const noexcept
    {
        return mName;
    }
    //---------------------------------------------------------------------
    void ResourceManager::ResourcePool::clear()
    {
        for (auto & mItem : mItems)
        {
            mItem->getCreator()->remove(mItem->getHandle());
        }
        mItems.clear();
    }
}




