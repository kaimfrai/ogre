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
#include <cstring>

struct mz_stream_s;

export module Ogre.Core:Deflate;

export import :DataStream;
export import :Prerequisites;
export import :SharedPtr;

export
namespace Ogre
{
    /** Template version of cache based on static array.
     'cacheSize' defines size of cache in bytes. */
    template <size_t cacheSize>
    class StaticCache
    {
    private:
        /// Static buffer
        char mBuffer[cacheSize];

        /// Number of bytes valid in cache (written from the beginning of static buffer)
        size_t mValidBytes;
        /// Current read position
        size_t mPos;
    public:
        /// Constructor
        StaticCache()
        {
            mValidBytes = 0;
            mPos = 0;
            memset(mBuffer, 0, cacheSize);
        }

        /** Cache data pointed by 'buf'. If 'count' is greater than cache size, we cache only last bytes.
         Returns number of bytes written to cache. */
        auto cacheData(const void* buf, size_t count) -> size_t
        {
            assert(avail() == 0 && "It is assumed that you cache data only after you have read everything.");

            if (count < cacheSize)
            {
                // number of bytes written is less than total size of cache
                if (count + mValidBytes <= cacheSize)
                {
                    // just append
                    memcpy(mBuffer + mValidBytes, buf, count);
                    mValidBytes += count;
                }
                else
                {
                    size_t begOff = count - (cacheSize - mValidBytes);
                    // override old cache content in the beginning
                    memmove(mBuffer, mBuffer + begOff, mValidBytes - begOff);
                    // append new data
                    memcpy(mBuffer + cacheSize - count, buf, count);
                    mValidBytes = cacheSize;
                }
                mPos = mValidBytes;
                return count;
            }
            else
            {
                // discard all
                memcpy(mBuffer, (const char*)buf + count - cacheSize, cacheSize);
                mValidBytes = mPos = cacheSize;
                return cacheSize;
            }
        }
        /** Read data from cache to 'buf' (maximum 'count' bytes). Returns number of bytes read from cache. */
        auto read(void* buf, size_t count) -> size_t
        {
            size_t rb = avail();
            rb = (rb < count) ? rb : count;
            memcpy(buf, mBuffer + mPos, rb);
            mPos += rb;
            return rb;
        }

        /** Step back in cached stream by 'count' bytes. Returns 'true' if cache contains resulting position. */
        auto rewind(size_t count) -> bool
        {
            if (mPos < count)
            {
                clear();
                return false;
            }
            else
            {
                mPos -= count;
                return true;
            }
        }
        /** Step forward in cached stream by 'count' bytes. Returns 'true' if cache contains resulting position. */
        auto ff(size_t count) -> bool
        {
            if (avail() < count)
            {
                clear();
                return false;
            }
            else
            {
                mPos += count;
                return true;
            }
        }

        /** Returns number of bytes available for reading in cache after rewinding. */
        [[nodiscard]] auto avail() const -> size_t
        {
            return mValidBytes - mPos;
        }

        /** Clear the cache */
        void clear()
        {
            mValidBytes = 0;
            mPos = 0;
        }
    };

    /** Stream which compresses / uncompresses data using the 'deflate' compression
        algorithm.
    @remarks
        This stream is designed to wrap another stream for the actual source / destination
        of the compressed data, it has no concrete source / data itself. The idea is
        that you pass uncompressed data through this stream, and the underlying
        stream reads/writes compressed data to the final source.
    @note
        This is an alternative to using a compressed archive since it is able to 
        compress & decompress regardless of the actual source of the stream.
        You should avoid using this with already compressed archives.
        Also note that this cannot be used as a read / write stream, only a read-only
        or write-only stream.
    */
    class DeflateStream : public DataStream
    {
    public:
        /** Requested stream type. All are essentially the same deflate stream with varying wrapping.
            ZLib is used by default.
        */
        enum class StreamType
        {
            Invalid = -1, /// Unexpected stream type or uncompressed data
            Deflate = 0,  /// no header, no checksum, rfc1951
            ZLib = 1,     /// 2 byte header, 4 byte footer with adler32 checksum, rfc1950
            GZip = 2,     /// 10 byte header, 8 byte footer with crc32 checksum and unpacked size, rfc1952
        };

        using enum StreamType;
    private:
        DataStreamPtr mCompressedStream;
        DataStreamPtr mTmpWriteStream;
        String mTempFileName;
        mz_stream_s* mZStream;
        int mStatus;
        size_t mCurrentPos;
        size_t mAvailIn;
        
        /// Cache for read data in case skipping around
        StaticCache<16 * OGRE_STREAM_TEMP_SIZE> mReadCache;
        
        /// Intermediate buffer for read / write
        unsigned char *mTmp;
        
        /// Whether the underlying stream is valid compressed data
        StreamType mStreamType;
        
        void init();
        void destroy();
        void compressFinal();

        auto getAvailInForSinglePass() -> size_t;
    public:
        /** Constructor for creating unnamed stream wrapping another stream.
         @param compressedStream The stream that this stream will use when reading / 
            writing compressed data. The access mode from this stream will be matched.
         @param tmpFileName Path/Filename to be used for temporary storage of incoming data
         @param avail_in Available data length to be uncompressed. With it we can uncompress
            DataStream partly.
        */
        DeflateStream(const DataStreamPtr& compressedStream, std::string_view tmpFileName = "",
            size_t avail_in = 0);
        /** Constructor for creating named stream wrapping another stream.
         @param name The name to give this stream
         @param compressedStream The stream that this stream will use when reading / 
            writing compressed data. The access mode from this stream will be matched.
         @param tmpFileName Path/Filename to be used for temporary storage of incoming data
         @param avail_in Available data length to be uncompressed. With it we can uncompress
            DataStream partly.
         */
        DeflateStream(std::string_view name, const DataStreamPtr& compressedStream, std::string_view tmpFileName="",
            size_t avail_in = 0);
        /** Constructor for creating named stream wrapping another stream.
         @param name The name to give this stream
         @param compressedStream The stream that this stream will use when reading / 
            writing compressed data. The access mode from this stream will be matched.
         @param streamType The type of compressed stream
         @param tmpFileName Path/Filename to be used for temporary storage of incoming data
         @param avail_in Available data length to be uncompressed. With it we can uncompress
            DataStream partly.
         */
        DeflateStream(std::string_view name, const DataStreamPtr& compressedStream, StreamType streamType, std::string_view tmpFileName="",
            size_t avail_in = 0);
        
        ~DeflateStream() override;
        
        /** Returns whether the compressed stream is valid deflated data.
         @remarks
            If you pass this class a READ stream which is not compressed with the 
            deflate algorithm, this method returns false and all read commands
            will actually be executed as passthroughs as a fallback. 
        */
        [[nodiscard]] auto isCompressedStreamValid() const noexcept -> bool { return mStreamType != Invalid; }
        
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
}
