module;

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
/** Internal include file -- do not use externally */

module Ogre.Core:PixelConversions;

import :PixelFormat;

import <utility>;

//using namespace Ogre;
template<typename T>

auto constexpr FMTCONVERTERID(T from, T to) -> T
{
    return static_cast<T>
    (   (std::to_underlying(from) << 8)
    bitor
        std::to_underlying(to)
    );
}

/** \addtogroup Core
*  @{
*/
/** \addtogroup Image
*  @{
*/
/**
 * Convert a box of pixel from one type to another. Who needs automatic code 
 * generation when we have C++ templates and the policy design pattern.
 * 
 * @remarks Policy class to facilitate pixel-to-pixel conversion. This class
 *    has at least two typedefs: SrcType and DstType. SrcType is the source element type,
 *    dstType is the destination element type. It also has a static method, pixelConvert, that
 *    converts a srcType into a dstType.
 */
template <class U> struct PixelBoxConverter 
{
    static const auto ID = U::ID;
    static void conversion(const Ogre::PixelBox &src, const Ogre::PixelBox &dst)
    {
        typename U::SrcType *srcptr = reinterpret_cast<typename U::SrcType*>(src.data)
            + (src.left + src.top * src.rowPitch + src.front * src.slicePitch);
        typename U::DstType *dstptr = reinterpret_cast<typename U::DstType*>(dst.data)
            + (dst.left + dst.top * dst.rowPitch + dst.front * dst.slicePitch);
        const size_t srcSliceSkip = src.getSliceSkip();
        const size_t dstSliceSkip = dst.getSliceSkip();
        const size_t k = src.right - src.left;
        for(size_t z=src.front; z<src.back; z++) 
        {
            for(size_t y=src.top; y<src.bottom; y++)
            {
                for(size_t x=0; x<k; x++)
                {
                    dstptr[x] = U::pixelConvert(srcptr[x]);
                }
                srcptr += src.rowPitch;
                dstptr += dst.rowPitch;
            }
            srcptr += srcSliceSkip;
            dstptr += dstSliceSkip;
        }    
    }
};

template <typename T, typename U, Ogre::PixelFormat id> struct PixelConverter {
    static const auto constexpr ID = id;
    using SrcType = T;
    using DstType = U;    
    
    //inline static DstType pixelConvert(const SrcType &inp);
};
/** Type for PixelFormat::R8G8B8/PixelFormat::B8G8R8 */

struct Col3b {
    Col3b(unsigned int a, unsigned int b, unsigned int c): 
        x((Ogre::uint8)a), y((Ogre::uint8)b), z((Ogre::uint8)c) { }
    Ogre::uint8 x,y,z;
};
/** Type for PixelFormat::FLOAT32_RGB */

struct Col3f {
    Col3f(float inR, float inG, float inB):
        r(inR), g(inG), b(inB) { }
    float r,g,b;
};
/** Type for PixelFormat::FLOAT32_RGBA */

struct Col4f {
    Col4f(float inR, float inG, float inB, float inA):
        r(inR), g(inG), b(inB), a(inA) { }
    float r,g,b,a;
};

struct A8R8G8B8toA8B8G8R8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::A8R8G8B8, Ogre::PixelFormat::A8B8G8R8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x000000FF)<<16)|(inp&0xFF00FF00)|((inp&0x00FF0000)>>16);
    }
};

struct A8R8G8B8toB8G8R8A8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::A8R8G8B8, Ogre::PixelFormat::B8G8R8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x000000FF)<<24)|((inp&0x0000FF00)<<8)|((inp&0x00FF0000)>>8)|((inp&0xFF000000)>>24);
    }
};

struct A8R8G8B8toR8G8B8A8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::A8R8G8B8, Ogre::PixelFormat::R8G8B8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x00FFFFFF)<<8)|((inp&0xFF000000)>>24);
    }
};

struct A8B8G8R8toA8R8G8B8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::A8B8G8R8, Ogre::PixelFormat::A8R8G8B8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x000000FF)<<16)|(inp&0xFF00FF00)|((inp&0x00FF0000)>>16);
    }
};

struct A8B8G8R8toB8G8R8A8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::A8B8G8R8, Ogre::PixelFormat::B8G8R8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x00FFFFFF)<<8)|((inp&0xFF000000)>>24);
    }
};

struct A8B8G8R8toR8G8B8A8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::A8B8G8R8, Ogre::PixelFormat::R8G8B8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x000000FF)<<24)|((inp&0x0000FF00)<<8)|((inp&0x00FF0000)>>8)|((inp&0xFF000000)>>24);
    }
};

struct B8G8R8A8toA8R8G8B8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::B8G8R8A8, Ogre::PixelFormat::A8R8G8B8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x000000FF)<<24)|((inp&0x0000FF00)<<8)|((inp&0x00FF0000)>>8)|((inp&0xFF000000)>>24);
    }
};

struct B8G8R8A8toA8B8G8R8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::B8G8R8A8, Ogre::PixelFormat::A8B8G8R8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x000000FF)<<24)|((inp&0xFFFFFF00)>>8);
    }
};

struct B8G8R8A8toR8G8B8A8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::B8G8R8A8, Ogre::PixelFormat::R8G8B8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x0000FF00)<<16)|(inp&0x00FF00FF)|((inp&0xFF000000)>>16);
    }
};

struct R8G8B8A8toA8R8G8B8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::R8G8B8A8, Ogre::PixelFormat::A8R8G8B8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x000000FF)<<24)|((inp&0xFFFFFF00)>>8);
    }
};

struct R8G8B8A8toA8B8G8R8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::R8G8B8A8, Ogre::PixelFormat::A8B8G8R8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x000000FF)<<24)|((inp&0x0000FF00)<<8)|((inp&0x00FF0000)>>8)|((inp&0xFF000000)>>24);
    }
};

struct R8G8B8A8toB8G8R8A8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::R8G8B8A8, Ogre::PixelFormat::B8G8R8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x0000FF00)<<16)|(inp&0x00FF00FF)|((inp&0xFF000000)>>16);
    }
};

struct A8B8G8R8toR8: public PixelConverter <Ogre::uint32, Ogre::uint8, FMTCONVERTERID(Ogre::PixelFormat::A8B8G8R8, Ogre::PixelFormat::R8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return (Ogre::uint8)(inp&0x000000FF);
    }
};

struct R8toA8B8G8R8: public PixelConverter <Ogre::uint8, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::R8, Ogre::PixelFormat::A8B8G8R8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return 0xFF000000|((unsigned int)inp);
    }
};

struct A8R8G8B8toR8: public PixelConverter <Ogre::uint32, Ogre::uint8, FMTCONVERTERID(Ogre::PixelFormat::A8R8G8B8, Ogre::PixelFormat::R8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return (Ogre::uint8)((inp&0x00FF0000)>>16);
    }
};

struct R8toA8R8G8B8: public PixelConverter <Ogre::uint8, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::R8, Ogre::PixelFormat::A8R8G8B8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return 0xFF000000|(((unsigned int)inp)<<16);
    }
};

struct B8G8R8A8toR8: public PixelConverter <Ogre::uint32, Ogre::uint8, FMTCONVERTERID(Ogre::PixelFormat::B8G8R8A8, Ogre::PixelFormat::R8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return (Ogre::uint8)((inp&0x0000FF00)>>8);
    }
};

struct R8toB8G8R8A8: public PixelConverter <Ogre::uint8, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::R8, Ogre::PixelFormat::B8G8R8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return 0x000000FF|(((unsigned int)inp)<<8);
    }
};

struct A8B8G8R8toL8: public PixelConverter <Ogre::uint32, Ogre::uint8, FMTCONVERTERID(Ogre::PixelFormat::A8B8G8R8, Ogre::PixelFormat::L8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return (Ogre::uint8)(inp&0x000000FF);
    }
};

struct L8toA8B8G8R8: public PixelConverter <Ogre::uint8, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::L8, Ogre::PixelFormat::A8B8G8R8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return 0xFF000000|(((unsigned int)inp)<<0)|(((unsigned int)inp)<<8)|(((unsigned int)inp)<<16);
    }
};

struct A8R8G8B8toL8: public PixelConverter <Ogre::uint32, Ogre::uint8, FMTCONVERTERID(Ogre::PixelFormat::A8R8G8B8, Ogre::PixelFormat::L8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return (Ogre::uint8)((inp&0x00FF0000)>>16);
    }
};

struct L8toA8R8G8B8: public PixelConverter <Ogre::uint8, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::L8, Ogre::PixelFormat::A8R8G8B8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return 0xFF000000|(((unsigned int)inp)<<0)|(((unsigned int)inp)<<8)|(((unsigned int)inp)<<16);
    }
};

struct B8G8R8A8toL8: public PixelConverter <Ogre::uint32, Ogre::uint8, FMTCONVERTERID(Ogre::PixelFormat::B8G8R8A8, Ogre::PixelFormat::L8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return (Ogre::uint8)((inp&0x0000FF00)>>8);
    }
};

struct L8toB8G8R8A8: public PixelConverter <Ogre::uint8, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::L8, Ogre::PixelFormat::B8G8R8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return 0x000000FF|(((unsigned int)inp)<<8)|(((unsigned int)inp)<<16)|(((unsigned int)inp)<<24);
    }
};

struct L8toL16: public PixelConverter <Ogre::uint8, Ogre::uint16, FMTCONVERTERID(Ogre::PixelFormat::L8, Ogre::PixelFormat::L16)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return (Ogre::uint16)((((unsigned int)inp)<<8)|(((unsigned int)inp)));
    }
};

struct L16toL8: public PixelConverter <Ogre::uint16, Ogre::uint8, FMTCONVERTERID(Ogre::PixelFormat::L16, Ogre::PixelFormat::L8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return (Ogre::uint8)(inp>>8);
    }
};

struct R8G8B8toB8G8R8: public PixelConverter <Col3b, Col3b, FMTCONVERTERID(Ogre::PixelFormat::R8G8B8, Ogre::PixelFormat::B8G8R8)>
{
    inline static auto pixelConvert(const SrcType &inp) -> DstType
    {
        return {inp.z, inp.y, inp.x};
    }  
};

struct B8G8R8toR8G8B8: public PixelConverter <Col3b, Col3b, FMTCONVERTERID(Ogre::PixelFormat::B8G8R8, Ogre::PixelFormat::R8G8B8)>
{
    inline static auto pixelConvert(const SrcType &inp) -> DstType
    {
        return {inp.z, inp.y, inp.x};
    }  
};

// X8Y8Z8 ->  X8<<xshift Y8<<yshift Z8<<zshift A8<<ashift
template <Ogre::PixelFormat id, unsigned int xshift, unsigned int yshift, unsigned int zshift, unsigned int ashift> struct Col3btoUint32swizzler:
    public PixelConverter <Col3b, Ogre::uint32, id>
{
    inline static auto pixelConvert(const Col3b &inp) -> Ogre::uint32
    {
        return (0xFF<<ashift) | (((unsigned int)inp.x)<<zshift) | (((unsigned int)inp.y)<<yshift) | (((unsigned int)inp.z)<<xshift);
    }
};

struct R8G8B8toA8R8G8B8: public Col3btoUint32swizzler<FMTCONVERTERID(Ogre::PixelFormat::R8G8B8, Ogre::PixelFormat::A8R8G8B8), 16, 8, 0, 24> { };

struct B8G8R8toA8R8G8B8: public Col3btoUint32swizzler<FMTCONVERTERID(Ogre::PixelFormat::B8G8R8, Ogre::PixelFormat::A8R8G8B8), 0, 8, 16, 24> { };

struct R8G8B8toA8B8G8R8: public Col3btoUint32swizzler<FMTCONVERTERID(Ogre::PixelFormat::R8G8B8, Ogre::PixelFormat::A8B8G8R8), 0, 8, 16, 24> { };

struct B8G8R8toA8B8G8R8: public Col3btoUint32swizzler<FMTCONVERTERID(Ogre::PixelFormat::B8G8R8, Ogre::PixelFormat::A8B8G8R8), 16, 8, 0, 24> { };

struct R8G8B8toB8G8R8A8: public Col3btoUint32swizzler<FMTCONVERTERID(Ogre::PixelFormat::R8G8B8, Ogre::PixelFormat::B8G8R8A8), 8, 16, 24, 0> { };

struct B8G8R8toB8G8R8A8: public Col3btoUint32swizzler<FMTCONVERTERID(Ogre::PixelFormat::B8G8R8, Ogre::PixelFormat::B8G8R8A8), 24, 16, 8, 0> { };

struct A8R8G8B8toR8G8B8: public PixelConverter <Ogre::uint32, Col3b, FMTCONVERTERID(Ogre::PixelFormat::A8R8G8B8, Ogre::PixelFormat::BYTE_RGB)>
{
    inline static auto pixelConvert(Ogre::uint32 inp) -> DstType
    {
        return {(Ogre::uint8)((inp>>16)&0xFF), (Ogre::uint8)((inp>>8)&0xFF), (Ogre::uint8)((inp>>0)&0xFF)};
    }
};

struct A8R8G8B8toB8G8R8: public PixelConverter <Ogre::uint32, Col3b, FMTCONVERTERID(Ogre::PixelFormat::A8R8G8B8, Ogre::PixelFormat::BYTE_BGR)>
{
    inline static auto pixelConvert(Ogre::uint32 inp) -> DstType
    {
        return {(Ogre::uint8)((inp>>0)&0xFF), (Ogre::uint8)((inp>>8)&0xFF), (Ogre::uint8)((inp>>16)&0xFF)};
    }
};

// Only conversions from X8R8G8B8 to formats with alpha need to be defined, the rest is implicitly the same
// as A8R8G8B8
struct X8R8G8B8toA8R8G8B8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::X8R8G8B8, Ogre::PixelFormat::A8R8G8B8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return inp | 0xFF000000;
    }
};

struct X8R8G8B8toA8B8G8R8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::X8R8G8B8, Ogre::PixelFormat::A8B8G8R8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x0000FF)<<16)|((inp&0xFF0000)>>16)|(inp&0x00FF00)|0xFF000000;
    }
};

struct X8R8G8B8toB8G8R8A8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::X8R8G8B8, Ogre::PixelFormat::B8G8R8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x0000FF)<<24)|((inp&0xFF0000)>>8)|((inp&0x00FF00)<<8)|0x000000FF;
    }
};

struct X8R8G8B8toR8G8B8A8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::X8R8G8B8, Ogre::PixelFormat::R8G8B8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0xFFFFFF)<<8)|0x000000FF;
    }
};

// X8B8G8R8
struct X8B8G8R8toA8R8G8B8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::X8B8G8R8, Ogre::PixelFormat::A8R8G8B8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x0000FF)<<16)|((inp&0xFF0000)>>16)|(inp&0x00FF00)|0xFF000000;
    }
};

struct X8B8G8R8toA8B8G8R8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::X8B8G8R8, Ogre::PixelFormat::A8B8G8R8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return inp | 0xFF000000;
    }
};

struct X8B8G8R8toB8G8R8A8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::X8B8G8R8, Ogre::PixelFormat::B8G8R8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0xFFFFFF)<<8)|0x000000FF;
    }
};

struct X8B8G8R8toR8G8B8A8: public PixelConverter <Ogre::uint32, Ogre::uint32, FMTCONVERTERID(Ogre::PixelFormat::X8B8G8R8, Ogre::PixelFormat::R8G8B8A8)>
{
    inline static auto pixelConvert(SrcType inp) -> DstType
    {
        return ((inp&0x0000FF)<<24)|((inp&0xFF0000)>>8)|((inp&0x00FF00)<<8)|0x000000FF;
    }
};

#define CASECONVERTER(type) case type::ID : PixelBoxConverter<type>::conversion(src, dst); return 1;
inline auto doOptimizedConversion(const Ogre::PixelBox &src, const Ogre::PixelBox &dst) -> int
{;
    switch(FMTCONVERTERID(src.format, dst.format))
    {
        // Register converters here
        CASECONVERTER(A8R8G8B8toA8B8G8R8);
        CASECONVERTER(A8R8G8B8toB8G8R8A8);
        CASECONVERTER(A8R8G8B8toR8G8B8A8);
        CASECONVERTER(A8B8G8R8toA8R8G8B8);
        CASECONVERTER(A8B8G8R8toB8G8R8A8);
        CASECONVERTER(A8B8G8R8toR8G8B8A8);
        CASECONVERTER(B8G8R8A8toA8R8G8B8);
        CASECONVERTER(B8G8R8A8toA8B8G8R8);
        CASECONVERTER(B8G8R8A8toR8G8B8A8);
        CASECONVERTER(R8G8B8A8toA8R8G8B8);
        CASECONVERTER(R8G8B8A8toA8B8G8R8);
        CASECONVERTER(R8G8B8A8toB8G8R8A8);
        CASECONVERTER(A8B8G8R8toR8);
        CASECONVERTER(R8toA8B8G8R8);
        CASECONVERTER(A8R8G8B8toR8);
        CASECONVERTER(R8toA8R8G8B8);
        CASECONVERTER(B8G8R8A8toR8);
        CASECONVERTER(R8toB8G8R8A8);
        CASECONVERTER(A8B8G8R8toL8);
        CASECONVERTER(L8toA8B8G8R8);
        CASECONVERTER(A8R8G8B8toL8);
        CASECONVERTER(L8toA8R8G8B8);
        CASECONVERTER(B8G8R8A8toL8);
        CASECONVERTER(L8toB8G8R8A8);
        CASECONVERTER(L8toL16);
        CASECONVERTER(L16toL8);
        CASECONVERTER(B8G8R8toR8G8B8);
        CASECONVERTER(R8G8B8toB8G8R8);
        CASECONVERTER(R8G8B8toA8R8G8B8);
        CASECONVERTER(B8G8R8toA8R8G8B8);
        CASECONVERTER(R8G8B8toA8B8G8R8);
        CASECONVERTER(B8G8R8toA8B8G8R8);
        CASECONVERTER(R8G8B8toB8G8R8A8);
        CASECONVERTER(B8G8R8toB8G8R8A8);
        CASECONVERTER(A8R8G8B8toR8G8B8);
        CASECONVERTER(A8R8G8B8toB8G8R8);
        CASECONVERTER(X8R8G8B8toA8R8G8B8);
        CASECONVERTER(X8R8G8B8toA8B8G8R8);
        CASECONVERTER(X8R8G8B8toB8G8R8A8);
        CASECONVERTER(X8R8G8B8toR8G8B8A8);
        CASECONVERTER(X8B8G8R8toA8R8G8B8);
        CASECONVERTER(X8B8G8R8toA8B8G8R8);
        CASECONVERTER(X8B8G8R8toB8G8R8A8);
        CASECONVERTER(X8B8G8R8toR8G8B8A8);

        default:
            return 0;
    }
}
#undef CASECONVERTER
/** @} */
/** @} */
