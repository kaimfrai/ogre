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
#include <cstddef>

export module Ogre.Core:ColourValue;

export import :Platform;
export import :Prerequisites;

export import <ostream>;

export
namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */

    using RGBA = uint32;
    using ARGB = uint32;
    using ABGR = uint32;
    using BGRA = uint32;

    /** Class representing colour.
        @remarks
            Colour is represented as 4 components, each of which is a
            floating-point value from 0.0 to 1.0.
        @par
            The 3 'normal' colour components are red, green and blue, a higher
            number indicating greater amounts of that component in the colour.
            The forth component is the 'alpha' value, which represents
            transparency. In this case, 0.0 is completely transparent and 1.0 is
            fully opaque.
    */
    struct ColourValue
    {
        static const ColourValue ZERO;
        static const ColourValue Black;
        static const ColourValue White;
        static const ColourValue Red;
        static const ColourValue Green;
        static const ColourValue Blue;

        float r{1.0f},g{1.0f},b{1.0f},a{1.0f};

        [[nodiscard]]
        static auto constexpr FromBytes(const uchar* byte) -> ColourValue
        {
            ColourValue val
            {   static_cast<float>(byte[0]),
                static_cast<float>(byte[1]),
                static_cast<float>(byte[2]),
                static_cast<float>(byte[3])
            };
            return val / 255;
        }

        [[nodiscard]] auto operator==(const ColourValue& rhs) const noexcept -> bool = default;

        /// @name conversions from/ to native-endian packed formats
        /// @{

        /// value packed as #PixelFormat::R8G8B8A8
        [[nodiscard]] auto getAsRGBA() const -> RGBA;

        /// value packed as #PixelFormat::A8R8G8B8
        [[nodiscard]] auto getAsARGB() const -> ARGB;

        /// value packed as #PixelFormat::B8G8R8A8
        [[nodiscard]] auto getAsBGRA() const -> BGRA;

        /// value packed as #PixelFormat::A8B8G8R8
        [[nodiscard]] auto getAsABGR() const -> ABGR;

        /// value packed as #PixelFormat::BYTE_RGBA
        [[nodiscard]] auto getAsBYTE() const -> RGBA
        {
            return getAsABGR();
        }

        /// Set value from #PixelFormat::R8G8B8A8
        void setAsRGBA(RGBA val);

        /// Set value from #PixelFormat::A8R8G8B8
        void setAsARGB(ARGB val);

        /// Set value from #PixelFormat::B8G8R8A8
        void setAsBGRA(BGRA val);

        /// Set value from #PixelFormat::A8B8G8R8
        void setAsABGR(ABGR val);
        /// @}

        /** Clamps colour value to the range [0, 1].
        */
        void saturate()
        {
            if (r < 0)
                r = 0;
            else if (r > 1)
                r = 1;

            if (g < 0)
                g = 0;
            else if (g > 1)
                g = 1;

            if (b < 0)
                b = 0;
            else if (b > 1)
                b = 1;

            if (a < 0)
                a = 0;
            else if (a > 1)
                a = 1;
        }

        /** As saturate, except that this colour value is unaffected and
            the saturated colour value is returned as a copy. */
        [[nodiscard]] auto saturateCopy() const -> ColourValue
        {
            ColourValue ret = *this;
            ret.saturate();
            return ret;
        }

        /// Array accessor operator
        auto operator [] ( const size_t i ) const -> float
        {
            assert( i < 4 );

            return *(&r+i);
        }

        /// Array accessor operator
        auto operator [] ( const size_t i ) -> float&
        {
            assert( i < 4 );

            return *(&r+i);
        }

        /// Pointer accessor for direct copying
        auto ptr() -> float*
        {
            return &r;
        }
        /// Pointer accessor for direct copying
        [[nodiscard]] auto ptr() const -> const float*
        {
            return &r;
        }

        
        // arithmetic operations
        auto operator + ( const ColourValue& rkVector ) const -> ColourValue
        {
            ColourValue kSum;

            kSum.r = r + rkVector.r;
            kSum.g = g + rkVector.g;
            kSum.b = b + rkVector.b;
            kSum.a = a + rkVector.a;

            return kSum;
        }

        auto operator - ( const ColourValue& rkVector ) const -> ColourValue
        {
            ColourValue kDiff;

            kDiff.r = r - rkVector.r;
            kDiff.g = g - rkVector.g;
            kDiff.b = b - rkVector.b;
            kDiff.a = a - rkVector.a;

            return kDiff;
        }

        auto operator * (const float fScalar ) const -> ColourValue
        {
            ColourValue kProd;

            kProd.r = fScalar*r;
            kProd.g = fScalar*g;
            kProd.b = fScalar*b;
            kProd.a = fScalar*a;

            return kProd;
        }

        auto operator * ( const ColourValue& rhs) const -> ColourValue
        {
            ColourValue kProd;

            kProd.r = rhs.r * r;
            kProd.g = rhs.g * g;
            kProd.b = rhs.b * b;
            kProd.a = rhs.a * a;

            return kProd;
        }

        auto operator / ( const ColourValue& rhs) const -> ColourValue
        {
            ColourValue kProd;

            kProd.r = r / rhs.r;
            kProd.g = g / rhs.g;
            kProd.b = b / rhs.b;
            kProd.a = a / rhs.a;

            return kProd;
        }

        auto operator / (const float fScalar ) const -> ColourValue
        {
            assert( fScalar != 0.0 );

            ColourValue kDiv;

            float fInv = 1.0f / fScalar;
            kDiv.r = r * fInv;
            kDiv.g = g * fInv;
            kDiv.b = b * fInv;
            kDiv.a = a * fInv;

            return kDiv;
        }

        friend auto operator * (const float fScalar, const ColourValue& rkVector ) -> ColourValue
        {
            ColourValue kProd;

            kProd.r = fScalar * rkVector.r;
            kProd.g = fScalar * rkVector.g;
            kProd.b = fScalar * rkVector.b;
            kProd.a = fScalar * rkVector.a;

            return kProd;
        }

        // arithmetic updates
        auto operator += ( const ColourValue& rkVector ) -> ColourValue&
        {
            r += rkVector.r;
            g += rkVector.g;
            b += rkVector.b;
            a += rkVector.a;

            return *this;
        }

        auto operator -= ( const ColourValue& rkVector ) -> ColourValue&
        {
            r -= rkVector.r;
            g -= rkVector.g;
            b -= rkVector.b;
            a -= rkVector.a;

            return *this;
        }

        auto operator *= (const float fScalar ) -> ColourValue&
        {
            r *= fScalar;
            g *= fScalar;
            b *= fScalar;
            a *= fScalar;
            return *this;
        }

        auto operator /= (const float fScalar ) -> ColourValue&
        {
            assert( fScalar != 0.0 );

            float fInv = 1.0f / fScalar;

            r *= fInv;
            g *= fInv;
            b *= fInv;
            a *= fInv;

            return *this;
        }

        /** Set a colour value from Hue, Saturation and Brightness.
        @param hue Hue value, scaled to the [0,1] range as opposed to the 0-360
        @param saturation Saturation level, [0,1]
        @param brightness Brightness level, [0,1]
        */
        void setHSB(float hue, float saturation, float brightness);

        /** Convert the current colour to Hue, Saturation and Brightness values. 
        @param hue Output hue value, scaled to the [0,1] range as opposed to the 0-360
        @param saturation Output saturation level, [0,1]
        @param brightness Output brightness level, [0,1]
        */
        void getHSB(float& hue, float& saturation, float& brightness) const;

        /** Function for writing to a stream.
        */
        inline friend auto operator <<
            ( std::ostream& o, const ColourValue& c ) -> std::ostream&
        {
            o << "ColourValue{" << c.r << ", " << c.g << ", " << c.b << ", " << c.a << "}";
            return o;
        }

    };
    /** @} */
    /** @} */

} // namespace
static_assert(std::is_aggregate_v<Ogre::ColourValue>);
static_assert(std::is_standard_layout_v<Ogre::ColourValue>);
