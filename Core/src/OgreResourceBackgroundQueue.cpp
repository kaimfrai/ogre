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
#include <utility>

#include "OgreResourceBackgroundQueue.hpp"
#include "OgreAny.hpp"
#include "OgreException.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreResourceManager.hpp"
#include "OgreRoot.hpp"
#include "OgreSharedPtr.hpp"

namespace Ogre {

    // Note, no locks are required here anymore because all of the parallelisation
    // is now contained in WorkQueue - this class is entirely single-threaded
    //------------------------------------------------------------------------
    //-----------------------------------------------------------------------
    template<> ResourceBackgroundQueue* Singleton<ResourceBackgroundQueue>::msSingleton = nullptr;
    ResourceBackgroundQueue* ResourceBackgroundQueue::getSingletonPtr() noexcept
    {
        return msSingleton;
    }
    ResourceBackgroundQueue& ResourceBackgroundQueue::getSingleton() noexcept
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    /** Enumerates the type of requests */
    enum RequestType
    {
        RT_INITIALISE_GROUP = 0,
        RT_INITIALISE_ALL_GROUPS = 1,
        RT_PREPARE_GROUP = 2,
        RT_PREPARE_RESOURCE = 3,
        RT_LOAD_GROUP = 4,
        RT_LOAD_RESOURCE = 5,
        RT_UNLOAD_GROUP = 6,
        RT_UNLOAD_RESOURCE = 7
    };
    /** Encapsulates a queued request for the background queue */
    struct ResourceRequest
    {
        RequestType type;
        String resourceName;
        ResourceHandle resourceHandle;
        String resourceType;
        String groupName;
        bool isManual;
        ManualResourceLoader* loader;
        NameValuePairList* loadParams;
        ResourceBackgroundQueue::Listener* listener;
        BackgroundProcessResult result;
    };
    /// Struct that holds details of queued notifications
    struct ResourceResponse
    {
        ResourceResponse(ResourcePtr r, ResourceRequest  req)
            : resource(r), request(std::move(req))
        {}

        ResourcePtr resource;
        ResourceRequest request;
    };
    //------------------------------------------------------------------------
    ResourceBackgroundQueue::ResourceBackgroundQueue()  
    = default;
    //------------------------------------------------------------------------
    ResourceBackgroundQueue::~ResourceBackgroundQueue()
    {
        shutdown();
    }
    //---------------------------------------------------------------------
    void ResourceBackgroundQueue::initialise()
    {
        WorkQueue* wq = Root::getSingleton().getWorkQueue();
        mWorkQueueChannel = wq->getChannel("Ogre/ResourceBGQ");
        wq->addResponseHandler(mWorkQueueChannel, this);
        wq->addRequestHandler(mWorkQueueChannel, this);
    }
    //---------------------------------------------------------------------
    void ResourceBackgroundQueue::shutdown()
    {
        WorkQueue* wq = Root::getSingleton().getWorkQueue();
        wq->abortRequestsByChannel(mWorkQueueChannel);
        wq->removeRequestHandler(mWorkQueueChannel, this);
        wq->removeResponseHandler(mWorkQueueChannel, this);
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::initialiseResourceGroup(
        const String& name, ResourceBackgroundQueue::Listener* listener)
    {
        // queue a request
        ResourceRequest req;
        req.type = RT_INITIALISE_GROUP;
        req.groupName = name;
        req.listener = listener;
        return addRequest(req);
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket 
    ResourceBackgroundQueue::initialiseAllResourceGroups( 
        ResourceBackgroundQueue::Listener* listener)
    {
        // queue a request
        ResourceRequest req;
        req.type = RT_INITIALISE_ALL_GROUPS;
        req.listener = listener;
        return addRequest(req);
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::prepareResourceGroup(
        const String& name, ResourceBackgroundQueue::Listener* listener)
    {
        // queue a request
        ResourceRequest req;
        req.type = RT_PREPARE_GROUP;
        req.groupName = name;
        req.listener = listener;
        return addRequest(req);
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::loadResourceGroup(
        const String& name, ResourceBackgroundQueue::Listener* listener)
    {
        // queue a request
        ResourceRequest req;
        req.type = RT_LOAD_GROUP;
        req.groupName = name;
        req.listener = listener;
        return addRequest(req);
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::prepare(
        const String& resType, const String& name, 
        const String& group, bool isManual, 
        ManualResourceLoader* loader, 
        const NameValuePairList* loadParams, 
        ResourceBackgroundQueue::Listener* listener)
    {
        // queue a request
        ResourceRequest req;
        req.type = RT_PREPARE_RESOURCE;
        req.resourceType = resType;
        req.resourceName = name;
        req.groupName = group;
        req.isManual = isManual;
        req.loader = loader;
        // Make instance copy of loadParams for thread independence
        req.loadParams = ( loadParams ? new NameValuePairList( *loadParams ) : nullptr );
        req.listener = listener;
        return addRequest(req);
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::load(
        const String& resType, const String& name, 
        const String& group, bool isManual, 
        ManualResourceLoader* loader, 
        const NameValuePairList* loadParams, 
        ResourceBackgroundQueue::Listener* listener)
    {
        // queue a request
        ResourceRequest req;
        req.type = RT_LOAD_RESOURCE;
        req.resourceType = resType;
        req.resourceName = name;
        req.groupName = group;
        req.isManual = isManual;
        req.loader = loader;
        // Make instance copy of loadParams for thread independence
        req.loadParams = ( loadParams ? new NameValuePairList( *loadParams ) : nullptr );
        req.listener = listener;
        return addRequest(req);
    }
    //---------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::unload(
        const String& resType, const String& name, Listener* listener)
    {
        // queue a request
        ResourceRequest req;
        req.type = RT_UNLOAD_RESOURCE;
        req.resourceType = resType;
        req.resourceName = name;
        req.listener = listener;
        return addRequest(req);
    }
    //---------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::unload(
        const String& resType, ResourceHandle handle, Listener* listener)
    {
        // queue a request
        ResourceRequest req;
        req.type = RT_UNLOAD_RESOURCE;
        req.resourceType = resType;
        req.resourceHandle = handle;
        req.listener = listener;
        return addRequest(req);
    }
    //---------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::unloadResourceGroup(
        const String& name, Listener* listener)
    {
        // queue a request
        ResourceRequest req;
        req.type = RT_UNLOAD_GROUP;
        req.groupName = name;
        req.listener = listener;
        return addRequest(req);
    }
    //------------------------------------------------------------------------
    bool ResourceBackgroundQueue::isProcessComplete(
            BackgroundProcessTicket ticket)
    {
        return mOutstandingRequestSet.find(ticket) == mOutstandingRequestSet.end();
    }
    //------------------------------------------------------------------------
    void ResourceBackgroundQueue::abortRequest( BackgroundProcessTicket ticket )
    {
        WorkQueue* queue = Root::getSingleton().getWorkQueue();

        queue->abortRequest( ticket );
    }
    //------------------------------------------------------------------------
    BackgroundProcessTicket ResourceBackgroundQueue::addRequest(ResourceRequest& req)
    {
        WorkQueue* queue = Root::getSingleton().getWorkQueue();

        Any data(req);

        WorkQueue::RequestID requestID = 
            queue->addRequest(mWorkQueueChannel, (uint16)req.type, data);


        mOutstandingRequestSet.insert(requestID);

        return requestID;
    }
    //-----------------------------------------------------------------------
    bool ResourceBackgroundQueue::canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
    {
        return true;
    }
    //-----------------------------------------------------------------------
    WorkQueue::Response* ResourceBackgroundQueue::handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
    {

        auto resreq = any_cast<ResourceRequest>(req->getData());

        if( req->getAborted() )
        {
            if( resreq.type == RT_PREPARE_RESOURCE || resreq.type == RT_LOAD_RESOURCE )
            {
                delete resreq.loadParams;
                resreq.loadParams = nullptr;
            }
            resreq.result.error = false;
            ResourceResponse resresp(ResourcePtr(), resreq);
            return new WorkQueue::Response(req, true, resresp);
        }

        ResourceManager* rm = nullptr;
        ResourcePtr resource;
        try
        {

            switch (resreq.type)
            {
            case RT_INITIALISE_GROUP:
                ResourceGroupManager::getSingleton().initialiseResourceGroup(
                    resreq.groupName);
                break;
            case RT_INITIALISE_ALL_GROUPS:
                ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
                break;
            case RT_PREPARE_GROUP:
                ResourceGroupManager::getSingleton().prepareResourceGroup(
                    resreq.groupName);
                break;
            case RT_LOAD_GROUP:
                ResourceGroupManager::getSingleton().loadResourceGroup(
                    resreq.groupName);
                break;
            case RT_UNLOAD_GROUP:
                ResourceGroupManager::getSingleton().unloadResourceGroup(
                    resreq.groupName);
                break;
            case RT_PREPARE_RESOURCE:
                rm = ResourceGroupManager::getSingleton()._getResourceManager(
                    resreq.resourceType);
                resource = rm->prepare(resreq.resourceName, resreq.groupName, resreq.isManual, 
                    resreq.loader, resreq.loadParams, true);
                break;
            case RT_LOAD_RESOURCE:
                rm = ResourceGroupManager::getSingleton()._getResourceManager(
                    resreq.resourceType);
                resource = rm->load(resreq.resourceName, resreq.groupName, resreq.isManual, 
                    resreq.loader, resreq.loadParams, true);
                break;
            case RT_UNLOAD_RESOURCE:
                rm = ResourceGroupManager::getSingleton()._getResourceManager(
                    resreq.resourceType);
                if (resreq.resourceName.empty())
                    rm->unload(resreq.resourceHandle);
                else
                    rm->unload(resreq.resourceName, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
                break;
            };
        }
        catch (Exception& e)
        {
            if( resreq.type == RT_PREPARE_RESOURCE || resreq.type == RT_LOAD_RESOURCE )
            {
                delete resreq.loadParams;
                resreq.loadParams = nullptr;
            }
            resreq.result.error = true;
            resreq.result.message = e.getFullDescription();

            // return error response
            ResourceResponse resresp(resource, resreq);
            return new WorkQueue::Response(req, false, resresp, e.getFullDescription());
        }


        // success
        if( resreq.type == RT_PREPARE_RESOURCE || resreq.type == RT_LOAD_RESOURCE )
        {
            delete resreq.loadParams;
            resreq.loadParams = nullptr;
        }
        resreq.result.error = false;
        ResourceResponse resresp(resource, resreq);
        return new WorkQueue::Response(req, true, resresp);

    }
    //------------------------------------------------------------------------
    bool ResourceBackgroundQueue::canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
    {
        return true;
    }
    //------------------------------------------------------------------------
    void ResourceBackgroundQueue::handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
    {
        if( res->getRequest()->getAborted() )
        {
            mOutstandingRequestSet.erase(res->getRequest()->getID());
            return ;
        }

        auto resresp = any_cast<ResourceResponse>(res->getData());

        // Complete full loading in main thread if semithreading
        const ResourceRequest& req = resresp.request;

        if (res->succeeded())
        {
            mOutstandingRequestSet.erase(res->getRequest()->getID());

            // Call resource listener
            if (resresp.resource) 
            {

                if (req.type == RT_LOAD_RESOURCE) 
                {
                    resresp.resource->_fireLoadingComplete( true );
                } 
                else 
                {
                    resresp.resource->_firePreparingComplete( true );
                }
            }
        }
        // Call queue listener
        if (req.listener)
            req.listener->operationCompleted(res->getRequest()->getID(), req.result);
    }
    //------------------------------------------------------------------------

}



