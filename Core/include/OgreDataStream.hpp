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
#ifndef OGRE_CORE_DATASTREAM_H
#define OGRE_CORE_DATASTREAM_H

#include <cstdio>
#include <istream>
#include <list>
#include <utility>

#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSharedPtr.hpp"

namespace Ogre {
    
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    auto constexpr inline OGRE_STREAM_TEMP_SIZE = 128;

    /** General purpose class used for encapsulating the reading and writing of data.
    @remarks
        This class performs basically the same tasks as std::basic_istream, 
        except that it does not have any formatting capabilities, and is
        designed to be subclassed to receive data from multiple sources,
        including libraries which have no compatibility with the STL's
        stream interfaces. As such, this is an abstraction of a set of 
        wrapper classes which pretend to be standard stream classes but 
        can actually be implemented quite differently. 
    @par
        Generally, if a plugin or application provides an ArchiveFactory, 
        it should also provide a DataStream subclass which will be used
        to stream data out of that Archive implementation, unless it can 
        use one of the common implementations included.
    @note
        Ogre makes no guarantees about thread safety, for performance reasons.
        If you wish to access stream data asynchronously then you should
        organise your own mutexes to avoid race conditions. 
    */
    class DataStream : public StreamAlloc
    {
    public:
        enum AccessMode
        {
            READ = 1, 
            WRITE = 2
        };
    protected:
        /// The name (e.g. resource name) that can be used to identify the source for this data (optional)
        String mName;       
        /// Size of the data in the stream (may be 0 if size cannot be determined)
        size_t mSize;
        /// What type of access is allowed (AccessMode)
        uint16 mAccess;

    public:
        /// Constructor for creating unnamed streams
        DataStream(uint16 accessMode = READ) : mSize(0), mAccess(accessMode) {}
        /// Constructor for creating named streams
        DataStream(std::string_view name, uint16 accessMode = READ) 
            : mName(name), mSize(0), mAccess(accessMode) {}
        /// Returns the name of the stream, if it has one.
        auto getName() noexcept -> std::string_view { return mName; }
        /// Gets the access mode of the stream
        [[nodiscard]] auto getAccessMode() const noexcept -> uint16 { return mAccess; }
        /** Reports whether this stream is readable. */
        [[nodiscard]] virtual auto isReadable() const noexcept -> bool { return (mAccess & READ) != 0; }
        /** Reports whether this stream is writeable. */
        [[nodiscard]] virtual auto isWriteable() const noexcept -> bool { return (mAccess & WRITE) != 0; }
        virtual ~DataStream() = default;
        // Streaming operators
        template<typename T> auto operator>>(T& val) -> DataStream&;
        /** Read the requisite number of bytes from the stream, 
            stopping at the end of the file.
        @param buf Reference to a buffer pointer
        @param count Number of bytes to read
        @return The number of bytes read
        */
        virtual auto read(void* buf, size_t count) -> size_t = 0;
        /** Write the requisite number of bytes from the stream (only applicable to 
            streams that are not read-only)
        @param buf Pointer to a buffer containing the bytes to write
        @param count Number of bytes to write
        @return The number of bytes written
        */
        virtual auto write(const void* buf, size_t count) -> size_t
        {
                        (void)buf;
                        (void)count;
            // default to not supported
            return 0;
        }

        /** Get a single line from the stream.
        @remarks
            The delimiter character is not included in the data
            returned, and it is skipped over so the next read will occur
            after it. The buffer contents will include a
            terminating character.
        @note
            If you used this function, you <b>must</b> open the stream in <b>binary mode</b>,
            otherwise, it'll produce unexpected results.
        @param buf Reference to a buffer pointer
        @param maxCount The maximum length of data to be read, excluding the terminating character
        @param delim The delimiter to stop at
        @return The number of bytes read, excluding the terminating character
        */
        virtual auto readLine(char* buf, size_t maxCount, std::string_view delim = "\n") -> size_t;
        
        /** Returns a String containing the next line of data, optionally 
            trimmed for whitespace. 
        @remarks
            This is a convenience method for text streams only, allowing you to 
            retrieve a String object containing the next line of data. The data
            is read up to the next newline character and the result trimmed if
            required.
        @note
            If you used this function, you <b>must</b> open the stream in <b>binary mode</b>,
            otherwise, it'll produce unexpected results.
        @param 
            trimAfter If true, the line is trimmed for whitespace (as in 
            String.trim(true,true))
        */
        virtual auto getLine( bool trimAfter = true ) -> String;

        /** Returns a String containing the entire stream. 
        @remarks
            This is a convenience method for text streams only, allowing you to 
            retrieve a String object containing all the data in the stream.
        */
        virtual auto getAsString() -> String;

        /** Skip a single line from the stream.
        @note
            If you used this function, you <b>must</b> open the stream in <b>binary mode</b>,
            otherwise, it'll produce unexpected results.
        @par
            delim The delimiter(s) to stop at
        @return The number of bytes skipped
        */
        virtual auto skipLine(std::string_view delim = "\n") -> size_t;

        /** Skip a defined number of bytes. This can also be a negative value, in which case
        the file pointer rewinds a defined number of bytes. */
        virtual void skip(long count) = 0;
    
        /** Repositions the read point to a specified byte.
        */
        virtual void seek( size_t pos ) = 0;
        
        /** Returns the current byte offset from beginning */
        [[nodiscard]] virtual auto tell() const -> size_t = 0;

        /** Returns true if the stream has reached the end.
        */
        [[nodiscard]] virtual auto eof() const -> bool = 0;

        /** Returns the total size of the data to be read from the stream, 
            or 0 if this is indeterminate for this stream. 
        */
        [[nodiscard]] auto size() const noexcept -> size_t { return mSize; }

        /** Close the stream; this makes further operations invalid. */
        virtual void close() = 0;
        

    };

    /// List of DataStream items
    using DataStreamList = std::list<DataStreamPtr>;

    /** Common subclass of DataStream for handling data from chunks of memory.
    */
    class MemoryDataStream : public DataStream
    {
    private:
        /// Pointer to the start of the data area
        uchar* mData;
        /// Pointer to the current position in the memory
        uchar* mPos;
        /// Pointer to the end of the memory
        uchar* mEnd;
        /// Do we delete the memory on close
        bool mFreeOnClose;          
    public:
        
        /** Wrap an existing memory chunk in a stream.
        @param pMem Pointer to the existing memory
        @param size The size of the memory chunk in bytes
        @param freeOnClose If true, the memory associated will be destroyed
            when the stream is closed.
        @param readOnly Whether to make the stream on this memory read-only once created
        */
        MemoryDataStream(void* pMem, size_t size, bool freeOnClose = false, bool readOnly = false);
        
        /** Wrap an existing memory chunk in a named stream.
        @param name The name to give the stream
        @param pMem Pointer to the existing memory
        @param size The size of the memory chunk in bytes
        @param freeOnClose If true, the memory associated will be destroyed
            when the stream is destroyed.
        @param readOnly Whether to make the stream on this memory read-only once created
        */
        MemoryDataStream(std::string_view name, void* pMem, size_t size, 
                bool freeOnClose = false, bool readOnly = false);

        /** Create a stream which pre-buffers the contents of another stream.
        @remarks
            This constructor can be used to intentionally read in the entire
            contents of another stream, copying them to the internal buffer
            and thus making them available in memory as a single unit.
        @param sourceStream Another DataStream which will provide the source
            of data
        @param freeOnClose If true, the memory associated will be destroyed
            when the stream is destroyed.
        @param readOnly Whether to make the stream on this memory read-only once created
        */
        MemoryDataStream(DataStream& sourceStream, 
                bool freeOnClose = true, bool readOnly = false);
        
        /** Create a stream which pre-buffers the contents of another stream.
        @remarks
            This constructor can be used to intentionally read in the entire
            contents of another stream, copying them to the internal buffer
            and thus making them available in memory as a single unit.
        @param sourceStream Another DataStream which will provide the source
            of data
        @param freeOnClose If true, the memory associated will be destroyed
            when the stream is destroyed.
        @param readOnly Whether to make the stream on this memory read-only once created
        */
        MemoryDataStream(const DataStreamPtr& sourceStream,
                bool freeOnClose = true, bool readOnly = false);

        /** Create a named stream which pre-buffers the contents of 
            another stream.
        @remarks
            This constructor can be used to intentionally read in the entire
            contents of another stream, copying them to the internal buffer
            and thus making them available in memory as a single unit.
        @param name The name to give the stream
        @param sourceStream Another DataStream which will provide the source
            of data
        @param freeOnClose If true, the memory associated will be destroyed
            when the stream is destroyed.
        @param readOnly Whether to make the stream on this memory read-only once created
        */
        MemoryDataStream(std::string_view name, DataStream& sourceStream, 
                bool freeOnClose = true, bool readOnly = false);

        /** Create a named stream which pre-buffers the contents of 
        another stream.
        @remarks
        This constructor can be used to intentionally read in the entire
        contents of another stream, copying them to the internal buffer
        and thus making them available in memory as a single unit.
        @param name The name to give the stream
        @param sourceStream Another DataStream which will provide the source
        of data
        @param freeOnClose If true, the memory associated will be destroyed
        when the stream is destroyed.
        @param readOnly Whether to make the stream on this memory read-only once created
        */
        MemoryDataStream(std::string_view name, const DataStreamPtr& sourceStream, 
            bool freeOnClose = true, bool readOnly = false);

        /** Create a stream with a brand new empty memory chunk.
        @param size The size of the memory chunk to create in bytes
        @param freeOnClose If true, the memory associated will be destroyed
            when the stream is destroyed.
        @param readOnly Whether to make the stream on this memory read-only once created
        */
        MemoryDataStream(size_t size, bool freeOnClose = true, bool readOnly = false);
        /** Create a named stream with a brand new empty memory chunk.
        @param name The name to give the stream
        @param size The size of the memory chunk to create in bytes
        @param freeOnClose If true, the memory associated will be destroyed
            when the stream is destroyed.
        @param readOnly Whether to make the stream on this memory read-only once created
        */
        MemoryDataStream(std::string_view name, size_t size, 
                bool freeOnClose = true, bool readOnly = false);

        ~MemoryDataStream() override;

        /** Get a pointer to the start of the memory block this stream holds. */
        auto getPtr() noexcept -> uchar* { return mData; }
        
        /** Get a pointer to the current position in the memory block this stream holds. */
        auto getCurrentPtr() noexcept -> uchar* { return mPos; }
        
        /** @copydoc DataStream::read
        */
        auto read(void* buf, size_t count) -> size_t override;

        /** @copydoc DataStream::write
        */
        auto write(const void* buf, size_t count) -> size_t override;

        /** @copydoc DataStream::readLine
        */
        auto readLine(char* buf, size_t maxCount, std::string_view delim = "\n") -> size_t override;
        
        /** @copydoc DataStream::skipLine
        */
        auto skipLine(std::string_view delim = "\n") -> size_t override;

        /** @copydoc DataStream::skip
        */
        void skip(long count) override;
    
        /** @copydoc DataStream::seek
        */
        void seek( size_t pos ) override;
        
        /** @copydoc DataStream::tell
        */
        [[nodiscard]] auto tell() const -> size_t override;

        /** @copydoc DataStream::eof
        */
        [[nodiscard]] auto eof() const -> bool override;

        /** @copydoc DataStream::close
        */
        void close() override;

        /** Sets whether or not to free the encapsulated memory on close. */
        void setFreeOnClose(bool free) { mFreeOnClose = free; }
    };

    /** Common subclass of DataStream for handling data from 
        std::basic_istream.
    */
    class FileStreamDataStream : public DataStream
    {
    private:
        /// Reference to source stream (read)
        std::istream* mInStream;
        /// Reference to source file stream (read-only)
        std::ifstream* mFStreamRO;
        /// Reference to source file stream (read-write)
        std::fstream* mFStream;
        bool mFreeOnClose;  

        void determineAccess();
    public:
        /** Construct a read-only stream from an STL stream
        @param s Pointer to source stream
        @param freeOnClose Whether to delete the underlying stream on 
            destruction of this class
        */
        FileStreamDataStream(std::ifstream* s, 
            bool freeOnClose = true);
        /** Construct a read-write stream from an STL stream
        @param s Pointer to source stream
        @param freeOnClose Whether to delete the underlying stream on 
        destruction of this class
        */
        FileStreamDataStream(std::fstream* s, 
            bool freeOnClose = true);

        /** Construct named read-only stream from an STL stream
        @param name The name to give this stream
        @param s Pointer to source stream
        @param freeOnClose Whether to delete the underlying stream on 
            destruction of this class
        */
        FileStreamDataStream(std::string_view name, 
            std::ifstream* s, 
            bool freeOnClose = true);

        /** Construct named read-write stream from an STL stream
        @param name The name to give this stream
        @param s Pointer to source stream
        @param freeOnClose Whether to delete the underlying stream on 
        destruction of this class
        */
        FileStreamDataStream(std::string_view name, 
            std::fstream* s, 
            bool freeOnClose = true);

        /** Construct named read-only stream from an STL stream, and tell it the size
        @remarks
            This variant tells the class the size of the stream too, which 
            means this class does not need to seek to the end of the stream 
            to determine the size up-front. This can be beneficial if you have
            metadata about the contents of the stream already.
        @param name The name to give this stream
        @param s Pointer to source stream
        @param size Size of the stream contents in bytes
        @param freeOnClose Whether to delete the underlying stream on 
            destruction of this class.
        */
        FileStreamDataStream(std::string_view name, 
            std::ifstream* s, 
            size_t size, 
            bool freeOnClose = true);

        /** Construct named read-write stream from an STL stream, and tell it the size
        @remarks
        This variant tells the class the size of the stream too, which 
        means this class does not need to seek to the end of the stream 
        to determine the size up-front. This can be beneficial if you have
        metadata about the contents of the stream already.
        @param name The name to give this stream
        @param s Pointer to source stream
        @param size Size of the stream contents in bytes
        @param freeOnClose Whether to delete the underlying stream on 
        destruction of this class.
        */
        FileStreamDataStream(std::string_view name, 
            std::fstream* s, 
            size_t size, 
            bool freeOnClose = true);

        ~FileStreamDataStream() override;

        /** @copydoc DataStream::read
        */
        auto read(void* buf, size_t count) -> size_t override;

        /** @copydoc DataStream::write
        */
        auto write(const void* buf, size_t count) -> size_t override;

        /** @copydoc DataStream::readLine
        */
        auto readLine(char* buf, size_t maxCount, std::string_view delim = "\n") -> size_t override;
        
        /** @copydoc DataStream::skip
        */
        void skip(long count) override;
    
        /** @copydoc DataStream::seek
        */
        void seek( size_t pos ) override;

        /** @copydoc DataStream::tell
        */
        [[nodiscard]] auto tell() const -> size_t override;

        /** @copydoc DataStream::eof
        */
        [[nodiscard]] auto eof() const -> bool override;

        /** @copydoc DataStream::close
        */
        void close() override;
        
        
    };

    /** Common subclass of DataStream for handling data from C-style file 
        handles.
    @remarks
        Use of this class is generally discouraged; if you want to wrap file
        access in a DataStream, you should definitely be using the C++ friendly
        FileStreamDataStream. However, since there are quite a few applications
        and libraries still wedded to the old FILE handle access, this stream
        wrapper provides some backwards compatibility.
    */
    class FileHandleDataStream : public DataStream
    {
    private:
        FILE* mFileHandle;
    public:
        /// Create stream from a C file handle
        FileHandleDataStream(FILE* handle, uint16 accessMode = READ);
        /// Create named stream from a C file handle
        FileHandleDataStream(std::string_view name, FILE* handle, uint16 accessMode = READ);
        ~FileHandleDataStream() override;

        /** @copydoc DataStream::read
        */
        auto read(void* buf, size_t count) -> size_t override;

        /** @copydoc DataStream::write
        */
        auto write(const void* buf, size_t count) -> size_t override;

        /** @copydoc DataStream::skip
        */
        void skip(long count) override;
    
        /** @copydoc DataStream::seek
        */
        void seek( size_t pos ) override;

        /** @copydoc DataStream::tell
        */
        [[nodiscard]] auto tell() const -> size_t override;

        /** @copydoc DataStream::eof
        */
        [[nodiscard]] auto eof() const -> bool override;

        /** @copydoc DataStream::close
        */
        void close() override;

    };
    /** @} */
    /** @} */
}

#endif

