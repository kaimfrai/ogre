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

#include <cstddef>

export module Ogre.Core:WorkQueue;

export import :Common;
export import :MemoryAllocatorConfig;
export import :Platform;
export import :Prerequisites;
export import :SharedPtr;

export import <algorithm>;
export import <any>;
export import <deque>;
export import <list>;
export import <map>;
export import <mutex>;
export import <string>;

export
namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */

    /** Interface to a general purpose request / response style background work queue.
    @remarks
        A work queue is a simple structure, where requests for work are placed
        onto the queue, then removed by a worker for processing, then finally
        a response is placed on the result queue for the originator to pick up
        at their leisure. The typical use for this is in a threaded environment, 
        although any kind of deferred processing could use this approach to 
        decouple and distribute work over a period of time even 
        if it was single threaded.
    @par
        WorkQueues also incorporate thread pools. One or more background worker threads
        can wait on the queue and be notified when a request is waiting to be
        processed. For maximal thread usage, a WorkQueue instance should be shared
        among many sources of work, rather than many work queues being created.
        This way, you can share a small number of hardware threads among a large 
        number of background tasks. This doesn't mean you have to implement all the
        request processing in one class, you can plug in many handlers in order to
        process the requests.
    @par
        This is an abstract interface definition; users can subclass this and 
        provide their own implementation if required to centralise task management
        in their own subsystems. We also provide a default implementation in the
        form of DefaultWorkQueue.
    */
    class WorkQueue : public UtilityAlloc
    {
    protected:
        using ChannelMap = std::map<std::string_view, uint16>;
        ChannelMap mChannelMap;
        uint16 mNextChannel{0};
        mutable std::recursive_mutex mChannelMapMutex;
    public:
        /// Numeric identifier for a request
        using RequestID = unsigned long long;

        /** General purpose request structure. 
        */
        class Request : public UtilityAlloc
        {
            friend class WorkQueue;
        protected:
            /// The request channel, as an integer 
            uint16 mChannel;
            /// The request type, as an integer within the channel (user can define enumerations on this)
            uint16 mType;
            /// The details of the request (user defined)
            ::std::any mData;
            /// Retry count - set this to non-zero to have the request try again on failure
            uint8 mRetryCount;
            /// Identifier (assigned by the system)
            RequestID mID;
            /// Abort Flag
            mutable bool mAborted{false};

        public:
            /// Constructor 
            Request(uint16 channel, uint16 rtype, ::std::any  rData, uint8 retry, RequestID rid);
            ~Request();
            /// Set the abort flag
            void abortRequest() const { mAborted = true; }
            /// Get the request channel (top level categorisation)
            auto getChannel() const noexcept -> uint16 { return mChannel; }
            /// Get the type of this request within the given channel
            auto getType() const noexcept -> uint16 { return mType; }
            /// Get the user details of this request
            auto getData() const noexcept -> ::std::any const& { return mData; }
            /// Get the remaining retry count
            auto getRetryCount() const noexcept -> uint8 { return mRetryCount; }
            /// Get the identifier of this request
            auto getID() const noexcept -> RequestID { return mID; }
            /// Get the abort flag
            auto getAborted() const noexcept -> bool { return mAborted; }
        };

        /** General purpose response structure. 
        */
        struct Response
        {
            /// Pointer to the request that this response is in relation to
            ::std::unique_ptr<Request const> mRequest;
            /// Whether the work item succeeded or not
            bool mSuccess;
            /// Data associated with the result of the process
            ::std::any mData;
            /// Any diagnostic messages
            std::string_view mMessages = "";

            /// Get the request that this is a response to (NB destruction destroys this)
            [[nodiscard]] auto getRequest() const noexcept -> const Request* { return mRequest.get(); }
            /// Return whether this is a successful response
            [[nodiscard]] auto succeeded() const noexcept -> bool { return mSuccess; }
            /// Get any diagnostic messages about the process
            [[nodiscard]] auto getMessages() const noexcept -> std::string_view { return mMessages; }
            /// Return the response data (user defined, only valid on success)
            [[nodiscard]] auto getData() const noexcept -> ::std::any const& { return mData; }
            /// Abort the request
            void abortRequest() { mRequest->abortRequest(); mData.reset(); }
        };

        /** Interface definition for a handler of requests. 
        @remarks
        User classes are expected to implement this interface in order to
        process requests on the queue. It's important to realise that
        the calls to this class may be in a separate thread to the main
        render context, and as such it may not be possible to make
        rendersystem or other GPU-dependent calls in this handler. You can only
        do so if the queue was created with 'workersCanAccessRenderSystem'
        set to true, but this puts extra strain
        on the thread safety of the render system and is not recommended.
        It is best to perform CPU-side work in these handlers and let the
        response handler transfer results to the GPU in the main render thread.
        */
        class RequestHandler
        {
        public:
            RequestHandler() = default;
            virtual ~RequestHandler() = default;

            /** Return whether this handler can process a given request. 
            @remarks
            Defaults to true, but if you wish to add several handlers each of
            which deal with different types of request, you can override
            this method. 
            */
            virtual auto canHandleRequest(const Request* req, const WorkQueue* srcQ) -> bool 
            { (void)srcQ; return !req->getAborted(); }

            /** The handler method every subclass must implement. 
            If a failure is encountered, return a Response with a failure
            result rather than raise an exception.
            @param req The Request structure, which is effectively owned by the
            handler during this call. It must be attached to the returned
            Response regardless of success or failure.
            @param srcQ The work queue that this request originated from
            @return Pointer to a Response object - the caller is responsible
            for deleting the object.
            */
            [[nodiscard]]
            virtual auto handleRequest(const Request* req, const WorkQueue* srcQ) -> Response* = 0;
        };

        /** Interface definition for a handler of responses. 
        @remarks
        User classes are expected to implement this interface in order to
        process responses from the queue. All calls to this class will be 
        in the main render thread and thus all GPU resources will be
        available. 
        */
        class ResponseHandler
        {
        public:
            ResponseHandler() = default;
            virtual ~ResponseHandler() = default;

            /** Return whether this handler can process a given response. 
            @remarks
            Defaults to true, but if you wish to add several handlers each of
            which deal with different types of response, you can override
            this method. 
            */
            virtual auto canHandleResponse(const Response* res, const WorkQueue* srcQ) -> bool 
            { (void)srcQ; return !res->getRequest()->getAborted(); }

            /** The handler method every subclass must implement. 
            @param res The Response structure. The caller is responsible for
            deleting this after the call is made, none of the data contained
            (except pointers to structures in user Any data) will persist
            after this call is returned.
            @param srcQ The work queue that this request originated from
            */
            virtual void handleResponse(const Response* res, const WorkQueue* srcQ) = 0;
        };

        WorkQueue()  = default;
        virtual ~WorkQueue() = default;

        /** Start up the queue with the options that have been set.
        @param forceRestart If the queue is already running, whether to shut it
            down and restart.
        */
        virtual void startup(bool forceRestart = true) = 0;
        /** Add a request handler instance to the queue. 
        @remarks
            Every queue must have at least one request handler instance for each 
            channel in which requests are raised. If you 
            add more than one handler per channel, then you must implement canHandleRequest 
            differently in each if you wish them to respond to different requests.
        @param channel The channel for requests you want to handle
        @param rh Your handler
        */
        virtual void addRequestHandler(uint16 channel, RequestHandler* rh) = 0;
        /** Remove a request handler. */
        virtual void removeRequestHandler(uint16 channel, RequestHandler* rh) = 0;

        /** Add a response handler instance to the queue. 
        @remarks
            Every queue must have at least one response handler instance for each 
            channel in which requests are raised. If you add more than one, then you 
            must implement canHandleResponse differently in each if you wish them 
            to respond to different responses.
        @param channel The channel for responses you want to handle
        @param rh Your handler
        */
        virtual void addResponseHandler(uint16 channel, ResponseHandler* rh) = 0;
        /** Remove a Response handler. */
        virtual void removeResponseHandler(uint16 channel, ResponseHandler* rh) = 0;

        /** Add a new request to the queue.
        @param channel The channel this request will go into = 0; the channel is the top-level
            categorisation of the request
        @param requestType An identifier that's unique within this channel which
            identifies the type of the request (user decides the actual value)
        @param rData The data required by the request process. 
        @param retryCount The number of times the request should be retried
            if it fails.
        @param forceSynchronous Forces the request to be processed immediately
            even if threading is enabled.
        @param idleThread Request should be processed on the idle thread.
            Idle requests will be processed on a single worker thread. You should use this in the following situations:
            1. If a request handler can't process multiple requests in parallel.
            2. If you add lot of requests, but you want to keep the game fast.
            3. If you have lot of more important threads. (example: physics).
        @return The ID of the request that has been added
        */
        virtual auto addRequest(uint16 channel, uint16 requestType, ::std::any const& rData, uint8 retryCount = 0, 
            bool forceSynchronous = false, bool idleThread = false) -> RequestID = 0;

        /** Abort a previously issued request.
        If the request is still waiting to be processed, it will be 
        removed from the queue.
        @param id The ID of the previously issued request.
        */
        virtual void abortRequest(RequestID id) = 0;

        /** Abort request if it is not being processed currently.
         *
         * @param id The ID of the previously issued request.
         *
         * @retval true If request was aborted successfully.
         * @retval false If request is already being processed so it can not be aborted.
         */
        virtual auto abortPendingRequest(RequestID id) -> bool = 0;

        /** Abort all previously issued requests in a given channel.
        Any requests still waiting to be processed of the given channel, will be 
        removed from the queue.
        Requests which are processed, but response handler is not called will also be removed.
        @param channel The type of request to be aborted
        */
        virtual void abortRequestsByChannel(uint16 channel) = 0;

        /** Abort all previously issued requests in a given channel.
        Any requests still waiting to be processed of the given channel, will be 
        removed from the queue.
        It will not remove requests, where the request handler is already called.
        @param channel The type of request to be aborted
        */
        virtual void abortPendingRequestsByChannel(uint16 channel) = 0;

        /** Abort all previously issued requests.
        Any requests still waiting to be processed will be removed from the queue.
        Any requests that are being processed will still complete.
        */
        virtual void abortAllRequests() = 0;
        
        /** Set whether to pause further processing of any requests. 
        If true, any further requests will simply be queued and not processed until
        setPaused(false) is called. Any requests which are in the process of being
        worked on already will still continue. 
        */
        virtual void setPaused(bool pause) = 0;
        /// Return whether the queue is paused ie not sending more work to workers
        virtual auto isPaused() const noexcept -> bool = 0;

        /** Set whether to accept new requests or not. 
        If true, requests are added to the queue as usual. If false, requests
        are silently ignored until setRequestsAccepted(true) is called. 
        */
        virtual void setRequestsAccepted(bool accept) = 0;
        /// Returns whether requests are being accepted right now
        virtual auto getRequestsAccepted() const noexcept -> bool = 0;

        /** Process the responses in the queue.
        @remarks
            This method is public, and must be called from the main render
            thread to 'pump' responses through the system. The method will usually
            try to clear all responses before returning = 0; however, you can specify
            a time limit on the response processing to limit the impact of
            spikes in demand by calling setResponseProcessingTimeLimit.
        */
        virtual void processResponses() = 0; 

        /** Get the time limit imposed on the processing of responses in a
            single frame, in milliseconds (0 indicates no limit).
        */
        virtual auto getResponseProcessingTimeLimit() const noexcept -> unsigned long = 0;

        /** Set the time limit imposed on the processing of responses in a
            single frame, in milliseconds (0 indicates no limit).
            This sets the maximum time that will be spent in processResponses() in 
            a single frame. The default is 8ms.
        */
        virtual void setResponseProcessingTimeLimit(unsigned long ms) = 0;

        /** Shut down the queue.
        */
        virtual void shutdown() = 0;

        /** Get a channel ID for a given channel name. 
        @remarks
            Channels are assigned on a first-come, first-served basis and are
            not persistent across application instances. This method allows 
            applications to not worry about channel clashes through manually
            assigned channel numbers.
        */
        virtual auto getChannel(std::string_view channelName) -> uint16;

    };

    /** Base for a general purpose request / response style background work queue.
    */
    class DefaultWorkQueueBase : public WorkQueue
    {
    public:

        /** Constructor.
            Call startup() to initialise.
        @param name Optional name, just helps to identify logging output
        */
        DefaultWorkQueueBase(std::string_view name = BLANKSTRING);
        ~DefaultWorkQueueBase() override = default;
        /// Get the name of the work queue
        auto getName() const noexcept -> std::string_view ;
        /** Get the number of worker threads that this queue will start when 
            startup() is called. 
        */
        virtual auto getWorkerThreadCount() const -> size_t;

        /** Set the number of worker threads that this queue will start
            when startup() is called (default 1).
            Calling this will have no effect unless the queue is shut down and
            restarted.
        */
        virtual void setWorkerThreadCount(size_t c);

        /** Get whether worker threads will be allowed to access render system
            resources. 
            Accessing render system resources from a separate thread can require that
            a context is maintained for that thread. Threads can not use GPU resources, and the render system can
            work in non-threadsafe mode, which is more efficient.
        */
        virtual auto getWorkersCanAccessRenderSystem() const noexcept -> bool;


        /** Set whether worker threads will be allowed to access render system
            resources. 
            Accessing render system resources from a separate thread can require that
            a context is maintained for that thread. Threads can not use GPU resources, and the render system can
            work in non-threadsafe mode, which is more efficient.
            Calling this will have no effect unless the queue is shut down and
            restarted.
        */
        virtual void setWorkersCanAccessRenderSystem(bool access);

        /** Process the next request on the queue. 
        @remarks
            This method is public, but only intended for advanced users to call. 
            The only reason you would call this, is if you were using your 
            own thread to drive the worker processing. The thread calling this
            method will be the thread used to call the RequestHandler.
        */
        virtual void _processNextRequest();

        /// Main function for each thread spawned.
        virtual void _threadMain() = 0;

        /** Returns whether the queue is trying to shut down. */
        virtual auto isShuttingDown() const noexcept -> bool { return mShuttingDown; }

        /// @copydoc WorkQueue::addRequestHandler
        void addRequestHandler(uint16 channel, RequestHandler* rh) override;
        /// @copydoc WorkQueue::removeRequestHandler
        void removeRequestHandler(uint16 channel, RequestHandler* rh) override;
        /// @copydoc WorkQueue::addResponseHandler
        void addResponseHandler(uint16 channel, ResponseHandler* rh) override;
        /// @copydoc WorkQueue::removeResponseHandler
        void removeResponseHandler(uint16 channel, ResponseHandler* rh) override;

        /// @copydoc WorkQueue::addRequest
        auto addRequest(uint16 channel, uint16 requestType, ::std::any const& rData, uint8 retryCount = 0, 
            bool forceSynchronous = false, bool idleThread = false) -> RequestID override;
        /// @copydoc WorkQueue::abortRequest
        void abortRequest(RequestID id) override;
        /// @copydoc WorkQueue::abortPendingRequest
        auto abortPendingRequest(RequestID id) -> bool override;
        /// @copydoc WorkQueue::abortRequestsByChannel
        void abortRequestsByChannel(uint16 channel) override;
        /// @copydoc WorkQueue::abortPendingRequestsByChannel
        void abortPendingRequestsByChannel(uint16 channel) override;
        /// @copydoc WorkQueue::abortAllRequests
        void abortAllRequests() override;
        /// @copydoc WorkQueue::setPaused
        void setPaused(bool pause) override;
        /// @copydoc WorkQueue::isPaused
        auto isPaused() const noexcept -> bool override;
        /// @copydoc WorkQueue::setRequestsAccepted
        void setRequestsAccepted(bool accept) override;
        /// @copydoc WorkQueue::getRequestsAccepted
        auto getRequestsAccepted() const noexcept -> bool override;
        /// @copydoc WorkQueue::processResponses
        void processResponses() override; 
        /// @copydoc WorkQueue::getResponseProcessingTimeLimit
        auto getResponseProcessingTimeLimit() const noexcept -> unsigned long override { return mResposeTimeLimitMS; }
        /// @copydoc WorkQueue::setResponseProcessingTimeLimit
        void setResponseProcessingTimeLimit(unsigned long ms) override { mResposeTimeLimitMS = ms; }
    protected:
        String mName;
        size_t mWorkerThreadCount{1};
        bool mWorkerRenderSystemAccess{false};
        bool mIsRunning{false};
        unsigned long mResposeTimeLimitMS{8};

        using RequestQueue = std::deque<::std::unique_ptr<Request>>;
        using ResponseQueue = std::deque<::std::unique_ptr<Response>>;
        RequestQueue mRequestQueue; // Guarded by mRequestMutex
        std::deque<Request *> mProcessQueue; // Guarded by mProcessMutex
        ResponseQueue mResponseQueue; // Guarded by mResponseMutex

        /// Thread function
        struct WorkerFunc
        {
            DefaultWorkQueueBase* mQueue;

            void operator()();
            
            void operator()() const;

            void run();
        };
        WorkerFunc* mWorkerFunc{nullptr};

        /** Intermediate structure to hold a pointer to a request handler which 
            provides insurance against the handler itself being disconnected
            while the list remains unchanged.
        */
        class RequestHandlerHolder : public UtilityAlloc
        {
        protected:
            mutable std::recursive_mutex mRWMutex;
            RequestHandler* mHandler;
        public:
            RequestHandlerHolder(RequestHandler* handler)
                : mHandler(handler) {}

            // Disconnect the handler to allow it to be destroyed
            void disconnectHandler()
            {
                // write lock - must wait for all requests to finish
                std::unique_lock<std::recursive_mutex> ogrenameLock(mRWMutex);
                mHandler = nullptr;
            }

            /** Get handler pointer - note, only use this for == comparison or similar,
                do not attempt to call it as it is not thread safe. 
            */
            auto getHandler() noexcept -> RequestHandler* { return mHandler; }

            /** Process a request if possible.
            @return Valid response if processed, null otherwise
            */
            auto handleRequest(const Request* req, const WorkQueue* srcQ) -> Response*
            {
                // Read mutex so that multiple requests can be processed by the
                // same handler in parallel if required
                std::unique_lock<std::recursive_mutex> ogrenameLock(mRWMutex);
                Response* response = nullptr;
                if (mHandler)
                {
                    if (mHandler->canHandleRequest(req, srcQ))
                    {
                        response = mHandler->handleRequest(req, srcQ);
                    }
                }
                return response;
            }

        };
        // Hold these by shared pointer so they can be copied keeping same instance
        using RequestHandlerHolderPtr = SharedPtr<RequestHandlerHolder>;

        using RequestHandlerList = std::list<RequestHandlerHolderPtr>;
        using ResponseHandlerList = std::list<ResponseHandler *>;
        using RequestHandlerListByChannel = std::map<uint16, RequestHandlerList>;
        using ResponseHandlerListByChannel = std::map<uint16, ResponseHandlerList>;

        RequestHandlerListByChannel mRequestHandlers;
        ResponseHandlerListByChannel mResponseHandlers;
        RequestID mRequestCount{0}; // Guarded by mRequestMutex
        bool mPaused{false};
        bool mAcceptRequests{true};
        bool mShuttingDown{false};

        //NOTE: If you lock multiple mutexes at the same time, the order is important!
        // For example if threadA locks mIdleMutex first then tries to lock mProcessMutex,
        // and threadB locks mProcessMutex first, then mIdleMutex. In this case you can get livelock and the system is dead!
        //RULE: Lock mProcessMutex before other mutex, to prevent livelocks
        mutable std::recursive_mutex mIdleMutex;
        mutable std::recursive_mutex mRequestMutex;
        mutable std::recursive_mutex mProcessMutex;
        mutable std::recursive_mutex mResponseMutex;
        mutable std::recursive_mutex mRequestHandlerMutex;


        void processRequestResponse(Request* r, bool synchronous);
        auto processRequest(Request* r) -> Response*;
        void processResponse(Response* r);
        /// Notify workers about a new request. 
        virtual void notifyWorkers() = 0;
        /// Put a Request on the queue with a specific RequestID.
        void addRequestWithRID(RequestID rid, uint16 channel, uint16 requestType, ::std::any const& rData, uint8 retryCount);
        
        RequestQueue mIdleRequestQueue; // Guarded by mIdleMutex
        bool mIdleThreadRunning{false}; // Guarded by mIdleMutex
        Request* mIdleProcessed{nullptr}; // Guarded by mProcessMutex
        

        auto processIdleRequests() -> bool;
    };





    /** @} */
    /** @} */

}
