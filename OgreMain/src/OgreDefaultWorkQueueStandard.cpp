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
#include <condition_variable>
#include <cxxabi.h>
#include <memory>
#include <mutex>
#include <stddef.h>
#include <thread>
#include <vector>

#include "OgreStableHeaders.h"
#include "OgreDefaultWorkQueueStandard.h"
#include "OgreLog.h"
#include "OgreLogManager.h"
#include "OgreMemoryAllocatorConfig.h"
#include "OgrePrerequisites.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreWorkQueue.h"

namespace Ogre
{
    //---------------------------------------------------------------------
    DefaultWorkQueue::DefaultWorkQueue(const String& name)
    : DefaultWorkQueueBase(name), mNumThreadsRegisteredWithRS(0)
    {
    }
    //---------------------------------------------------------------------
    DefaultWorkQueue::~DefaultWorkQueue()
    {
        shutdown();
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueue::startup(bool forceRestart)
    {
        if (mIsRunning)
        {
            if (forceRestart)
                shutdown();
            else
                return;
        }

        mShuttingDown = false;

        mWorkerFunc = new WorkerFunc(this);

        LogManager::getSingleton().stream() <<
            "DefaultWorkQueue('" << mName << "') initialising on thread " <<
            std::this_thread::get_id()
            << ".";

        if (mWorkerRenderSystemAccess)
            Root::getSingleton().getRenderSystem()->preExtraThreadsStarted();

        mNumThreadsRegisteredWithRS = 0;
        for (size_t i = 0; i < mWorkerThreadCount; ++i)
        {
            std::thread* t = new std::thread(*mWorkerFunc);
            mWorkers.push_back(t);
        }

        if (mWorkerRenderSystemAccess)
        {
            std::unique_lock<std::recursive_mutex> initLock(mInitMutex);
            // have to wait until all threads are registered with the render system
            while (mNumThreadsRegisteredWithRS < mWorkerThreadCount)
                mInitSync.wait(initLock);

            Root::getSingleton().getRenderSystem()->postExtraThreadsStarted();

        }

        mIsRunning = true;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueue::notifyThreadRegistered()
    {
        std::unique_lock<std::recursive_mutex> ogrenameLock(mInitMutex);

        ++mNumThreadsRegisteredWithRS;

        // wake up main thread
        mInitSync.notify_all();
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueue::shutdown()
    {
        if( !mIsRunning )
            return;

        LogManager::getSingleton().stream() <<
            "DefaultWorkQueue('" << mName << "') shutting down on thread " <<
            std::this_thread::get_id()
            << ".";

        mShuttingDown = true;
        abortAllRequests();
        // wake all threads (they should check shutting down as first thing after wait)
        mRequestCondition.notify_all();

        // all our threads should have been woken now, so join
        for (WorkerThreadList::iterator i = mWorkers.begin(); i != mWorkers.end(); ++i)
        {
            (*i)->join();
            delete *i;
        }
        mWorkers.clear();

        delete mWorkerFunc;
        mWorkerFunc = 0;

        mIsRunning = false;
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueue::notifyWorkers()
    {
        // wake up waiting thread
        mRequestCondition.notify_one();
    }

    //---------------------------------------------------------------------
    void DefaultWorkQueue::waitForNextRequest()
    {
        // Lock; note that wait will free the lock
        std::unique_lock<std::recursive_mutex> queueLock(mRequestMutex);
        if (mRequestQueue.empty())
        {
            // frees lock and suspends the thread
            mRequestCondition.wait(queueLock);
        }
        // When we get back here, it's because we've been notified 
        // and thus the thread has been woken up. Lock has also been
        // re-acquired, but we won't use it. It's safe to try processing and fail
        // if another thread has got in first and grabbed the request
    }
    //---------------------------------------------------------------------
    void DefaultWorkQueue::_threadMain()
    {
        // default worker thread
        LogManager::getSingleton().stream() << 
            "DefaultWorkQueue('" << getName() << "')::WorkerFunc - thread " 
            << std::this_thread::get_id() << " starting.";

        // Initialise the thread for RS if necessary
        if (mWorkerRenderSystemAccess)
        {
            Root::getSingleton().getRenderSystem()->registerThread();
            notifyThreadRegistered();
        }

        // Spin forever until we're told to shut down
        while (!isShuttingDown())
        {
            waitForNextRequest();
            _processNextRequest();
        }

        LogManager::getSingleton().stream() << 
            "DefaultWorkQueue('" << getName() << "')::WorkerFunc - thread " 
            << std::this_thread::get_id() << " stopped.";
    }

}
