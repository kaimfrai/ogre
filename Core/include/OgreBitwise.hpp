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
export module Ogre.Core:Bitwise;

export import :Prerequisites;

export import <bit>;

export
namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Math
    *  @{
    */

    /** Class for manipulating bit patterns.
    */
    class Bitwise {
    public:
        /** Returns value with reversed bytes order.
        */
        static inline auto bswap16(uint16 arg) -> uint16
        {
            return ::std::byteswap<uint16>(arg);
        }
        /** Returns value with reversed bytes order.
        */
        static inline auto bswap32(uint32 arg) -> uint32
        {
            return  ::std::byteswap<uint32>(arg);
        }
        /** Returns value with reversed bytes order.
        */
        static inline auto bswap64(uint64 arg) -> uint64
        {
            return ::std::byteswap<uint64>(arg);
        }

        /** Reverses byte order of buffer. Use bswap16/32/64 instead if possible.
        */
        static inline void bswapBuffer(void * pData, size_t size)
        {
            char swapByte;
            for(char *p0 = (char*)pData, *p1 = p0 + size - 1; p0 < p1; ++p0, --p1)
            {
                swapByte = *p0;
                *p0 = *p1;
                *p1 = swapByte;
            }
        }
        /** Reverses byte order of chunks in buffer, where 'size' is size of one chunk.
        */
        static inline void bswapChunks(void * pData, size_t size, size_t count)
        {
            for(size_t c = 0; c < count; ++c)
            {
                char swapByte;
                for(char *p0 = (char*)pData + c * size, *p1 = p0 + size - 1; p0 < p1; ++p0, --p1)
                {
                    swapByte = *p0;
                    *p0 = *p1;
                    *p1 = swapByte;
                }
            }
        }

        /** Returns the most significant bit set in a value.
        */
        static inline auto mostSignificantBitSet(unsigned int value) -> unsigned int
        {
            //                                     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F
            static const unsigned char msb[16] = { 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };

            unsigned int result = 0;
            if(value & 0xFFFF0000) { result += 16;value >>= 16; }
            if(value & 0x0000FF00) { result += 8; value >>= 8; }
            if(value & 0x000000F0) { result += 4; value >>= 4; }
            result += msb[value];
            return result-1;
        }
        /** Returns the closest power-of-two number greater or equal to value.
            @note 0 and 1 are powers of two, so 
                firstPO2From(0)==0 and firstPO2From(1)==1.
        */
        static inline auto firstPO2From(uint32 n) -> uint32
        {
            --n;            
            n |= n >> 16;
            n |= n >> 8;
            n |= n >> 4;
            n |= n >> 2;
            n |= n >> 1;
            ++n;
            return n;
        }
        /** Determines whether the number is power-of-two or not.
            @note 0 and 1 are tread as power of two.
        */
        template<typename T>
        static inline auto isPO2(T n) -> bool
        {
            return (n & (n-1)) == 0;
        }
        /** Returns the number of bits a pattern must be shifted right by to
            remove right-hand zeros.
        */
        template<typename T>
        static inline auto getBitShift(T mask) -> unsigned int
        {
            if (mask == 0)
                return 0;

            unsigned int result = 0;
            while ((mask & 1) == 0) {
                ++result;
                mask >>= 1;
            }
            return result;
        }

        /** Takes a value with a given src bit mask, and produces another
            value with a desired bit mask.
            @remarks
                This routine is useful for colour conversion.
        */
        template<typename SrcT, typename DestT>
        static inline auto convertBitPattern(SrcT srcValue, SrcT srcBitMask, DestT destBitMask) -> DestT
        {
            // Mask off irrelevant source value bits (if any)
            srcValue = srcValue & srcBitMask;

            // Shift source down to bottom of DWORD
            const unsigned int srcBitShift = getBitShift(srcBitMask);
            srcValue >>= srcBitShift;

            // Get max value possible in source from srcMask
            const SrcT srcMax = srcBitMask >> srcBitShift;

            // Get max available in dest
            const unsigned int destBitShift = getBitShift(destBitMask);
            const DestT destMax = destBitMask >> destBitShift;

            // Scale source value into destination, and shift back
            DestT destValue = (srcValue * destMax) / srcMax;
            return (destValue << destBitShift);
        }

        /**
         * Convert N bit colour channel value to P bits. It fills P bits with the
         * bit pattern repeated. (this is /((1<<n)-1) in fixed point)
         */
        static inline auto fixedToFixed(uint32 value, unsigned int n, unsigned int p) -> unsigned int 
        {
            if(n > p) 
            {
                // Less bits required than available; this is easy
                value >>= n-p;
            } 
            else if(n < p)
            {
                // More bits required than are there, do the fill
                // Use old fashioned division, probably better than a loop
                if(value == 0)
                        value = 0;
                else if(value == (static_cast<unsigned int>(1)<<n)-1)
                        value = (1<<p)-1;
                else    value = value*(1<<p)/((1<<n)-1);
            }
            return value;    
        }

        /**
         * Convert floating point colour channel value between 0.0 and 1.0 (otherwise clamped) 
         * to integer of a certain number of bits. Works for any value of bits between 0 and 31.
         */
        static inline auto floatToFixed(const float value, const unsigned int bits) -> unsigned int
        {
            if(value <= 0.0f) return 0;
            else if (value >= 1.0f) return (1<<bits)-1;
            else return (unsigned int)(value * float(1<<bits));
        }

        /**
         * Fixed point to float
         */
        static inline auto fixedToFloat(unsigned value, unsigned int bits) -> float
        {
            return (float)value/(float)((1<<bits)-1);
        }

        /**
         * Write a n*8 bits integer value to memory in native endian.
         */
        static inline void intWrite(void *dest, const int n, const unsigned int value)
        {
            switch(n) {
                case 1:
                    ((uint8*)dest)[0] = (uint8)value;
                    break;
                case 2:
                    ((uint16*)dest)[0] = (uint16)value;
                    break;
                case 3:
                    ((uint8*)dest)[2] = (uint8)((value >> 16) & 0xFF);
                    ((uint8*)dest)[1] = (uint8)((value >> 8) & 0xFF);
                    ((uint8*)dest)[0] = (uint8)(value & 0xFF);
                    break;
                case 4:
                    ((uint32*)dest)[0] = (uint32)value;                
                    break;                
            }        
        }
        /**
         * Read a n*8 bits integer value to memory in native endian.
         */
        static inline auto intRead(const void *src, int n) -> unsigned int {
            switch(n) {
                case 1:
                    return ((const uint8*)src)[0];
                case 2:
                    return ((const uint16*)src)[0];
                case 3:

                    return ((uint32)((const uint8*)src)[0])|
                            ((uint32)((const uint8*)src)[1]<<8)|
                            ((uint32)((const uint8*)src)[2]<<16);
                case 4:
                    return ((const uint32*)src)[0];
            } 
            return 0; // ?
        }

        /** Convert a float32 to a float16 (NV_half_float)
            Courtesy of OpenEXR
        */
        static inline auto floatToHalf(float i) -> uint16
        {
            return floatToHalfI(::std::bit_cast<uint32>(i));
        }
        /** Converts float in uint32 format to a a half in uint16 format
        */
        static inline auto floatToHalfI(uint32 i) -> uint16
        {
            int s =  (i >> 16) & 0x00008000;
            int e = ((i >> 23) & 0x000000ff) - (127 - 15);
            int m =   i        & 0x007fffff;
        
            if (e <= 0)
            {
                if (e < -10)
                {
                    return 0;
                }
                m = (m | 0x00800000) >> (1 - e);
        
                return static_cast<uint16>(s | (m >> 13));
            }
            else if (e == 0xff - (127 - 15))
            {
                if (m == 0) // Inf
                {
                    return static_cast<uint16>(s | 0x7c00);
                } 
                else    // NAN
                {
                    m >>= 13;
                    return static_cast<uint16>(s | 0x7c00 | m | (m == 0));
                }
            }
            else
            {
                if (e > 30) // Overflow
                {
                    return static_cast<uint16>(s | 0x7c00);
                }
        
                return static_cast<uint16>(s | (e << 10) | (m >> 13));
            }
        }
        
        /**
         * Convert a float16 (NV_half_float) to a float32
         * Courtesy of OpenEXR
         */
        static inline auto halfToFloat(uint16 y) -> float
        {
            return ::std::bit_cast<float>(halfToFloatI(y));
        }
        /** Converts a half in uint16 format to a float
            in uint32 format
         */
        static inline auto halfToFloatI(uint16 y) -> uint32
        {
            int s = (y >> 15) & 0x00000001;
            int e = (y >> 10) & 0x0000001f;
            int m =  y        & 0x000003ff;
        
            if (e == 0)
            {
                if (m == 0) // Plus or minus zero
                {
                    return s << 31;
                }
                else // Denormalized number -- renormalize it
                {
                    while (!(m & 0x00000400))
                    {
                        m <<= 1;
                        e -=  1;
                    }
        
                    e += 1;
                    m &= ~0x00000400;
                }
            }
            else if (e == 31)
            {
                if (m == 0) // Inf
                {
                    return (s << 31) | 0x7f800000;
                }
                else // NaN
                {
                    return (s << 31) | 0x7f800000 | (m << 13);
                }
            }
        
            e = e + (127 - 15);
            m = m << 13;
        
            return (s << 31) | (e << 23) | m;
        }
         

    };
    /** @} */
    /** @} */

}
