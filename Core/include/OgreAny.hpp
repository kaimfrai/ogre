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
#include <typeinfo>

namespace Ogre
{
	// resolve circular dependancy
    class Any;
    template<typename ValueType> auto
    any_cast(const Any & operand) -> ValueType;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */
    /** Variant type that can hold Any other type.
    */
    class Any 
    {
    public: // constructors

        Any()
          : mContent(0)
        {
        }

        template<typename ValueType>
        Any(const ValueType & value)
          : mContent(new holder<ValueType>(value))
        {
        }

        Any(const Any & other)
          : mContent(other.mContent ? other.mContent->clone() : 0)
        {
        }

        virtual ~Any()
        {
            reset();
        }

    public: // modifiers

        auto swap(Any & rhs) -> Any&
        {
            std::swap(mContent, rhs.mContent);
            return *this;
        }

        template<typename ValueType>
        auto operator=(const ValueType & rhs) -> Any&
        {
            Any(rhs).swap(*this);
            return *this;
        }

        auto operator=(const Any & rhs) -> Any &
        {
            Any(rhs).swap(*this);
            return *this;
        }

    public: // queries

        auto has_value() const -> bool
        {
            return mContent != NULL;
        }

        auto type() const -> const std::type_info&
        {
            return mContent ? mContent->getType() : typeid(void);
        }

        void reset()
        {
            delete mContent;
            mContent = NULL;
        }

    protected: // types

        class placeholder 
        {
        public: // structors
    
            virtual ~placeholder()
            {
            }

        public: // queries

            virtual auto getType() const -> const std::type_info& = 0;

            virtual auto clone() const -> placeholder * = 0;
    
            virtual void writeToStream(std::ostream& o) = 0;

        };

        template<typename ValueType>
        class holder : public placeholder
        {
        public: // structors

            holder(const ValueType & value)
              : held(value)
            {
            }

        public: // queries

            virtual auto getType() const -> const std::type_info &
            {
                return typeid(ValueType);
            }

            virtual auto clone() const -> placeholder *
            {
                return new holder(held);
            }

            virtual void writeToStream(std::ostream& o)
            {
                o << "Any::ValueType";
            }


        public: // representation
            ValueType held;
        };

    protected: // representation
        placeholder * mContent;

        template<typename ValueType>
        friend auto any_cast(Any *) -> ValueType *;
    };


    /** Specialised Any class which has built in arithmetic operators, but can 
        hold only types which support operator +,-,* and / .
    */
    class AnyNumeric : public Any
    {
    public:
        AnyNumeric()
        : Any()
        {
        }

        template<typename ValueType>
        AnyNumeric(const ValueType & value)
            
        {
            mContent = new numholder<ValueType>(value);
        }

        AnyNumeric(const AnyNumeric & other)
            : Any()
        {
            mContent = other.mContent ? other.mContent->clone() : 0; 
        }

    protected:
        class numplaceholder : public Any::placeholder
        {
        public: // structors

            ~numplaceholder()
            {
            }
            virtual auto add(placeholder* rhs) -> placeholder* = 0;
            virtual auto subtract(placeholder* rhs) -> placeholder* = 0;
            virtual auto multiply(placeholder* rhs) -> placeholder* = 0;
            virtual auto multiply(Real factor) -> placeholder* = 0;
            virtual auto divide(placeholder* rhs) -> placeholder* = 0;
        };

        template<typename ValueType>
        class numholder : public numplaceholder
        {
        public: // structors

            numholder(const ValueType & value)
                : held(value)
            {
            }

        public: // queries

            virtual auto getType() const -> const std::type_info &
            {
                return typeid(ValueType);
            }

            virtual auto clone() const -> placeholder *
            {
                return new numholder(held);
            }

            virtual auto add(placeholder* rhs) -> placeholder*
            {
                return new numholder(held + static_cast<numholder*>(rhs)->held);
            }
            virtual auto subtract(placeholder* rhs) -> placeholder*
            {
                return new numholder(held - static_cast<numholder*>(rhs)->held);
            }
            virtual auto multiply(placeholder* rhs) -> placeholder*
            {
                return new numholder(held * static_cast<numholder*>(rhs)->held);
            }
            virtual auto multiply(Real factor) -> placeholder*
            {
                return new numholder(held * factor);
            }
            virtual auto divide(placeholder* rhs) -> placeholder*
            {
                return new numholder(held / static_cast<numholder*>(rhs)->held);
            }
            virtual void writeToStream(std::ostream& o)
            {
                o << held;
            }

        public: // representation

            ValueType held;

        };

        /// Construct from holder
        AnyNumeric(placeholder* pholder)
        {
            mContent = pholder;
        }

    public:
        auto operator=(const AnyNumeric & rhs) -> AnyNumeric &
        {
            AnyNumeric(rhs).swap(*this);
            return *this;
        }
        auto operator+(const AnyNumeric& rhs) const -> AnyNumeric
        {
            return {
                static_cast<numplaceholder*>(mContent)->add(rhs.mContent)};
        }
        auto operator-(const AnyNumeric& rhs) const -> AnyNumeric
        {
            return {
                static_cast<numplaceholder*>(mContent)->subtract(rhs.mContent)};
        }
        auto operator*(const AnyNumeric& rhs) const -> AnyNumeric
        {
            return {
                static_cast<numplaceholder*>(mContent)->multiply(rhs.mContent)};
        }
        auto operator*(Real factor) const -> AnyNumeric
        {
            return {
                static_cast<numplaceholder*>(mContent)->multiply(factor)};
        }
        auto operator/(const AnyNumeric& rhs) const -> AnyNumeric
        {
            return {
                static_cast<numplaceholder*>(mContent)->divide(rhs.mContent)};
        }
        auto operator+=(const AnyNumeric& rhs) -> AnyNumeric&
        {
            *this = AnyNumeric(
                static_cast<numplaceholder*>(mContent)->add(rhs.mContent));
            return *this;
        }
        auto operator-=(const AnyNumeric& rhs) -> AnyNumeric&
        {
            *this = AnyNumeric(
                static_cast<numplaceholder*>(mContent)->subtract(rhs.mContent));
            return *this;
        }
        auto operator*=(const AnyNumeric& rhs) -> AnyNumeric&
        {
            *this = AnyNumeric(
                static_cast<numplaceholder*>(mContent)->multiply(rhs.mContent));
            return *this;
        }
        auto operator/=(const AnyNumeric& rhs) -> AnyNumeric&
        {
            *this = AnyNumeric(
                static_cast<numplaceholder*>(mContent)->divide(rhs.mContent));
            return *this;
        }




    };


    template<typename ValueType>
    auto any_cast(Any * operand) -> ValueType *
    {
        return operand &&
                (operand->type() == typeid(ValueType))
                    ? &static_cast<Any::holder<ValueType> *>(operand->mContent)->held
                    : 0;
    }

    template<typename ValueType>
    auto any_cast(const Any * operand) -> const ValueType *
    {
        return any_cast<ValueType>(const_cast<Any *>(operand));
    }

    template<typename ValueType>
    auto any_cast(const Any & operand) -> ValueType
    {
        const ValueType * result = any_cast<ValueType>(&operand);
        if(!result)
        {
            throw std::bad_cast();
        }
        return *result;
    }
    /** @} */
    /** @} */


}

#endif

