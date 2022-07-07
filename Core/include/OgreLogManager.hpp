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

#ifndef OGRE_CORE_LOGMANAGER_H
#define OGRE_CORE_LOGMANAGER_H

#include <map>
#include <memory>
#include <string>

#include "OgreLog.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSingleton.hpp"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */
    /** The log manager handles the creation and retrieval of logs for the
        application.
        @remarks
            This class will create new log files and will retrieve instances
            of existing ones. Other classes wishing to log output can either
            create a fresh log or retrieve an existing one to output to.
            One log is the default log, and is the one written to when the
            logging methods of this class are called.
        @par
            By default, Root will instantiate a LogManager (which becomes the 
            Singleton instance) on construction, and will create a default log
            based on the Root construction parameters. If you want more control,
            for example redirecting log output right from the start or suppressing
            debug output, you need to create a LogManager yourself before creating
            a Root instance, then create a default log. Root will detect that 
            you've created one yourself and won't create one of its own, thus
            using all your logging preferences from the first instance.
    */
    class LogManager : public Singleton<LogManager>, public LogAlloc
    {
    private:
        using LogList = std::map<std::string_view, ::std::unique_ptr<Log>>;

        /// A list of all the logs the manager can access
        LogList mLogs;

        /// The default log to which output is done
        Log* mDefaultLog;

    public:
        LogManager();

        /** Creates a new log with the given name.
            @param
                name The name to give the log e.g. 'Ogre.log'
            @param
                defaultLog If true, this is the default log output will be
                sent to if the generic logging methods on this class are
                used. The first log created is always the default log unless
                this parameter is set.
            @param
                debuggerOutput If true, output to this log will also be
                routed to the debugger's output window.
            @param
                suppressFileOutput If true, this is a logical rather than a physical
                log and no file output will be written. If you do this you should
                register a LogListener so log output is not lost.
        */
        auto createLog( std::string_view name, bool defaultLog = false, bool debuggerOutput = true, 
            bool suppressFileOutput = false) -> Log*;

        /** Retrieves a log managed by this class.
        */
        auto getLog( std::string_view name) -> Log*;

        /** Returns a pointer to the default log.
        */
        auto getDefaultLog() noexcept -> Log*;

        /** Closes and removes a named log. */
        void destroyLog(std::string_view name);
        /** Closes and removes a log. */
        void destroyLog(Log* log);

        /** Sets the passed in log as the default log.
        @return The previous default log.
        */
        auto setDefaultLog(Log* newLog) -> Log*;

        /** Log a message to the default log.
        */
        void logMessage( std::string_view message, LogMessageLevel lml = LML_NORMAL, 
            bool maskDebug = false);

        /// @overload
        void logError(std::string_view message, bool maskDebug = false );
        /// @overload
        void logWarning(std::string_view message, bool maskDebug = false );

        /** Log a message to the default log (signature for backward compatibility).
        */
        void logMessage( LogMessageLevel lml, std::string_view message,  
            bool maskDebug = false) { logMessage(message, lml, maskDebug); }

        /** Get a stream on the default log. */
        auto stream(LogMessageLevel lml = LML_NORMAL, 
            bool maskDebug = false) -> Log::Stream;

        /// sets the minimal #LogMessageLevel for the default log
        void setMinLogLevel(LogMessageLevel lml);
        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> LogManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> LogManager*;

    };


    /** @} */
    /** @} */
}

#endif
