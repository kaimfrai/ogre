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
module Ogre.Core;

import :Animable;

import <any>;
import <utility>;

namespace Ogre {
    //--------------------------------------------------------------------------
    void AnimableObject::createAnimableDictionary() const
    {
        if (msAnimableDictionary.find(getAnimableDictionaryName()) == msAnimableDictionary.end())
        {
            StringVector vec;
            initialiseAnimableDictionary(vec);
            msAnimableDictionary[getAnimableDictionaryName()] = vec;
        }
    }

    /// Get an updateable reference to animable value list
    auto AnimableObject::_getAnimableValueNames() -> StringVector&
    {
        auto i = msAnimableDictionary.find(getAnimableDictionaryName());
        if (i != msAnimableDictionary.end())
        {
            return i->second;
        }

        OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("Animable value list not found for {}", getAnimableDictionaryName()));
    }

    auto AnimableObject::getAnimableValueNames() const noexcept -> const StringVector&
    {
        createAnimableDictionary();

        auto i = msAnimableDictionary.find(getAnimableDictionaryName());
        if (i != msAnimableDictionary.end())
        {
            return i->second;
        }

        OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("Animable value list not found for {}", getAnimableDictionaryName()));
    }

    void AnimableValue::resetToBaseValue()
    {
        ::std::visit
        (   [this]  (   auto const
                    &   i_rValue
                )
            {
                setValue(i_rValue);
            }
        ,   mBaseValue
        );
    }
    //--------------------------------------------------------------------------
    void AnimableValue::setAsBaseValue(::std::any const& val)
    {
        ::std::visit
        (   [   this
            ,   &val
            ]   <   typename
                        t_tValue
                >(  t_tValue
                    &
                )
            {
                setAsBaseValue(any_cast<t_tValue>(val));
            }
        ,   mBaseValue
        );
    }
    //--------------------------------------------------------------------------
    void AnimableValue::setValue(::std::any const& val)
    {
        ::std::visit
        (   [   this
            ,   &val
            ]   <   typename
                        t_tValue
                >(  t_tValue
                    &
                )
            {
                setValue(any_cast<t_tValue>(val));
            }
        ,   mBaseValue
        );
    }
    //--------------------------------------------------------------------------
    void AnimableValue::applyDeltaValue(::std::any const& val)
    {
        ::std::visit
        (   [   this
            ,   &val
            ]   <   typename
                        t_tValue
                >(  t_tValue
                    &
                )
            {
                applyDeltaValue(any_cast<t_tValue>(val));
            }
        ,   mBaseValue
        );
    }



}
