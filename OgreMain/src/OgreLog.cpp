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
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "OgreStableHeaders.h"
#include "OgreBuildSettings.h"
#include "OgreLog.h"
#include "OgrePlatform.h"
#include "OgrePrerequisites.h"
#include "OgreStringConverter.h"

// LogMessageLevel + LoggingLevel > OGRE_LOG_THRESHOLD = message logged
#define OGRE_LOG_THRESHOLD 4

namespace {
    const char* RED = "\x1b[31;1m";
    const char* YELLOW = "\x1b[33;1m";
    const char* RESET = "\x1b[0m";
}

namespace Ogre
{
    //-----------------------------------------------------------------------
    Log::Log( const String& name, bool debuggerOutput, bool suppressFile ) : 
        mLogLevel(LML_NORMAL), mDebugOut(debuggerOutput),
        mSuppressFile(suppressFile), mTimeStamp(true), mLogName(name), mTermHasColours(false)
    {
        if (!mSuppressFile)
        {
            mLog.open(name.c_str());
        }

        char* val = getenv("OGRE_MIN_LOGLEVEL");
        int min_lml;
        if(val && StringConverter::parse(val, min_lml))
            setMinLogLevel(LogMessageLevel(min_lml));

        if(mDebugOut)
        {
            val = getenv("TERM");
            mTermHasColours = val && String(val).find("xterm") != String::npos;
        }
    }
    //-----------------------------------------------------------------------
    Log::~Log()
    {
        if (!mSuppressFile)
        {
            mLog.close();
        }
    }

    //-----------------------------------------------------------------------
    void Log::logMessage( const String& message, LogMessageLevel lml, bool maskDebug )
    {
        if (lml >= mLogLevel)
        {
            bool skipThisMessage = false;
            for( mtLogListener::iterator i = mListeners.begin(); i != mListeners.end(); ++i )
                (*i)->messageLogged( message, lml, maskDebug, mLogName, skipThisMessage);
            
            if (!skipThisMessage)
            {
                if (mDebugOut && !maskDebug)
                {
                    std::ostream& os = int(lml) >= int(LML_WARNING) ? std::cerr : std::cout;

                    if(mTermHasColours) {
                        if(lml == LML_WARNING)
                            os << YELLOW;
                        if(lml == LML_CRITICAL)
                            os << RED;
                    }

                    os << message;

                    if(mTermHasColours) {
                        os << RESET;
                    }

                    os << std::endl;
                }

                // Write time into log
                if (!mSuppressFile)
                {
                    if (mTimeStamp)
                    {
                        struct tm *pTime;
                        time_t ctTime; time(&ctTime);
                        pTime = localtime( &ctTime );
                        mLog << std::setw(2) << std::setfill('0') << pTime->tm_hour
                            << ":" << std::setw(2) << std::setfill('0') << pTime->tm_min
                            << ":" << std::setw(2) << std::setfill('0') << pTime->tm_sec
                            << ": ";
                    }
                    mLog << message << std::endl;

                    // Flush stcmdream to ensure it is written (incase of a crash, we need log to be up to date)
                    mLog.flush();
                }
            }
        }
    }
    
    //-----------------------------------------------------------------------
    void Log::setTimeStampEnabled(bool timeStamp)
    {
        mTimeStamp = timeStamp;
    }

    //-----------------------------------------------------------------------
    void Log::setDebugOutputEnabled(bool debugOutput)
    {
        mDebugOut = debugOutput;
    }

    //-----------------------------------------------------------------------
    void Log::setLogDetail(LoggingLevel ll)
    {
        mLogLevel = LogMessageLevel(OGRE_LOG_THRESHOLD - ll);
    }

    void Log::setMinLogLevel(LogMessageLevel lml)
    {
        mLogLevel = lml;
    }

    //-----------------------------------------------------------------------
    void Log::addListener(LogListener* listener)
    {
        if (std::find(mListeners.begin(), mListeners.end(), listener) == mListeners.end())
            mListeners.push_back(listener);
    }

    //-----------------------------------------------------------------------
    void Log::removeListener(LogListener* listener)
    {
        mtLogListener::iterator i = std::find(mListeners.begin(), mListeners.end(), listener);
        if (i != mListeners.end())
            mListeners.erase(i);
    }
    //---------------------------------------------------------------------
    Log::Stream Log::stream(LogMessageLevel lml, bool maskDebug) 
    {
        return Stream(this, lml, maskDebug);

    }
}
