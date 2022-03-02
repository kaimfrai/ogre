/*-------------------------------------------------------------------------
This source file is a part of OGRE
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
THE SOFTWARE
-------------------------------------------------------------------------*/
#ifndef __OgreThreadDefinesSTD_H__
#define __OgreThreadDefinesSTD_H__

// Thread objects and related functions
#define OGRE_THREAD_TYPE std::thread
#define OGRE_THREAD_CREATE(name, worker) std::thread* name = OGRE_NEW_T(std::thread, MEMCATEGORY_GENERAL)(worker)
#define OGRE_THREAD_DESTROY(name) OGRE_DELETE_T(name, thread, MEMCATEGORY_GENERAL)
#define OGRE_THREAD_HARDWARE_CONCURRENCY std::thread::hardware_concurrency()
#define OGRE_THREAD_CURRENT_ID std::this_thread::get_id()
#define OGRE_THREAD_WORKER_INHERIT

#define OGRE_WQ_MUTEX(name) mutable std::recursive_mutex name
#define OGRE_WQ_LOCK_MUTEX(name) std::unique_lock<std::recursive_mutex> OGRE_TOKEN_PASTE(ogrenameLock, __LINE__) (name)
#define OGRE_WQ_LOCK_MUTEX_NAMED(mutexName, lockName) std::unique_lock<std::recursive_mutex> lockName(mutexName)

// Read-write mutex
#define OGRE_WQ_RW_MUTEX(name) mutable std::recursive_mutex name
#define OGRE_WQ_LOCK_RW_MUTEX_READ(name) std::unique_lock<std::recursive_mutex> OGRE_TOKEN_PASTE(ogrenameLock, __LINE__) (name)
#define OGRE_WQ_LOCK_RW_MUTEX_WRITE(name) std::unique_lock<std::recursive_mutex> OGRE_TOKEN_PASTE(ogrenameLock, __LINE__) (name)

#define OGRE_WQ_THREAD_SYNCHRONISER(sync) std::condition_variable_any sync
#define OGRE_THREAD_WAIT(sync, mutex, lock) sync.wait(lock)
#define OGRE_THREAD_NOTIFY_ONE(sync) sync.notify_one()
#define OGRE_THREAD_NOTIFY_ALL(sync) sync.notify_all()

#endif
