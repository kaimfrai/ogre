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
#include "OgreFileSystemLayer.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <format>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "OgrePrerequisites.hpp"
#include "OgreString.hpp"
#include "OgreStringVector.hpp"

namespace Ogre
{
    namespace {
        /** Get actual file pointed to by symlink */
        auto resolveSymlink(StringView symlink) -> const Ogre::String
        {
            ssize_t bufsize = 256;
            char* resolved = nullptr;
            do
            {
                char* buf = new char[bufsize];
                ssize_t retval = readlink(symlink.data(), buf, bufsize);
                if (retval == -1)
                {
                    delete[] buf;
                    break;
                }

                if (retval < bufsize)
                {
                    // operation was successful.
                    // readlink does not guarantee to 0-terminate, so do this manually
                    buf[retval] = '\0';
                    resolved = buf;
                }
                else
                {
                    // buffer was too small, grow buffer and try again
                    delete[] buf;
                    bufsize <<= 1;
                }
            } while (!resolved);

            if (resolved)
            {
                Ogre::String result (resolved);
                delete[] resolved;
                return result;
            }
            else
                return "";
        }
    }

    auto FileSystemLayer::resolveBundlePath(String path) -> String
    {
        // With Ubuntu snaps absolute paths are relative to the snap package.
        char* env_SNAP = getenv("SNAP");
        if (env_SNAP && !path.empty() && path[0] == '/' && // only adjust absolute dirs
            !StringUtil::startsWith(path, "/snap")) // not a snap path already
            path = env_SNAP + path;

        return path;
    }
    //---------------------------------------------------------------------
    void FileSystemLayer::getConfigPaths()
    {
        // try to determine the application's path:
        // recent systems should provide the executable path via the /proc system
        Ogre::String appPath = resolveSymlink("/proc/self/exe");
        if (appPath.empty())
        {
            // if /proc/self/exe isn't available, try it via the program's pid
            pid_t pid = getpid();
            char proc[64];
            int retval = snprintf(proc, sizeof(proc), "/proc/%llu/exe", (unsigned long long) pid);
            if (retval > 0 && retval < (long)sizeof(proc))
                appPath = resolveSymlink(proc);
        }

        if (!appPath.empty())
        {
            // we need to strip the executable name from the path
            Ogre::String::size_type pos = appPath.rfind('/');
            if (pos != Ogre::String::npos)
                appPath.erase(pos);

            // use application path as first config search path
            mConfigPaths.push_back(appPath + '/');
        }

        Dl_info info;
        if (dladdr((const void*)resolveSymlink, &info))
        {
            String base(info.dli_fname);
            // need to strip the module filename from the path
            String::size_type pos = base.rfind('/');
            if (pos != String::npos)
                base.erase(pos);

            // search inside ../share/OGRE
            mConfigPaths.push_back(StringUtil::normalizeFilePath(::std::format("{}/../share/OGRE/", base), false));
            // then look relative to PIP structure
            mConfigPaths.push_back(StringUtil::normalizeFilePath(::std::format("{}/../../../../share/OGRE/", base)));
        }

        // then try system wide /etc
        mConfigPaths.push_back("/etc/OGRE/");
    }
    //---------------------------------------------------------------------
    void FileSystemLayer::prepareUserHome(StringView subdir)
    {
        char* xdg_cache = getenv("XDG_CACHE_HOME");

        if(xdg_cache) {
            mHomePath = xdg_cache;
            mHomePath.append("/");
        } else {
            struct passwd* pwd = getpwuid(getuid());
            if (pwd)
            {
                mHomePath = pwd->pw_dir;
            }
            else
            {
                // try the $HOME environment variable
                mHomePath = getenv("HOME");
            }

            if(!mHomePath.empty()) {
                mHomePath.append("/.cache/");
            }
        }

        if (!mHomePath.empty())
        {
            // create the given subdir
            mHomePath.append(std::format("{}/", subdir));
            if (mkdir(mHomePath.c_str(), 0755) != 0 && errno != EEXIST)
            {
                // can't create dir
                mHomePath.clear();
            }
        }

        if (mHomePath.empty())
        {
            // couldn't create dir in home directory, fall back to cwd
            mHomePath = "./";
        }
    }
    //---------------------------------------------------------------------
    auto FileSystemLayer::fileExists(StringView path) -> bool
    {
        return access(path.data(), R_OK) == 0;
    }
    //---------------------------------------------------------------------
    auto FileSystemLayer::createDirectory(StringView path) -> bool
    {
        return !mkdir(path.data(), 0755) || errno == EEXIST;
    }
    //---------------------------------------------------------------------
    auto FileSystemLayer::removeDirectory(StringView path) -> bool
    {
        return !rmdir(path.data()) || errno == ENOENT;
    }
    //---------------------------------------------------------------------
    auto FileSystemLayer::removeFile(StringView path) -> bool
    {
        return !unlink(path.data()) || errno == ENOENT;
    }
    //---------------------------------------------------------------------
    auto FileSystemLayer::renameFile(StringView oldname, StringView newname) -> bool
    {
        return !rename(oldname.data(), newname.data());
    }
}
