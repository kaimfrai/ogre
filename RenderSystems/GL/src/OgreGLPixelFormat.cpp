/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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
module Ogre.RenderSystems.GL;

import :PixelFormat;

import Ogre.Core;

#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT                     0x140B
#endif

namespace Ogre  {

    struct GLPixelFormatDescription {
        GLenum format;
        GLenum type;
        GLenum internalFormat;
    };

    static GLPixelFormatDescription _pixelFormats[int(PixelFormat::COUNT)] = {
            {GL_NONE},                                           // PixelFormat::UNKNOWN
            {GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_LUMINANCE8},     // PixelFormat::L8
            {GL_LUMINANCE, GL_UNSIGNED_SHORT, GL_LUMINANCE16},   // PixelFormat::L16
            {GL_ALPHA, GL_UNSIGNED_BYTE, GL_ALPHA8},             // PixelFormat::A8
            {GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GL_LUMINANCE8_ALPHA8},// PixelFormat::BYTE_LA
            {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB5},          // PixelFormat::R5G6B5
            {GL_BGR, GL_UNSIGNED_SHORT_5_6_5, GL_RGB5},          // PixelFormat::B5G6R5
            {GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_RGBA4},  // PixelFormat::A4R4G4B4
            {GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_RGB5_A1},// PixelFormat::A1R5G5B5

            {GL_BGR, GL_UNSIGNED_BYTE, GL_RGB8},                 // PixelFormat::R8G8B8
            {GL_RGB, GL_UNSIGNED_BYTE, GL_RGB8},                 // PixelFormat::B8G8R8

            {GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA8},    // PixelFormat::A8R8G8B8
            {GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA8},    // PixelFormat::A8B8G8R8
            {GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA8},        // PixelFormat::B8G8R8A8
            {GL_NONE},                                           // PixelFormat::A2R10G10B10
            {GL_NONE},                                           // PixelFormat::A2B10G10R10
            {GL_NONE, GL_NONE, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT},// PixelFormat::DXT1
            {GL_NONE},                                           // PixelFormat::DXT2
            {GL_NONE, GL_NONE, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT},// PixelFormat::DXT3
            {GL_NONE},                                           // PixelFormat::DXT4
            {GL_NONE, GL_NONE, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT},// PixelFormat::DXT5
            {GL_RGB, GL_HALF_FLOAT, GL_RGB16F_ARB},              // PixelFormat::FLOAT16_RGB
            {GL_RGBA, GL_HALF_FLOAT, GL_RGBA16F_ARB},            // PixelFormat::FLOAT16_RGBA
            {GL_RGB, GL_FLOAT, GL_RGB32F_ARB},                   // PixelFormat::FLOAT32_RGB
            {GL_RGBA, GL_FLOAT, GL_RGBA32F_ARB},                 // PixelFormat::FLOAT32_RGBA
            {GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA8},    // PixelFormat::X8R8G8B8
            {GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA8},    // PixelFormat::X8B8G8R8
            {GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA8},        // PixelFormat::R8G8B8A8
            {GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, GL_DEPTH_COMPONENT16}, // PixelFormat::DEPTH16
            {GL_RGBA, GL_UNSIGNED_SHORT, GL_RGBA16},             // PixelFormat::SHORT_RGBA
            {GL_RGB, GL_UNSIGNED_BYTE_3_3_2, GL_R3_G3_B2},       // PixelFormat::R3G3B2
            {GL_LUMINANCE, GL_HALF_FLOAT, GL_LUMINANCE16F_ARB},  // PixelFormat::FLOAT16_R
            {GL_LUMINANCE, GL_FLOAT, GL_LUMINANCE32F_ARB},       // PixelFormat::FLOAT32_R
            {GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT, GL_LUMINANCE16_ALPHA16},// PixelFormat::SHORT_GR
            {GL_LUMINANCE_ALPHA, GL_HALF_FLOAT, GL_LUMINANCE_ALPHA16F_ARB}, // PixelFormat::FLOAT16_GR
            {GL_LUMINANCE_ALPHA, GL_FLOAT, GL_LUMINANCE_ALPHA32F_ARB},      // PixelFormat::FLOAT32_GR
            {GL_RGB, GL_UNSIGNED_SHORT, GL_RGB16},               // PixelFormat::SHORT_RGB
            // limited format support: the rest is PixelFormat::NONE
    };

    //-----------------------------------------------------------------------------
    auto GLPixelUtil::getGLOriginFormat(PixelFormat pf) -> GLenum
    {
        return _pixelFormats[std::to_underlying(pf)].format;
    }
    //----------------------------------------------------------------------------- 
    auto GLPixelUtil::getGLOriginDataType(PixelFormat pf) -> GLenum
    {
        return _pixelFormats[std::to_underlying(pf)].type;
    }
    
    auto GLPixelUtil::getGLInternalFormat(PixelFormat pf, bool hwGamma) -> GLenum
    {
        GLenum ret = _pixelFormats[std::to_underlying(pf)].internalFormat;

        if(!hwGamma)
            return ret;

        switch(ret)
        {
        case GL_RGB8:
            return GL_SRGB8;
        case GL_RGBA8:
            return GL_SRGB8_ALPHA8;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
        default:
            return ret;
        }
    }
    
    //-----------------------------------------------------------------------------     
    auto GLPixelUtil::getClosestOGREFormat(GLenum format) -> PixelFormat
    {
        switch(format)
        {
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
        case GL_DEPTH_COMPONENT32F:
        case GL_DEPTH_COMPONENT:
            return PixelFormat::DEPTH16;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
            return PixelFormat::DXT1;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
            return PixelFormat::DXT3;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            return PixelFormat::DXT5;
        case GL_SRGB8:
        case GL_RGB8:  // prefer native endian byte format
            return PixelFormat::BYTE_RGB;
        case GL_SRGB8_ALPHA8:
        case GL_RGBA8:  // prefer native endian byte format
            return PixelFormat::BYTE_RGBA;
        };

        for(int pf = 0; pf < std::to_underlying(PixelFormat::COUNT); pf++) {
            if(_pixelFormats[pf].internalFormat == format)
                return (PixelFormat)pf;
        }

        return PixelFormat::BYTE_RGBA;
    }
    //-----------------------------------------------------------------------------    
    auto GLPixelUtil::optionalPO2(uint32 value) -> uint32
    {
        const RenderSystemCapabilities *caps = Root::getSingleton().getRenderSystem()->getCapabilities();
        if(caps->hasCapability(Capabilities::NON_POWER_OF_2_TEXTURES))
            return value;
        else
            return Bitwise::firstPO2From(value);
    }   

    
}
