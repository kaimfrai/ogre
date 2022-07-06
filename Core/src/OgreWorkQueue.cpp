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
#include <algorithm>
#include <ostream>
#include <ranges>
#include <thread>
#include <utility>

#include "OgreLog.hpp"
#include "OgreLogManager.hpp"
#include "OgreRoot.hpp"
#include "OgreTimer.hpp"
#include "OgreWorkQueue.hpp"

namespace Ogre {
    //---------------------------------------------------------------------
    auto WorkQueue::getChannel(const String& channelName) -> uint16
    {
        std::unique_lock<std::recursive_mutex> ogrenameLock(mChannelMapMutex);

        auto i = mChannelMap.find(channelName);
        if (i == mChannelMap.end())
        {
            i = mChannelMap.insert(ChannelMap::value_type(channelName, mNextChannel++)).first;
        }
        return i->second;
    }
    //---------------------------------------------------------------------
    WorkQueue::Request::Request(uint16 channel, uint16 rtype, ::std::any  rData, uint8 retry, RequestID rid)
        : mChannel(channel), mType(rtype), mData(std::move(rData)), mRetryCount(retry), mID(rid) 
    {

    }
    //---------------------------------------------------------------------
    WorkQueue::Request::~Request()
    = default;
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    WorkQueue::Response::Response(const Request* rq, bool success, ::std::any  data, std::string_view msg)
        : mRequest(rq), mSuccess(success), mMessages(msg), mData(std::move(data))
    {
        
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    DefaultWorkQueueBase::DefaultWorkQueueBase(std::string_view name)
        : mName(name)
         
    {
    }
    //---------------------------------------------------------------------
    auto DefaultWorkQueueBase::getName() const noexcept -> const String&
    {
        return mName;
    }
    //---------------------------------------------------------------------
    auto DefaultWorkQueueBase::getWorkerThreadCount() const -> size_t
    {
        return mWorkerThreadCount;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::setWorkerThreadCount(size_t c)
    {
        mWorkerThreadCount = c;
    }
    //---------------------------------------------------------------------
    auto DefaultWorkQueueBase::getWorkersCanAccessRenderSystem() const noexcept -> bool
    {
        return mWorkerRenderSystemAccess;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::setWorkersCanAccessRenderSystem(bool access)
    {
        mWorkerRenderSystemAccess = access;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::addRequestHandler(uint16 channel, RequestHandler* rh)
    {
        std::unique_lock<std::recursive_mutex> ogrenameLock(mRequestHandlerMutex);

        auto i = mRequestHandlers.emplace(channel, RequestHandlerList()).first;

        RequestHandlerList& handlers = i->second;
        bool duplicate = false;
        for (auto & handler : handlers)
        {
            if (handler->getHandler() == rh)
            {
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
            handlers.push_back(RequestHandlerHolderPtr(new RequestHandlerHolder(rh)));

    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::removeRequestHandler(uint16 channel, RequestHandler* rh)
    {
        std::unique_lock<std::recursive_mutex> ogrenameLock(mRequestHandlerMutex);

        auto i = mRequestHandlers.find(channel);
        if (i != mRequestHandlers.end())
        {
            RequestHandlerList& handlers = i->second;
            for (auto j = handlers.begin(); j != handlers.end(); ++j)
            {
                if ((*j)->getHandler() == rh)
                {
                    // Disconnect - this will make it safe across copies of the list
                    // this is threadsafe and will wait for existing processes to finish
                    (*j)->disconnectHandler();
                    handlers.erase(j);  
                    break;
                }
            }

        }

    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::addResponseHandler(uint16 channel, ResponseHandler* rh)
    {
        auto i = mResponseHandlers.emplace(channel, ResponseHandlerList()).first;

        ResponseHandlerList& handlers = i->second;
        if (std::ranges::find(handlers, rh) == handlers.end())
            handlers.push_back(rh);
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::removeResponseHandler(uint16 channel, ResponseHandler* rh)
    {
        auto i = mResponseHandlers.find(channel);
        if (i != mResponseHandlers.end())
        {
            ResponseHandlerList& handlers = i->second;
            auto j = std::ranges::find(handlers, rh);
            if (j != handlers.end())
                handlers.erase(j);

        }
    }
    //---------------------------------------------------------------------
    auto DefaultWorkQueueBase::addRequest(uint16 channel, uint16 requestType, 
        ::std::any const& rData, uint8 retryCount, bool forceSynchronous, bool idleThread) -> WorkQueue::RequestID
    {
        ::std::unique_ptr<Request> req = nullptr;
        RequestID rid = 0;

        {
            // lock to acquire rid and push request to the queue
            std::unique_lock<std::recursive_mutex> ogrenameLock(mRequestMutex);

            if (!mAcceptRequests || mShuttingDown)
                return 0;

            rid = ++mRequestCount;
            req = ::std::make_unique<Request>(channel, requestType, rData, retryCount, rid);

            LogManager::getSingleton().stream(LML_TRIVIAL) << 
                "DefaultWorkQueueBase('" << mName << "') - QUEUED(thread:" <<
                std::this_thread::get_id()
                << "): ID=" << rid
                << " channel=" << channel << " requestType=" << requestType;
            if (!forceSynchronous&& !idleThread)
            {
                mRequestQueue.push_back(::std::move(req));
                notifyWorkers();
                return rid;
            }
        }
        if(idleThread){
            std::unique_lock<std::recursive_mutex> ogrenameLock(mIdleMutex);
            mIdleRequestQueue.push_back(::std::move(req));
            if(!mIdleThreadRunning)
            {
                notifyWorkers();
            }
        } else { //forceSynchronous
            processRequestResponse(req.release(), true);
        }
        return rid;

    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::addRequestWithRID(WorkQueue::RequestID rid, uint16 channel, 
        uint16 requestType, ::std::any const& rData, uint8 retryCount)
    {
        // lock to push request to the queue
        std::unique_lock<std::recursive_mutex> ogrenameLock(mRequestMutex);

        if (mShuttingDown)
            return;

        LogManager::getSingleton().stream(LML_TRIVIAL) << 
            "DefaultWorkQueueBase('" << mName << "') - REQUEUED(thread:" <<
            std::this_thread::get_id()
            << "): ID=" << rid
                   << " channel=" << channel << " requestType=" << requestType;
        mRequestQueue.push_back(::std::make_unique<Request>(channel, requestType, rData, retryCount, rid));
        notifyWorkers();
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::abortRequest(RequestID id)
    {
        std::unique_lock<std::recursive_mutex> ogrenameLock1(mProcessMutex);

        // NOTE: Pending requests are exist any of RequestQueue, ProcessQueue and
        // ResponseQueue when keeping ProcessMutex, so we check all of these queues.

        for (auto & i : mProcessQueue)
        {
            if (i->getID() == id)
            {
                i->abortRequest();
                break;
            }
        }

        {
            std::unique_lock<std::recursive_mutex> ogrenameLock2(mRequestMutex);

            for (auto & i : mRequestQueue)
            {
                if (i->getID() == id)
                {
                    i->abortRequest();
                    break;
                }
            }
        }

        {
            if(mIdleProcessed)
            {
                mIdleProcessed->abortRequest();
            }

            std::unique_lock<std::recursive_mutex> ogrenameLock3(mIdleMutex);
            for (auto & i : mIdleRequestQueue)
            {
                i->abortRequest();
            }
        }

        {
            std::unique_lock<std::recursive_mutex> ogrenameLock4(mResponseMutex);

            for (auto & i : mResponseQueue)
            {
                if( i->getRequest()->getID() == id )
                {
                    i->abortRequest();
                    break;
                }
            }
        }
    }
    //---------------------------------------------------------------------
    auto DefaultWorkQueueBase::abortPendingRequest(RequestID id) -> bool
    {
        // Request should not exist in idle queue and request queue simultaneously.
        {
            std::unique_lock<std::recursive_mutex> ogrenameLock1(mRequestMutex);
            for (auto & i : mRequestQueue)
            {
                if (i->getID() == id)
                {
                    i->abortRequest();
                    return true;
                }
            }
        }
        {
            std::unique_lock<std::recursive_mutex> ogrenameLock2(mIdleMutex);
            for (auto & i : mIdleRequestQueue)
            {
                if (i->getID() == id)
                {
                    i->abortRequest();
                    return true;
                }
            }
        }

        return false;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::abortRequestsByChannel(uint16 channel)
    {
        std::unique_lock<std::recursive_mutex> ogrenameLock1(mProcessMutex);

        for (auto & i : mProcessQueue)
        {
            if (i->getChannel() == channel)
            {
                i->abortRequest();
            }
        }

        {
            std::unique_lock<std::recursive_mutex> ogrenameLock2(mRequestMutex);

            for (auto & i : mRequestQueue)
            {
                if (i->getChannel() == channel)
                {
                    i->abortRequest();
                }
            }
        }
        {
            if (mIdleProcessed && mIdleProcessed->getChannel() == channel)
            {
                mIdleProcessed->abortRequest();
            }

            std::unique_lock<std::recursive_mutex> ogrenameLock3(mIdleMutex);

            for (auto & i : mIdleRequestQueue)
            {
                if (i->getChannel() == channel)
                {
                    i->abortRequest();
                }
            }
        }

        {
            std::unique_lock<std::recursive_mutex> ogrenameLock4(mResponseMutex);

            for (auto & i : mResponseQueue)
            {
                if( i->getRequest()->getChannel() == channel )
                {
                    i->abortRequest();
                }
            }
        }
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::abortPendingRequestsByChannel(uint16 channel)
    {
        {
            std::unique_lock<std::recursive_mutex> ogrenameLock(mRequestMutex);
            for (auto & i : mRequestQueue)
            {
                if (i->getChannel() == channel)
                {
                    i->abortRequest();
                }
            }
        }
        {
            std::unique_lock<std::recursive_mutex> ogrenameLock(mIdleMutex);

            for (auto & i : mIdleRequestQueue)
            {
                if (i->getChannel() == channel)
                {
                    i->abortRequest();
                }
            }
        }
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::abortAllRequests()
    {
        std::unique_lock<std::recursive_mutex> ogrenameLock(mProcessMutex);
        {
            for (auto & i : mProcessQueue)
            {
                i->abortRequest();
            }
        }
        

        {
            std::unique_lock<std::recursive_mutex> ogrenameLock1(mRequestMutex);

            for (auto & i : mRequestQueue)
            {
                i->abortRequest();
            }
        }

        {

            if(mIdleProcessed)
            {
                mIdleProcessed->abortRequest();
            }

            std::unique_lock<std::recursive_mutex> ogrenameLock2(mIdleMutex);

            for (auto & i : mIdleRequestQueue)
            {
                i->abortRequest();
            }
        }

        {
            std::unique_lock<std::recursive_mutex> ogrenameLock3(mResponseMutex);

            for (auto & i : mResponseQueue)
            {
                i->abortRequest();
            }
        }

    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::setPaused(bool pause)
    {
        std::unique_lock<std::recursive_mutex> ogrenameLock(mRequestMutex);

        mPaused = pause;
    }
    //---------------------------------------------------------------------
    auto DefaultWorkQueueBase::isPaused() const noexcept -> bool
    {
        return mPaused;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::setRequestsAccepted(bool accept)
    {
        std::unique_lock<std::recursive_mutex> ogrenameLock(mRequestMutex);

        mAcceptRequests = accept;
    }
    //---------------------------------------------------------------------
    auto DefaultWorkQueueBase::getRequestsAccepted() const noexcept -> bool
    {
        return mAcceptRequests;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::_processNextRequest()
    {
        if(processIdleRequests()){
            // Found idle requests.
            return;
        }
        Request* request = nullptr;
        {
            // scoped to only lock while retrieving the next request
            std::unique_lock<std::recursive_mutex> ogrenameLock1(mProcessMutex);
            {
                std::unique_lock<std::recursive_mutex> ogrenameLock2(mRequestMutex);

                if (!mRequestQueue.empty())
                {
                    request = mRequestQueue.front().release();
                    mRequestQueue.pop_front();
                    mProcessQueue.push_back( request );
                }
            }
        }

        if (request)
        {
            processRequestResponse(request, false);
        }
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::processRequestResponse(Request* r, bool synchronous)
    {
        Response* response = processRequest(r);

        std::unique_lock<std::recursive_mutex> ogrenameLock1(mProcessMutex);

        for(auto it = mProcessQueue.begin(); it != mProcessQueue.end(); ++it )
        {
            if( (*it) == r )
            {
                mProcessQueue.erase( it );
                break;
            }
        }
        if( mIdleProcessed == r )
        {
            mIdleProcessed = nullptr;
        }
        if (response)
        {
            if (!response->succeeded())
            {
                // Failed, should we retry?
                const Request* req = response->getRequest();
                if (req->getRetryCount())
                {
                    addRequestWithRID(req->getID(), req->getChannel(), req->getType(), req->getData(), 
                        req->getRetryCount() - 1);
                    // discard response (this also deletes request)
                    delete response;
                    return;
                }
            }
            if (synchronous)
            {
                processResponse(response);
                delete response;
            }
            else
            {
                if( response->getRequest()->getAborted() )
                {
                    // destroy response user data
                    response->abortRequest();
                }
                // Queue response
                std::unique_lock<std::recursive_mutex> ogrenameLock2(mResponseMutex);
                mResponseQueue.emplace_back(response);
                // no need to wake thread, this is processed by the main thread
            }

        }
        else
        {
            if (!r->getAborted())
            {
            // no response, delete request
            LogManager::getSingleton().stream(LML_WARNING) <<
                "DefaultWorkQueueBase('" << mName << "') warning: no handler processed request "
                << r->getID() << ", channel " << r->getChannel()
                << ", type " << r->getType();
            }
            delete r;
        }

    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::processResponses() 
    {
        unsigned long msStart = Root::getSingleton().getTimer()->getMilliseconds();
        unsigned long msCurrent = 0;

        // keep going until we run out of responses or out of time
        for(;;)
        {
            Response* response = nullptr;
            {
                std::unique_lock<std::recursive_mutex> ogrenameLock(mResponseMutex);

                if (mResponseQueue.empty())
                    break; // exit loop
                else
                {
                    response = mResponseQueue.front().release();
                    mResponseQueue.pop_front();
                }
            }

            if (response)
            {
                processResponse(response);

                delete response;

            }

            // time limit
            if (mResposeTimeLimitMS)
            {
                msCurrent = Root::getSingleton().getTimer()->getMilliseconds();
                if (msCurrent - msStart > mResposeTimeLimitMS)
                    break;
            }
        }
    }
    //---------------------------------------------------------------------
    auto DefaultWorkQueueBase::processRequest(Request* r) -> WorkQueue::Response*
    {
        RequestHandlerListByChannel handlerListCopy;
        {
            // lock the list only to make a copy of it, to maximise parallelism
            std::unique_lock<std::recursive_mutex> ogrenameLock(mRequestHandlerMutex);
            
            handlerListCopy = mRequestHandlers;
        }

        Response* response = nullptr;

        StringStream dbgMsg;
        dbgMsg <<
            "main"
            << "): ID=" << r->getID() << " channel=" << r->getChannel() 
            << " requestType=" << r->getType();

        LogManager::getSingleton().stream(LML_TRIVIAL) << 
            "DefaultWorkQueueBase('" << mName << "') - PROCESS_REQUEST_START(" << dbgMsg.str();

        auto i = handlerListCopy.find(r->getChannel());
        if (i != handlerListCopy.end())
        {
            RequestHandlerList& handlers = i->second;
            for (auto & handler : std::ranges::reverse_view(handlers))
            {
                // threadsafe call which tests canHandleRequest and calls it if so 
                response = handler->handleRequest(r, this);

                if (response)
                    break;
            }
        }

        LogManager::getSingleton().stream(LML_TRIVIAL) << 
            "DefaultWorkQueueBase('" << mName << "') - PROCESS_REQUEST_END(" << dbgMsg.str()
            << " processed=" << (response!=nullptr);

        return response;

    }
    //---------------------------------------------------------------------
    void DefaultWorkQueueBase::processResponse(Response* r)
    {
        StringStream dbgMsg;
        dbgMsg << "thread:" <<
            std::this_thread::get_id()
            << "): ID=" << r->getRequest()->getID()
            << " success=" << r->succeeded() << " messages=[" << r->getMessages() << "] channel=" 
            << r->getRequest()->getChannel() << " requestType=" << r->getRequest()->getType();

        LogManager::getSingleton().stream(LML_TRIVIAL) << 
            "DefaultWorkQueueBase('" << mName << "') - PROCESS_RESPONSE_START(" << dbgMsg.str();

        auto i = mResponseHandlers.find(r->getRequest()->getChannel());
        if (i != mResponseHandlers.end())
        {
            ResponseHandlerList& handlers = i->second;
            for (auto & handler : std::ranges::reverse_view(handlers))
            {
                if (handler->canHandleResponse(r, this))
                {
                    handler->handleResponse(r, this);
                }
            }
        }
        LogManager::getSingleton().stream(LML_TRIVIAL) << 
            "DefaultWorkQueueBase('" << mName << "') - PROCESS_RESPONSE_END(" << dbgMsg.str();

    }

    auto DefaultWorkQueueBase::processIdleRequests() -> bool
    {
        {
            std::unique_lock<std::recursive_mutex> ogrenameLock1(mIdleMutex);
            if(mIdleRequestQueue.empty() || mIdleThreadRunning){
                return false;
            } else {
                mIdleThreadRunning = true;
            }
        }
        try {
            for(;;){
                {
                    std::unique_lock<std::recursive_mutex> ogrenameLock2(mProcessMutex); // mProcessMutex needs to be the top mutex to prevent livelocks
                    {
                        std::unique_lock<std::recursive_mutex> ogrenameLock3(mIdleMutex);
                        if(!mIdleRequestQueue.empty()){
                            mIdleProcessed = mIdleRequestQueue.front().release();
                            mIdleRequestQueue.pop_front();
                        } else {
                            mIdleProcessed = nullptr;
                            mIdleThreadRunning = false;
                            return true;
                        }
                    }
                }
                processRequestResponse(mIdleProcessed, false);
            }
        } catch (...) { // Normally this should not happen.
            {
                // It is very important to clean up or the idle thread will be locked forever!
                std::unique_lock<std::recursive_mutex> ogrenameLock4(mProcessMutex);
                {
                    std::unique_lock<std::recursive_mutex> ogrenameLock5(mIdleMutex);
                    if(mIdleProcessed){
                        mIdleProcessed->abortRequest();
                    }
                    mIdleProcessed = nullptr;
                    mIdleThreadRunning = false;
                }
            }
            Ogre::LogManager::getSingleton().stream() << "Exception caught in top of worker thread!";

            return true;
        }
    }


    //---------------------------------------------------------------------

    void DefaultWorkQueueBase::WorkerFunc::operator()()
    {
        mQueue->_threadMain();
    }
    
    void DefaultWorkQueueBase::WorkerFunc::operator()() const
    {
        mQueue->_threadMain();
    }

    void DefaultWorkQueueBase::WorkerFunc::run()
    {
        mQueue->_threadMain();
    }
}
