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
#ifndef OGRE_CORE_ANIMABLE_H
#define OGRE_CORE_ANIMABLE_H

#include <any>
#include <cstring>
#include <format>
#include <map>
#include <string>
#include <variant>

#include "OgreColourValue.hpp"
#include "OgreCommon.hpp"
#include "OgreException.hpp"
#include "OgreMath.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreQuaternion.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreStringVector.hpp"
#include "OgreVector.hpp"

namespace Ogre {

    /** \addtogroup Core
    *  @{
    */

    /** \addtogroup Animation
    *  @{
    */

    /** Defines an object property which is animable, i.e. may be keyframed.
    @remarks
        Animable properties are those which can be altered over time by a 
        predefined keyframe sequence. They may be set directly, or they may
        be modified from their existing state (common if multiple animations
        are expected to apply at once). Implementors of this interface are
        expected to override the 'setValue', 'setCurrentStateAsBaseValue' and 
        'applyDeltaValue' methods appropriate to the type in question, and to 
        initialise the type.
    @par
        AnimableValue instances are accessible through any class which extends
        AnimableObject in order to expose it's animable properties.
    @note
        This class is an instance of the Adapter pattern, since it generalises
        access to a particular property. Whilst it could have been templated
        such that the type which was being referenced was compiled in, this would
        make it more difficult to aggregated generically, and since animations
        are often comprised of multiple properties it helps to be able to deal
        with all values through a single class.
    */
    class AnimableValue : public AnimableAlloc
    {
    protected:
        ::std::variant<int, Real, Vector2, Vector3, Vector4, Quaternion, ColourValue, Radian, Degree> mBaseValue;

        /// Internal method to set a value as base
        virtual void setAsBaseValue(int val) { mBaseValue = val; }
        /// Internal method to set a value as base
        virtual void setAsBaseValue(Real val) { mBaseValue = val; }
        /// Internal method to set a value as base
        virtual void setAsBaseValue(const Vector2& val) 
        { mBaseValue = val; }
        /// Internal method to set a value as base
        virtual void setAsBaseValue(const Vector3& val) 
        { mBaseValue = val; }
        /// Internal method to set a value as base
        virtual void setAsBaseValue(const Vector4& val) 
        { mBaseValue = val; }
        /// Internal method to set a value as base
        virtual void setAsBaseValue(const Quaternion& val)
        {
            mBaseValue = val;
        }
        /// Internal method to set a value as base
        virtual void setAsBaseValue(::std::any const& val);
        /// Internal method to set a value as base
        virtual void setAsBaseValue(const ColourValue& val)
        { 
            mBaseValue = val;
        }
        /// Internal method to set a value as base
        virtual void setAsBaseValue(const Radian& val)
        { 
            mBaseValue = val;
        }
        /// Internal method to set a value as base
        virtual void setAsBaseValue(const Degree& val)
        { 
            mBaseValue = val;
        }


    public:
        template<typename T>
        AnimableValue(::std::in_place_type_t<T> t) : mBaseValue(t) {}
        virtual ~AnimableValue() = default;

        /// Sets the current state as the 'base' value; used for delta animation
        virtual void setCurrentStateAsBaseValue() = 0;

        /// Set value 
        virtual void setValue(int) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Set value 
        virtual void setValue(Real) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Set value 
        virtual void setValue(const Vector2&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Set value 
        virtual void setValue(const Vector3&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Set value 
        virtual void setValue(const Vector4&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Set value 
        virtual void setValue(const Quaternion&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Set value 
        virtual void setValue(const ColourValue&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Set value 
        virtual void setValue(const Radian&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Set value 
        virtual void setValue(const Degree&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Set value 
        virtual void setValue(::std::any const& val);

        // reset to base value
        virtual void resetToBaseValue();

        /// Apply delta value
        virtual void applyDeltaValue(int) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Set value 
        virtual void applyDeltaValue(Real) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Apply delta value 
        virtual void applyDeltaValue(const Vector2&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Apply delta value 
        virtual void applyDeltaValue(const Vector3&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Apply delta value 
        virtual void applyDeltaValue(const Vector4&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Apply delta value 
        virtual void applyDeltaValue(const Quaternion&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Apply delta value 
        virtual void applyDeltaValue(const ColourValue&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Apply delta value 
        virtual void applyDeltaValue(const Degree&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Apply delta value 
        virtual void applyDeltaValue(const Radian&) {
            OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED, "", "");
        }
        /// Apply delta value 
        virtual void applyDeltaValue(::std::any const& val);


    };

    /** Defines an interface to classes which have one or more AnimableValue
        instances to expose.
    */
    class AnimableObject
    {
    protected:
        using AnimableDictionaryMap = std::map<std::string_view, StringVector>;
        /// Static map of class name to list of animable value names
        static AnimableDictionaryMap msAnimableDictionary;
        /** Get the name of the animable dictionary for this class.
        @remarks
            Subclasses must override this if they want to support animation of
            their values.
        */
        [[nodiscard]] virtual auto getAnimableDictionaryName() const noexcept -> std::string_view
        { return BLANKSTRING; }
        /** Internal method for creating a dictionary of animable value names 
            for the class, if it does not already exist.
        */
        void createAnimableDictionary() const;
    
        /// Get an updateable reference to animable value list
        auto _getAnimableValueNames() -> StringVector&;

        /** Internal method for initialising dictionary; should be implemented by 
            subclasses wanting to expose animable parameters.
        */
        virtual void initialiseAnimableDictionary(StringVector&) const {}


    public:
        AnimableObject() = default;
        virtual ~AnimableObject() = default;

        /** Gets a list of animable value names for this object. */
        [[nodiscard]] auto getAnimableValueNames() const noexcept -> const StringVector&;

        /** Create a reference-counted AnimableValuePtr for the named value.
        @remarks
            You can use the returned object to animate a value on this object,
            using AnimationTrack. Subclasses must override this if they wish 
            to support animation of their values.
        */
        virtual auto createAnimableValue(std::string_view valueName) -> AnimableValuePtr
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, 
                ::std::format("No animable value named '{}' present.", valueName ), 
                "AnimableObject::createAnimableValue");
        }



    };

    /** @} */
    /** @} */

}

#endif
