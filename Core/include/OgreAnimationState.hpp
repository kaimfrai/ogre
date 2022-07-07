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

#ifndef OGRE_CORE_ANIMATIONSTATE_H
#define OGRE_CORE_ANIMATIONSTATE_H

#include <cassert>
#include <cstddef>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "OgreController.hpp"
#include "OgreControllerManager.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePrerequisites.hpp"

namespace Ogre {

    template <typename T> class MapIterator;
    template <typename T> class ConstMapIterator;
    template <typename T> class ConstVectorIterator;
class AnimationStateSet;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Animation
    *  @{
    */

    /** Represents the state of an animation and the weight of its influence. 
    @remarks
        Other classes can hold instances of this class to store the state of any animations
        they are using.
    */
    class AnimationState : public AnimationAlloc
    {
    public:

        /// Typedef for an array of float values used as a bone blend mask
        using BoneBlendMask = std::vector<float>;

        /** Normal constructor with all params supplied
            @param
                animName The name of this state.
            @param
                parent The parent AnimationStateSet that this state will belong to.
            @param
                timePos The position, in seconds, where this state will begin.
            @param
                length The length, in seconds, of this animation state.
            @param
                weight Weight to apply the animation state with.
            @param
                enabled Whether the animation state is enabled.
        */
        AnimationState(std::string_view animName, AnimationStateSet *parent, 
            Real timePos, Real length, Real weight = 1.0, bool enabled = false);
        /// Constructor to copy from an existing state with new parent
        AnimationState(AnimationStateSet* parent, const AnimationState &rhs);
        
        /// Gets the name of the animation to which this state applies
        [[nodiscard]] auto getAnimationName() const noexcept -> std::string_view;
        /// Gets the time position for this animation
        [[nodiscard]] auto getTimePosition() const -> Real;
        /// Sets the time position for this animation
        void setTimePosition(Real timePos);
        /// Gets the total length of this animation (may be shorter than whole animation)
        [[nodiscard]] auto getLength() const -> Real;
        /// Sets the total length of this animation (may be shorter than whole animation)
        void setLength(Real len);
        /// Gets the weight (influence) of this animation
        [[nodiscard]] auto getWeight() const -> Real;
        /// Sets the weight (influence) of this animation
        void setWeight(Real weight);
        /** Modifies the time position, adjusting for animation length
        @param offset The amount of time, in seconds, to extend the animation.
        @remarks
            This method loops at the edges if animation looping is enabled.
        */
        void addTime(Real offset);

        /// Returns true if the animation has reached the end and is not looping
        [[nodiscard]] auto hasEnded() const -> bool;

        /// Returns true if this animation is currently enabled
        [[nodiscard]] auto getEnabled() const noexcept -> bool;
        /// Sets whether this animation is enabled
        void setEnabled(bool enabled);

        /// Equality operator
        [[nodiscard]] auto operator==(const AnimationState& rhs) const noexcept -> bool;

        /** Sets whether or not an animation loops at the start and end of
            the animation if the time continues to be altered.
        */
        void setLoop(bool loop) { mLoop = loop; }
        /// Gets whether or not this animation loops            
        [[nodiscard]] auto getLoop() const noexcept -> bool { return mLoop; }
     
        /** Copies the states from another animation state, preserving the animation name
        (unlike operator=) but copying everything else.
        @param animState Reference to animation state which will use as source.
        */
        void copyStateFrom(const AnimationState& animState);

        /// Get the parent animation state set
        [[nodiscard]] auto getParent() const noexcept -> AnimationStateSet* { return mParent; }

      /** @brief Create a new blend mask with the given number of entries
       *
       * In addition to assigning a single weight value to a skeletal animation,
       * it may be desirable to assign animation weights per bone using a 'blend mask'.
       *
       * @param blendMaskSizeHint 
       *   The number of bones of the skeleton owning this AnimationState.
       * @param initialWeight
       *   The value all the blend mask entries will be initialised with (negative to skip initialisation)
       */
      void createBlendMask(size_t blendMaskSizeHint, float initialWeight = 1.0f);
      /// Destroy the currently set blend mask
      void destroyBlendMask();
      /** @brief Set the blend mask data (might be dangerous)
       *
       * @par The size of the array should match the number of entries the
       *      blend mask was created with.
       *
       * @par Stick to the setBlendMaskEntry method if you don't know exactly what you're doing.
       */
      void _setBlendMaskData(const float* blendMaskData);
      /** @brief Set the blend mask
       *
       * @par The size of the array should match the number of entries the
       *      blend mask was created with.
       *
       * @par Stick to the setBlendMaskEntry method if you don't know exactly what you're doing.
       */
      void _setBlendMask(const BoneBlendMask* blendMask);
      /// Get the current blend mask (const version, may be 0) 
      [[nodiscard]] auto getBlendMask() const noexcept -> const BoneBlendMask* {return mBlendMask;}
      /// Return whether there is currently a valid blend mask set
      [[nodiscard]] auto hasBlendMask() const -> bool {return mBlendMask != nullptr;}
      /// Set the weight for the bone identified by the given handle
      void setBlendMaskEntry(size_t boneHandle, float weight);
      /// Get the weight for the bone identified by the given handle
      [[nodiscard]] inline auto getBlendMaskEntry(size_t boneHandle) const -> float
      {
          assert(mBlendMask && mBlendMask->size() > boneHandle);
          return (*mBlendMask)[boneHandle];
      }
    private:
        /// The blend mask (containing per bone weights)
        BoneBlendMask* mBlendMask;

        String mAnimationName;
        AnimationStateSet* mParent;
        Real mTimePos;
        Real mLength;
        Real mWeight;
        bool mEnabled;
        bool mLoop;

    };

    // A map of animation states
    using AnimationStateMap = std::map<std::string_view, AnimationState *>;
    using AnimationStateIterator = MapIterator<AnimationStateMap>;
    using ConstAnimationStateIterator = ConstMapIterator<AnimationStateMap>;
    // A list of enabled animation states
    using EnabledAnimationStateList = std::list<AnimationState *>;
    using ConstEnabledAnimationStateIterator = ConstVectorIterator<EnabledAnimationStateList>;

    /** Class encapsulating a set of AnimationState objects.
    */
    class AnimationStateSet : public AnimationAlloc
    {
    public:
        /// Create a blank animation state set
        AnimationStateSet();
        /// Create an animation set by copying the contents of another
        AnimationStateSet(const AnimationStateSet& rhs);

        ~AnimationStateSet();

        /** Create a new AnimationState instance. 
        @param animName The name of the animation
        @param timePos Starting time position
        @param length Length of the animation to play
        @param weight Weight to apply the animation with 
        @param enabled Whether the animation is enabled
        */
        auto createAnimationState(std::string_view animName,  
            Real timePos, Real length, Real weight = 1.0, bool enabled = false) -> AnimationState*;
        /// Get an animation state by the name of the animation
        [[nodiscard]] auto getAnimationState(std::string_view name) const -> AnimationState*;
        /// Tests if state for the named animation is present
        [[nodiscard]] auto hasAnimationState(std::string_view name) const -> bool;
        /// Remove animation state with the given name
        void removeAnimationState(std::string_view name);
        /// Remove all animation states
        void removeAllAnimationStates();

        /** Get an iterator over all the animation states in this set.
        @deprecated use getAnimationStates()
        */
        auto getAnimationStateIterator() -> AnimationStateIterator;

        /** Get all the animation states in this set.
        @note
            This method is not threadsafe,
            you will need to manually lock the public mutex on this
            class to ensure thread safety if you need it.
        */
        [[nodiscard]] auto getAnimationStates() const noexcept -> const AnimationStateMap& {
            return mAnimationStates;
        }

        /// Copy the state of any matching animation states from this to another
        void copyMatchingState(AnimationStateSet* target) const;
        /// Set the dirty flag and dirty frame number on this state set
        void _notifyDirty();
        /// Get the latest animation state been altered frame number
        [[nodiscard]] auto getDirtyFrameNumber() const noexcept -> unsigned long { return mDirtyFrameNumber; }

        /// Internal method respond to enable/disable an animation state
        void _notifyAnimationStateEnabled(AnimationState* target, bool enabled);
        /// Tests if exists enabled animation state in this set
        [[nodiscard]] auto hasEnabledAnimationState() const -> bool { return !mEnabledAnimationStates.empty(); }

        /** Get an iterator over all the enabled animation states in this set
        @note
            The iterator returned from this method is not threadsafe,
            you will need to manually lock the public mutex on this
            class to ensure thread safety if you need it.
        */
        [[nodiscard]] auto getEnabledAnimationStates() const noexcept -> const EnabledAnimationStateList& {
            return mEnabledAnimationStates;
        }

    private:
        unsigned long mDirtyFrameNumber;
        AnimationStateMap mAnimationStates;
        EnabledAnimationStateList mEnabledAnimationStates;

    };

    /** ControllerValue wrapper class for AnimationState.
    @remarks
        In Azathoth and earlier, AnimationState was a ControllerValue but this
        actually causes memory problems since Controllers delete their values
        automatically when there are no further references to them, but AnimationState
        is deleted explicitly elsewhere so this causes double-free problems.
        This wrapper acts as a bridge and it is this which is destroyed automatically.
    */
    class AnimationStateControllerValue : public ControllerValue<Real>
    {
    private:
        AnimationState* mTargetAnimationState;
        bool mAddTime;
    public:
        /// @deprecated use create instead
        AnimationStateControllerValue(AnimationState* targetAnimationState, bool addTime = false)
            : mTargetAnimationState(targetAnimationState), mAddTime(addTime) {}

        /**
         * create an instance of this class
         * @param targetAnimationState
         * @param addTime if true, increment time instead of setting to an absolute position
         */
        static auto create(AnimationState* targetAnimationState, bool addTime = false) -> ControllerValueRealPtr;

        /** ControllerValue implementation. */
        [[nodiscard]] auto getValue() const -> Real override
        {
            return mTargetAnimationState->getTimePosition() / mTargetAnimationState->getLength();
        }

        /** ControllerValue implementation. */
        void setValue(Real value) override
        {
            if(mAddTime)
                mTargetAnimationState->addTime(value);
            else
                mTargetAnimationState->setTimePosition(value * mTargetAnimationState->getLength());
        }
    };

    /** @} */   
    /** @} */
}

#endif
