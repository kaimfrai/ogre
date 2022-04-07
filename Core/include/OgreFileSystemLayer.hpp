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
export module Ogre.Core:FileSystemLayer;

export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :StringVector;

export import <string>;
export import <vector>;

export
namespace Ogre
{
    /** Provides methods to find out where the Ogre config files are stored
        and where logs and settings files should be written to.

        In modern multi-user OS, a standard user account will often not
        have write access to the path where the application is stored.
        In order to still be able to store graphics settings and log
        output and for the user to overwrite the default Ogre config files,
        this class tries to create a folder inside the user's home directory.
      */
    class FileSystemLayer : public FileSystemLayerAlloc
    {
    public:
        /** Creates a concrete platform-dependent implementation of FileSystemLayer.
         @param subdir
         A subdirectory inside the user's path to distinguish between
         different Ogre applications.
         */
        FileSystemLayer(const Ogre::String& subdir)
        {
            // determine directories to search for config files
            getConfigPaths();
            // prepare write location in user directory
            prepareUserHome(subdir);
        }
        
        /** Search for the given config file in a set of predefined locations

         The search order is
         1. Subdirectory in user Home (see @ref getWritablePath)
         2. OS dependant config-paths
         3. Current working directory

         @param filename The config file name (without path)
         @return The full path to the config file
         */
        [[nodiscard]]
        auto getConfigFilePath(Ogre::String filename) const -> Ogre::String
        {
            // look for the requested file in several locations:
            
            // 1. in the writable path (so user can provide custom files)
            Ogre::String path = getWritablePath(filename);
            if (fileExists(path))
                return path;
            
            // 2. in the config file search paths
            for (const String& cpath : mConfigPaths)
            {
                path = cpath + filename;
                if (fileExists(path))
                    return path;
            }
            
            // 3. fallback to current working dir
            return filename;
        }

        /** Find a path where the given filename can be written to. This path
         will usually be a subdirectory in the user's home directory.
         This function should be used for any output like logs and graphics settings.

         | Platform         | Location |
         |------------------|----------|
         | Windows          | Documents/$subdir/ |
         | Linux            | ~/.cache/$subdir/ |
         | OSX              | ~/Library/Application Support/$subdir/ |
         | iOS              | NSDocumentDirectory |
         | Android / Emscripten | n/a |

         @param filename Name of the file.
         @return The full path to a writable location for the given filename.
         */
        [[nodiscard]]
        auto getWritablePath(const Ogre::String& filename) const -> Ogre::String
        {
            return mHomePath + filename;
        }
        
        void setConfigPaths(const Ogre::StringVector &paths){
            mConfigPaths = paths;
        }
        
        void setHomePath(const Ogre::String &path){
            mHomePath = path;
        }
        
        /** Resolve path inside the application bundle
         * on some platforms Ogre is delivered as an application bundle
         * this function resolves the given path such that it points inside that bundle
         * @param path
         * @return path inside the bundle
         */
        static auto resolveBundlePath(String path) -> String;

        /** Create a directory. */
        static auto createDirectory(const Ogre::String& name) -> bool;
        /** Delete a directory. Should be empty */
        static auto removeDirectory(const Ogre::String& name) -> bool;
        /** Test if the given file exists. */
        static auto fileExists(const Ogre::String& path) -> bool;
        /** Delete a file. */
        static auto removeFile(const Ogre::String& path) -> bool;
        /** Rename a file. */
        static auto renameFile(const Ogre::String& oldpath, const Ogre::String& newpath) -> bool;

    private:
        Ogre::StringVector mConfigPaths;
        Ogre::String mHomePath;
        
        /** Determine config search paths. */
        void getConfigPaths();
        
        /** Create an Ogre directory and the given subdir in the user's home. */
        void prepareUserHome(const Ogre::String& subdir);
    };

}
