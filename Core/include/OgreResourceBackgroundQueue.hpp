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
export module Ogre.Core:ResourceBackgroundQueue;

export import :Common;
export import :MemoryAllocatorConfig;
export import :Platform;
export import :Prerequisites;
export import :Resource;
export import :Singleton;
export import :WorkQueue;

export import <set>;

export
namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    /// Identifier of a background process
    using BackgroundProcessTicket = WorkQueue::RequestID;

    /** Encapsulates the result of a background queue request */
    struct BackgroundProcessResult
    {
        /// Whether an error occurred
        bool error{false};
        /// Any messages from the process
        std::string_view message;
    };


    struct ResourceRequest;

    /** This class is used to perform Resource operations in a
        background thread. 
    @remarks
        All these requests are now queued via Root::getWorkQueue in order
        to share the thread pool amongst all background tasks. You should therefore
        refer to that class for configuring the behaviour of the threads
        themselves, this class merely provides an interface that is specific
        to resource loading around this common functionality.
    @par
        The general approach here is that on requesting a background resource
        process, your request is placed on a queue ready for the background
        thread to be picked up, and you will get a 'ticket' back, identifying
        the request. Your call will then return and your thread can
        proceed, knowing that at some point in the background the operation will 
        be performed. In it's own thread, the resource operation will be 
        performed, and once finished the ticket will be marked as complete. 
        You can check the status of tickets by calling isProcessComplete() 
        from your queueing thread. 
    */
    class ResourceBackgroundQueue : public Singleton<ResourceBackgroundQueue>, public ResourceAlloc, 
        public WorkQueue::RequestHandler, public WorkQueue::ResponseHandler
    {
    public:
        /** This abstract listener interface lets you get notifications of
        completed background processes instead of having to poll ticket 
        statuses.
        @note
        For simplicity, these callbacks are not issued direct from the background
        loading thread, they are queued themselves to be sent from the main thread
        so that you don't have to be concerned about thread safety. 
        */
        class Listener
        {
        public:
            /** Called when a requested operation completes, queued into main thread. 
            @note
                For simplicity, this callback is not issued direct from the background
                loading thread, it is queued to be sent from the main thread
                so that you don't have to be concerned about thread safety. 
            */
            virtual void operationCompleted(BackgroundProcessTicket ticket, const BackgroundProcessResult& result) = 0;
            /// Need virtual destructor in case subclasses use it
            virtual ~Listener() = default;

        };

    private:

        uint16 mWorkQueueChannel{0};

        using OutstandingRequestSet = std::set<BackgroundProcessTicket>;   
        OutstandingRequestSet mOutstandingRequestSet;

        auto addRequest(ResourceRequest& req) -> BackgroundProcessTicket;

    public:
        ResourceBackgroundQueue();
        ~ResourceBackgroundQueue() override;

        /** Initialise the background queue system. 
        @note Called automatically by Root::initialise.
        */
        virtual void initialise();

        /** Shut down the background queue system. 
        @note Called automatically by Root::shutdown.
        */
        virtual void shutdown();

        /** Initialise a resource group in the background.
        @see ResourceGroupManager::initialiseResourceGroup
        @param name The name of the resource group to initialise
        @param listener Optional callback interface, take note of warnings in 
            the header and only use if you understand them.
        @return Ticket identifying the request, use isProcessComplete() to 
            determine if completed if not using listener
        */
        virtual auto initialiseResourceGroup(
            std::string_view name, Listener* listener = nullptr) -> BackgroundProcessTicket;

        /** Initialise all resource groups which are yet to be initialised in 
            the background.
        @see ResourceGroupManager::intialiseResourceGroup
        @param listener Optional callback interface, take note of warnings in 
            the header and only use if you understand them.
        @return Ticket identifying the request, use isProcessComplete() to 
            determine if completed if not using listener
        */
        virtual auto initialiseAllResourceGroups( 
            Listener* listener = nullptr) -> BackgroundProcessTicket;
        /** Prepares a resource group in the background.
        @see ResourceGroupManager::prepareResourceGroup
        @param name The name of the resource group to prepare
        @param listener Optional callback interface, take note of warnings in 
            the header and only use if you understand them.
        @return Ticket identifying the request, use isProcessComplete() to 
            determine if completed if not using listener
        */
        virtual auto prepareResourceGroup(std::string_view name, 
            Listener* listener = nullptr) -> BackgroundProcessTicket;

        /** Loads a resource group in the background.
        @see ResourceGroupManager::loadResourceGroup
        @param name The name of the resource group to load
        @param listener Optional callback interface, take note of warnings in 
            the header and only use if you understand them.
        @return Ticket identifying the request, use isProcessComplete() to 
            determine if completed if not using listener
        */
        virtual auto loadResourceGroup(std::string_view name, 
            Listener* listener = nullptr) -> BackgroundProcessTicket;


        /** Unload a single resource in the background. 
        @see ResourceManager::unload
        @param resType The type of the resource 
            (from ResourceManager::getResourceType())
        @param name The name of the Resource
        @param listener Optional callback interface, take note of warnings in
            the header and only use if you understand them.
        */
        virtual auto unload(
            std::string_view resType, std::string_view name, 
            Listener* listener = nullptr) -> BackgroundProcessTicket;

        /** Unload a single resource in the background. 
        @see ResourceManager::unload
        @param resType The type of the resource 
            (from ResourceManager::getResourceType())
        @param handle Handle to the resource 
        @param listener Optional callback interface, take note of warnings in
            the header and only use if you understand them.
        */
        virtual auto unload(
            std::string_view resType, ResourceHandle handle, 
            Listener* listener = nullptr) -> BackgroundProcessTicket;

        /** Unloads a resource group in the background.
        @see ResourceGroupManager::unloadResourceGroup
        @param name The name of the resource group to load
        @param listener Optional callback interface, take note of warnings in
            the header and only use if you understand them.
        @return Ticket identifying the request, use isProcessComplete() to 
            determine if completed if not using listener
        */
        virtual auto unloadResourceGroup(std::string_view name, 
            Listener* listener = nullptr) -> BackgroundProcessTicket;


        /** Prepare a single resource in the background. 
        @see ResourceManager::prepare
        @param resType The type of the resource 
            (from ResourceManager::getResourceType())
        @param name The name of the Resource
        @param group The resource group to which this resource will belong
        @param isManual Is the resource to be manually loaded? If so, you should
            provide a value for the loader parameter
        @param loader The manual loader which is to perform the required actions
            when this resource is loaded; only applicable when you specify true
            for the previous parameter. NOTE: must be thread safe!!
        @param loadParams Optional pointer to a list of name/value pairs 
            containing loading parameters for this type of resource. Remember 
            that this must have a lifespan longer than the return of this call!
        @param listener Optional callback interface, take note of warnings in
            the header and only use if you understand them.
        */
        virtual auto prepare(
            std::string_view resType, std::string_view name, 
            std::string_view group, bool isManual = false, 
            ManualResourceLoader* loader = nullptr, 
            const NameValuePairList* loadParams = nullptr, 
            Listener* listener = nullptr) -> BackgroundProcessTicket;

        /** Load a single resource in the background. 
        @see ResourceManager::load
        @param resType The type of the resource 
            (from ResourceManager::getResourceType())
        @param name The name of the Resource
        @param group The resource group to which this resource will belong
        @param isManual Is the resource to be manually loaded? If so, you should
            provide a value for the loader parameter
        @param loader The manual loader which is to perform the required actions
            when this resource is loaded; only applicable when you specify true
            for the previous parameter. NOTE: must be thread safe!!
        @param loadParams Optional pointer to a list of name/value pairs 
            containing loading parameters for this type of resource. Remember 
            that this must have a lifespan longer than the return of this call!
        @param listener Optional callback interface, take note of warnings in
            the header and only use if you understand them.
        */
        virtual auto load(
            std::string_view resType, std::string_view name, 
            std::string_view group, bool isManual = false, 
            ManualResourceLoader* loader = nullptr, 
            const NameValuePairList* loadParams = nullptr, 
            Listener* listener = nullptr) -> BackgroundProcessTicket;
        /** Returns whether a previously queued process has completed or not. 
        @remarks
            This method of checking that a background process has completed is
            the 'polling' approach. Each queued method takes an optional listener
            parameter to allow you to register a callback instead, which is
            arguably more efficient.
        @param ticket The ticket which was returned when the process was queued
        @return true if process has completed (or if the ticket is 
            unrecognised), false otherwise
        @note Tickets are not stored once complete so do not accumulate over 
            time.
        This is why a non-existent ticket will return 'true'.
        */
        virtual auto isProcessComplete(BackgroundProcessTicket ticket) -> bool;

        /** Aborts background process.
        */
        void abortRequest( BackgroundProcessTicket ticket );

        /// Implementation for WorkQueue::RequestHandler
        auto canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) -> bool override;
        /// Implementation for WorkQueue::RequestHandler
        auto handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) -> WorkQueue::Response* override;
        /// Implementation for WorkQueue::ResponseHandler
        auto canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) -> bool override;
        /// Implementation for WorkQueue::ResponseHandler
        void handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override;

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> ResourceBackgroundQueue&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> ResourceBackgroundQueue*;

    };

    /** @} */
    /** @} */

}
