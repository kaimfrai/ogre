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
#include <format>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "OgreArchive.hpp"
#include "OgreArchiveManager.hpp"
#include "OgreCommon.hpp"
#include "OgreDataStream.hpp"
#include "OgreException.hpp"
#include "OgreLog.hpp"
#include "OgreLogManager.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResource.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreResourceManager.hpp"
#include "OgreScriptLoader.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreSingleton.hpp"
#include "OgreString.hpp"
#include "OgreStringVector.hpp"

namespace Ogre {

    //-----------------------------------------------------------------------
    template<> ResourceGroupManager* Singleton<ResourceGroupManager>::msSingleton = nullptr;
    auto ResourceGroupManager::getSingletonPtr() noexcept -> ResourceGroupManager*
    {
        return msSingleton;
    }
    auto ResourceGroupManager::getSingleton() noexcept -> ResourceGroupManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }

    const char* const RGN_DEFAULT = "General";
    const char* const RGN_INTERNAL = "OgreInternal";
    const char* const RGN_AUTODETECT = "OgreAutodetect";

    const String ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME = RGN_DEFAULT;
    const String ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME = RGN_INTERNAL;
    const String ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME = RGN_AUTODETECT;

    // A reference count of 3 means that only RGM and RM have references
    // RGM has one (this one) and RM has 2 (by name and by handle)
    const long ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS = 3;
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    ResourceGroupManager::ResourceGroupManager()
         
    {
        // Create the 'General' group
        createResourceGroup(DEFAULT_RESOURCE_GROUP_NAME, true); // the "General" group is synonymous to global pool
        // Create the 'Internal' group
        createResourceGroup(INTERNAL_RESOURCE_GROUP_NAME, true);
        // Create the 'Autodetect' group (only used for temp storage)
        createResourceGroup(AUTODETECT_RESOURCE_GROUP_NAME, true); // autodetect includes the global pool
        // default world group to the default group
        mWorldGroupName = DEFAULT_RESOURCE_GROUP_NAME;
    }
    //-----------------------------------------------------------------------
    ResourceGroupManager::~ResourceGroupManager()
    {
        // delete all resource groups
        for (auto & i : mResourceGroupMap)
        {
            deleteGroup(i.second);
        }
        mResourceGroupMap.clear();
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::createResourceGroup(StringView name, bool inGlobalPool)
    {
        LogManager::getSingleton().logMessage(::std::format("Creating resource group {}", name));
        if (getResourceGroup(name))
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
                ::std::format("Resource group with name '{}' already exists!", name ), 
                "ResourceGroupManager::createResourceGroup");
        }
        auto* grp = new ResourceGroup();
        grp->groupStatus = ResourceGroup::UNINITIALSED;
        grp->name = name;
        grp->inGlobalPool = inGlobalPool;
        grp->customStageCount = 0;

        mResourceGroupMap.emplace(name, grp);
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::initialiseResourceGroup(StringView name)
    {
        LogManager::getSingleton().logMessage(::std::format("Initialising resource group {}", name));
        ResourceGroup* grp = getResourceGroup(name, true);

        if (grp->groupStatus == ResourceGroup::UNINITIALSED)
        {
            // in the process of initialising
            grp->groupStatus = ResourceGroup::INITIALISING;
            // Set current group
            parseResourceGroupScripts(grp);
            mCurrentGroup = grp;
            LogManager::getSingleton().logMessage(::std::format("Creating resources for group {}", name));
            createDeclaredResources(grp);
            grp->groupStatus = ResourceGroup::INITIALISED;
            LogManager::getSingleton().logMessage("All done");
            // Reset current group
            mCurrentGroup = nullptr;
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::initialiseAllResourceGroups()
    {
        // Intialise all declared resource groups
        for (auto const& [key, grp] : mResourceGroupMap)
        {
            if (grp->groupStatus == ResourceGroup::UNINITIALSED)
            {
                // in the process of initialising
                grp->groupStatus = ResourceGroup::INITIALISING;
                // Set current group
                mCurrentGroup = grp;
                parseResourceGroupScripts(grp);
                LogManager::getSingleton().logMessage(::std::format("Creating resources for group {}", key));
                createDeclaredResources(grp);
                grp->groupStatus = ResourceGroup::INITIALISED;
                LogManager::getSingleton().logMessage("All done");
                // Reset current group
                mCurrentGroup = nullptr;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::prepareResourceGroup(StringView name)
    {
        LogManager::getSingleton().stream() << "Preparing resource group '" << name << "'";
        // load all created resources
        ResourceGroup* grp = getResourceGroup(name, true);
        // Set current group
        mCurrentGroup = grp;

        // Count up resources for starting event
        size_t resourceCount = 0;
        for (auto const& [key, value] : grp->loadResourceOrderMap)
        {
            resourceCount += value.size();
        }

        fireResourceGroupPrepareStarted(name, resourceCount);

        // Now load for real
        for (auto const& [key, value] : grp->loadResourceOrderMap)
        {
            size_t n = 0;
            auto l = value.begin();
            while (l != value.end())
            {
                ResourcePtr res = *l;

                // Fire resource events no matter whether resource needs preparing
                // or not. This ensures that the number of callbacks
                // matches the number originally estimated, which is important
                // for progress bars.
                fireResourcePrepareStarted(res);

                // If preparing one of these resources cascade-prepares another resource,
                // the list will get longer! But these should be prepared immediately
                // Call prepare regardless, already prepared or loaded resources will be skipped
                res->prepare();

                fireResourcePrepareEnded();

                ++n;

                // Did the resource change group? if so, our iterator will have
                // been invalidated
                if (res->getGroup() != name)
                {
                    l = value.begin();
                    std::advance(l, n);
                }
                else
                {
                    ++l;
                }
            }
        }
        fireResourceGroupPrepareEnded(name);

        // reset current group
        mCurrentGroup = nullptr;
        
        LogManager::getSingleton().logMessage(::std::format("Finished preparing resource group {}", name));
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::loadResourceGroup(StringView name)
    {
        LogManager::getSingleton().stream() << "Loading resource group '" << name << "'";
        // load all created resources
        ResourceGroup* grp = getResourceGroup(name, true);
        // Set current group
        mCurrentGroup = grp;

        // Count up resources for starting event
        size_t resourceCount = grp->customStageCount;
        for (auto const& [key, value] : grp->loadResourceOrderMap)
        {
            resourceCount += value.size();
        }

        fireResourceGroupLoadStarted(name, resourceCount);

        // Now load for real
        for (auto const& [key, value] : grp->loadResourceOrderMap)
        {
            size_t n = 0;
            auto l = value.begin();
            while (l != value.end())
            {
                ResourcePtr res = *l;

                // Fire resource events no matter whether resource is already
                // loaded or not. This ensures that the number of callbacks
                // matches the number originally estimated, which is important
                // for progress bars.
                fireResourceLoadStarted(res);

                // If loading one of these resources cascade-loads another resource,
                // the list will get longer! But these should be loaded immediately
                // Call load regardless, already loaded resources will be skipped
                res->load();

                fireResourceLoadEnded();

                ++n;

                // Did the resource change group? if so, our iterator will have
                // been invalidated
                if (res->getGroup() != name)
                {
                    l = value.begin();
                    std::advance(l, n);
                }
                else
                {
                    ++l;
                }
            }
        }
        fireResourceGroupLoadEnded(name);

        // group is loaded
        grp->groupStatus = ResourceGroup::LOADED;

        // reset current group
        mCurrentGroup = nullptr;
        
        LogManager::getSingleton().logMessage(::std::format("Finished loading resource group {}", name));
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::unloadResourceGroup(StringView name, bool reloadableOnly)
    {
        LogManager::getSingleton().logMessage(::std::format("Unloading resource group {}", name));
        ResourceGroup* grp = getResourceGroup(name, true);
        // Set current group
        mCurrentGroup = grp;

        // Count up resources for starting event
        // unload in reverse order
        for (auto & oi : std::ranges::reverse_view(grp->loadResourceOrderMap))
        {
            for (auto & l : oi.second)
            {
                Resource* resource = l.get();
                if (!reloadableOnly || resource->isReloadable())
                {
                    resource->unload();
                }
            }
        }

        grp->groupStatus = ResourceGroup::INITIALISED;

        // reset current group
        mCurrentGroup = nullptr;
        LogManager::getSingleton().logMessage(::std::format("Finished unloading resource group {}", name));
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::unloadUnreferencedResourcesInGroup(
        StringView name, bool reloadableOnly )
    {
        LogManager::getSingleton().logMessage(
            ::std::format("Unloading unused resources in resource group {}", name));
        ResourceGroup* grp = getResourceGroup(name, true);
        // Set current group
        mCurrentGroup = grp;

        // unload in reverse order
        for (auto & oi : std::ranges::reverse_view(grp->loadResourceOrderMap))
        {
            for (auto & l : oi.second)
            {
                // A use count of 3 means that only RGM and RM have references
                // RGM has one (this one) and RM has 2 (by name and by handle)
                if (l.use_count() == RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS)
                {
                    Resource* resource = l.get();
                    if (!reloadableOnly || resource->isReloadable())
                    {
                        resource->unload();
                    }
                }
            }
        }

        grp->groupStatus = ResourceGroup::INITIALISED;

        // reset current group
        mCurrentGroup = nullptr;
        LogManager::getSingleton().logMessage(
            ::std::format("Finished unloading unused resources in resource group {}", name));
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::clearResourceGroup(StringView name)
    {
            LogManager::getSingleton().logMessage(::std::format("Clearing resource group {}", name));
        ResourceGroup* grp = getResourceGroup(name, true);
        // set current group
        mCurrentGroup = grp;
        dropGroupContents(grp);
        // clear initialised flag
        grp->groupStatus = ResourceGroup::UNINITIALSED;
        // reset current group
        mCurrentGroup = nullptr;
        LogManager::getSingleton().logMessage(::std::format("Finished clearing resource group {}", name));
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::destroyResourceGroup(StringView name)
    {
        LogManager::getSingleton().logMessage(::std::format("Destroying resource group {}", name));
        ResourceGroup* grp = getResourceGroup(name, true);
        // set current group
        mCurrentGroup = grp;
        unloadResourceGroup(name, false); // will throw an exception if name not valid
        dropGroupContents(grp);
        deleteGroup(grp);
        mResourceGroupMap.erase(mResourceGroupMap.find(name));
        // reset current group
        mCurrentGroup = nullptr;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::isResourceGroupInitialised(StringView name) const -> bool
    {
        ResourceGroup* grp = getResourceGroup(name, true);
        return (grp->groupStatus != ResourceGroup::UNINITIALSED &&
            grp->groupStatus != ResourceGroup::INITIALISING);
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::isResourceGroupLoaded(StringView name) const -> bool
    {
        return getResourceGroup(name, true)->groupStatus == ResourceGroup::LOADED;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::resourceGroupExists(StringView name) const -> bool
    {
        return getResourceGroup(name) ? true : false;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::resourceLocationExists(StringView name, 
        StringView resGroup) const -> bool
    {
        ResourceGroup* grp = getResourceGroup(resGroup);
        if (!grp)
            return false;

        for (auto & li : grp->locationList)
        {
            Archive* pArch = li.archive;
            if (pArch->getName() == name)
                // Delete indexes
                return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::addResourceLocation(StringView name, 
        StringView locType, StringView resGroup, bool recursive, bool readOnly)
    {
        // Get archive
        Archive* pArch = ArchiveManager::getSingleton().load( name, locType, readOnly );
        // Add to location list

        ResourceLocation loc = {pArch, recursive};
        StringVectorPtr vec = pArch->find("*", recursive);

        ResourceGroup* grp = getResourceGroup(resGroup);
        if (!grp)
        {
            createResourceGroup(resGroup);
            grp = getResourceGroup(resGroup);
        }

        grp->locationList.push_back(loc);

        // Index resources
        for(auto & it : *vec)
            grp->addToIndex(it, pArch);
        
        StringStream msg;
        msg << "Added resource location '" << name << "' of type '" << locType
            << "' to resource group '" << resGroup << "'";
        if (recursive)
            msg << " with recursive option";
        LogManager::getSingleton().logMessage(msg.str());

    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::removeResourceLocation(StringView name, 
        StringView resGroup)
    {
        ResourceGroup* grp = getResourceGroup(resGroup, true);

        // Remove from location list
        for (auto li = grp->locationList.begin(); li != grp->locationList.end(); ++li)
        {
            Archive* pArch = li->archive;
            if (pArch->getName() == name)
            {
                grp->removeFromIndex(pArch);
                grp->locationList.erase(li);
                ArchiveManager::getSingleton().unload(pArch);
                break;
            }

        }

        LogManager::getSingleton().logMessage(::std::format("Removed resource location {}", name));


    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::declareResource(StringView name, 
        StringView resourceType, StringView groupName,
        const NameValuePairList& loadParameters)
    {
        declareResource(name, resourceType, groupName, nullptr, loadParameters);
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::declareResource(StringView name, 
        StringView resourceType, StringView groupName,
        ManualResourceLoader* loader,
        const NameValuePairList& loadParameters)
    {
        ResourceGroup* grp = getResourceGroup(groupName, true);
        ResourceDeclaration dcl;
        dcl.loader = loader;
        dcl.parameters = loadParameters;
        dcl.resourceName = name;
        dcl.resourceType = resourceType;

        grp->resourceDeclarations.push_back(dcl);
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::undeclareResource(StringView name, 
        StringView groupName)
    {
        ResourceGroup* grp = getResourceGroup(groupName, true);

        for (auto i = grp->resourceDeclarations.begin();
            i != grp->resourceDeclarations.end(); ++i)
        {
            if (i->resourceName == name)
            {
                grp->resourceDeclarations.erase(i);
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::openResourceImpl(StringView resourceName,
                                                     StringView groupName,
                                                     bool searchGroupsIfNotFound,
                                                     Resource* resourceBeingLoaded,
                                                     bool throwOnFailure) const -> DataStreamPtr
    {
        OgreAssert(!resourceName.empty(), "resourceName is empty string");
        if(mLoadingListener)
        {
            DataStreamPtr stream = mLoadingListener->resourceLoading(resourceName, groupName, resourceBeingLoaded);
            if(stream)
                return stream;
        }

        // Try to find in resource index first
        ResourceGroup* grp = getResourceGroup(groupName, throwOnFailure);
        if (!grp)
        {
            // we only get here if throwOnFailure is false
            return {};
        }

        Archive* pArch = resourceExists(grp, resourceName);

        if (pArch == nullptr && (searchGroupsIfNotFound ||
            groupName == AUTODETECT_RESOURCE_GROUP_NAME || grp->inGlobalPool))
        {
            auto[key, value] = resourceExistsInAnyGroupImpl(resourceName);

            if(value && resourceBeingLoaded && !grp->inGlobalPool) {
                resourceBeingLoaded->changeGroupOwnership(value->name);
            }

            pArch = key;
        }

        if (pArch)
        {
            DataStreamPtr stream = pArch->open(resourceName);
            if (mLoadingListener)
                mLoadingListener->resourceStreamOpened(resourceName, groupName, resourceBeingLoaded, stream);
            return stream;
        }

        if(!throwOnFailure)
            return {};

        OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND, ::std::format("Cannot locate resource {} in resource group {}.", resourceName, groupName ),
            "ResourceGroupManager::openResource");

    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::openResources(
        StringView pattern, StringView groupName) const -> DataStreamList
    {
        ResourceGroup* grp = getResourceGroup(groupName, true);

        // Iterate through all the archives and build up a combined list of
        // streams
        DataStreamList ret;

        for (auto & li : grp->locationList)
        {
            Archive* arch = li.archive;
            // Find all the names based on whether this archive is recursive
            StringVectorPtr names = arch->find(pattern, li.recursive);

            // Iterate over the names and load a stream for each
            for (auto & ni : *names)
            {
                DataStreamPtr ptr = arch->open(ni);
                if (ptr)
                {
                    ret.push_back(ptr);
                }
            }
        }
        return ret;
        
    }
    //---------------------------------------------------------------------
    auto ResourceGroupManager::createResource(StringView filename, 
        StringView groupName, bool overwrite, StringView locationPattern) -> DataStreamPtr
    {
        ResourceGroup* grp = getResourceGroup(groupName, true);
        
        for (auto li = grp->locationList.begin(); 
            li != grp->locationList.end(); ++li)
        {
            Archive* arch = li->archive;

            if (!arch->isReadOnly() && 
                (locationPattern.empty() || StringUtil::match(arch->getName(), locationPattern, false)))
            {
                if (!overwrite && arch->exists(filename))
                    OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
                        ::std::format("Cannot overwrite existing file {}", filename), 
                        "ResourceGroupManager::createResource");
                
                // create it
                DataStreamPtr ret = arch->create(filename);
                grp->addToIndex(filename, arch);


                return ret;
            }
        }

        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
            ::std::format("Cannot find a writable location in group {}", groupName), 
            "ResourceGroupManager::createResource");

    }
    //---------------------------------------------------------------------
    void ResourceGroupManager::deleteResource(StringView filename, StringView groupName, 
        StringView locationPattern)
    {
        ResourceGroup* grp = getResourceGroup(groupName, true);
        
        for (auto li = grp->locationList.begin(); 
            li != grp->locationList.end(); ++li)
        {
            Archive* arch = li->archive;

            if (!arch->isReadOnly() && 
                (locationPattern.empty() || StringUtil::match(arch->getName(), locationPattern, false)))
            {
                if (arch->exists(filename))
                {
                    arch->remove(filename);
                    grp->removeFromIndex(filename, arch);

                    // only remove one file
                    break;
                }
            }
        }

    }
    //---------------------------------------------------------------------
    void ResourceGroupManager::deleteMatchingResources(StringView filePattern, 
        StringView groupName, StringView locationPattern)
    {
        ResourceGroup* grp = getResourceGroup(groupName, true);
        
        for (auto li = grp->locationList.begin(); 
            li != grp->locationList.end(); ++li)
        {
            Archive* arch = li->archive;

            if (!arch->isReadOnly() && 
                (locationPattern.empty() || StringUtil::match(arch->getName(), locationPattern, false)))
            {
                StringVectorPtr matchingFiles = arch->find(filePattern);
                for (auto & f : *matchingFiles)
                {
                    arch->remove(f);
                    grp->removeFromIndex(f, arch);

                }
            }
        }


    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::addResourceGroupListener(ResourceGroupListener* l)
    {
        mResourceGroupListenerList.push_back(l);
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::removeResourceGroupListener(ResourceGroupListener* l)
    {
        for (auto i = mResourceGroupListenerList.begin();
            i != mResourceGroupListenerList.end(); ++i)
        {
            if (*i == l)
            {
                mResourceGroupListenerList.erase(i);
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_registerResourceManager(
        StringView resourceType, ResourceManager* rm)
    {
        LogManager::getSingleton().logMessage(
            ::std::format("Registering ResourceManager for type {}", resourceType));
        mResourceManagerMap[resourceType] = rm;
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_unregisterResourceManager(
        StringView resourceType)
    {
        LogManager::getSingleton().logMessage(
            ::std::format("Unregistering ResourceManager for type {}", resourceType));
        
        auto i = mResourceManagerMap.find(resourceType);
        if (i != mResourceManagerMap.end())
        {
            mResourceManagerMap.erase(i);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_registerScriptLoader(ScriptLoader* su)
    {
        mScriptLoaderOrderMap.emplace(su->getLoadingOrder(), su);
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_unregisterScriptLoader(ScriptLoader* su)
    {
        Real order = su->getLoadingOrder();
        auto oi = mScriptLoaderOrderMap.find(order);
        while (oi != mScriptLoaderOrderMap.end() && oi->first == order)
        {
            if (oi->second == su)
            {
                // erase does not invalidate on multimap, except current
                auto del = oi++;
                mScriptLoaderOrderMap.erase(del);
            }
            else
            {
                ++oi;
            }
        }
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::_findScriptLoader(StringView pattern) const -> ScriptLoader *
    {
        for (auto const& [key, su] : mScriptLoaderOrderMap)
        {
            const StringVector& patterns = su->getScriptPatterns();

            // Search for matches in the patterns
            for (const auto & p : patterns)
            {
                if(p == pattern)
                    return su;
            }
        }

        return nullptr; // No loader was found
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::parseResourceGroupScripts(ResourceGroup* grp) const
    {

        LogManager::getSingleton().logMessage(
            ::std::format("Parsing scripts for resource group {}", grp->name));

        // Count up the number of scripts we have to parse
        using LoaderFileListPair = std::pair<ScriptLoader *, FileInfoList>;
        using ScriptLoaderFileList = std::vector<LoaderFileListPair>;
        ScriptLoaderFileList scriptLoaderFileList;
        size_t scriptCount = 0;
        // Iterate over script users in loading order and get streams
        for (auto const& [key, su] : mScriptLoaderOrderMap)
        {
            scriptLoaderFileList.push_back(LoaderFileListPair(su, FileInfoList()));

            // Get all the patterns and search them
            const StringVector& patterns = su->getScriptPatterns();
            for (const auto & pattern : patterns)
            {
                FileInfoListPtr fileList = findResourceFileInfo(grp->name, pattern);
                FileInfoList& lst = scriptLoaderFileList.back().second;
                lst.insert(lst.end(), fileList->begin(), fileList->end());
            }

            scriptCount += scriptLoaderFileList.back().second.size();
        }
        // Fire scripting event
        fireResourceGroupScriptingStarted(grp->name, scriptCount);

        // Iterate over scripts and parse
        // Note we respect original ordering
        for (auto const& [su, item] : scriptLoaderFileList)
        {
            // Iterate over each item in the list
            for (auto & fii : item)
            {
                bool skipScript = false;
                fireScriptStarted(fii.filename, skipScript);
                if(skipScript)
                {
                    LogManager::getSingleton().logMessage(
                        ::std::format("Skipping script {}", fii.filename));
                }
                else
                {
                    LogManager::getSingleton().logMessage(
                        ::std::format("Parsing script {}", fii.filename));
                    DataStreamPtr stream = fii.archive->open(fii.filename);
                    if (stream)
                    {
                        if (mLoadingListener)
                            mLoadingListener->resourceStreamOpened(fii.filename, grp->name, nullptr, stream);

                        if(fii.archive->getType() == "FileSystem" && stream->size() <= 1024 * 1024)
                        {
                            DataStreamPtr cachedCopy(new MemoryDataStream(stream->getName(), stream));
                            su->parseScript(cachedCopy, grp->name);
                        }
                        else
                            su->parseScript(stream, grp->name);
                    }
                }
                fireScriptEnded(fii.filename, skipScript);
            }
        }

        fireResourceGroupScriptingEnded(grp->name);
        LogManager::getSingleton().logMessage(
            ::std::format("Finished parsing scripts for resource group {}", grp->name));
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::createDeclaredResources(ResourceGroup* grp)
    {

        for (auto i = grp->resourceDeclarations.begin();
            i != grp->resourceDeclarations.end(); ++i)
        {
            ResourceDeclaration& dcl = *i;
            // Retrieve the appropriate manager
            ResourceManager* mgr = _getResourceManager(dcl.resourceType);
            // Create the resource
            ResourcePtr res = mgr->createResource(dcl.resourceName, grp->name,
                dcl.loader != nullptr, dcl.loader, &dcl.parameters);
            // Add resource to load list
            auto li = 
                grp->loadResourceOrderMap.find(mgr->getLoadingOrder());

            if (li == grp->loadResourceOrderMap.end())
            {
                grp->loadResourceOrderMap[mgr->getLoadingOrder()] = LoadUnloadResourceList();
            }
        }

    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_notifyResourceCreated(ResourcePtr& res) const
    {
        if (mCurrentGroup && res->getGroup() == mCurrentGroup->name)
        {
            // Use current group (batch loading)
            addCreatedResource(res, *mCurrentGroup);
        }
        else
        {
            // Find group
            ResourceGroup* grp = getResourceGroup(res->getGroup());
            if (grp)
            {
                addCreatedResource(res, *grp);
            }
        }

        fireResourceCreated(res);
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_notifyResourceRemoved(const ResourcePtr& res) const
    {
        fireResourceRemove(res);

        if (mCurrentGroup && res->getGroup() == mCurrentGroup->name)
        {
            // Do nothing - we're batch unloading so list will be cleared
        }
        else
        {
            // Find group
            ResourceGroup* grp = getResourceGroup(res->getGroup());
            if (grp)
            {
                auto i = 
                    grp->loadResourceOrderMap.find(
                        res->getCreator()->getLoadingOrder());
                if (i != grp->loadResourceOrderMap.end())
                {
                    // Iterate over the resource list and remove
                    LoadUnloadResourceList& resList = i->second;
                    for (auto l = resList.begin();
                        l != resList.end(); ++ l)
                    {
                        if ((*l).get() == res.get())
                        {
                            // this is the one
                            resList.erase(l);
                            break;
                        }
                    }
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_notifyResourceGroupChanged(StringView oldGroup, 
        Resource* res) const
    {
        ResourcePtr resPtr;
    
        // find old entry
        ResourceGroup* grp = getResourceGroup(oldGroup);

        if (grp)
        {
            Real order = res->getCreator()->getLoadingOrder();
            auto i = 
                grp->loadResourceOrderMap.find(order);
            assert(i != grp->loadResourceOrderMap.end());
            LoadUnloadResourceList& loadList = i->second;
            for (auto l = loadList.begin();
                l != loadList.end(); ++l)
            {
                if ((*l).get() == res)
                {
                    resPtr = *l;
                    loadList.erase(l);
                    break;
                }
            }
        }

        if (resPtr)
        {
            // New group
            ResourceGroup* newGrp = getResourceGroup(res->getGroup());

            addCreatedResource(resPtr, *newGrp);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_notifyAllResourcesRemoved(ResourceManager* manager) const
    {
        // Iterate over all groups
        for (const auto & grpi : mResourceGroupMap)
        {
            // Iterate over all priorities
            for (auto& [key, value] : grpi.second->loadResourceOrderMap)
            {
                // Iterate over all resources
                for (auto l = value.begin();
                    l != value.end(); )
                {
                    if ((*l)->getCreator() == manager)
                    {
                        // Increment first since iterator will be invalidated
                        auto del = l++;
                        value.erase(del);
                    }
                    else
                    {
                        ++l;
                    }
                }
            }

        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::addCreatedResource(ResourcePtr& res, ResourceGroup& grp) const
    {
        Real order = res->getCreator()->getLoadingOrder();

        auto i = grp.loadResourceOrderMap.find(order);
        LoadUnloadResourceList& loadList =
            i == grp.loadResourceOrderMap.end() ? grp.loadResourceOrderMap[order] : i->second;

        loadList.push_back(res);
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::getResourceGroup(StringView name,
                                                                                bool throwOnFailure) const -> ResourceGroupManager::ResourceGroup*
    {
        auto i = mResourceGroupMap.find(name);

        if (i == mResourceGroupMap.end())
        {
            if (throwOnFailure)
                OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, ::std::format("Cannot locate a resource group called '{}'", name ));

            return nullptr;
        }

        return i->second;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::_getResourceManager(StringView resourceType) const -> ResourceManager*
    {
        auto i = mResourceManagerMap.find(resourceType);
        if (i == mResourceManagerMap.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                std::format(
                "Cannot locate resource manager for resource type '{}'", resourceType), "ResourceGroupManager::_getResourceManager");
        }
        return i->second;

    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::dropGroupContents(ResourceGroup* grp)
    {
        bool groupSet = false;
        if (!mCurrentGroup)
        {
            // Set current group to indicate ignoring of notifications
            mCurrentGroup = grp;
            groupSet = true;
        }
        // delete all the load list entries
        for (auto & j : grp->loadResourceOrderMap)
        {
            // Iterate over resources
            for (auto & k : j.second)
            {
                k->getCreator()->remove(k);
            }
        }
        grp->loadResourceOrderMap.clear();

        if (groupSet)
        {
            mCurrentGroup = nullptr;
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::deleteGroup(ResourceGroup* grp)
    {
        // delete all the load list entries
        grp->loadResourceOrderMap.clear();

        // delete ResourceGroup
        delete grp;
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupScriptingStarted(StringView groupName, size_t scriptCount) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->resourceGroupScriptingStarted(groupName, scriptCount);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireScriptStarted(StringView scriptName, bool &skipScript) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            bool temp = false;
            l->scriptParseStarted(scriptName, temp);
            if(temp)
                skipScript = true;
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireScriptEnded(StringView scriptName, bool skipped) const
    {
            for (auto l : mResourceGroupListenerList)
            {
                l->scriptParseEnded(scriptName, skipped);
            }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupScriptingEnded(StringView groupName) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->resourceGroupScriptingEnded(groupName);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupLoadStarted(StringView groupName, size_t resourceCount) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->resourceGroupLoadStarted(groupName, resourceCount);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceLoadStarted(const ResourcePtr& resource) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->resourceLoadStarted(resource);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceLoadEnded() const
    {
            for (auto l : mResourceGroupListenerList)
            {
                l->resourceLoadEnded();
            }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_notifyCustomStageStarted(StringView desc) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->customStageStarted(desc);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::_notifyCustomStageEnded() const
    {
            for (auto l : mResourceGroupListenerList)
            {
                l->customStageEnded();
            }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupLoadEnded(StringView groupName) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->resourceGroupLoadEnded(groupName);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupPrepareStarted(StringView groupName, size_t resourceCount) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->resourceGroupPrepareStarted(groupName, resourceCount);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourcePrepareStarted(const ResourcePtr& resource) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->resourcePrepareStarted(resource);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourcePrepareEnded() const
    {
            for (auto l : mResourceGroupListenerList)
            {
                l->resourcePrepareEnded();
            }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceGroupPrepareEnded(StringView groupName) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->resourceGroupPrepareEnded(groupName);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceCreated(const ResourcePtr& resource) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->resourceCreated(resource);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::fireResourceRemove(const ResourcePtr& resource) const
    {
        for (auto l : mResourceGroupListenerList)
        {
            l->resourceRemove(resource);
        }
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::shutdownAll()
    {
        for (auto & i : mResourceManagerMap)
        {
            i.second->removeAll();
        }
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::listResourceNames(StringView groupName, bool dirs) const -> StringVectorPtr
    {
        // MEMCATEGORY_GENERAL is the only category supported for SharedPtr
        StringVectorPtr vec(new StringVector());

        // Try to find in resource index first
        ResourceGroup* grp = getResourceGroup(groupName, true);

        // Iterate over the archives
        for (auto & i : grp->locationList)
        {
            StringVectorPtr lst = i.archive->list(i.recursive, dirs);
            vec->insert(vec->end(), lst->begin(), lst->end());
        }

        return vec;


    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::listResourceFileInfo(StringView groupName, bool dirs) const -> FileInfoListPtr
    {
        // MEMCATEGORY_GENERAL is the only category supported for SharedPtr
        FileInfoListPtr vec(new FileInfoList());

        // Try to find in resource index first
        ResourceGroup* grp = getResourceGroup(groupName, true);

        // Iterate over the archives
        for (auto & i : grp->locationList)
        {
            FileInfoListPtr lst = i.archive->listFileInfo(i.recursive, dirs);
            vec->insert(vec->end(), lst->begin(), lst->end());
        }

        return vec;

    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::findResourceNames(StringView groupName, 
        StringView pattern, bool dirs) const -> StringVectorPtr
    {
        // MEMCATEGORY_GENERAL is the only category supported for SharedPtr
        StringVectorPtr vec(new StringVector());

        // Try to find in resource index first
        ResourceGroup* grp = getResourceGroup(groupName, true);

            // Iterate over the archives
            for (auto & i : grp->locationList)
        {
            StringVectorPtr lst = i.archive->find(pattern, i.recursive, dirs);
            vec->insert(vec->end(), lst->begin(), lst->end());
        }

        return vec;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::findResourceFileInfo(StringView groupName, 
        StringView pattern, bool dirs) const -> FileInfoListPtr
    {
        // MEMCATEGORY_GENERAL is the only category supported for SharedPtr
        FileInfoListPtr vec(new FileInfoList());

        // Try to find in resource index first
        ResourceGroup* grp = getResourceGroup(groupName, true);

            // Iterate over the archives
            for (auto & i : grp->locationList)
        {
            FileInfoListPtr lst = i.archive->findFileInfo(pattern, i.recursive, dirs);
            vec->insert(vec->end(), lst->begin(), lst->end());
        }

        return vec;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::resourceExists(StringView groupName, StringView resourceName) const -> bool
    {
        // Try to find in resource index first
        ResourceGroup* grp = getResourceGroup(groupName, true);
        return resourceExists(grp, resourceName) != nullptr;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::resourceExists(ResourceGroup* grp, StringView resourceName) const -> Archive*
    {
        // Try indexes first
        auto rit = grp->resourceIndexCaseSensitive.find(resourceName);
        if (rit != grp->resourceIndexCaseSensitive.end())
        {
            // Found in the index
            return rit->second;
        }

        return nullptr;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::resourceModifiedTime(StringView groupName, StringView resourceName) const -> time_t
    {
        // Try to find in resource index first
        ResourceGroup* grp = getResourceGroup(groupName, true);
        return resourceModifiedTime(grp, resourceName);
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::resourceModifiedTime(ResourceGroup* grp, StringView resourceName) const -> time_t
    {
        Archive* arch = resourceExists(grp, resourceName);
        if (arch)
        {
            return arch->getModifiedTime(resourceName);
        }

        return 0;
    }
    //-----------------------------------------------------------------------
    auto
    ResourceGroupManager::resourceExistsInAnyGroupImpl(StringView filename) const -> std::pair<Archive*, ResourceGroupManager::ResourceGroup*>
    {
        OgreAssert(!filename.empty(), "resourceName is empty string");

            // Iterate over resource groups and find
        for (const auto & i : mResourceGroupMap)
        {
            Archive* arch = resourceExists(i.second, filename);
            if (arch)
                return std::make_pair(arch, i.second);
        }
        // Not found
        return {};
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::resourceExistsInAnyGroup(StringView filename) const -> bool
    {
        return resourceExistsInAnyGroupImpl(filename).first != 0;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::findGroupContainingResource(StringView filename) const -> StringView
    {
        ResourceGroup* grp = resourceExistsInAnyGroupImpl(filename).second;

        if(grp)
            return grp->name;

        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
            std::format("Unable to derive resource group for {} automatically since the resource was not "
            "found.", filename),
            "ResourceGroupManager::findGroupContainingResource");
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::listResourceLocations(StringView groupName) const -> StringVectorPtr
    {
        StringVectorPtr vec(new StringVector());

        // Try to find in resource index first
        ResourceGroup* grp = getResourceGroup(groupName, true);

        // Iterate over the archives
        for (auto & i : grp->locationList)
        {
            vec->push_back(i.archive->getName());
        }

        return vec;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::findResourceLocation(StringView groupName, StringView pattern) const -> StringVectorPtr
    {
        StringVectorPtr vec(new StringVector());

        // Try to find in resource index first
        ResourceGroup* grp = getResourceGroup(groupName, true);

        // Iterate over the archives
        for (auto & i : grp->locationList)
        {
            String location = i.archive->getName();
            // Search for the pattern
            if(StringUtil::match(location, pattern))
            {
                vec->push_back(location);
            }
        }

        return vec;
    }
    //-----------------------------------------------------------------------
    void ResourceGroupManager::setCustomStagesForResourceGroup(StringView group, uint32 stageCount)
    {
        ResourceGroup* grp = getResourceGroup(group, true);
        grp->customStageCount = stageCount;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::getCustomStagesForResourceGroup(StringView group) -> uint32
    {
        ResourceGroup* grp = getResourceGroup(group, true);
        return grp->customStageCount;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::isResourceGroupInGlobalPool(StringView name) const -> bool
    {
        return getResourceGroup(name, true)->inGlobalPool;
    }
    //-----------------------------------------------------------------------
    auto ResourceGroupManager::getResourceGroups() const -> StringVector
    {
        StringVector vec;
        for (const auto & i : mResourceGroupMap)
        {
            vec.push_back(i.second->name);
        }
        return vec;
    }
    //-----------------------------------------------------------------------
    auto 
    ResourceGroupManager::getResourceDeclarationList(StringView group) const -> ResourceGroupManager::ResourceDeclarationList
    {
        return getResourceGroup(group, true)->resourceDeclarations;
    }
    //---------------------------------------------------------------------
    auto 
    ResourceGroupManager::getResourceLocationList(StringView group) const -> const ResourceGroupManager::LocationList&
    {
        return getResourceGroup(group, true)->locationList;
    }
    //-------------------------------------------------------------------------
    void ResourceGroupManager::setLoadingListener(ResourceLoadingListener *listener)
    {
        mLoadingListener = listener;
    }
    //-------------------------------------------------------------------------
    auto ResourceGroupManager::getLoadingListener() const -> ResourceLoadingListener *
    {
        return mLoadingListener;
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    void ResourceGroupManager::ResourceGroup::addToIndex(StringView filename, Archive* arch)
    {
        // internal, assumes mutex lock has already been obtained
        this->resourceIndexCaseSensitive.emplace(filename, arch);
    }
    //---------------------------------------------------------------------
    void ResourceGroupManager::ResourceGroup::removeFromIndex(StringView filename, Archive* arch)
    {
        // internal, assumes mutex lock has already been obtained
        auto i = this->resourceIndexCaseSensitive.find(filename);
        if (i != this->resourceIndexCaseSensitive.end() && i->second == arch)
            this->resourceIndexCaseSensitive.erase(i);
    }
    //---------------------------------------------------------------------
    void ResourceGroupManager::ResourceGroup::removeFromIndex(Archive* arch)
    {
        // Delete indexes
        for (auto rit = this->resourceIndexCaseSensitive.begin(); rit != this->resourceIndexCaseSensitive.end();)
        {
            if (rit->second == arch)
            {
                auto del = rit++;
                this->resourceIndexCaseSensitive.erase(del);
            }
            else
            {
                ++rit;
            }
        }

    }
}
