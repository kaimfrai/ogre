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

#include <clocale>
#include <cstdlib>

module Ogre.Core;

import :ColourValue;
import :Common;
import :Exception;
import :Matrix3;
import :Matrix4;
import :Platform;
import :Prerequisites;
import :Quaternion;
import :String;
import :StringConverter;
import :StringVector;
import :Vector;

import <locale>;
import <ostream>;
import <string>;
import <vector>;

namespace Ogre {

    template<typename T>
    auto StringConverter::_toString(T val, uint16 width, char fill, std::ios::fmtflags flags) -> String
    {
        StringStream stream;
        stream.width(width);
        stream.fill(fill);
        if (flags & std::ios::basefield) {
            stream.setf(flags, std::ios::basefield);
            stream.setf((flags & ~std::ios::basefield) | std::ios::showbase);
        }
        else if (flags)
            stream.setf(flags);

        stream << val;

        return stream.str();
    }

    //-----------------------------------------------------------------------
    auto StringConverter::toString(float val, unsigned short precision,
                                     unsigned short width, char fill, std::ios::fmtflags flags) -> String
    {
        StringStream stream;
        stream.precision(precision);
        stream.width(width);
        stream.fill(fill);
        if (flags)
            stream.setf(flags);
        stream << val;
        return stream.str();
    }

    //-----------------------------------------------------------------------
    auto StringConverter::toString(double val, unsigned short precision,
                                     unsigned short width, char fill, std::ios::fmtflags flags) -> String
    {
        StringStream stream;
        stream.precision(precision);
        stream.width(width);
        stream.fill(fill);
        if (flags)
            stream.setf(flags);
        stream << val;
        return stream.str();
    }

    //-----------------------------------------------------------------------
    auto StringConverter::toString(long double val, unsigned short precision,
                                     unsigned short width, char fill, std::ios::fmtflags flags) -> String
    {
        StringStream stream;
        stream.precision(precision);
        stream.width(width);
        stream.fill(fill);
        if (flags)
            stream.setf(flags);
        stream << val;
        return stream.str();
    }


    //-----------------------------------------------------------------------
    auto StringConverter::toString(const Vector2& val) -> String
    {
        StringStream stream;
        stream << val.x << " " << val.y;
        return stream.str();
    }
    //-----------------------------------------------------------------------
    auto StringConverter::toString(const Vector3& val) -> String
    {
        StringStream stream;
        stream << val.x << " " << val.y << " " << val.z;
        return stream.str();
    }
    //-----------------------------------------------------------------------
    auto StringConverter::toString(const Vector4& val) -> String
    {
        StringStream stream;
        stream << val.x << " " << val.y << " " << val.z << " " << val.w;
        return stream.str();
    }
    //-----------------------------------------------------------------------
    auto StringConverter::toString(const Matrix3& val) -> String
    {
        StringStream stream;
        stream << val[0][0] << " "
            << val[0][1] << " "             
            << val[0][2] << " "             
            << val[1][0] << " "             
            << val[1][1] << " "             
            << val[1][2] << " "             
            << val[2][0] << " "             
            << val[2][1] << " "             
            << val[2][2];
        return stream.str();
    }
    //-----------------------------------------------------------------------
    auto StringConverter::toString(bool val, bool yesNo) -> String
    {
        if (val)
        {
            if (yesNo)
            {
                return "yes";
            }
            else
            {
                return "true";
            }
        }
        else
            if (yesNo)
            {
                return "no";
            }
            else
            {
                return "false";
            }
    }
    //-----------------------------------------------------------------------
    auto StringConverter::toString(const Matrix4& val) -> String
    {
        StringStream stream;
        stream << val[0][0] << " "
            << val[0][1] << " "             
            << val[0][2] << " "             
            << val[0][3] << " "             
            << val[1][0] << " "             
            << val[1][1] << " "             
            << val[1][2] << " "             
            << val[1][3] << " "             
            << val[2][0] << " "             
            << val[2][1] << " "             
            << val[2][2] << " "             
            << val[2][3] << " "             
            << val[3][0] << " "             
            << val[3][1] << " "             
            << val[3][2] << " "             
            << val[3][3];
        return stream.str();
    }
    //-----------------------------------------------------------------------
    auto StringConverter::toString(const Quaternion& val) -> String
    {
        StringStream stream;
        stream  << val.w << " " << val.x << " " << val.y << " " << val.z;
        return stream.str();
    }
    //-----------------------------------------------------------------------
    auto StringConverter::toString(const ColourValue& val) -> String
    {
        StringStream stream;
        stream << val.r << " " << val.g << " " << val.b << " " << val.a;
        return stream.str();
    }
    //-----------------------------------------------------------------------
    auto StringConverter::toString(const StringVector& val) -> String
    {
        StringStream stream;
        for (auto i = val.begin(); i != val.end(); ++i)
        {
            if (i != val.begin())
                stream << " ";

            stream << *i; 
        }
        return stream.str();
    }

    //-----------------------------------------------------------------------
    template <typename T> static auto assignValid(bool valid, const T& val, T& ret) -> bool
    {
        if (valid)
            ret = val;
        return valid;
    }

    auto StringConverter::parse(std::string_view val, float& ret) -> bool
    {
        char* end;
        auto tmp = (float)strtod_l(val.data(), &end, _numLocale);
        return assignValid(val.data() != end, tmp, ret);
    }
    auto StringConverter::parse(std::string_view val, double& ret) -> bool
    {
        char* end;
        auto tmp = strtod_l(val.data(), &end, _numLocale);
        return assignValid(val.data() != end, tmp, ret);
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, int32& ret) -> bool
    {
        char* end;
        auto tmp = (int32)strtol_l(val.data(), &end, 0, _numLocale);
        return assignValid(val.data() != end, tmp, ret);
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, int64& ret) -> bool
    {
        char* end;
        int64 tmp = strtoll_l(val.data(), &end, 0, _numLocale);
        return assignValid(val.data() != end, tmp, ret);
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, unsigned long& ret) -> bool
    {
        char* end;
        unsigned long tmp = strtoull_l(val.data(), &end, 0, _numLocale);
        return assignValid(val.data() != end, tmp, ret);
    }
    auto StringConverter::parse(std::string_view val, unsigned long long& ret) -> bool
    {
        char* end;
        unsigned long long tmp = strtoull_l(val.data(), &end, 0, _numLocale);
        return assignValid(val.data() != end, tmp, ret);
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, uint32& ret) -> bool
    {
        char* end;
        auto tmp = (uint32)strtoul_l(val.data(), &end, 0, _numLocale);
        return assignValid(val.data() != end, tmp, ret);
    }
    auto StringConverter::parse(std::string_view val, bool& ret) -> bool
    {
        //FIXME Returns both parsed value and error in same value - ambiguous.
        // Suggested alternatives: implement exception handling or make either
        // error or parsed value a parameter.
        if ((StringUtil::startsWith(val, "true") || StringUtil::startsWith(val, "yes")
             || StringUtil::startsWith(val, "1") ||  StringUtil::startsWith(val, "on")))
            ret = true;
        else if ((StringUtil::startsWith(val, "false") || StringUtil::startsWith(val, "no")
                  || StringUtil::startsWith(val, "0") ||  StringUtil::startsWith(val, "off")))
            ret = false;
        else
            return false;

        return true;
    }

    template<typename T>
    static auto parseReals(std::string_view val, T* dst, size_t n) -> bool
    {
        // Split on space
        auto const vec = StringUtil::split(val);
        if(vec.size() != n)
            return false;

        bool ret = true;
        for(size_t i = 0; i < n; i++)
            ret &= StringConverter::parse(vec[i], dst[i]);
        return ret;
    }

    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, Vector2& ret) -> bool
    {
        return parseReals(val, ret.ptr(), 2);
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, Vector3& ret) -> bool
    {
        return parseReals(val, ret.ptr(), 3);
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, Vector4& ret) -> bool
    {
        return parseReals(val, ret.ptr(), 4);
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, Matrix3& ret) -> bool
    {
        return parseReals(val, &ret[0][0], 9);
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, Matrix4& ret) -> bool
    {
        return parseReals(val, &ret[0][0], 16);
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, Quaternion& ret) -> bool
    {
        return parseReals(val, ret.ptr(), 4);
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parse(std::string_view val, ColourValue& ret) -> bool
    {
        // Split on space
        auto const vec = StringUtil::split(val);

        if (vec.size() == 4)
        {
            return parse(vec[0], ret.r) && parse(vec[1], ret.g) && parse(vec[2], ret.b) &&
                   parse(vec[3], ret.a);
        }
        else if (vec.size() == 3)
        {
            ret.a = 1.0f;
            return parse(vec[0], ret.r) && parse(vec[1], ret.g) && parse(vec[2], ret.b);
        }
        else
        {
            return false;
        }
    }
    //-----------------------------------------------------------------------
    auto StringConverter::isNumber(std::string_view val) -> bool
    {
        char* end;
        strtod(val.data(), &end);
        return end == (val.data() + val.size());
    }
    //-----------------------------------------------------------------------
    auto StringConverter::toString(StereoModeType val) -> String
    {
		StringStream stream;
        using enum StereoModeType;
		switch (val)
		{
		case NONE:
		  stream << "None";
		  break;
		case FRAME_SEQUENTIAL:
		  stream << "Frame Sequential";
		  break;
		default:
		  OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "Unsupported stereo mode value", "StringConverter::toString(const StereoModeType& val)");
		}

		return stream.str();
    }
    //-----------------------------------------------------------------------
    auto StringConverter::parseStereoMode(std::string_view val, StereoModeType defaultValue) -> StereoModeType
    {
		StereoModeType result = defaultValue;
		if (val.compare("None") == 0)
		{
			result = StereoModeType::NONE;
		}
		else if (val.compare("Frame Sequential") == 0)
		{
			result = StereoModeType::FRAME_SEQUENTIAL;
		}
		
		return result;
    }
	//-----------------------------------------------------------------------
}
