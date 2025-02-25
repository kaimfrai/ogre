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
#include <cstdio>
#include <cstring>

module Ogre.Core;

import :DataStream;
import :Exception;
import :LogManager;
import :Platform;
import :Prerequisites;
import :SharedPtr;
import :String;

import <algorithm>;
import <fstream>;
import <string>;

namespace Ogre {

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    template <typename T> auto DataStream::operator >>(T& val) -> DataStream&
    {
        read(static_cast<void*>(&val), sizeof(T));
        return *this;
    }
    //-----------------------------------------------------------------------
    auto DataStream::getLine(bool trimAfter) -> String
    {
        char tmpBuf[OGRE_STREAM_TEMP_SIZE];
        String retString;
        size_t readCount;
        // Keep looping while not hitting delimiter
        while ((readCount = read(tmpBuf, OGRE_STREAM_TEMP_SIZE-1)) != 0)
        {
            // Terminate string
            tmpBuf[readCount] = '\0';

            char* p = strchr(tmpBuf, '\n');
            if (p != nullptr)
            {
                // Reposition backwards
                skip((long)(p + 1 - tmpBuf - readCount));
                *p = '\0';
            }

            retString += tmpBuf;

            if (p != nullptr)
            {
                // Trim off trailing CR if this was a CR/LF entry
                if (retString.length() && retString[retString.length()-1] == '\r')
                {
                    retString.erase(retString.length()-1, 1);
                }

                // Found terminator, break out
                break;
            }
        }

        if (trimAfter)
        {
            StringUtil::trim(retString);
        }

        return retString;
    }
    //-----------------------------------------------------------------------
    auto DataStream::readLine(char* buf, size_t maxCount, std::string_view delim) -> size_t
    {
        // Deal with both Unix & Windows LFs
        bool trimCR = false;
        if (delim.find_first_of('\n') != String::npos)
        {
            trimCR = true;
        }

        char tmpBuf[OGRE_STREAM_TEMP_SIZE];
        size_t chunkSize = std::min(maxCount, (size_t)OGRE_STREAM_TEMP_SIZE-1);
        size_t totalCount = 0;
        size_t readCount; 
        while (chunkSize && (readCount = read(tmpBuf, chunkSize)) != 0)
        {
            // Terminate
            tmpBuf[readCount] = '\0';

            // Find first delimiter
            size_t pos = strcspn(tmpBuf, delim.data());

            if (pos < readCount)
            {
                // Found terminator, reposition backwards
                skip((long)(pos + 1 - readCount));
            }

            // Are we genuinely copying?
            if (buf)
            {
                memcpy(buf+totalCount, tmpBuf, pos);
            }
            totalCount += pos;

            if (pos < readCount)
            {
                // Trim off trailing CR if this was a CR/LF entry
                if (trimCR && totalCount && buf && buf[totalCount-1] == '\r')
                {
                    --totalCount;
                }

                // Found terminator, break out
                break;
            }

            // Adjust chunkSize for next time
            chunkSize = std::min(maxCount-totalCount, (size_t)OGRE_STREAM_TEMP_SIZE-1);
        }

        // Terminate
        if(buf)
            buf[totalCount] = '\0';

        return totalCount;
    }
    //-----------------------------------------------------------------------
    auto DataStream::skipLine(std::string_view delim) -> size_t
    {
        char tmpBuf[OGRE_STREAM_TEMP_SIZE];
        size_t total = 0;
        size_t readCount;
        // Keep looping while not hitting delimiter
        while ((readCount = read(tmpBuf, OGRE_STREAM_TEMP_SIZE-1)) != 0)
        {
            // Terminate string
            tmpBuf[readCount] = '\0';

            // Find first delimiter
            size_t pos = strcspn(tmpBuf, delim.data());

            if (pos < readCount)
            {
                // Found terminator, reposition backwards
                skip((long)(pos + 1 - readCount));

                total += pos + 1;

                // break out
                break;
            }

            total += readCount;
        }

        return total;
    }
    //-----------------------------------------------------------------------
    auto DataStream::getAsString() -> String
    {
        // Read the entire buffer - ideally in one read, but if the size of
        // the buffer is unknown, do multiple fixed size reads.
        size_t bufSize = (mSize > 0 ? mSize : 4096);
        char* pBuf = new char[bufSize];
        // Ensure read from begin of stream
        seek(0);
        String result;
        while (!eof())
        {
            size_t nr = read(pBuf, bufSize);
            result.append(pBuf, nr);
        }
        delete[] pBuf;
        return result;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    MemoryDataStream::MemoryDataStream(void* pMem, size_t inSize, bool freeOnClose, bool readOnly)
        : DataStream(static_cast<uint16>(readOnly ? READ : (READ | WRITE)))
    {
        mData = mPos = static_cast<uchar*>(pMem);
        mSize = inSize;
        mEnd = mData + mSize;
        mFreeOnClose = freeOnClose;
        assert(mEnd >= mPos);
    }
    //-----------------------------------------------------------------------
    MemoryDataStream::MemoryDataStream(std::string_view name, void* pMem, size_t inSize,
        bool freeOnClose, bool readOnly)
        : DataStream(name, static_cast<uint16>(readOnly ? READ : (READ | WRITE)))
    {
        mData = mPos = static_cast<uchar*>(pMem);
        mSize = inSize;
        mEnd = mData + mSize;
        mFreeOnClose = freeOnClose;
        assert(mEnd >= mPos);
    }
    //-----------------------------------------------------------------------
    MemoryDataStream::MemoryDataStream(DataStream& sourceStream, 
        bool freeOnClose, bool readOnly)
        : DataStream(static_cast<uint16>(readOnly ? READ : (READ | WRITE)))
    {
        // Copy data from incoming stream
        mSize = sourceStream.size();
        if (mSize == 0 && !sourceStream.eof())
        {
            // size of source is unknown, read all of it into memory
            String contents = sourceStream.getAsString();
            mSize = contents.size();
            mData = new  uchar[mSize];
            mPos = mData;
            memcpy(mData, contents.data(), mSize);
            mEnd = mData + mSize;
        }
        else
        {
            mData = new uchar[mSize];
            mPos = mData;
            mEnd = mData + sourceStream.read(mData, mSize);
        }
        mFreeOnClose = freeOnClose;
        assert(mEnd >= mPos);
    }
    //-----------------------------------------------------------------------
    MemoryDataStream::MemoryDataStream(const DataStreamPtr& sourceStream,
        bool freeOnClose, bool readOnly)
        : DataStream(static_cast<uint16>(readOnly ? READ : (READ | WRITE)))
    {
        // Copy data from incoming stream
        mSize = sourceStream->size();
        if (mSize == 0 && !sourceStream->eof())
        {
            // size of source is unknown, read all of it into memory
            String contents = sourceStream->getAsString();
            mSize = contents.size();
            mData = static_cast<uchar*>(malloc(mSize));
            mPos = mData;
            memcpy(mData, contents.data(), mSize);
            mEnd = mData + mSize;
        }
        else
        {
            mData = static_cast<uchar*>(malloc(mSize));
            mPos = mData;
            mEnd = mData + sourceStream->read(mData, mSize);
        }
        mFreeOnClose = freeOnClose;
        assert(mEnd >= mPos);
    }
    //-----------------------------------------------------------------------
    MemoryDataStream::MemoryDataStream(std::string_view name, DataStream& sourceStream,
        bool freeOnClose, bool readOnly)
        : DataStream(name, static_cast<uint16>(readOnly ? READ : (READ | WRITE)))
    {
        // Copy data from incoming stream
        mSize = sourceStream.size();
        if (mSize == 0 && !sourceStream.eof())
        {
            // size of source is unknown, read all of it into memory
            String contents = sourceStream.getAsString();
            mSize = contents.size();
            mData = new uchar[mSize];
            mPos = mData;
            memcpy(mData, contents.data(), mSize);
            mEnd = mData + mSize;
        }
        else
        {
            mData = new uchar[mSize];
            mPos = mData;
            mEnd = mData + sourceStream.read(mData, mSize);
        }
        mFreeOnClose = freeOnClose;
        assert(mEnd >= mPos);
    }
    //-----------------------------------------------------------------------
    MemoryDataStream::MemoryDataStream(std::string_view name, const DataStreamPtr& sourceStream,
        bool freeOnClose, bool readOnly)
        : DataStream(name, static_cast<uint16>(readOnly ? READ : (READ | WRITE)))
    {
        // Copy data from incoming stream
        mSize = sourceStream->size();
        if (mSize == 0 && !sourceStream->eof())
        {
            // size of source is unknown, read all of it into memory
            String contents = sourceStream->getAsString();
            mSize = contents.size();
            mData = new uchar[mSize];
            mPos = mData;
            memcpy(mData, contents.data(), mSize);
            mEnd = mData + mSize;
        }
        else
        {
            mData = static_cast<uchar*>(malloc(mSize));
            mPos = mData;
            mEnd = mData + sourceStream->read(mData, mSize);
        }
        mFreeOnClose = freeOnClose;
        assert(mEnd >= mPos);
    }
    //-----------------------------------------------------------------------
    MemoryDataStream::MemoryDataStream(size_t inSize, bool freeOnClose, bool readOnly)
        : DataStream(static_cast<uint16>(readOnly ? READ : (READ | WRITE)))
    {
        mSize = inSize;
        mData = static_cast<uchar*>(malloc(mSize));
        mPos = mData;
        mEnd = mData + mSize;
        mFreeOnClose = freeOnClose;
        assert(mEnd >= mPos);
    }
    //-----------------------------------------------------------------------
    MemoryDataStream::MemoryDataStream(std::string_view name, size_t inSize,
        bool freeOnClose, bool readOnly)
        : DataStream(name, static_cast<uint16>(readOnly ? READ : (READ | WRITE)))
    {
        mSize = inSize;
        mData = new uchar[mSize];
        mPos = mData;
        mEnd = mData + mSize;
        mFreeOnClose = freeOnClose;
        assert(mEnd >= mPos);
    }
    //-----------------------------------------------------------------------
    MemoryDataStream::~MemoryDataStream()
    {
        close();
    }
    //-----------------------------------------------------------------------
    auto MemoryDataStream::read(void* buf, size_t count) -> size_t
    {
        size_t cnt = count;
        // Read over end of memory?
        if (mPos + cnt > mEnd)
            cnt = mEnd - mPos;
        if (cnt == 0)
            return 0;

        assert (cnt<=count);

        memcpy(buf, mPos, cnt);
        mPos += cnt;
        return cnt;
    }
    //---------------------------------------------------------------------
    auto MemoryDataStream::write(const void* buf, size_t count) -> size_t
    {
        size_t written = 0;
        if (isWriteable())
        {
            written = count;
            // we only allow writing within the extents of allocated memory
            // check for buffer overrun & disallow
            if (mPos + written > mEnd)
                written = mEnd - mPos;
            if (written == 0)
                return 0;

            memcpy(mPos, buf, written);
            mPos += written;
        }
        return written;
    }
    //-----------------------------------------------------------------------
    auto MemoryDataStream::readLine(char* buf, size_t maxCount, 
        std::string_view delim) -> size_t
    {
        // Deal with both Unix & Windows LFs
        bool trimCR = false;
        if (delim.find_first_of('\n') != String::npos)
        {
            trimCR = true;
        }

        size_t pos = 0;

        // Make sure pos can never go past the end of the data 
        while (pos < maxCount && mPos < mEnd)
        {
            if (delim.find(*mPos) != String::npos)
            {
                // Trim off trailing CR if this was a CR/LF entry
                if (trimCR && pos && buf[pos-1] == '\r')
                {
                    // terminate 1 character early
                    --pos;
                }

                // Found terminator, skip and break out
                ++mPos;
                break;
            }

            buf[pos++] = *mPos++;
        }

        // terminate
        buf[pos] = '\0';

        return pos;
    }
    //-----------------------------------------------------------------------
    auto MemoryDataStream::skipLine(std::string_view delim) -> size_t
    {
        size_t pos = 0;

        // Make sure pos can never go past the end of the data 
        while (mPos < mEnd)
        {
            ++pos;
            if (delim.find(*mPos++) != String::npos)
            {
                // Found terminator, break out
                break;
            }
        }

        return pos;

    }
    //-----------------------------------------------------------------------
    void MemoryDataStream::skip(long count)
    {
        auto newpos = (size_t)( ( mPos - mData ) + count );
        assert( mData + newpos <= mEnd );        

        mPos = mData + newpos;
    }
    //-----------------------------------------------------------------------
    void MemoryDataStream::seek( size_t pos )
    {
        assert( mData + pos <= mEnd );
        mPos = mData + pos;
    }
    //-----------------------------------------------------------------------
    auto MemoryDataStream::tell() const -> size_t
    {
        //mData is start, mPos is current location
        return mPos - mData;
    }
    //-----------------------------------------------------------------------
    auto MemoryDataStream::eof() const -> bool
    {
        return mPos >= mEnd;
    }
    //-----------------------------------------------------------------------
    void MemoryDataStream::close()    
    {
        mAccess = 0;
        if (mFreeOnClose && mData)
        {
            free(mData);
            mData = nullptr;
        }
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    FileStreamDataStream::FileStreamDataStream(std::ifstream* s, bool freeOnClose)
        : DataStream(), mInStream(s), mFStreamRO(s), mFStream(nullptr), mFreeOnClose(freeOnClose)
    {
        // calculate the size
        mInStream->seekg(0, std::ios_base::end);
        mSize = (size_t)mInStream->tellg();
        mInStream->seekg(0, std::ios_base::beg);
        determineAccess();
    }
    //-----------------------------------------------------------------------
    FileStreamDataStream::FileStreamDataStream(std::string_view name,
        std::ifstream* s, bool freeOnClose)
        : DataStream(name), mInStream(s), mFStreamRO(s), mFStream(nullptr), mFreeOnClose(freeOnClose)
    {
        // calculate the size
        mInStream->seekg(0, std::ios_base::end);
        mSize = (size_t)mInStream->tellg();
        mInStream->seekg(0, std::ios_base::beg);
        determineAccess();
    }
    //-----------------------------------------------------------------------
    FileStreamDataStream::FileStreamDataStream(std::string_view name,
        std::ifstream* s, size_t inSize, bool freeOnClose)
        : DataStream(name), mInStream(s), mFStreamRO(s), mFStream(nullptr), mFreeOnClose(freeOnClose)
    {
        // Size is passed in
        mSize = inSize;
        determineAccess();
    }
    //---------------------------------------------------------------------
    FileStreamDataStream::FileStreamDataStream(std::fstream* s, bool freeOnClose)
        : DataStream(false), mInStream(s), mFStreamRO(nullptr), mFStream(s), mFreeOnClose(freeOnClose)
    {
        // writeable!
        // calculate the size
        mInStream->seekg(0, std::ios_base::end);
        mSize = (size_t)mInStream->tellg();
        mInStream->seekg(0, std::ios_base::beg);
        determineAccess();

    }
    //-----------------------------------------------------------------------
    FileStreamDataStream::FileStreamDataStream(std::string_view name,
        std::fstream* s, bool freeOnClose)
        : DataStream(name, false), mInStream(s), mFStreamRO(nullptr), mFStream(s), mFreeOnClose(freeOnClose)
    {
        // writeable!
        // calculate the size
        mInStream->seekg(0, std::ios_base::end);
        mSize = (size_t)mInStream->tellg();
        mInStream->seekg(0, std::ios_base::beg);
        determineAccess();
    }
    //-----------------------------------------------------------------------
    FileStreamDataStream::FileStreamDataStream(std::string_view name,
        std::fstream* s, size_t inSize, bool freeOnClose)
        : DataStream(name, false), mInStream(s), mFStreamRO(nullptr), mFStream(s), mFreeOnClose(freeOnClose)
    {
        // writeable!
        // Size is passed in
        mSize = inSize;
        determineAccess();
    }
    //---------------------------------------------------------------------
    void FileStreamDataStream::determineAccess()
    {
        mAccess = 0;
        if (mInStream)
            mAccess |= std::to_underlying(READ);
        if (mFStream)
            mAccess |= std::to_underlying(WRITE);
    }
    //-----------------------------------------------------------------------
    FileStreamDataStream::~FileStreamDataStream()
    {
        close();
    }
    //-----------------------------------------------------------------------
    auto FileStreamDataStream::read(void* buf, size_t count) -> size_t
    {
        mInStream->read(static_cast<char*>(buf), static_cast<std::streamsize>(count));
        return (size_t)mInStream->gcount();
    }
    //-----------------------------------------------------------------------
    auto FileStreamDataStream::write(const void* buf, size_t count) -> size_t
    {
        size_t written = 0;
        if (isWriteable() && mFStream)
        {
            mFStream->write(static_cast<const char*>(buf), static_cast<std::streamsize>(count));
            written = count;
        }
        return written;
    }
    //-----------------------------------------------------------------------
    auto FileStreamDataStream::readLine(char* buf, size_t maxCount, 
        std::string_view delim) -> size_t
    {
        if (delim.empty())
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "No delimiter provided",
                "FileStreamDataStream::readLine");
        }
        if (delim.size() > 1)
        {
            LogManager::getSingleton().logWarning(
                "FileStreamDataStream::readLine - using only first delimiter");
        }
        // Deal with both Unix & Windows LFs
        bool trimCR = false;
        if (delim.at(0) == '\n') 
        {
            trimCR = true;
        }
        // maxCount + 1 since count excludes terminator in getline
        mInStream->getline(buf, static_cast<std::streamsize>(maxCount+1), delim.at(0));
        auto ret = (size_t)mInStream->gcount();
        // three options
        // 1) we had an eof before we read a whole line
        // 2) we ran out of buffer space
        // 3) we read a whole line - in this case the delim character is taken from the stream but not written in the buffer so the read data is of length ret-1 and thus ends at index ret-2
        // in all cases the buffer will be null terminated for us

        if (mInStream->eof()) 
        {
            // no problem
        }
        else if (mInStream->fail())
        {
            // Did we fail because of maxCount hit? No - no terminating character
            // in included in the count in this case
            if (ret == maxCount)
            {
                // clear failbit for next time 
                mInStream->clear();
            }
            else
            {
                OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, 
                    "Streaming error occurred", 
                    "FileStreamDataStream::readLine");
            }
        }
        else 
        {
            // we need to adjust ret because we want to use it as a
            // pointer to the terminating null character and it is
            // currently the length of the data read from the stream
            // i.e. 1 more than the length of the data in the buffer and
            // hence 1 more than the _index_ of the NULL character
            --ret;
        }

        // trim off CR if we found CR/LF
        if (trimCR && ret && buf[ret-1] == '\r')
        {
            --ret;
            buf[ret] = '\0';
        }
        return ret;
    }
    //-----------------------------------------------------------------------
    void FileStreamDataStream::skip(long count)
    {
        mInStream->clear(); //Clear fail status in case eof was set
        mInStream->seekg(static_cast<std::ifstream::pos_type>(count), std::ios::cur);
    }
    //-----------------------------------------------------------------------
    void FileStreamDataStream::seek( size_t pos )
    {
        mInStream->clear(); //Clear fail status in case eof was set
        mInStream->seekg(static_cast<std::streamoff>(pos), std::ios::beg);
    }
    //-----------------------------------------------------------------------
    auto FileStreamDataStream::tell() const -> size_t
    {
        mInStream->clear(); //Clear fail status in case eof was set
        return (size_t)mInStream->tellg();
    }
    //-----------------------------------------------------------------------
    auto FileStreamDataStream::eof() const -> bool
    {
        return mInStream->eof();
    }
    //-----------------------------------------------------------------------
    void FileStreamDataStream::close()
    {
        mAccess = 0;
        if (mInStream)
        {
            // Unfortunately, there is no file-specific shared class hierarchy between fstream and ifstream (!!)
            if (mFStreamRO)
                mFStreamRO->close();
            if (mFStream)
            {
                mFStream->flush();
                mFStream->close();
            }

            if (mFreeOnClose)
            {
                // delete the stream too
                delete mFStreamRO;
                delete mFStream;
            }

            mInStream = nullptr;
            mFStreamRO = nullptr; 
            mFStream = nullptr; 
        }
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    FileHandleDataStream::FileHandleDataStream(FILE* handle, uint16 accessMode)
        : DataStream(accessMode), mFileHandle(handle)
    {
        // Determine size
        fseek(mFileHandle, 0, SEEK_END);
        mSize = ftell(mFileHandle);
        fseek(mFileHandle, 0, SEEK_SET);
    }
    //-----------------------------------------------------------------------
    FileHandleDataStream::FileHandleDataStream(std::string_view name, FILE* handle, uint16 accessMode)
        : DataStream(name, accessMode), mFileHandle(handle)
    {
        // Determine size
        fseek(mFileHandle, 0, SEEK_END);
        mSize = ftell(mFileHandle);
        fseek(mFileHandle, 0, SEEK_SET);
    }
    //-----------------------------------------------------------------------
    FileHandleDataStream::~FileHandleDataStream()
    {
        close();
    }
    //-----------------------------------------------------------------------
    auto FileHandleDataStream::read(void* buf, size_t count) -> size_t
    {
        return fread(buf, 1, count, mFileHandle);
    }
    //-----------------------------------------------------------------------
    auto FileHandleDataStream::write(const void* buf, size_t count) -> size_t
    {
        if (!isWriteable())
            return 0;
        else
            return fwrite(buf, 1, count, mFileHandle);
    }
    //---------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void FileHandleDataStream::skip(long count)
    {
        fseek(mFileHandle, count, SEEK_CUR);
    }
    //-----------------------------------------------------------------------
    void FileHandleDataStream::seek( size_t pos )
    {
        fseek(mFileHandle, static_cast<long>(pos), SEEK_SET);
    }
    //-----------------------------------------------------------------------
    auto FileHandleDataStream::tell() const -> size_t
    {
        return ftell( mFileHandle );
    }
    //-----------------------------------------------------------------------
    auto FileHandleDataStream::eof() const -> bool
    {
        return feof(mFileHandle) != 0;
    }
    //-----------------------------------------------------------------------
    void FileHandleDataStream::close()
    {
        mAccess = 0;
        if (mFileHandle != nullptr)
        {
            fclose(mFileHandle);
            mFileHandle = nullptr;
        }
    }
    //-----------------------------------------------------------------------

}
