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

#include <cmath>
#include <cstring>

module Ogre.Core;

import :ASTCCodec;
import :Codec;
import :Common;
import :DataStream;
import :Exception;
import :Image;
import :Log;
import :LogManager;
import :Platform;
import :SharedPtr;

namespace Ogre {

    const uint32 ASTC_MAGIC = 0x5CA1AB13;

    using ASTCHeader = struct
    {
        uint8 magic[4];
        uint8 blockdim_x;
        uint8 blockdim_y;
        uint8 blockdim_z;
        uint8 xsize[3];			// x-size = xsize[0] + xsize[1] + xsize[2]
        uint8 ysize[3];			// x-size, y-size and z-size are given in texels;
        uint8 zsize[3];			// block count is inferred
    };

    auto ASTCCodec::getBitrateForPixelFormat(PixelFormat fmt) -> float
    {
        using enum PixelFormat;
        switch (fmt)
        {
        case ASTC_RGBA_4X4_LDR:
            return 8.00;
        case ASTC_RGBA_5X4_LDR:
            return 6.40;
        case ASTC_RGBA_5X5_LDR:
            return 5.12;
        case ASTC_RGBA_6X5_LDR:
            return 4.27;
        case ASTC_RGBA_6X6_LDR:
            return 3.56;
        case ASTC_RGBA_8X5_LDR:
            return 3.20;
        case ASTC_RGBA_8X6_LDR:
            return 2.67;
        case ASTC_RGBA_8X8_LDR:
            return 2.00;
        case ASTC_RGBA_10X5_LDR:
            return 2.56;
        case ASTC_RGBA_10X6_LDR:
            return 2.13;
        case ASTC_RGBA_10X8_LDR:
            return 1.60;
        case ASTC_RGBA_10X10_LDR:
            return 1.28;
        case ASTC_RGBA_12X10_LDR:
            return 1.07;
        case ASTC_RGBA_12X12_LDR:
            return 0.89;

        default:
            return 0;
        }
    }

    // Utility function to determine 2D block dimensions from a target bitrate. Used for 3D textures.
    // Taken from astc_toplevel.cpp in ARM's ASTC Evaluation Codec
    void ASTCCodec::getClosestBlockDim2d(float targetBitrate, int *x, int *y) const
    {
        int blockdims[6] = { 4, 5, 6, 8, 10, 12 };

        float best_error = 1000;
        float aspect_of_best = 1;
        int i, j;

        // Y dimension
        for (i = 0; i < 6; i++)
        {
            // X dimension
            for (j = i; j < 6; j++)
            {
                //              NxN       MxN         8x5               10x5              10x6
                int is_legal = (j==i) || (j==i+1) || (j==3 && i==1) || (j==4 && i==1) || (j==4 && i==2);

                if(is_legal)
                {
                    float bitrate = 128.0f / (blockdims[i] * blockdims[j]);
                    float bitrate_error = std::fabs(bitrate - targetBitrate);
                    float aspect = (float)blockdims[j] / blockdims[i];
                    if (bitrate_error < best_error || (bitrate_error == best_error && aspect < aspect_of_best))
                    {
                        *x = blockdims[j];
                        *y = blockdims[i];
                        best_error = bitrate_error;
                        aspect_of_best = aspect;
                    }
                }
            }
        }
    }

    // Taken from astc_toplevel.cpp in ARM's ASTC Evaluation Codec
    void ASTCCodec::getClosestBlockDim3d(float targetBitrate, int *x, int *y, int *z)
    {
        int blockdims[4] = { 3, 4, 5, 6 };

        float best_error = 1000;
        float aspect_of_best = 1;
        int i, j, k;

        for (i = 0; i < 4; i++)	// Z
        {
            for (j = i; j < 4; j++) // Y
            {
                for (k = j; k < 4; k++) // X
                {
                    //              NxNxN              MxNxN                  MxMxN
                    int is_legal = ((k==j)&&(j==i)) || ((k==j+1)&&(j==i)) || ((k==j)&&(j==i+1));

                    if(is_legal)
                    {
                        float bitrate = 128.0f / (blockdims[i] * blockdims[j] * blockdims[k]);
                        float bitrate_error = std::fabs(bitrate - targetBitrate);
                        float aspect = (float)blockdims[k] / blockdims[j] + (float)blockdims[j] / blockdims[i] + (float)blockdims[k] / blockdims[i];

                        if (bitrate_error < best_error || (bitrate_error == best_error && aspect < aspect_of_best))
                        {
                            *x = blockdims[k];
                            *y = blockdims[j];
                            *z = blockdims[i];
                            best_error = bitrate_error;
                            aspect_of_best = aspect;
                        }
                    }
                }
            }
        }
    }
	//---------------------------------------------------------------------
	ASTCCodec* ASTCCodec::msInstance = nullptr;
	//---------------------------------------------------------------------
	void ASTCCodec::startup()
	{
		if (!msInstance)
		{
			msInstance = new ASTCCodec();
			Codec::registerCodec(msInstance);
		}

        LogManager::getSingleton().logMessage(LogMessageLevel::Normal,
                                              "ASTC codec registering");
	}
	//---------------------------------------------------------------------
	void ASTCCodec::shutdown()
	{
		if(msInstance)
		{
			Codec::unregisterCodec(msInstance);
			delete msInstance;
			msInstance = nullptr;
		}
	}
	//---------------------------------------------------------------------
    ASTCCodec::ASTCCodec():
        mType("astc")
    { 
    }
    //---------------------------------------------------------------------
    auto ASTCCodec::decode(const DataStreamPtr& stream) const -> ImageCodec::DecodeResult
    {
        DecodeResult ret;
        ASTCHeader header;

        // Read the ASTC header
        stream->read(&header, sizeof(ASTCHeader));

		if (memcmp(&ASTC_MAGIC, &header.magic, sizeof(uint32)) != 0 )
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "This is not a valid ASTC file!", "ASTCCodec::decode");

        int xdim = header.blockdim_x;
        int ydim = header.blockdim_y;
        int zdim = header.blockdim_z;

        int xsize = header.xsize[0] + 256 * header.xsize[1] + 65536 * header.xsize[2];
        int ysize = header.ysize[0] + 256 * header.ysize[1] + 65536 * header.ysize[2];
        int zsize = header.zsize[0] + 256 * header.zsize[1] + 65536 * header.zsize[2];

        auto *imgData = new ImageData();
        imgData->width = xsize;
        imgData->height = ysize;
        imgData->depth = zsize;
		imgData->num_mipmaps = {}; // Always 1 mip level per file

        // For 3D we calculate the bitrate then find the nearest 2D block size.
        if(zdim > 1)
        {
            float bitrate = 128.0f / (xdim * ydim * zdim);
            getClosestBlockDim2d(bitrate, &xdim, &ydim);
        }

        if(xdim == 4)
        {
            imgData->format = PixelFormat::ASTC_RGBA_4X4_LDR;
        }
        else if(xdim == 5)
        {
            if(ydim == 4)
                imgData->format = PixelFormat::ASTC_RGBA_5X4_LDR;
            else if(ydim == 5)
                imgData->format = PixelFormat::ASTC_RGBA_5X5_LDR;
        }
        else if(xdim == 6)
        {
            if(ydim == 5)
                imgData->format = PixelFormat::ASTC_RGBA_6X5_LDR;
            else if(ydim == 6)
                imgData->format = PixelFormat::ASTC_RGBA_6X6_LDR;
        }
        else if(xdim == 8)
        {
            if(ydim == 5)
                imgData->format = PixelFormat::ASTC_RGBA_8X5_LDR;
            else if(ydim == 6)
                imgData->format = PixelFormat::ASTC_RGBA_8X6_LDR;
            else if(ydim == 8)
                imgData->format = PixelFormat::ASTC_RGBA_8X8_LDR;
        }
        else if(xdim == 10)
        {
            if(ydim == 5)
                imgData->format = PixelFormat::ASTC_RGBA_10X5_LDR;
            else if(ydim == 6)
                imgData->format = PixelFormat::ASTC_RGBA_10X6_LDR;
            else if(ydim == 8)
                imgData->format = PixelFormat::ASTC_RGBA_10X8_LDR;
            else if(ydim == 10)
                imgData->format = PixelFormat::ASTC_RGBA_10X10_LDR;
        }
        else if(xdim == 12)
        {
            if(ydim == 10)
                imgData->format = PixelFormat::ASTC_RGBA_12X10_LDR;
            else if(ydim == 12)
                imgData->format = PixelFormat::ASTC_RGBA_12X12_LDR;
        }

        imgData->flags = ImageFlags::COMPRESSED;

		uint32 numFaces = 1; // Always one face, cubemaps are not currently supported
                             // Calculate total size from number of mipmaps, faces and size
		imgData->size = Image::calculateSize(imgData->num_mipmaps, numFaces,
                                             imgData->width, imgData->height, imgData->depth, imgData->format);

		// Bind output buffer
		MemoryDataStreamPtr output(new MemoryDataStream(imgData->size));

		// Now deal with the data
		uchar* destPtr = output->getPtr();
        stream->read(destPtr, imgData->size);

		ret.first = output;
		ret.second = CodecDataPtr(imgData);
        
		return ret;
    }
    //---------------------------------------------------------------------    
    auto ASTCCodec::getType() const -> std::string_view
    {
        return mType;
    }
    //---------------------------------------------------------------------    
	auto ASTCCodec::magicNumberToFileExt(const char *magicNumberPtr, size_t maxbytes) const -> std::string_view
	{
		if (maxbytes >= sizeof(uint32))
		{
			uint32 fileType;
			memcpy(&fileType, magicNumberPtr, sizeof(uint32));
			flipEndian(&fileType, sizeof(uint32), 1);

			if (ASTC_MAGIC == fileType)
				return {"astc"};
		}

        return BLANKSTRING;
	}
}
