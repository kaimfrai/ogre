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
// -- Based on boost::any, original copyright information follows --
// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompAnying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// -- End original copyright --

#ifndef OGRE_CORE_ANY_H
#define OGRE_CORE_ANY_H

#include "OgrePrerequisites.hpp"

#include <any>
#include <cassert>
#include <type_traits>
#include <utility>

namespace Ogre
{
    /** Specialised Any class which has built in arithmetic operators, but can
        hold only types which support operator +,-,* and / .
    */
    class AnyNumeric
    {
    private:
        ::std::any mContent;
        using Assignment = auto(AnyNumeric&, AnyNumeric const&) noexcept -> void;
        using Scale = auto(AnyNumeric&, Real) noexcept -> void;
        struct VTable
        {
            Assignment* additionAssignment;
            Assignment* subtractionAssignment;
            Assignment* multiplicationAssignment;
            Scale* scale;
            Assignment* divisionAssignment;
        };

        VTable const* mVTable;

        template<typename ValueType>
        [[nodiscard]] static ValueType& get(AnyNumeric& numeric) noexcept
        {
            return *::std::any_cast<ValueType>(&numeric.mContent);
        }

        template<typename ValueType>
        [[nodiscard]] static ValueType const& get(AnyNumeric const& numeric) noexcept
        {
            return *::std::any_cast<ValueType const>(&numeric.mContent);
        }

        template<typename ValueType>
        static VTable const constexpr VTableFor
        {   .additionAssignment = +[](AnyNumeric& lhs, AnyNumeric const& rhs) noexcept
            {
                get<ValueType>(lhs) += get<ValueType>(rhs);
            }
        ,   .subtractionAssignment = +[](AnyNumeric& lhs, AnyNumeric const& rhs) noexcept
            {
                get<ValueType>(lhs) -= get<ValueType>(rhs);
            }
        ,   .multiplicationAssignment = +[](AnyNumeric& lhs, AnyNumeric const& rhs) noexcept
            {
                get<ValueType>(lhs) *= get<ValueType>(rhs);
            }
        ,   .scale = +[](AnyNumeric& lhs, Real rhs) noexcept
            {
                get<ValueType>(lhs) *= rhs;
            }
        ,   .divisionAssignment = +[](AnyNumeric& lhs, AnyNumeric const& rhs) noexcept
            {
                get<ValueType>(lhs) /= get<ValueType>(rhs);
            }
        };

    public:
        AnyNumeric() = default;

        template<typename ValueType>
        AnyNumeric(ValueType&& value)
        : mContent(::std::forward<ValueType>(value))
        , mVTable{&VTableFor<::std::decay_t<ValueType>>}
        {}

        [[nodiscard]] friend AnyNumeric operator+(AnyNumeric lhs, AnyNumeric const& rhs) noexcept
        {
            return lhs += rhs;
        }
        [[nodiscard]] friend AnyNumeric operator-(AnyNumeric lhs, AnyNumeric const& rhs) noexcept
        {
            return lhs -= rhs;
        }
        [[nodiscard]] friend AnyNumeric operator*(AnyNumeric lhs, AnyNumeric const& rhs) noexcept
        {
            return lhs *= rhs;
        }
        [[nodiscard]] friend AnyNumeric operator*(AnyNumeric lhs, Real factor) noexcept
        {
            return lhs *= factor;
        }
        [[nodiscard]] friend AnyNumeric operator/(AnyNumeric lhs, AnyNumeric const& rhs) noexcept
        {
            return lhs /= rhs;
        }
        AnyNumeric& operator+=(AnyNumeric const& rhs) & noexcept
        {
            assert(mVTable == rhs.mVTable);
            mVTable->additionAssignment(*this, rhs);
            return *this;
        }
        AnyNumeric& operator-=(AnyNumeric const& rhs) & noexcept
        {
            assert(mVTable == rhs.mVTable);
            mVTable->subtractionAssignment(*this, rhs);
            return *this;
        }
        AnyNumeric& operator*=(AnyNumeric const& rhs) & noexcept
        {
            assert(mVTable == rhs.mVTable);
            mVTable->multiplicationAssignment(*this, rhs);
            return *this;
        }
        AnyNumeric& operator*=(Real rhs) & noexcept
        {
            mVTable->scale(*this, rhs);
            return *this;
        }
        AnyNumeric& operator/=(AnyNumeric const& rhs) & noexcept
        {
            assert(mVTable == rhs.mVTable);
            mVTable->divisionAssignment(*this, rhs);
            return *this;
        }
    };

    /** @} */
    /** @} */

}

#endif

