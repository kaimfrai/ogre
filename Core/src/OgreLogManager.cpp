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

#include <cassert>
#include <cstddef>

module Ogre.Core;

import :Exception;
import :Log;
import :LogManager;
import :Prerequisites;
import :Singleton;

import <map>;
import <string>;
import <utility>;

namespace Ogre {

    //-----------------------------------------------------------------------
    auto LogManager::getSingletonPtr() noexcept -> LogManager*
    {
        return msSingleton;
    }
    auto LogManager::getSingleton() noexcept -> LogManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    LogManager::LogManager()
    {
        mDefaultLog = nullptr;
    }
    //-----------------------------------------------------------------------
    auto LogManager::createLog( std::string_view name, bool defaultLog, bool debuggerOutput, 
        bool suppressFileOutput) -> Log*
    {
        Log* newLog = new Log(name, debuggerOutput, suppressFileOutput);

        if( !mDefaultLog || defaultLog )
        {
            mDefaultLog = newLog;
        }

        mLogs.emplace(name, newLog);

        return newLog;
    }
    //-----------------------------------------------------------------------
    auto LogManager::getDefaultLog() noexcept -> Log*
    {
        return mDefaultLog;
    }
    //-----------------------------------------------------------------------
    auto LogManager::setDefaultLog(Log* newLog) -> Log*
    {
        Log* oldLog = mDefaultLog;
        mDefaultLog = newLog;
        return oldLog;
    }
    //-----------------------------------------------------------------------
    auto LogManager::getLog( std::string_view name) -> Log*
    {
        auto i = mLogs.find(name);
        OgreAssert(i != mLogs.end(), "Log not found");
        return i->second.get();
    }
    //-----------------------------------------------------------------------
    void LogManager::destroyLog(std::string_view name)
    {
        auto i = mLogs.find(name);
        if (i != mLogs.end())
        {
            if (mDefaultLog == i->second.get())
            {
                mDefaultLog = nullptr;
            }
            mLogs.erase(i);
        }

        // Set another default log if this one removed
        if (!mDefaultLog && !mLogs.empty())
        {
            mDefaultLog = mLogs.begin()->second.get();
        }
    }
    //-----------------------------------------------------------------------
    void LogManager::destroyLog(Log* log)
    {
        OgreAssert(log, "Cannot destroy a null log");
        destroyLog(log->getName());
    }
    //-----------------------------------------------------------------------
    void LogManager::logMessage( std::string_view message, LogMessageLevel lml, bool maskDebug)
    {
        if (mDefaultLog)
        {
            mDefaultLog->logMessage(message, lml, maskDebug);
        }
    }

    void LogManager::logError(std::string_view message, bool maskDebug )
    {
        stream(LogMessageLevel::Critical, maskDebug) << "Error: " << message;
    }

    void LogManager::logWarning(std::string_view message, bool maskDebug )
    {
        stream(LogMessageLevel::Warning, maskDebug) << "Warning: " << message;
    }

    //-----------------------------------------------------------------------
    void LogManager::setMinLogLevel(LogMessageLevel lml)
    {
        if (mDefaultLog)
        {
            mDefaultLog->setMinLogLevel(lml);
        }
    }
    //---------------------------------------------------------------------
    auto LogManager::stream(LogMessageLevel lml, bool maskDebug) -> Log::Stream
    {
        OgreAssert(mDefaultLog, "Default log not found");
        return mDefaultLog->stream(lml, maskDebug);
    }
}
