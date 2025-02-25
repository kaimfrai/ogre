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

#include <cstdint>
#include <cstdio>

module Ogre.Core;

import :Archive;
import :DataStream;
import :Exception;
import :FileSystem;
import :Prerequisites;
import :SharedPtr;
import :String;
import :StringConverter;
import :StringVector;

import <format>;
import <fstream>;
import <string>;

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
        void findFiles(std::string_view pattern, bool recursive, bool dirs,
            StringVector* simpleList, FileInfoList* detailList) const;

    public:
        FileSystemArchive(std::string_view name, std::string_view archType, bool readOnly );
        ~FileSystemArchive() override;

        /// @copydoc Archive::isCaseSensitive
        [[nodiscard]] auto isCaseSensitive() const noexcept -> bool override;

        /// @copydoc Archive::load
        void load() override;
        /// @copydoc Archive::unload
        void unload() override;

        /// @copydoc Archive::open
        [[nodiscard]] auto open(std::string_view filename, bool readOnly = true) const -> DataStreamPtr override;

        /// @copydoc Archive::create
        auto create(std::string_view filename) -> DataStreamPtr override;

        /// @copydoc Archive::remove
        void remove(std::string_view filename) override;

        /// @copydoc Archive::list
        [[nodiscard]] auto list(bool recursive = true, bool dirs = false) const -> StringVectorPtr override;

        /// @copydoc Archive::listFileInfo
        [[nodiscard]] auto listFileInfo(bool recursive = true, bool dirs = false) const -> FileInfoListPtr override;

        /// @copydoc Archive::find
        [[nodiscard]] auto find(std::string_view pattern, bool recursive = true,
            bool dirs = false) const -> StringVectorPtr override;

        /// @copydoc Archive::findFileInfo
        [[nodiscard]] auto findFileInfo(std::string_view pattern, bool recursive = true,
            bool dirs = false) const -> FileInfoListPtr override;

        /// @copydoc Archive::exists
        [[nodiscard]] auto exists(std::string_view filename) const -> bool override;

        /// @copydoc Archive::getModifiedTime
        [[nodiscard]] auto getModifiedTime(std::string_view filename) const -> std::filesystem::file_time_type override;
    };

    bool gIgnoreHidden = true;
}

    //-----------------------------------------------------------------------
    FileSystemArchive::FileSystemArchive(std::string_view name, std::string_view archType, bool readOnly )
        : Archive(name, archType)
    {
        // Even failed attempt to write to read only location violates Apple AppStore validation process.
        // And successful writing to some probe file does not prove that whole location with subfolders 
        // is writable. Therefore we accept read only flag from outside and do not try to be too smart.
        mReadOnly = readOnly;
    }
    //-----------------------------------------------------------------------
    auto FileSystemArchive::isCaseSensitive() const noexcept -> bool
    {
        return true;
    }
    //-----------------------------------------------------------------------
    static auto is_reserved_dir (std::filesystem::path const& fn) -> bool
    {
        return fn.is_relative() and not fn.has_parent_path();
    }
    //-----------------------------------------------------------------------
    static auto is_absolute_path(std::filesystem::path const& path) -> bool
    {
        return path.is_absolute();
    }
    //-----------------------------------------------------------------------
    static auto concatenate_path(std::filesystem::path const& base, std::filesystem::path const& name) -> std::filesystem::path
    {
        if (base.empty() || is_absolute_path(name))
            return name;
        else
            return base / name;
    }

    //-----------------------------------------------------------------------
    void FileSystemArchive::findFiles(std::string_view pattern, bool recursive,
        bool dirs, StringVector* simpleList, FileInfoList* detailList) const
    {
        std::filesystem::path path{concatenate_path(mName, pattern)};

        auto const directory = path.parent_path();
        auto const filename = path.filename();
        std::string_view const native = filename.native();
        auto const starPos = native.find_first_of('*');
        auto const prefix = native.substr(0, starPos);
        auto const suffix = native.substr(starPos + 1);

        auto const fProcessEntry
        =   [   this
            ,   dirs
            ,   simpleList
            ,   detailList
            ,   directory
            ,   prefix
            ,   suffix
            ]   (   std::filesystem::directory_entry const
                    &   entry
                )
            {
                if (dirs)
                {
                    if (not entry.is_directory())
                        return;
                    if (is_reserved_dir(entry.path().filename()))
                        return;
                }
                else
                {
                    if (not entry.is_regular_file())
                        return;
                    // hidden path on linux
                    if (entry.path().filename().native()[0] == '.')
                        return;
                }

                if (not entry.path().native().starts_with(prefix))
                    return;
                if (not entry.path().native().ends_with(suffix))
                    return;

                if (simpleList)
                    simpleList->push_back(relative(entry.path(), directory));
                else if (detailList)
                {
                    FileInfo fi;
                    fi.archive = this;

                    fi.filename = relative(entry.path(), directory);
                    fi.basename = entry.path().filename();

                    fi.path = relative(entry.path(), directory).parent_path() / "";
                    fi.compressedSize = entry.file_size();
                    fi.uncompressedSize = entry.file_size();
                    detailList->push_back(fi);
                }
            }
        ;

        if (recursive)
        {
            for (auto const& entry : std::filesystem::recursive_directory_iterator{directory})
            {
                fProcessEntry(entry);
            }
        }
        else
        {
            for (auto const& entry : std::filesystem::directory_iterator{directory})
            {
                fProcessEntry(entry);
            }
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

    static auto _openFileStream(std::filesystem::path const& full_path, std::ios::openmode mode, std::filesystem::path const& name) -> DataStreamPtr
    {
        // Use filesystem to determine size 
        // (quicker than streaming to the end and back)

        std::error_code ec{};
        auto st_size = std::filesystem::file_size(full_path, ec);

        if (ec != std::error_code{})
            st_size = 0;

        std::istream* baseStream = nullptr;
        std::ifstream* roStream = nullptr;
        std::fstream* rwStream = nullptr;

        if (mode & std::ios::out)
        {
            rwStream = new std::fstream();

            rwStream->open(full_path, mode);

            baseStream = rwStream;
        }
        else
        {
            roStream = new std::ifstream();

            roStream->open(full_path, mode);

            baseStream = roStream;
        }


        // Should check ensure open succeeded, in case fail for some reason.
        if (baseStream->fail())
        {
            delete roStream;
            delete rwStream;
            OGRE_EXCEPT(ExceptionCodes::FILE_NOT_FOUND, ::std::format("Cannot open file: {}", full_path));
        }

        /// Construct return stream, tell it to delete on destroy
        FileStreamDataStream* stream = nullptr;
        auto const& streamname = name.empty() ? full_path : name;
        if (rwStream)
        {
            // use the writeable stream
            stream = new FileStreamDataStream(streamname.native(), rwStream, st_size);
        }
        else
        {
            OgreAssertDbg(ec == std::error_code{}, "Problem getting file size");
            // read-only stream
            stream = new FileStreamDataStream(streamname.native(), roStream, st_size);
        }
        return DataStreamPtr(stream);
    }
    //-----------------------------------------------------------------------
    auto FileSystemArchive::open(std::string_view filename, bool readOnly) const -> DataStreamPtr
    {
        if (!readOnly && isReadOnly())
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Cannot open a file in read-write mode in a read-only archive");
        }

        // Always open in binary mode
        // Also, always include reading
        std::ios::openmode mode = std::ios::in | std::ios::binary;

        if(!readOnly) mode |= std::ios::out;

        return _openFileStream(concatenate_path(mName, filename), mode, std::filesystem::path{filename});
    }

    auto  _openFileStream(std::string_view full_path, std::ios::openmode mode, std::string_view name) -> DataStreamPtr
    {
        return _openFileStream(std::filesystem::path{full_path}, mode, std::filesystem::path{name});
    }

    //---------------------------------------------------------------------
    auto FileSystemArchive::create(std::string_view filename) -> DataStreamPtr
    {
        if (isReadOnly())
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Cannot create a file in a read-only archive");
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
            OGRE_EXCEPT(ExceptionCodes::FILE_NOT_FOUND, ::std::format("Cannot open file: {}", filename));
        }

        /// Construct return stream, tell it to delete on destroy
        auto* stream = new FileStreamDataStream(filename,
                rwStream, 0, true);

        return DataStreamPtr(stream);
    }
    //---------------------------------------------------------------------
    void FileSystemArchive::remove(std::string_view filename)
    {
        if (isReadOnly())
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Cannot remove a file from a read-only archive");
        }
        auto const full_path = concatenate_path(mName, filename);

        std::filesystem::remove(full_path);
    }
    //-----------------------------------------------------------------------
    auto FileSystemArchive::list(bool recursive, bool dirs) const -> StringVectorPtr
    {
        // directory change requires locking due to saved returns
        StringVectorPtr ret(new StringVector());

        findFiles("*", recursive, dirs, ret.get(), nullptr);

        return ret;
    }
    //-----------------------------------------------------------------------
    auto FileSystemArchive::listFileInfo(bool recursive, bool dirs) const -> FileInfoListPtr
    {
        FileInfoListPtr ret(new FileInfoList());

        findFiles("*", recursive, dirs, nullptr, ret.get());

        return ret;
    }
    //-----------------------------------------------------------------------
    auto FileSystemArchive::find(std::string_view pattern,
                                            bool recursive, bool dirs) const -> StringVectorPtr
    {
        StringVectorPtr ret(new StringVector());

        findFiles(pattern, recursive, dirs, ret.get(), nullptr);

        return ret;

    }
    //-----------------------------------------------------------------------
    auto FileSystemArchive::findFileInfo(std::string_view pattern,
        bool recursive, bool dirs) const -> FileInfoListPtr
    {
        FileInfoListPtr ret(new FileInfoList());

        findFiles(pattern, recursive, dirs, nullptr, ret.get());

        return ret;
    }
    //-----------------------------------------------------------------------
    auto FileSystemArchive::exists(std::string_view filename) const -> bool
    {
        if (filename.empty())
            return false;

        auto const full_path = concatenate_path(mName, filename);

        bool ret = std::filesystem::exists(full_path);

        // stat will return true if the filename is absolute, but we need to check
        // the file is actually in this archive
        if (ret && is_absolute_path(filename))
        {
            // only valid if full path starts with our base

            // case sensitive
            ret = Ogre::StringUtil::startsWith(full_path.native(), mName, false);
        }

        return ret;
    }
    //---------------------------------------------------------------------
    auto FileSystemArchive::getModifiedTime(std::string_view filename) const -> std::filesystem::file_time_type
    {
        auto const full_path = concatenate_path(mName, filename);

        std::error_code ec{};
        auto const lastWriteTime = std::filesystem::last_write_time(full_path, ec);
        if (ec == std::error_code{})
        {
            return lastWriteTime;
        }
        else
        {
            return {};
        }

    }
    //-----------------------------------------------------------------------
    auto FileSystemArchiveFactory::getType() const noexcept -> std::string_view
    {
        static std::string_view const constinit name = "FileSystem";
        return name;
    }

    auto FileSystemArchiveFactory::createInstance( std::string_view name, bool readOnly ) -> Archive *
    {
        return new FileSystemArchive(name, getType(), readOnly);
    }

    void FileSystemArchiveFactory::setIgnoreHidden(bool ignore)
    {
        gIgnoreHidden = ignore;
    }

    auto FileSystemArchiveFactory::getIgnoreHidden() noexcept -> bool
    {
        return gIgnoreHidden;
    }
}
