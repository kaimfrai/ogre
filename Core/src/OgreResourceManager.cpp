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
module Ogre.Core;

import :Exception;
import :ResourceManager;
import :ScriptCompiler;
import :StringConverter;

import <format>;
import <limits>;
import <utility>;

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
    void ResourceManager::parseScript(DataStreamPtr& stream, std::string_view groupName)
    {
        ScriptCompilerManager::getSingleton().parseScript(stream, groupName);
    }
    //-----------------------------------------------------------------------
    auto ResourceManager::createResource(std::string_view name, std::string_view group,
        bool isManual, ManualResourceLoader* loader, const NameValuePairList* params) -> ResourcePtr
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
    auto 
    ResourceManager::createOrRetrieve(
        std::string_view name, std::string_view group,
        bool isManual, ManualResourceLoader* loader, 
        const NameValuePairList* params) -> ResourceManager::ResourceCreateOrRetrieveResult
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
    auto ResourceManager::prepare(std::string_view name,
        std::string_view group, bool isManual, ManualResourceLoader* loader,
        const NameValuePairList* loadParams, bool backgroundThread) -> ResourcePtr
    {
        ResourcePtr r = createOrRetrieve(name,group,isManual,loader,loadParams).first;
        // ensure prepared
        r->prepare(backgroundThread);
        return r;
    }
    //-----------------------------------------------------------------------
    auto ResourceManager::load(std::string_view name,
        std::string_view group, bool isManual, ManualResourceLoader* loader,
        const NameValuePairList* loadParams, bool backgroundThread) -> ResourcePtr
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
            OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM,
                        ::std::format("{} with the name {} already exists.", getResourceType(), res->getName()),
                        "ResourceManager::add");
        }

        // Insert the handle
        std::pair<ResourceHandleMap::iterator, bool> resultHandle = mResourcesByHandle.emplace(res->getHandle(), res);
        if (!resultHandle.second)
        {
            OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM,
                        std::format("{} with the handle {} already exists.", getResourceType(), (long) (res->getHandle())),
                        "ResourceManager::add");
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
    auto ResourceManager::getMemoryBudget() const -> size_t
    {
        return mMemoryBudget;
    }
    //-----------------------------------------------------------------------
    void ResourceManager::unload(std::string_view name, std::string_view group)
    {
        ResourcePtr res = getResourceByName(name, group);

        if (!res)
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        ::std::format("attempting to unload unknown resource: {} in group {}", name, group));

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
        bool reloadableOnly = (flags & Resource::LoadingFlags::INCLUDE_NON_RELOADABLE) == Resource::LoadingFlags{};
        bool unreferencedOnly = (flags & Resource::LoadingFlags::ONLY_UNREFERENCED) != Resource::LoadingFlags{};

        for (auto const& [key, res] : mResources)
        {
            // A use count of 3 means that only RGM and RM have references
            // RGM has one (this one) and RM has 2 (by name and by handle)
            if (!unreferencedOnly || res.use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
            {
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
        bool reloadableOnly = (flags & Resource::LoadingFlags::INCLUDE_NON_RELOADABLE) == Resource::LoadingFlags{};
        bool unreferencedOnly = (flags & Resource::LoadingFlags::ONLY_UNREFERENCED) != Resource::LoadingFlags{};

        for (auto const& [key, res] : mResources)
        {
            // A use count of 3 means that only RGM and RM have references
            // RGM has one (this one) and RM has 2 (by name and by handle)
            if (!unreferencedOnly || res.use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
            {
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
    void ResourceManager::remove(std::string_view name, std::string_view group)
    {
        ResourcePtr res = getResourceByName(name, group);

        if (!res)
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        ::std::format("attempting to remove unknown resource: {} in group {}", name, group));

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
    auto ResourceManager::getResourceByName(std::string_view name, std::string_view groupName) const -> ResourcePtr
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
            for (auto const& [key, value] : mResourcesWithGroup)
            {
                auto resMapIt = value.find(name);

                if( resMapIt != value.end())
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
    auto ResourceManager::getByHandle(ResourceHandle handle) const -> ResourcePtr
    {
        auto it = mResourcesByHandle.find(handle);
        return it == mResourcesByHandle.end() ? ResourcePtr() : it->second;
    }
    //-----------------------------------------------------------------------
    auto ResourceManager::getNextHandle() -> ResourceHandle
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
            for (auto const& [key, res] : mResources)
            {
                if (getMemoryUsage() <= mMemoryBudget)
                    break;

                // A use count of 3 means that only RGM and RM have references
                // RGM has one (this one) and RM has 2 (by name and by handle)
                if (res.use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
                {
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
    auto ResourceManager::getResourcePool(std::string_view name) -> ResourceManager::ResourcePool*
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
    void ResourceManager::destroyResourcePool(std::string_view name)
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
    ResourceManager::ResourcePool::ResourcePool(std::string_view name)
        : mName(name)
    {

    }
    //---------------------------------------------------------------------
    ResourceManager::ResourcePool::~ResourcePool()
    {
        clear();
    }
    //---------------------------------------------------------------------
    auto ResourceManager::ResourcePool::getName() const noexcept -> std::string_view
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
