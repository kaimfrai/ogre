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

#include <cstdlib>
#include <cstring>

module Ogre.Core;

import :Bitwise;
import :DataStream;
import :Exception;
import :Platform;
import :Prerequisites;
import :Quaternion;
import :Serializer;
import :SharedPtr;
import :Vector;

import <format>;
import <string>;
import <string_view>;

namespace Ogre {

    const uint16 HEADER_STREAM_ID = 0x1000;
    const uint16 OTHER_ENDIAN_HEADER_STREAM_ID = 0x0010;
    //---------------------------------------------------------------------
    Serializer::Serializer() :
        mVersion("[Serializer_v1.00]")
        
    {
    }

    //---------------------------------------------------------------------
    Serializer::~Serializer()
    = default;
    //---------------------------------------------------------------------
    void Serializer::determineEndianness(const DataStreamPtr& stream)
    {
        if (stream->tell() != 0)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                "Can only determine the endianness of the input stream if it "
                "is at the start", "Serializer::determineEndianness");
        }
                
        uint16 dest;
        // read header id manually (no conversion)
        size_t actually_read = stream->read(&dest, sizeof(uint16));
        // skip back
        stream->skip(0 - (long)actually_read);
        if (actually_read != sizeof(uint16))
        {
            // end of file?
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "Couldn't read 16 bit header value from input stream.",
                        "Serializer::determineEndianness");
        }
        if (dest == HEADER_STREAM_ID)
        {
            mFlipEndian = false;
        }
        else if (dest == OTHER_ENDIAN_HEADER_STREAM_ID)
        {
            mFlipEndian = true;
        }
        else
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                "Header chunk didn't match either endian: Corrupted stream?",
                "Serializer::determineEndianness");
        }
    }
    //---------------------------------------------------------------------
    void Serializer::determineEndianness(std::endian requestedEndian)
    {
        using enum std::endian;
        switch(requestedEndian)
        {
        case big:
            mFlipEndian = true;
            break;
        case little:
            mFlipEndian = false;
            break;
        }
    }
    //---------------------------------------------------------------------
    void Serializer::writeFileHeader()
    {
        
        uint16 val = HEADER_STREAM_ID;
        writeShorts(&val, 1);

        writeString(mVersion);

    }
    //---------------------------------------------------------------------
    void Serializer::writeChunkHeader(uint16 id, size_t size)
    {
        writeShorts(&id, 1);
        auto uint32size = static_cast<uint32>(size);
        writeInts(&uint32size, 1);
    }
    //---------------------------------------------------------------------
    void Serializer::writeFloats(const float* const pFloat, size_t count)
    {
        if (mFlipEndian)
        {
            auto * pFloatToWrite = (float *)malloc(sizeof(float) * count);
            memcpy(pFloatToWrite, pFloat, sizeof(float) * count);
            
            flipToLittleEndian(pFloatToWrite, sizeof(float), count);
            writeData(pFloatToWrite, sizeof(float), count);
            
            free(pFloatToWrite);
        }
        else
        {
            writeData(pFloat, sizeof(float), count);
        }
    }
    //---------------------------------------------------------------------
    void Serializer::writeFloats(const double* const pDouble, size_t count)
    {
        // Convert to float, then write
        auto* tmp = new float[count];
        for (unsigned int i = 0; i < count; ++i)
        {
            tmp[i] = static_cast<float>(pDouble[i]);
        }
        if(mFlipEndian)
        {
            flipToLittleEndian(tmp, sizeof(float), count);
            writeData(tmp, sizeof(float), count);
        }
        else
        {
            writeData(tmp, sizeof(float), count);
        }
        delete[] tmp;
    }
    //---------------------------------------------------------------------
    void Serializer::writeShorts(const uint16* const pShort, size_t count = 1)
    {
        if(mFlipEndian)
        {
            auto * pShortToWrite = (unsigned short *)malloc(sizeof(unsigned short) * count);
            memcpy(pShortToWrite, pShort, sizeof(unsigned short) * count);
            
            flipToLittleEndian(pShortToWrite, sizeof(unsigned short), count);
            writeData(pShortToWrite, sizeof(unsigned short), count);
            
            free(pShortToWrite);
        }
        else
        {
            writeData(pShort, sizeof(unsigned short), count);
        }
    }
    //---------------------------------------------------------------------
    void Serializer::writeInts(const uint32* const pInt, size_t count = 1)
    {
        if(mFlipEndian)
        {
            auto * pIntToWrite = (uint32 *)malloc(sizeof(uint32) * count);
            memcpy(pIntToWrite, pInt, sizeof(uint32) * count);
            
            flipToLittleEndian(pIntToWrite, sizeof(uint32), count);
            writeData(pIntToWrite, sizeof(uint32), count);
            
            free(pIntToWrite);
        }
        else
        {
            writeData(pInt, sizeof(uint32), count);
        }
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    void Serializer::writeBools(const bool* const pBool, size_t count = 1)
    {
        //no endian flipping for 1-byte bools
        static_assert(sizeof(bool) == 1, "add conversion to char for your platform");
        writeData(pBool, sizeof(bool), count);
    }
    
    //---------------------------------------------------------------------
    void Serializer::writeData(const void* const buf, size_t size, size_t count)
    {
        mStream->write(buf, size * count);
    }
    //---------------------------------------------------------------------
    void Serializer::writeString(std::string_view string)
    {
        // Old, backwards compatible way - \n terminated
        mStream->write(string.data(), string.length());
        // Write terminating newline char
        char terminator = '\n';
        mStream->write(&terminator, 1);
    }
    //---------------------------------------------------------------------
    void Serializer::readFileHeader(const DataStreamPtr& stream)
    {
        unsigned short headerID;
        
        // Read header ID
        readShorts(stream, &headerID, 1);
        
        if (headerID == HEADER_STREAM_ID)
        {
            // Read version
            String ver = readString(stream);
            if (ver != mVersion)
            {
                OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, 
                    "Invalid file: version incompatible, file reports " + String(ver) +
                    ::std::format(" Serializer is version {}", mVersion),
                    "Serializer::readFileHeader");
            }
        }
        else
        {
            OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, "Invalid file: no header", 
                "Serializer::readFileHeader");
        }

    }
    //---------------------------------------------------------------------
    auto Serializer::readChunk(const DataStreamPtr& stream) -> unsigned short
    {
        unsigned short id;
        readShorts(stream, &id, 1);
        
        readInts(stream, &mCurrentstreamLen, 1);

        return id;
    }
    //---------------------------------------------------------------------
    void Serializer::readBools(const DataStreamPtr& stream, bool* pDest, size_t count)
    {
        static_assert(sizeof(bool) == 1, "add conversion to char for your platform");
        stream->read(pDest, sizeof(bool) * count);
    }
    //---------------------------------------------------------------------
    void Serializer::readFloats(const DataStreamPtr& stream, float* pDest, size_t count)
    {
        stream->read(pDest, sizeof(float) * count);
        flipFromLittleEndian(pDest, sizeof(float), count);
    }
    //---------------------------------------------------------------------
    void Serializer::readFloats(const DataStreamPtr& stream, double* pDest, size_t count)
    {
        // Read from float, convert to double
        auto* tmp = new float[count];
        float* ptmp = tmp;
        stream->read(tmp, sizeof(float) * count);
        flipFromLittleEndian(tmp, sizeof(float), count);
        // Convert to doubles (no cast required)
        while(count--)
        {
            *pDest++ = *ptmp++;
        }
        delete[] tmp;
    }
    //---------------------------------------------------------------------
    void Serializer::readShorts(const DataStreamPtr& stream, unsigned short* pDest, size_t count)
    {
        stream->read(pDest, sizeof(unsigned short) * count);
        flipFromLittleEndian(pDest, sizeof(unsigned short), count);
    }
    //---------------------------------------------------------------------
    void Serializer::readInts(const DataStreamPtr& stream, uint32* pDest, size_t count)
    {
        stream->read(pDest, sizeof(uint32) * count);
        flipFromLittleEndian(pDest, sizeof(uint32), count);
    }
    //---------------------------------------------------------------------
    auto Serializer::readString(const DataStreamPtr& stream, size_t numChars) -> String
    {
        OgreAssert(numChars <= 255, "");
        char str[255];
        stream->read(str, numChars);
        str[numChars] = '\0';
        return str;
    }
    //---------------------------------------------------------------------
    auto Serializer::readString(const DataStreamPtr& stream) -> String
    {
        return stream->getLine(false);
    }
    //---------------------------------------------------------------------
    void Serializer::writeObject(const Vector3& vec)
    {
        writeFloats(vec.ptr(), 3);
    }
    //---------------------------------------------------------------------
    void Serializer::writeObject(const Quaternion& q)
    {
        float tmp[4] = {
            static_cast<float>(q.x),
            static_cast<float>(q.y),
            static_cast<float>(q.z),
            static_cast<float>(q.w)
        };
        writeFloats(tmp, 4);
    }
    //---------------------------------------------------------------------
    void Serializer::readObject(const DataStreamPtr& stream, Vector3& pDest)
    {
        readFloats(stream, pDest.ptr(), 3);
    }
    //---------------------------------------------------------------------
    void Serializer::readObject(const DataStreamPtr& stream, Quaternion& pDest)
    {
        float tmp[4];
        readFloats(stream, tmp, 4);
        pDest.x = tmp[0];
        pDest.y = tmp[1];
        pDest.z = tmp[2];
        pDest.w = tmp[3];
    }
    //---------------------------------------------------------------------


    void Serializer::flipToLittleEndian(void* pData, size_t size, size_t count)
    {
        if(mFlipEndian)
        {
			Bitwise::bswapChunks(pData, size, count);
        }
    }
    
    void Serializer::flipFromLittleEndian(void* pData, size_t size, size_t count)
    {
        if(mFlipEndian)
        {
	        Bitwise::bswapChunks(pData, size, count);
        }
    }
    
    auto Serializer::calcChunkHeaderSize() -> size_t
    {
        return sizeof(uint16) + sizeof(uint32);
    }

    auto Serializer::calcStringSize( std::string_view string ) -> size_t
    {
        // string + terminating \n character
        return string.length() + 1;
    }

    void Serializer::pushInnerChunk(const DataStreamPtr& stream)
    {

    }
    void Serializer::backpedalChunkHeader(const DataStreamPtr& stream)
    {
        if (!stream->eof()){
            stream->skip(-(int)calcChunkHeaderSize());
        }

    }
    void Serializer::popInnerChunk(const DataStreamPtr& stream)
    {

    }

}
