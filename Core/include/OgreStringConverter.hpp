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

#include <cstddef>

export module Ogre.Core:StringConverter;

export import :ColourValue;
export import :Common;
export import :Math;
export import :Matrix3;
export import :Matrix4;
export import :Platform;
export import :Prerequisites;
export import :Quaternion;
export import :StringVector;
export import :Vector;

export import <filesystem>;
export import <format>;
export import <iosfwd>;
export import <locale>;
export import <string>;
export import <type_traits>;
export import <utility>;

export
namespace Ogre {

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */
    /** Class for converting the core Ogre data types to/from Strings.
    @remarks
        The code for converting values to and from strings is here as a separate
        class to avoid coupling String to other datatypes (and vice-versa) which reduces
        compilation dependency: important given how often the core types are used.
    @par
        This class is mainly used for parsing settings in text files. External applications
        can also use it to interface with classes which use the StringInterface template
        class.
    @par
        The String formats of each of the major types is listed with the methods. The basic types
        like int and Real just use the underlying C runtime library atof and atoi family methods,
        however custom types like Vector3, ColourValue and Matrix4 are also supported by this class
        using custom formats.
    */
    class StringConverter
    {
    public:
        static auto toString(int32 val) -> String { return std::to_string(val); };
        static auto toString(uint32 val) -> String { return std::to_string(val); };
        static auto toString(unsigned long val) -> String { return std::to_string(val); };
        static auto toString(unsigned long long val) -> String { return std::to_string(val); };
        static auto toString(long val) -> String { return std::to_string(val); };

        /** Converts a float to a String. */
        static auto toString(float val, unsigned short precision = 6,
                               unsigned short width = 0, char fill = ' ',
                               std::ios::fmtflags flags = std::ios::fmtflags(0)) -> String;

        /** Converts a double to a String. */
        static auto toString(double val, unsigned short precision = 6,
                               unsigned short width = 0, char fill = ' ',
                               std::ios::fmtflags flags = std::ios::fmtflags(0)) -> String;

        /** Converts a long double to a String. */
        static auto toString(long double val, unsigned short precision = 6,
                               unsigned short width = 0, char fill = ' ',
                               std::ios::fmtflags flags = std::ios::fmtflags(0)) -> String;

        /** Converts a Radian to a String. */
        static auto toString(Radian val, unsigned short precision = 6, 
            unsigned short width = 0, char fill = ' ', 
            std::ios::fmtflags flags = std::ios::fmtflags(0)) -> String
        {
            return toString(val.valueAngleUnits(), precision, width, fill, flags);
        }
        /** Converts a Degree to a String. */
        static auto toString(Degree val, unsigned short precision = 6, 
            unsigned short width = 0, char fill = ' ', 
            std::ios::fmtflags flags = std::ios::fmtflags(0)) -> String
        {
            return toString(val.valueAngleUnits(), precision, width, fill, flags);
        }

        /** Converts a boolean to a String.
        @param val
        @param yesNo If set to true, result is 'yes' or 'no' instead of 'true' or 'false'
        */
        static auto toString(bool val, bool yesNo = false) -> String;
        /** Converts a Vector2 to a String. 
        @remarks
            Format is "x y" (i.e. 2x Real values, space delimited)
        */
        static auto toString(const Vector2& val) -> String;
        /** Converts a Vector3 to a String. 
        @remarks
            Format is "x y z" (i.e. 3x Real values, space delimited)
        */
        static auto toString(const Vector3& val) -> String;
        /** Converts a Vector4 to a String. 
        @remarks
            Format is "x y z w" (i.e. 4x Real values, space delimited)
        */
        static auto toString(const Vector4& val) -> String;
        /** Converts a Matrix3 to a String. 
        @remarks
            Format is "00 01 02 10 11 12 20 21 22" where '01' means row 0 column 1 etc.
        */
        static auto toString(const Matrix3& val) -> String;
        /** Converts a Matrix4 to a String. 
        @remarks
            Format is "00 01 02 03 10 11 12 13 20 21 22 23 30 31 32 33" where 
            '01' means row 0 column 1 etc.
        */
        static auto toString(const Matrix4& val) -> String;
        /** Converts a Quaternion to a String. 
        @remarks
            Format is "w x y z" (i.e. 4x Real values, space delimited)
        */
        static auto toString(const Quaternion& val) -> String;
        /** Converts a ColourValue to a String. 
        @remarks
            Format is "r g b a" (i.e. 4x Real values, space delimited). 
        */
        static auto toString(const ColourValue& val) -> String;
        /** Converts a StringVector to a string.
        @remarks
            Strings must not contain spaces since space is used as a delimiter in
            the output.
        */
        static auto toString(const StringVector& val) -> String;

        template<typename T>
        static auto toString(T v) -> decltype(toString(std::to_underlying(v)))
        {
            return toString(std::to_underlying(v));
        }

        /** Converts a String to a basic value type
            @return whether the conversion was successful
        */
        static auto parse(std::string_view str, ColourValue& v) -> bool;
        static auto parse(std::string_view str, Quaternion& v) -> bool;
        static auto parse(std::string_view str, Matrix4& v) -> bool;
        static auto parse(std::string_view str, Matrix3& v) -> bool;
        static auto parse(std::string_view str, Vector4& v) -> bool;
        static auto parse(std::string_view str, Vector3& v) -> bool;
        static auto parse(std::string_view str, Vector2& v) -> bool;
        static auto parse(std::string_view str, int32& v) -> bool;
        static auto parse(std::string_view str, uint32& v) -> bool;
        static auto parse(std::string_view str, int64& v) -> bool;
        // provide both long long and long to catch size_t on all platforms
        static auto parse(std::string_view str, unsigned long& v) -> bool;
        static auto parse(std::string_view str, unsigned long long& v) -> bool;
        static auto parse(std::string_view str, bool& v) -> bool;
        static auto parse(std::string_view str, double& v) -> bool;
        static auto parse(std::string_view str, float& v) -> bool;

        template<typename T>
        static auto parse(std::string_view str, T& v) -> bool
            requires requires(std::underlying_type_t<T>& u)
            {
                parse(str, u);
            }
        {
            auto u = std::to_underlying(v);
            auto result = parse(str, u);
            v = static_cast<T>(u);
            return result;
        }

        /** Converts a String to a Real. 
        @return
            0.0 if the value could not be parsed, otherwise the Real version of the String.
        */
        static auto parseReal(std::string_view val, Real defaultValue = 0) -> Real
        {
            Real ret;
            return parse(val, ret) ? ret : defaultValue;
        }
        /** Converts a String to a Angle. 
        @return
            0.0 if the value could not be parsed, otherwise the Angle version of the String.
        */
        static auto parseAngle(std::string_view val, Radian defaultValue = Radian{0}) -> Radian {
            return Angle{parseReal(val, defaultValue.valueRadians())};
        }
        /** Converts a String to a whole number. 
        @return
            0.0 if the value could not be parsed, otherwise the numeric version of the String.
        */
        static auto parseInt(std::string_view val, int32 defaultValue = 0) -> int32
        {
            int32 ret;
            return parse(val, ret) ? ret : defaultValue;
        }
        /** Converts a String to a whole number. 
        @return
            0.0 if the value could not be parsed, otherwise the numeric version of the String.
        */
        static auto parseUnsignedInt(std::string_view val, uint32 defaultValue = 0) -> uint32
        {
            uint32 ret;
            return parse(val, ret) ? ret : defaultValue;
        }

        /** Converts a String to size_t. 
        @return
            defaultValue if the value could not be parsed, otherwise the numeric version of the String.
        */
        static auto parseSizeT(std::string_view val, size_t defaultValue = 0) -> size_t
        {
            size_t ret;
            return parse(val, ret) ? ret : defaultValue;
        }
        /** Converts a String to a boolean. 
        @remarks
            Returns true if case-insensitive match of the start of the string
            matches "true", "yes", "1", or "on", false if "false", "no", "0" 
            or "off".
        */
        static auto parseBool(std::string_view val, bool defaultValue = false) -> bool
        {
            bool ret;
            return parse(val, ret) ? ret : defaultValue;
        }
        /** Parses a Vector2 out of a String.
        @remarks
            Format is "x y" ie. 2 Real components, space delimited. Failure to parse returns
            Vector2::ZERO.
        */
        static auto parseVector2(std::string_view val, const Vector2& defaultValue = Vector2::ZERO) -> Vector2
        {
            Vector2 ret;
            return parse(val, ret) ? ret : defaultValue;
        }
        /** Parses a Vector3 out of a String.
        @remarks
            Format is "x y z" ie. 3 Real components, space delimited. Failure to parse returns
            Vector3::ZERO.
        */
        static auto parseVector3(std::string_view val, const Vector3& defaultValue = Vector3::ZERO) -> Vector3
        {
            Vector3 ret;
            return parse(val, ret) ? ret : defaultValue;
        }
        /** Parses a Vector4 out of a String.
        @remarks
            Format is "x y z w" ie. 4 Real components, space delimited. Failure to parse returns
            Vector4::ZERO.
        */
        static auto parseVector4(std::string_view val, const Vector4& defaultValue = Vector4::ZERO) -> Vector4
        {
            Vector4 ret;
            return parse(val, ret) ? ret : defaultValue;
        }
        /** Parses a Matrix3 out of a String.
        @remarks
            Format is "00 01 02 10 11 12 20 21 22" where '01' means row 0 column 1 etc.
            Failure to parse returns Matrix3::IDENTITY.
        */
        static auto parseMatrix3(std::string_view val, const Matrix3& defaultValue = Matrix3::IDENTITY) -> Matrix3
        {
            Matrix3 ret;
            return parse(val, ret) ? ret : defaultValue;
        }
        /** Parses a Matrix4 out of a String.
        @remarks
            Format is "00 01 02 03 10 11 12 13 20 21 22 23 30 31 32 33" where 
            '01' means row 0 column 1 etc. Failure to parse returns Matrix4::IDENTITY.
        */
        static auto parseMatrix4(std::string_view val, const Matrix4& defaultValue = Matrix4::IDENTITY) -> Matrix4
        {
            Matrix4 ret;
            return parse(val, ret) ? ret : defaultValue;
        }
        /** Parses a Quaternion out of a String. 
        @remarks
            Format is "w x y z" (i.e. 4x Real values, space delimited). 
            Failure to parse returns Quaternion::IDENTITY.
        */
        static auto parseQuaternion(std::string_view val, const Quaternion& defaultValue = Quaternion::IDENTITY) -> Quaternion
        {
            Quaternion ret;
            return parse(val, ret) ? ret : defaultValue;
        }
        /** Parses a ColourValue out of a String. 
        @remarks
            Format is "r g b a" (i.e. 4x Real values, space delimited), or "r g b" which implies
            an alpha value of 1.0 (opaque). Failure to parse returns ColourValue::Black.
        */
        static auto parseColourValue(std::string_view val, const ColourValue& defaultValue = ColourValue::Black) -> ColourValue
        {
            ColourValue ret;
            return parse(val, ret) ? ret : defaultValue;
        }

        /** Checks the String is a valid number value. */
        static auto isNumber(std::string_view val) -> bool;

		/** Converts a StereoModeType to a String
		@remarks
			String output format is "None", "Frame Sequential", etc.
		*/
		static auto toString(StereoModeType val) -> String;

		/** Converts a String to a StereoModeType
		@remarks
			String input format should be "None", "Frame Sequential", etc.
		*/
		static auto parseStereoMode(std::string_view val, StereoModeType defaultValue = StereoModeType::NONE) -> StereoModeType;

		static locale_t _numLocale;
    private:
        template<typename T>
        static auto _toString(T val, uint16 width, char fill, std::ios::fmtflags flags) -> String;
    };

    inline auto to_string(const Quaternion& v) -> String { return StringConverter::toString(v); }
    inline auto to_string(const ColourValue& v) -> String { return StringConverter::toString(v); }
    inline auto to_string(const Vector2& v) -> String { return StringConverter::toString(v); }
    inline auto to_string(const Vector3& v) -> String { return StringConverter::toString(v); }
    inline auto to_string(const Vector4& v) -> String { return StringConverter::toString(v); }
    inline auto to_string(const Matrix3& v) -> String { return StringConverter::toString(v); }
    inline auto to_string(const Matrix4& v) -> String { return StringConverter::toString(v); }
    /** @} */
    /** @} */
}

export
template<typename CharT>

export
struct std::formatter<Ogre::Vector3, CharT>
{
    std::formatter<Ogre::Real, CharT> realFormatter;

    auto constexpr parse(auto& pc)
    {
        return realFormatter.parse(pc);
    }

    auto constexpr format(Ogre::Vector3 const& vec, auto& fc)
    {
        auto x = realFormatter.format(vec.x, fc);
        *x++ = CharT{' '};
        fc.advance_to(x);
        auto y = realFormatter.format(vec.y, fc);
        *y++ = CharT{' '};
        fc.advance_to(y);
        return realFormatter.format(vec.z, fc);
    }
};

export
template<typename T, typename CharT>
requires

export
    ::std::is_enum_v<T>

export
struct std::formatter<T, CharT>
{
    std::formatter<::std::underlying_type_t<T>, CharT> underlyingFormatter;

    auto constexpr parse(auto& pc)
    {
        return underlyingFormatter.parse(pc);
    }

    auto constexpr format(T t, auto& fc)
    {
        return underlyingFormatter.format(std::to_underlying(t), fc);
    }
};

export
template<typename CharT>

export
struct std::formatter<std::filesystem::path, CharT>
{
    std::formatter<::std::string, CharT> stringFormatter;

    auto constexpr parse(auto& pc)
    {
        return stringFormatter.parse(pc);
    }

    auto constexpr format(std::filesystem::path const& val, auto& fc)
    {
        return stringFormatter.format(val.native(), fc);
    }
};

export
template<typename CharT>

export
struct std::formatter<Ogre::StringVector, CharT>
{
    std::formatter<::std::string, CharT> stringFormatter;

    auto constexpr parse(auto& pc)
    {
        return stringFormatter.parse(pc);
    }

    auto constexpr format(Ogre::StringVector const& val, auto& fc)
    {
        auto out = fc.out();
        for (auto i = val.begin(); i != val.end(); ++i)
        {
            if (i != val.begin())
            {
                *out++ = CharT{' '};
            }

            fc.advance_to(out);
            out = stringFormatter.format(*i, fc);
        }
        return out;
    }
};

export
template<typename CharT>

export
struct std::formatter<std::vector<std::string_view>, CharT>
{
    std::formatter<std::string_view, CharT> stringFormatter;

    auto constexpr parse(auto& pc)
    {
        return stringFormatter.parse(pc);
    }

    auto constexpr format(std::vector<std::string_view> const& val, auto& fc)
    {
        auto out = fc.out();
        for (auto i = val.begin(); i != val.end(); ++i)
        {
            if (i != val.begin())
            {
                *out++ = CharT{' '};
            }

            fc.advance_to(out);
            out = stringFormatter.format(*i, fc);
        }
        return out;
    }
};
