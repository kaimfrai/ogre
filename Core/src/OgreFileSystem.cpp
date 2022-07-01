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
#include <sys/stat.h>

#include <cstdint>
#include <cstdio>
#include <format>
#include <fstream> // IWYU pragma: keep
#include <string>

#include "OgreArchive.hpp"
#include "OgreDataStream.hpp"
#include "OgreException.hpp"
#include "OgreFileSystem.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSearchOps.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreString.hpp"
#include "OgreStringVector.hpp"

namespace Ogre {

namespace {
    /** Specialisation of the Archive class to allow reading of files from
        filesystem folders / directories.
    */
    class FileSystemArchive : public Archive
    {
    protected:
        /** Utility method to retrieve all files in a directory matching pattern.
        @param pattern
            File pattern.
        @param recursive
            Whether to cascade down directories.
        @param dirs
            Set to @c true if you want the directories to be listed instead of files.
        @param simpleList
            Populated if retrieving a simple list.
        @param detailList
            Populated if retrieving a detailed list.
        */
        void findFiles(const String& pattern, bool recursive, bool dirs,
            StringVector* simpleList, FileInfoList* detailList) const;

    public:
        FileSystemArchive(const String& name, const String& archType, bool readOnly );
        ~FileSystemArchive() override;

        /// @copydoc Archive::isCaseSensitive
        [[nodiscard]] bool isCaseSensitive() const noexcept override;

        /// @copydoc Archive::load
        void load() override;
        /// @copydoc Archive::unload
        void unload() override;

        /// @copydoc Archive::open
        [[nodiscard]] DataStreamPtr open(const String& filename, bool readOnly = true) const override;

        /// @copydoc Archive::create
        DataStreamPtr create(const String& filename) override;

        /// @copydoc Archive::remove
        void remove(const String& filename) override;

        /// @copydoc Archive::list
        [[nodiscard]] StringVectorPtr list(bool recursive = true, bool dirs = false) const override;

        /// @copydoc Archive::listFileInfo
        [[nodiscard]] FileInfoListPtr listFileInfo(bool recursive = true, bool dirs = false) const override;

        /// @copydoc Archive::find
        [[nodiscard]] StringVectorPtr find(const String& pattern, bool recursive = true,
            bool dirs = false) const override;

        /// @copydoc Archive::findFileInfo
        [[nodiscard]] FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true,
            bool dirs = false) const override;

        /// @copydoc Archive::exists
        [[nodiscard]] bool exists(const String& filename) const override;

        /// @copydoc Archive::getModifiedTime
        [[nodiscard]] time_t getModifiedTime(const String& filename) const override;
    };

    bool gIgnoreHidden = true;
}

    //-----------------------------------------------------------------------
    FileSystemArchive::FileSystemArchive(const String& name, const String& archType, bool readOnly )
        : Archive(name, archType)
    {
        // Even failed attempt to write to read only location violates Apple AppStore validation process.
        // And successful writing to some probe file does not prove that whole location with subfolders 
        // is writable. Therefore we accept read only flag from outside and do not try to be too smart.
        mReadOnly = readOnly;
    }
    //-----------------------------------------------------------------------
    bool FileSystemArchive::isCaseSensitive() const noexcept
    {
        return true;
    }
    //-----------------------------------------------------------------------
    static bool is_reserved_dir (const char *fn)
    {
        return (fn [0] == '.' && (fn [1] == 0 || (fn [1] == '.' && fn [2] == 0)));
    }
    //-----------------------------------------------------------------------
    static bool is_absolute_path(const char* path)
    {
        return path[0] == '/' || path[0] == '\\';
    }
    //-----------------------------------------------------------------------
    static String concatenate_path(const String& base, const String& name)
    {
        if (base.empty() || is_absolute_path(name.c_str()))
            return name;
        else
            return base + '/' + name;
    }

    //-----------------------------------------------------------------------
    void FileSystemArchive::findFiles(const String& pattern, bool recursive, 
        bool dirs, StringVector* simpleList, FileInfoList* detailList) const
    {
        intptr_t lHandle, res;

        struct _finddata_t tagData;

        // pattern can contain a directory name, separate it from mask
        size_t pos1 = pattern.rfind ('/');
        size_t pos2 = pattern.rfind ('\\');
        if (pos1 == pattern.npos || ((pos2 != pattern.npos) && (pos1 < pos2)))
            pos1 = pos2;
        String directory;
        if (pos1 != pattern.npos)
            directory = pattern.substr (0, pos1 + 1);

        String full_pattern = concatenate_path(mName, pattern);

        lHandle = _findfirst(full_pattern.c_str(), &tagData);

        res = 0;
        while (lHandle != -1 && res != -1)
        {
            if ((dirs == ((tagData.attrib & _A_SUBDIR) != 0)) &&
                ( !gIgnoreHidden || (tagData.attrib & _A_HIDDEN) == 0 ) &&
                (!dirs || !is_reserved_dir (tagData.name)))
            {
                if (simpleList)
                {
                    simpleList->push_back(directory + tagData.name);
                }
                else if (detailList)
                {
                    FileInfo fi;
                    fi.archive = this;

                    fi.filename = directory + tagData.name;
                    fi.basename = tagData.name;

                    fi.path = directory;
                    fi.compressedSize = tagData.size;
                    fi.uncompressedSize = tagData.size;
                    detailList->push_back(fi);
                }
            }

            res = _findnext( lHandle, &tagData );
        }
        // Close if we found any files
        if(lHandle != -1)
            _findclose(lHandle);

        // Now find directories
        if (recursive)
        {
            String base_dir = mName;
            if (!directory.empty ())
            {
                base_dir = concatenate_path(mName, directory);
                // Remove the last '/'
                base_dir.erase (base_dir.length () - 1);
            }
            base_dir.append ("/*");

            // Remove directory name from pattern
            String mask ("/");
            if (pos1 != pattern.npos)
                mask.append (pattern.substr (pos1 + 1));
            else
                mask.append (pattern);

            lHandle = _findfirst(base_dir.c_str (), &tagData);

            res = 0;
            while (lHandle != -1 && res != -1)
            {
                if ((tagData.attrib & _A_SUBDIR) &&
                    ( !gIgnoreHidden || (tagData.attrib & _A_HIDDEN) == 0 ) &&
                    !is_reserved_dir (tagData.name))
                {
                    // recurse
                    base_dir = directory;

                    base_dir.append (tagData.name).append (mask);

                    findFiles(base_dir, recursive, dirs, simpleList, detailList);
                }

                res = _findnext( lHandle, &tagData );
            }
            // Close if we found any files
            if(lHandle != -1)
                _findclose(lHandle);
        }
    }
    //-----------------------------------------------------------------------
    FileSystemArchive::~FileSystemArchive()
    {
        unload();
    }
    //-----------------------------------------------------------------------
    void FileSystemArchive::load()
    {
        // nothing to do here
    }
    //-----------------------------------------------------------------------
    void FileSystemArchive::unload()
    {
        // nothing to see here, move along
    }
    //-----------------------------------------------------------------------
    DataStreamPtr FileSystemArchive::open(const String& filename, bool readOnly) const
    {
        if (!readOnly && isReadOnly())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot open a file in read-write mode in a read-only archive");
        }

        // Always open in binary mode
        // Also, always include reading
        std::ios::openmode mode = std::ios::in | std::ios::binary;

        if(!readOnly) mode |= std::ios::out;

        return _openFileStream(concatenate_path(mName, filename), mode, filename);
    }
    DataStreamPtr _openFileStream(const String& full_path, std::ios::openmode mode, const String& name)
    {
        // Use filesystem to determine size 
        // (quicker than streaming to the end and back)

        struct stat tagStat;
        int ret = stat(full_path.c_str(), &tagStat);

        size_t st_size = ret == 0 ? tagStat.st_size : 0;

        std::istream* baseStream = nullptr;
        std::ifstream* roStream = nullptr;
        std::fstream* rwStream = nullptr;

        if (mode & std::ios::out)
        {
            rwStream = new std::fstream();

            rwStream->open(full_path.c_str(), mode);

            baseStream = rwStream;
        }
        else
        {
            roStream = new std::ifstream();

            roStream->open(full_path.c_str(), mode);

            baseStream = roStream;
        }


        // Should check ensure open succeeded, in case fail for some reason.
        if (baseStream->fail())
        {
            delete roStream;
            delete rwStream;
            OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND, ::std::format("Cannot open file: {}", full_path));
        }

        /// Construct return stream, tell it to delete on destroy
        FileStreamDataStream* stream = nullptr;
        const String& streamname = name.empty() ? full_path : name;
        if (rwStream)
        {
            // use the writeable stream
            stream = new FileStreamDataStream(streamname, rwStream, st_size);
        }
        else
        {
            OgreAssertDbg(ret == 0, "Problem getting file size");
            // read-only stream
            stream = new FileStreamDataStream(streamname, roStream, st_size);
        }
        return DataStreamPtr(stream);
    }
    //---------------------------------------------------------------------
    DataStreamPtr FileSystemArchive::create(const String& filename)
    {
        if (isReadOnly())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot create a file in a read-only archive");
        }

        String full_path = concatenate_path(mName, filename);

        // Always open in binary mode
        // Also, always include reading
        std::ios::openmode mode = std::ios::out | std::ios::binary;
        auto* rwStream = new std::fstream();

        rwStream->open(full_path.c_str(), mode);

        // Should check ensure open succeeded, in case fail for some reason.
        if (rwStream->fail())
        {
            delete rwStream;
            OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND, ::std::format("Cannot open file: {}", filename));
        }

        /// Construct return stream, tell it to delete on destroy
        auto* stream = new FileStreamDataStream(filename,
                rwStream, 0, true);

        return DataStreamPtr(stream);
    }
    //---------------------------------------------------------------------
    void FileSystemArchive::remove(const String& filename)
    {
        if (isReadOnly())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot remove a file from a read-only archive");
        }
        String full_path = concatenate_path(mName, filename);

        ::remove(full_path.c_str());
    }
    //-----------------------------------------------------------------------
    StringVectorPtr FileSystemArchive::list(bool recursive, bool dirs) const
    {
        // directory change requires locking due to saved returns
        StringVectorPtr ret(new StringVector());

        findFiles("*", recursive, dirs, ret.get(), nullptr);

        return ret;
    }
    //-----------------------------------------------------------------------
    FileInfoListPtr FileSystemArchive::listFileInfo(bool recursive, bool dirs) const
    {
        FileInfoListPtr ret(new FileInfoList());

        findFiles("*", recursive, dirs, nullptr, ret.get());

        return ret;
    }
    //-----------------------------------------------------------------------
    StringVectorPtr FileSystemArchive::find(const String& pattern,
                                            bool recursive, bool dirs) const
    {
        StringVectorPtr ret(new StringVector());

        findFiles(pattern, recursive, dirs, ret.get(), nullptr);

        return ret;

    }
    //-----------------------------------------------------------------------
    FileInfoListPtr FileSystemArchive::findFileInfo(const String& pattern, 
        bool recursive, bool dirs) const
    {
        FileInfoListPtr ret(new FileInfoList());

        findFiles(pattern, recursive, dirs, nullptr, ret.get());

        return ret;
    }
    //-----------------------------------------------------------------------
    bool FileSystemArchive::exists(const String& filename) const
    {
        if (filename.empty())
            return false;

        String full_path = concatenate_path(mName, filename);

        struct stat tagStat;
        bool ret = (stat(full_path.c_str(), &tagStat) == 0);

        // stat will return true if the filename is absolute, but we need to check
        // the file is actually in this archive
        if (ret && is_absolute_path(filename.c_str()))
        {
            // only valid if full path starts with our base

            // case sensitive
            ret = Ogre::StringUtil::startsWith(full_path, mName, false);
        }

        return ret;
    }
    //---------------------------------------------------------------------
    time_t FileSystemArchive::getModifiedTime(const String& filename) const
    {
        String full_path = concatenate_path(mName, filename);

        struct stat tagStat;
        bool ret = (stat(full_path.c_str(), &tagStat) == 0);

        if (ret)
        {
            return tagStat.st_mtime;
        }
        else
        {
            return 0;
        }

    }
    //-----------------------------------------------------------------------
    const String& FileSystemArchiveFactory::getType() const noexcept
    {
        static String name = "FileSystem";
        return name;
    }

    Archive *FileSystemArchiveFactory::createInstance( const String& name, bool readOnly )
    {
        return new FileSystemArchive(name, getType(), readOnly);
    }

    void FileSystemArchiveFactory::setIgnoreHidden(bool ignore)
    {
        gIgnoreHidden = ignore;
    }

    bool FileSystemArchiveFactory::getIgnoreHidden() noexcept
    {
        return gIgnoreHidden;
    }
}
