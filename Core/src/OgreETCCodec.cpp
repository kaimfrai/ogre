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

#include <cstring>
#include <utility>

#include "OgreCodec.hpp"
#include "OgreCommon.hpp"
#include "OgreDataStream.hpp"
#include "OgreETCCodec.hpp"
#include "OgreException.hpp"
#include "OgreImage.hpp"
#include "OgreLog.hpp"
#include "OgreLogManager.hpp"
#include "OgrePixelFormat.hpp"
#include "OgrePlatform.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreStableHeaders.hpp"

enum {
KTX_ENDIAN_REF =      (0x04030201),
KTX_ENDIAN_REF_REV =  (0x01020304)
};

// In a PKM-file, the codecs are stored using the following identifiers
//
// identifier                         value               codec
// --------------------------------------------------------------------
// ETC1_RGB_NO_MIPMAPS                  0                 GL_ETC1_RGB8_OES
// ETC2PACKAGE_RGB_NO_MIPMAPS           1                 GL_COMPRESSED_RGB8_ETC2
// ETC2PACKAGE_RGBA_NO_MIPMAPS_OLD      2, not used       -
// ETC2PACKAGE_RGBA_NO_MIPMAPS          3                 GL_COMPRESSED_RGBA8_ETC2_EAC
// ETC2PACKAGE_RGBA1_NO_MIPMAPS         4                 GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
// ETC2PACKAGE_R_NO_MIPMAPS             5                 GL_COMPRESSED_R11_EAC
// ETC2PACKAGE_RG_NO_MIPMAPS            6                 GL_COMPRESSED_RG11_EAC
// ETC2PACKAGE_R_SIGNED_NO_MIPMAPS      7                 GL_COMPRESSED_SIGNED_R11_EAC
// ETC2PACKAGE_RG_SIGNED_NO_MIPMAPS     8                 GL_COMPRESSED_SIGNED_RG11_EAC

namespace Ogre {

    const uint32 PKM_MAGIC = FOURCC('P', 'K', 'M', ' ');
    const uint32 KTX_MAGIC = FOURCC(0xAB, 0x4B, 0x54, 0x58);

    using PKMHeader = struct {
        uint8  name[4];
        uint8  version[2];
        uint8  iTextureTypeMSB;
        uint8  iTextureTypeLSB;
        uint8  iPaddedWidthMSB;
        uint8  iPaddedWidthLSB;
        uint8  iPaddedHeightMSB;
        uint8  iPaddedHeightLSB;
        uint8  iWidthMSB;
        uint8  iWidthLSB;
        uint8  iHeightMSB;
        uint8  iHeightLSB;
    };

    using KTXHeader = struct {
        uint8     identifier[12];
        uint32    endianness;
        uint32    glType;
        uint32    glTypeSize;
        uint32    glFormat;
        uint32    glInternalFormat;
        uint32    glBaseInternalFormat;
        uint32    pixelWidth;
        uint32    pixelHeight;
        uint32    pixelDepth;
        uint32    numberOfArrayElements;
        uint32    numberOfFaces;
        uint32    numberOfMipmapLevels;
        uint32    bytesOfKeyValueData;
    };

    //---------------------------------------------------------------------
    ETCCodec* ETCCodec::msPKMInstance = nullptr;
    ETCCodec* ETCCodec::msKTXInstance = nullptr;
    //---------------------------------------------------------------------
    void ETCCodec::startup()
    {
        if (!msPKMInstance)
        {
            msPKMInstance = new ETCCodec("pkm");
            Codec::registerCodec(msPKMInstance);
        }

        if (!msKTXInstance)
        {
            msKTXInstance = new ETCCodec("ktx");
            Codec::registerCodec(msKTXInstance);
        }

        LogManager::getSingleton().logMessage(LogMessageLevel::Normal,
                                              "ETC codec registering");
    }
    //---------------------------------------------------------------------
    void ETCCodec::shutdown()
    {
        if(msPKMInstance)
        {
            Codec::unregisterCodec(msPKMInstance);
            delete msPKMInstance;
            msPKMInstance = nullptr;
        }

        if(msKTXInstance)
        {
            Codec::unregisterCodec(msKTXInstance);
            delete msKTXInstance;
            msKTXInstance = nullptr;
        }
    }
    //---------------------------------------------------------------------
    ETCCodec::ETCCodec(std::string_view type):
        mType(type)
    {
    }
    //---------------------------------------------------------------------
    auto ETCCodec::decode(const DataStreamPtr& stream) const -> ImageCodec::DecodeResult
    {
        DecodeResult ret;
        if (decodeKTX(stream, ret))
            return ret;

        stream->seek(0);
        if (decodePKM(stream, ret))
            return ret;

        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                    "This is not a valid ETC file!", "ETCCodec::decode");
    }
    //---------------------------------------------------------------------
    auto ETCCodec::getType() const -> std::string_view
    {
        return mType;
    }
    //---------------------------------------------------------------------
    auto ETCCodec::magicNumberToFileExt(const char *magicNumberPtr, size_t maxbytes) const -> std::string_view
    {
        if (maxbytes >= sizeof(uint32))
        {
            uint32 fileType;
            memcpy(&fileType, magicNumberPtr, sizeof(uint32));
            flipEndian(&fileType, sizeof(uint32));

            if (PKM_MAGIC == fileType)
                return {"pkm"};

            if (KTX_MAGIC == fileType)
                return {"ktx"};
        }

        return BLANKSTRING;
    }
    //---------------------------------------------------------------------
    auto ETCCodec::decodePKM(const DataStreamPtr& stream, DecodeResult& result) const -> bool
    {
        PKMHeader header;

        // Read the ETC header
        stream->read(&header, sizeof(PKMHeader));

        if (PKM_MAGIC != FOURCC(header.name[0], header.name[1], header.name[2], header.name[3]) ) // "PKM 10"
            return false;

        uint16 width = (header.iWidthMSB << 8) | header.iWidthLSB;
        uint16 height = (header.iHeightMSB << 8) | header.iHeightLSB;
        uint16 paddedWidth = (header.iPaddedWidthMSB << 8) | header.iPaddedWidthLSB;
        uint16 paddedHeight = (header.iPaddedHeightMSB << 8) | header.iPaddedHeightLSB;
        uint16 type = (header.iTextureTypeMSB << 8) | header.iTextureTypeLSB;

        auto *imgData = new ImageData();
        imgData->depth = 1;
        imgData->width = width;
        imgData->height = height;

        // File version 2.0 supports ETC2 in addition to ETC1
        if(header.version[0] == '2' && header.version[1] == '0')
        {
            switch (type) {
                case 0:
                    imgData->format = PixelFormat::ETC1_RGB8;
                    break;

                    // GL_COMPRESSED_RGB8_ETC2
                case 1:
                    imgData->format = PixelFormat::ETC2_RGB8;
                    break;

                    // GL_COMPRESSED_RGBA8_ETC2_EAC
                case 3:
                    imgData->format = PixelFormat::ETC2_RGBA8;
                    break;

                    // GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
                case 4:
                    imgData->format = PixelFormat::ETC2_RGB8A1;
                    break;

                    // Default case is ETC1
                default:
                    imgData->format = PixelFormat::ETC1_RGB8;
                    break;
            }
        }
        else
            imgData->format = PixelFormat::ETC1_RGB8;

        // ETC has no support for mipmaps - malideveloper.com has a example
        // where the load mipmap levels from different external files
        imgData->num_mipmaps = {};

        // ETC is a compressed format
        imgData->flags |= ImageFlags::COMPRESSED;

        // Calculate total size from number of mipmaps, faces and size
        imgData->size = (paddedWidth * paddedHeight) >> 1;

        // Bind output buffer
        MemoryDataStreamPtr output(new MemoryDataStream(imgData->size));

        // Now deal with the data
        void *destPtr = output->getPtr();
        stream->read(destPtr, imgData->size);
        destPtr = static_cast<void*>(static_cast<uchar*>(destPtr));

        DecodeResult ret;
        ret.first = output;
        ret.second = CodecDataPtr(imgData);

        return true;
    }
    //---------------------------------------------------------------------
    auto ETCCodec::decodeKTX(const DataStreamPtr& stream, DecodeResult& result) const -> bool
    {
        KTXHeader header;
        // Read the KTX header
        stream->read(&header, sizeof(KTXHeader));

        const uint8 KTXFileIdentifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
        if (memcmp(KTXFileIdentifier, &header.identifier, sizeof(KTXFileIdentifier)) != 0 )
            return false;

        if (header.endianness == KTX_ENDIAN_REF_REV)
            flipEndian(&header.glType, sizeof(uint32));

        auto *imgData = new ImageData();
        imgData->depth = 1;
        imgData->width = header.pixelWidth;
        imgData->height = header.pixelHeight;
        imgData->num_mipmaps = static_cast<TextureMipmap>(header.numberOfMipmapLevels - 1);

        switch(header.glInternalFormat)
        {
        case 37492: // GL_COMPRESSED_RGB8_ETC2
            imgData->format = PixelFormat::ETC2_RGB8;
            break;
        case 37496:// GL_COMPRESSED_RGBA8_ETC2_EAC
            imgData->format = PixelFormat::ETC2_RGBA8;
            break;
        case 37494: // GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
            imgData->format = PixelFormat::ETC2_RGB8A1;
            break;
        case 35986: // ATC_RGB
            imgData->format = PixelFormat::ATC_RGB;
            break;
        case 35987: // ATC_RGB_Explicit
            imgData->format = PixelFormat::ATC_RGBA_EXPLICIT_ALPHA;
            break;
        case 34798: // ATC_RGB_Interpolated
            imgData->format = PixelFormat::ATC_RGBA_INTERPOLATED_ALPHA;
            break;
        case 33777: // DXT 1
            imgData->format = PixelFormat::DXT1;
            break;
        case 33778: // DXT 3
            imgData->format = PixelFormat::DXT3;
            break;
        case 33779: // DXT 5
            imgData->format = PixelFormat::DXT5;
            break;
         case 0x8c00: // COMPRESSED_RGB_PVRTC_4BPPV1_IMG
            imgData->format = PixelFormat::PVRTC_RGB4;
            break;
        case 0x8c01: // COMPRESSED_RGB_PVRTC_2BPPV1_IMG
            imgData->format = PixelFormat::PVRTC_RGB2;
            break;
        case 0x8c02: // COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
            imgData->format = PixelFormat::PVRTC_RGBA4;
            break;
        case 0x8c03: // COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
            imgData->format = PixelFormat::PVRTC_RGBA2;
            break;
        case 0x93B0: // COMPRESSED_RGBA_ASTC_4x4_KHR
            imgData->format = PixelFormat::ASTC_RGBA_4X4_LDR;
            break;
        case 0x93B1: // COMPRESSED_RGBA_ASTC_5x4_KHR
            imgData->format = PixelFormat::ASTC_RGBA_5X4_LDR;
            break;
        case 0x93B2: // COMPRESSED_RGBA_ASTC_5x5_KHR
            imgData->format = PixelFormat::ASTC_RGBA_5X5_LDR;
            break;
        case 0x93B3: // COMPRESSED_RGBA_ASTC_6x5_KHR
            imgData->format = PixelFormat::ASTC_RGBA_6X5_LDR;
            break;
        case 0x93B4: // COMPRESSED_RGBA_ASTC_6x6_KHR
            imgData->format = PixelFormat::ASTC_RGBA_6X6_LDR;
            break;
        case 0x93B5: // COMPRESSED_RGBA_ASTC_8x5_KHR
            imgData->format = PixelFormat::ASTC_RGBA_8X5_LDR;
            break;
        case 0x93B6: // COMPRESSED_RGBA_ASTC_8x6_KHR
            imgData->format = PixelFormat::ASTC_RGBA_8X6_LDR;
            break;
        case 0x93B7: // COMPRESSED_RGBA_ASTC_8x8_KHR
            imgData->format = PixelFormat::ASTC_RGBA_8X8_LDR;
            break;
        case 0x93B8: // COMPRESSED_RGBA_ASTC_10x5_KHR
            imgData->format = PixelFormat::ASTC_RGBA_10X5_LDR;
            break;
        case 0x93B9: // COMPRESSED_RGBA_ASTC_10x6_KHR
            imgData->format = PixelFormat::ASTC_RGBA_10X6_LDR;
            break;
        case 0x93BA: // COMPRESSED_RGBA_ASTC_10x8_KHR
            imgData->format = PixelFormat::ASTC_RGBA_10X8_LDR;
            break;
        case 0x93BB: // COMPRESSED_RGBA_ASTC_10x10_KHR
            imgData->format = PixelFormat::ASTC_RGBA_10X10_LDR;
            break;
        case 0x93BC: // COMPRESSED_RGBA_ASTC_12x10_KHR
            imgData->format = PixelFormat::ASTC_RGBA_12X10_LDR;
            break;
        case 0x93BD: // COMPRESSED_RGBA_ASTC_12x12_KHR
            imgData->format = PixelFormat::ASTC_RGBA_12X12_LDR;
            break;
        default:
            imgData->format = PixelFormat::ETC1_RGB8;
            break;
        }

        imgData->flags = {};
        if (header.glType == 0 || header.glFormat == 0)
            imgData->flags |= ImageFlags::COMPRESSED;

        uint32 numFaces = header.numberOfFaces;
        if (numFaces > 1)
            imgData->flags |= ImageFlags::CUBEMAP;
        // Calculate total size from number of mipmaps, faces and size
        imgData->size = Image::calculateSize(imgData->num_mipmaps, numFaces,
                                             imgData->width, imgData->height, imgData->depth, imgData->format);

        stream->skip(header.bytesOfKeyValueData);

        // Bind output buffer
        MemoryDataStreamPtr output(new MemoryDataStream(imgData->size));

        // Now deal with the data
        uchar* destPtr = output->getPtr();
        uint32 mipOffset = 0;
        for (uint32 level = 0; level < header.numberOfMipmapLevels; ++level)
        {
            uint32 imageSize = 0;
            stream->read(&imageSize, sizeof(uint32));

            for(uint32 face = 0; face < numFaces; ++face)
            {
                uchar* placePtr = destPtr + ((imgData->size)/numFaces)*face + mipOffset; // shuffle mip and face
                stream->read(placePtr, imageSize);
            }
            mipOffset += imageSize;
        }

        result.first = output;
        result.second = CodecDataPtr(imgData);

        return true;
    }
}
