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
    template<typename ValueType> ValueType
    any_cast(const Any & operand);

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
           
        {
        }

        template<typename ValueType>
        Any(const ValueType & value)
          : mContent(new holder<ValueType>(value))
        {
        }

        Any(const Any & other)
          : mContent(other.mContent ? other.mContent->clone() : nullptr)
        {
        }

        virtual ~Any()
        {
            reset();
        }

    public: // modifiers

        Any& swap(Any & rhs)
        {
            std::swap(mContent, rhs.mContent);
            return *this;
        }

        template<typename ValueType>
        Any& operator=(const ValueType & rhs)
        {
            Any(rhs).swap(*this);
            return *this;
        }

        Any & operator=(const Any & rhs)
        {
            Any(rhs).swap(*this);
            return *this;
        }

    public: // queries

        [[nodiscard]] bool has_value() const
        {
            return mContent != nullptr;
        }

        [[nodiscard]] const std::type_info& type() const
        {
            return mContent ? mContent->getType() : typeid(void);
        }

        void reset()
        {
            delete mContent;
            mContent = nullptr;
        }

    protected: // types

        class placeholder 
        {
        public: // structors
    
            virtual ~placeholder()
            {
            }

        public: // queries

            [[nodiscard]] virtual const std::type_info& getType() const noexcept = 0;

            [[nodiscard]] virtual placeholder * clone() const = 0;
    
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

            [[nodiscard]] const std::type_info & getType() const noexcept override
            {
                return typeid(ValueType);
            }

            [[nodiscard]] placeholder * clone() const override
            {
                return new holder(held);
            }

            void writeToStream(std::ostream& o) override
            {
                o << "Any::ValueType";
            }


        public: // representation
            ValueType held;
        };

    protected: // representation
        placeholder * mContent{nullptr};

        template<typename ValueType>
        friend ValueType * any_cast(Any *);
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
            mContent = other.mContent ? other.mContent->clone() : nullptr; 
        }

    protected:
        class numplaceholder : public Any::placeholder
        {
        public: // structors

            ~numplaceholder() override
            {
            }
            virtual placeholder* add(placeholder* rhs) = 0;
            virtual placeholder* subtract(placeholder* rhs) = 0;
            virtual placeholder* multiply(placeholder* rhs) = 0;
            virtual placeholder* multiply(Real factor) = 0;
            virtual placeholder* divide(placeholder* rhs) = 0;
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

            [[nodiscard]] const std::type_info & getType() const noexcept override
            {
                return typeid(ValueType);
            }

            [[nodiscard]] placeholder * clone() const override
            {
                return new numholder(held);
            }

            placeholder* add(placeholder* rhs) override
            {
                return new numholder(held + static_cast<numholder*>(rhs)->held);
            }
            placeholder* subtract(placeholder* rhs) override
            {
                return new numholder(held - static_cast<numholder*>(rhs)->held);
            }
            placeholder* multiply(placeholder* rhs) override
            {
                return new numholder(held * static_cast<numholder*>(rhs)->held);
            }
            placeholder* multiply(Real factor) override
            {
                return new numholder(held * factor);
            }
            placeholder* divide(placeholder* rhs) override
            {
                return new numholder(held / static_cast<numholder*>(rhs)->held);
            }
            void writeToStream(std::ostream& o) override
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
        AnyNumeric & operator=(const AnyNumeric & rhs)
        {
            AnyNumeric(rhs).swap(*this);
            return *this;
        }
        AnyNumeric operator+(const AnyNumeric& rhs) const
        {
            return {
                static_cast<numplaceholder*>(mContent)->add(rhs.mContent)};
        }
        AnyNumeric operator-(const AnyNumeric& rhs) const
        {
            return {
                static_cast<numplaceholder*>(mContent)->subtract(rhs.mContent)};
        }
        AnyNumeric operator*(const AnyNumeric& rhs) const
        {
            return {
                static_cast<numplaceholder*>(mContent)->multiply(rhs.mContent)};
        }
        AnyNumeric operator*(Real factor) const
        {
            return {
                static_cast<numplaceholder*>(mContent)->multiply(factor)};
        }
        AnyNumeric operator/(const AnyNumeric& rhs) const
        {
            return {
                static_cast<numplaceholder*>(mContent)->divide(rhs.mContent)};
        }
        AnyNumeric& operator+=(const AnyNumeric& rhs)
        {
            *this = AnyNumeric(
                static_cast<numplaceholder*>(mContent)->add(rhs.mContent));
            return *this;
        }
        AnyNumeric& operator-=(const AnyNumeric& rhs)
        {
            *this = AnyNumeric(
                static_cast<numplaceholder*>(mContent)->subtract(rhs.mContent));
            return *this;
        }
        AnyNumeric& operator*=(const AnyNumeric& rhs)
        {
            *this = AnyNumeric(
                static_cast<numplaceholder*>(mContent)->multiply(rhs.mContent));
            return *this;
        }
        AnyNumeric& operator/=(const AnyNumeric& rhs)
        {
            *this = AnyNumeric(
                static_cast<numplaceholder*>(mContent)->divide(rhs.mContent));
            return *this;
        }




    };


    template<typename ValueType>
    ValueType * any_cast(Any * operand)
    {
        return operand &&
                (operand->type() == typeid(ValueType))
                    ? &static_cast<Any::holder<ValueType> *>(operand->mContent)->held
                    : nullptr;
    }

    template<typename ValueType>
    const ValueType * any_cast(const Any * operand)
    {
        return any_cast<ValueType>(const_cast<Any *>(operand));
    }

    template<typename ValueType>
    ValueType any_cast(const Any & operand)
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

