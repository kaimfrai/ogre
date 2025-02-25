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

#include <ctime>

export module Ogre.Core:Archive;

export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :SharedPtr;
export import :StringVector;

export import <filesystem>;
export import <utility>;
export import <vector>;

export
namespace Ogre {
class Archive;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    /** Information about a file/directory within the archive will be
    returned using a FileInfo struct.
    @see
    Archive
    */
    struct FileInfo {
        /// The archive in which the file has been found (for info when performing
        /// multi-Archive searches, note you should still open through ResourceGroupManager)
        const Archive* archive;
        /// The file's fully qualified name
        std::string filename;
        /// Path name; separated by '/' and ending with '/'
        std::string path;
        /// Base filename
        std::string basename;
        /// Compressed size
        size_t compressedSize;
        /// Uncompressed size
        size_t uncompressedSize;
    };

    using FileInfoList = std::vector<FileInfo>;
    using FileInfoListPtr = SharedPtr<FileInfoList>;

    /** Archive-handling class.
    @remarks
        An archive is a generic term for a container of files. This may be a
        filesystem folder, it may be a compressed archive, it may even be 
        a remote location shared on the web. This class is designed to be 
        subclassed to provide access to a range of file locations. 
    @par
        Instances of this class are never constructed or even handled by end-user
        applications. They are constructed by custom ArchiveFactory classes, 
        which plugins can register new instances of using ArchiveManager. 
        End-user applications will typically use ResourceManager or 
        ResourceGroupManager to manage resources at a higher level, rather than 
        reading files directly through this class. Doing it this way allows you
        to benefit from OGRE's automatic searching of multiple file locations 
        for the resources you are looking for.
    */
    class Archive : public ArchiveAlloc
    {
    protected:
        /// Archive name
        String mName; 
        /// Archive type code
        String mType;
        /// Read-only flag
        bool mReadOnly{true};
    public:


        /** Constructor - don't call direct, used by ArchiveFactory.
        */
        Archive( std::string_view name, std::string_view archType )
            : mName(name), mType(archType) {}

        /** Default destructor.
        */
        virtual ~Archive() = default;

        /// Get the name of this archive
        [[nodiscard]] auto getName() const noexcept -> std::string_view { return mName; }

        /// Returns whether this archive is case sensitive in the way it matches files
        [[nodiscard]] virtual auto isCaseSensitive() const noexcept -> bool = 0;

        /** Loads the archive.
        @remarks
            This initializes all the internal data of the class.
        @warning
            Do not call this function directly, it is meant to be used
            only by the ArchiveManager class.
        */
        virtual void load() = 0;

        /** Unloads the archive.
        @warning
            Do not call this function directly, it is meant to be used
            only by the ArchiveManager class.
        */
        virtual void unload() = 0;

        /** Reports whether this Archive is read-only, or whether the contents
            can be updated. 
        */
        [[nodiscard]] virtual auto isReadOnly() const noexcept -> bool { return mReadOnly; }

        /** Open a stream on a given file. 
        @note
            There is no equivalent 'close' method; the returned stream
            controls the lifecycle of this file operation.
        @param filename The fully qualified name of the file
        @param readOnly Whether to open the file in read-only mode or not (note, 
            if the archive is read-only then this cannot be set to false)
        @return A shared pointer to a DataStream which can be used to 
            read / write the file. If the file is not present, returns a null
            shared pointer.
        */
        [[nodiscard]] virtual auto open(std::string_view filename, bool readOnly = true) const -> DataStreamPtr = 0;

        /** Create a new file (or overwrite one already there). 
        @note If the archive is read-only then this method will fail.
        @param filename The fully qualified name of the file
        @return A shared pointer to a DataStream which can be used to 
        read / write the file. 
        */
        virtual auto create(std::string_view filename) -> DataStreamPtr;

        /** Delete a named file.
        @remarks Not possible on read-only archives
        @param filename The fully qualified name of the file
        */
        virtual void remove(std::string_view filename);

        /** List all file names in the archive.
        @note
            This method only returns filenames, you can also retrieve other
            information using listFileInfo.
        @param recursive Whether all paths of the archive are searched (if the 
            archive has a concept of that)
        @param dirs Set to true if you want the directories to be listed
            instead of files
        @return A list of filenames matching the criteria, all are fully qualified
        */
        [[nodiscard]] virtual auto list(bool recursive = true, bool dirs = false) const -> StringVectorPtr = 0;
        
        /** List all files in the archive with accompanying information.
        @param recursive Whether all paths of the archive are searched (if the 
            archive has a concept of that)
        @param dirs Set to true if you want the directories to be listed
            instead of files
        @return A list of structures detailing quite a lot of information about
            all the files in the archive.
        */
        [[nodiscard]] virtual auto listFileInfo(bool recursive = true, bool dirs = false) const -> FileInfoListPtr = 0;

        /** Find all file or directory names matching a given pattern
            in this archive.
        @note
            This method only returns filenames, you can also retrieve other
            information using findFileInfo.
        @param pattern The pattern to search for; wildcards (*) are allowed
        @param recursive Whether all paths of the archive are searched (if the 
            archive has a concept of that)
        @param dirs Set to true if you want the directories to be listed
            instead of files
        @return A list of filenames matching the criteria, all are fully qualified
        */
        [[nodiscard]] virtual auto find(std::string_view pattern, bool recursive = true,
            bool dirs = false) const -> StringVectorPtr = 0;

        /** Find out if the named file exists (note: fully qualified filename required) */
        [[nodiscard]] virtual auto exists(std::string_view filename) const -> bool = 0;

        /** Retrieve the modification time of a given file */
        [[nodiscard]] virtual auto getModifiedTime(std::string_view filename) const -> std::filesystem::file_time_type = 0;


        /** Find all files or directories matching a given pattern in this
            archive and get some detailed information about them.
        @param pattern The pattern to search for; wildcards (*) are allowed
        @param recursive Whether all paths of the archive are searched (if the 
        archive has a concept of that)
        @param dirs Set to true if you want the directories to be listed
            instead of files
        @return A list of file information structures for all files matching 
            the criteria.
        */
        [[nodiscard]] virtual auto findFileInfo(std::string_view pattern, 
            bool recursive = true, bool dirs = false) const -> FileInfoListPtr = 0;

        /// Return the type code of this Archive
        [[nodiscard]] auto getType() const noexcept -> std::string_view { return mType; }
        
    };
    /** @} */
    /** @} */

}
