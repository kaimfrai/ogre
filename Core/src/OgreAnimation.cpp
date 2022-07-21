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

#include <cmath>

module Ogre.Core;

import :Animation;
import :Bone;
import :Entity;
import :Exception;
import :KeyFrame;
import :Mesh;
import :SharedPtr;
import :Skeleton;
import :StringConverter;
import :SubEntity;

import <algorithm>;
import <iterator>;
import <list>;
import <string>;
import <utility>;

namespace Ogre {

    Animation::InterpolationMode constinit Animation::msDefaultInterpolationMode = Animation::InterpolationMode::LINEAR;
    Animation::RotationInterpolationMode 
        Animation::msDefaultRotationInterpolationMode = Animation::RotationInterpolationMode::LINEAR;
    //---------------------------------------------------------------------
    Animation::Animation(std::string_view name, Real length)
        : mName(name)
        , mLength(length)
        , mInterpolationMode(msDefaultInterpolationMode)
        , mRotationInterpolationMode(msDefaultRotationInterpolationMode)
        , 
         mBaseKeyFrameAnimationName(BLANKSTRING)
         
    {
    }
    //---------------------------------------------------------------------
    Animation::~Animation()
    {
        destroyAllTracks();
    }
    //---------------------------------------------------------------------
    auto Animation::getLength() const -> Real
    {
        return mLength;
    }
    //---------------------------------------------------------------------
    void Animation::setLength(Real len)
    {
        mLength = len;
    }
    //---------------------------------------------------------------------
    auto Animation::createNodeTrack(unsigned short handle) -> NodeAnimationTrack*
    {
        if (hasNodeTrack(handle))
        {
            OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM, 
                ::std::format("Node track with the specified handle {} already exists", handle),
                "Animation::createNodeTrack");
        }

        auto* ret = new NodeAnimationTrack(this, handle);

        mNodeTrackList[handle] = ret;
        return ret;
    }
    //---------------------------------------------------------------------
    auto Animation::createNodeTrack(unsigned short handle, Node* node) -> NodeAnimationTrack*
    {
        NodeAnimationTrack* ret = createNodeTrack(handle);

        ret->setAssociatedNode(node);

        return ret;
    }
    //---------------------------------------------------------------------
    auto Animation::getNumNodeTracks() const noexcept -> unsigned short
    {
        return (unsigned short)mNodeTrackList.size();
    }
    //---------------------------------------------------------------------
    auto Animation::hasNodeTrack(unsigned short handle) const -> bool
    {
        return (mNodeTrackList.find(handle) != mNodeTrackList.end());
    }
    //---------------------------------------------------------------------
    auto Animation::getNodeTrack(unsigned short handle) const -> NodeAnimationTrack*
    {
        auto i = mNodeTrackList.find(handle);

        if (i == mNodeTrackList.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, 
                ::std::format("Cannot find node track with the specified handle {}", handle),
                "Animation::getNodeTrack");
        }

        return i->second;

    }
    //---------------------------------------------------------------------
    void Animation::destroyNodeTrack(unsigned short handle)
    {
        auto i = mNodeTrackList.find(handle);

        if (i != mNodeTrackList.end())
        {
            delete i->second;
            mNodeTrackList.erase(i);
            _keyFrameListChanged();
        }
    }
    //---------------------------------------------------------------------
    void Animation::destroyAllNodeTracks()
    {
        for (auto & i : mNodeTrackList)
        {
            delete i.second;
        }
        mNodeTrackList.clear();
        _keyFrameListChanged();
    }
    //---------------------------------------------------------------------
    auto Animation::createNumericTrack(unsigned short handle) -> NumericAnimationTrack*
    {
        if (hasNumericTrack(handle))
        {
            OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM, 
                ::std::format("Numeric track with the specified handle {} already exists", handle),
                "Animation::createNumericTrack");
        }

        auto* ret = new NumericAnimationTrack(this, handle);

        mNumericTrackList[handle] = ret;
        return ret;
    }
    //---------------------------------------------------------------------
    auto Animation::createNumericTrack(unsigned short handle, 
        const AnimableValuePtr& anim) -> NumericAnimationTrack*
    {
        NumericAnimationTrack* ret = createNumericTrack(handle);

        ret->setAssociatedAnimable(anim);

        return ret;
    }
    //---------------------------------------------------------------------
    auto Animation::getNumNumericTracks() const noexcept -> unsigned short
    {
        return (unsigned short)mNumericTrackList.size();
    }
    //---------------------------------------------------------------------
    auto Animation::hasNumericTrack(unsigned short handle) const -> bool
    {
        return (mNumericTrackList.find(handle) != mNumericTrackList.end());
    }
    //---------------------------------------------------------------------
    auto Animation::getNumericTrack(unsigned short handle) const -> NumericAnimationTrack*
    {
        auto i = mNumericTrackList.find(handle);

        if (i == mNumericTrackList.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, 
                ::std::format("Cannot find numeric track with the specified handle {}", handle),
                "Animation::getNumericTrack");
        }

        return i->second;

    }
    //---------------------------------------------------------------------
    void Animation::destroyNumericTrack(unsigned short handle)
    {
        auto i = mNumericTrackList.find(handle);

        if (i != mNumericTrackList.end())
        {
            delete i->second;
            mNumericTrackList.erase(i);
            _keyFrameListChanged();
        }
    }
    //---------------------------------------------------------------------
    void Animation::destroyAllNumericTracks()
    {
        for (auto & i : mNumericTrackList)
        {
            delete i.second;
        }
        mNumericTrackList.clear();
        _keyFrameListChanged();
    }
    //---------------------------------------------------------------------
    auto Animation::createVertexTrack(unsigned short handle, 
        VertexAnimationType animType) -> VertexAnimationTrack*
    {
        if (hasVertexTrack(handle))
        {
            OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM, 
                ::std::format("Vertex track with the specified handle {} already exists", handle),
                "Animation::createVertexTrack");
        }

        auto* ret = new VertexAnimationTrack(this, handle, animType);

        mVertexTrackList[handle] = ret;
        return ret;

    }
    //---------------------------------------------------------------------
    auto Animation::createVertexTrack(unsigned short handle, 
        VertexData* data, VertexAnimationType animType) -> VertexAnimationTrack*
    {
        VertexAnimationTrack* ret = createVertexTrack(handle, animType);

        ret->setAssociatedVertexData(data);

        return ret;
    }
    //---------------------------------------------------------------------
    auto Animation::getNumVertexTracks() const noexcept -> unsigned short
    {
        return (unsigned short)mVertexTrackList.size();
    }
    //---------------------------------------------------------------------
    auto Animation::hasVertexTrack(unsigned short handle) const -> bool
    {
        return (mVertexTrackList.find(handle) != mVertexTrackList.end());
    }
    //---------------------------------------------------------------------
    auto Animation::getVertexTrack(unsigned short handle) const -> VertexAnimationTrack*
    {
        auto i = mVertexTrackList.find(handle);

        if (i == mVertexTrackList.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, 
                ::std::format("Cannot find vertex track with the specified handle {}", handle),
                "Animation::getVertexTrack");
        }

        return i->second;

    }
    //---------------------------------------------------------------------
    void Animation::destroyVertexTrack(unsigned short handle)
    {
        auto i = mVertexTrackList.find(handle);

        if (i != mVertexTrackList.end())
        {
            delete  i->second;
            mVertexTrackList.erase(i);
            _keyFrameListChanged();
        }
    }
    //---------------------------------------------------------------------
    void Animation::destroyAllVertexTracks()
    {
        for (auto & i : mVertexTrackList)
        {
            delete  i.second;
        }
        mVertexTrackList.clear();
        _keyFrameListChanged();
    }
    //---------------------------------------------------------------------
    void Animation::destroyAllTracks()
    {
        destroyAllNodeTracks();
        destroyAllNumericTracks();
        destroyAllVertexTracks();
    }
    //---------------------------------------------------------------------
    auto Animation::getName() const noexcept -> std::string_view
    {
        return mName;
    }
    //---------------------------------------------------------------------
    void Animation::apply(Real timePos, Real weight, Real scale)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
        TimeIndex timeIndex = _getTimeIndex(timePos);

        for (auto & i : mNodeTrackList)
        {
            i.second->apply(timeIndex, weight, scale);
        }
        for (auto & j : mNumericTrackList)
        {
            j.second->apply(timeIndex, weight, scale);
        }
        for (auto & k : mVertexTrackList)
        {
            k.second->apply(timeIndex, weight, scale);
        }

    }
    //---------------------------------------------------------------------
    void Animation::applyToNode(Node* node, Real timePos, Real weight, Real scale)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
        TimeIndex timeIndex = _getTimeIndex(timePos);

        for (auto & i : mNodeTrackList)
        {
            i.second->applyToNode(node, timeIndex, weight, scale);
        }
    }
    //---------------------------------------------------------------------
    void Animation::apply(Skeleton* skel, Real timePos, Real weight, 
        Real scale)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
        TimeIndex timeIndex = _getTimeIndex(timePos);

        for (auto & i : mNodeTrackList)
        {
            // get bone to apply to 
            Bone* b = skel->getBone(i.first);
            i.second->applyToNode(b, timeIndex, weight, scale);
        }


    }
    //---------------------------------------------------------------------
    void Animation::apply(Skeleton* skel, Real timePos, float weight,
      const AnimationState::BoneBlendMask* blendMask, Real scale)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
      TimeIndex timeIndex = _getTimeIndex(timePos);

      for (auto & i : mNodeTrackList)
      {
        // get bone to apply to 
        Bone* b = skel->getBone(i.first);
        i.second->applyToNode(b, timeIndex, (*blendMask)[b->getHandle()] * weight, scale);
      }
    }
    //---------------------------------------------------------------------
    void Animation::apply(Entity* entity, Real timePos, Real weight, 
        bool software, bool hardware)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
        TimeIndex timeIndex = _getTimeIndex(timePos);

        for (auto const& [handle, track] : mVertexTrackList)
        {
            VertexData* swVertexData;
            VertexData* hwVertexData;
            if (handle == 0)
            {
                // shared vertex data
                swVertexData = entity->_getSoftwareVertexAnimVertexData();
                hwVertexData = entity->_getHardwareVertexAnimVertexData();
                entity->_markBuffersUsedForAnimation();
            }
            else
            {
                // sub entity vertex data (-1)
                SubEntity* s = entity->getSubEntity(handle - 1);
                // Skip this track if subentity is not visible
                if (!s->isVisible())
                    continue;
                swVertexData = s->_getSoftwareVertexAnimVertexData();
                hwVertexData = s->_getHardwareVertexAnimVertexData();
                s->_markBuffersUsedForAnimation();
            }
            // Apply to both hardware and software, if requested
            if (software)
            {
                track->setTargetMode(VertexAnimationTrack::TargetMode::SOFTWARE);
                track->applyToVertexData(swVertexData, timeIndex, weight, 
                    &(entity->getMesh()->getPoseList()));
            }
            if (hardware)
            {
                track->setTargetMode(VertexAnimationTrack::TargetMode::HARDWARE);
                track->applyToVertexData(hwVertexData, timeIndex, weight, 
                    &(entity->getMesh()->getPoseList()));
            }
        }

    }
    //---------------------------------------------------------------------
    void Animation::applyToAnimable(const AnimableValuePtr& anim, Real timePos, Real weight, Real scale)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
        _getTimeIndex(timePos);

        for (auto & j : mNumericTrackList)
        {
            j.second->applyToAnimable(anim, timePos, weight, scale);
        }
   }
    //---------------------------------------------------------------------
    void Animation::applyToVertexData(VertexData* data, Real timePos, Real weight)
    {
        _applyBaseKeyFrame();
        
        // Calculate time index for fast keyframe search
        TimeIndex timeIndex = _getTimeIndex(timePos);

        for (auto & k : mVertexTrackList)
        {
            k.second->applyToVertexData(data, timeIndex, weight);
        }
    }
    //---------------------------------------------------------------------
    void Animation::setInterpolationMode(InterpolationMode im)
    {
        mInterpolationMode = im;
    }
    //---------------------------------------------------------------------
    auto Animation::getInterpolationMode() const -> Animation::InterpolationMode
    {
        return mInterpolationMode;
    }
    //---------------------------------------------------------------------
    void Animation::setDefaultInterpolationMode(InterpolationMode im)
    {
        msDefaultInterpolationMode = im;
    }
    //---------------------------------------------------------------------
    auto Animation::getDefaultInterpolationMode() -> Animation::InterpolationMode
    {
        return msDefaultInterpolationMode;
    }
    //---------------------------------------------------------------------
    auto Animation::_getNodeTrackList() const -> const Animation::NodeTrackList&
    {
        return mNodeTrackList;

    }
    //---------------------------------------------------------------------
    auto Animation::_getNumericTrackList() const -> const Animation::NumericTrackList&
    {
        return mNumericTrackList;
    }
    //---------------------------------------------------------------------
    auto Animation::_getVertexTrackList() const -> const Animation::VertexTrackList&
    {
        return mVertexTrackList;
    }
    //---------------------------------------------------------------------
    void Animation::setRotationInterpolationMode(RotationInterpolationMode im)
    {
        mRotationInterpolationMode = im;
    }
    //---------------------------------------------------------------------
    auto Animation::getRotationInterpolationMode() const -> Animation::RotationInterpolationMode
    {
        return mRotationInterpolationMode;
    }
    //---------------------------------------------------------------------
    void Animation::setDefaultRotationInterpolationMode(RotationInterpolationMode im)
    {
        msDefaultRotationInterpolationMode = im;
    }
    //---------------------------------------------------------------------
    auto Animation::getDefaultRotationInterpolationMode() -> Animation::RotationInterpolationMode
    {
        return msDefaultRotationInterpolationMode;
    }
    //---------------------------------------------------------------------
    void Animation::optimise(bool discardIdentityNodeTracks)
    {
        optimiseNodeTracks(discardIdentityNodeTracks);
        optimiseVertexTracks();
        
    }
    //-----------------------------------------------------------------------
    void Animation::_collectIdentityNodeTracks(TrackHandleList& tracks) const
    {
        for (auto [key, track] : mNodeTrackList)
        {
            if (track->hasNonZeroKeyFrames())
            {
                tracks.erase(key);
            }
        }
    }
    //-----------------------------------------------------------------------
    void Animation::_destroyNodeTracks(const TrackHandleList& tracks)
    {
        for (unsigned short track : tracks)
        {
            destroyNodeTrack(track);
        }
    }
    //-----------------------------------------------------------------------
    void Animation::optimiseNodeTracks(bool discardIdentityTracks)
    {
        // Iterate over the node tracks and identify those with no useful keyframes
        std::list<unsigned short> tracksToDestroy;
        for (auto const& [key, track] : mNodeTrackList)
        {
            if (discardIdentityTracks && !track->hasNonZeroKeyFrames())
            {
                // mark the entire track for destruction
                tracksToDestroy.push_back(key);
            }
            else
            {
                track->optimise();
            }

        }

        // Now destroy the tracks we marked for death
        for(unsigned short & h : tracksToDestroy)
        {
            destroyNodeTrack(h);
        }
    }
    //-----------------------------------------------------------------------
    void Animation::optimiseVertexTracks()
    {
        // Iterate over the node tracks and identify those with no useful keyframes
        std::list<unsigned short> tracksToDestroy;
        for (auto const& [key, track] : mVertexTrackList)
        {
            if (!track->hasNonZeroKeyFrames())
            {
                // mark the entire track for destruction
                tracksToDestroy.push_back(key);
            }
            else
            {
                track->optimise();
            }

        }

        // Now destroy the tracks we marked for death
        for(unsigned short & h : tracksToDestroy)
        {
            destroyVertexTrack(h);
        }

    }
    //-----------------------------------------------------------------------
    auto Animation::clone(std::string_view newName) const -> Animation*
    {
        auto* newAnim = new Animation(newName, mLength);
        newAnim->mInterpolationMode = mInterpolationMode;
        newAnim->mRotationInterpolationMode = mRotationInterpolationMode;
        
        // Clone all tracks
        for (auto i : mNodeTrackList)
        {
            i.second->_clone(newAnim);
        }
        for (auto i : mNumericTrackList)
        {
            i.second->_clone(newAnim);
        }
        for (auto i : mVertexTrackList)
        {
            i.second->_clone(newAnim);
        }

        newAnim->_keyFrameListChanged();
        return newAnim;

    }
    //-----------------------------------------------------------------------
    auto Animation::_getTimeIndex(Real timePos) const -> TimeIndex
    {
        // Uncomment following statement for work as previous
        //return timePos;

        // Build keyframe time list on demand
        if (mKeyFrameTimesDirty)
        {
            buildKeyFrameTimeList();
        }

        // Wrap time
        Real totalAnimationLength = mLength;

        if( timePos > totalAnimationLength && totalAnimationLength > 0.0f )
            timePos = std::fmod( timePos, totalAnimationLength );

        // Search for global index
        auto it =
            std::ranges::lower_bound(mKeyFrameTimes, timePos);

        return {timePos, static_cast<uint>(std::distance(mKeyFrameTimes.begin(), it))};
    }
    //-----------------------------------------------------------------------
    void Animation::buildKeyFrameTimeList() const
    {
        // Clear old keyframe times
        mKeyFrameTimes.clear();

        // Collect all keyframe times from each track
        for (auto i : mNodeTrackList)
        {
            i.second->_collectKeyFrameTimes(mKeyFrameTimes);
        }
        for (auto j : mNumericTrackList)
        {
            j.second->_collectKeyFrameTimes(mKeyFrameTimes);
        }
        for (auto k : mVertexTrackList)
        {
            k.second->_collectKeyFrameTimes(mKeyFrameTimes);
        }

        // Build global index to local index map for each track
        for (auto i : mNodeTrackList)
        {
            i.second->_buildKeyFrameIndexMap(mKeyFrameTimes);
        }
        for (auto j : mNumericTrackList)
        {
            j.second->_buildKeyFrameIndexMap(mKeyFrameTimes);
        }
        for (auto k : mVertexTrackList)
        {
            k.second->_buildKeyFrameIndexMap(mKeyFrameTimes);
        }

        // Reset dirty flag
        mKeyFrameTimesDirty = false;
    }
    //-----------------------------------------------------------------------
    void Animation::setUseBaseKeyFrame(bool useBaseKeyFrame, Real keyframeTime, std::string_view baseAnimName)
    {
        if (useBaseKeyFrame != mUseBaseKeyFrame ||
            keyframeTime != mBaseKeyFrameTime ||
            baseAnimName != mBaseKeyFrameAnimationName)
        {
            mUseBaseKeyFrame = useBaseKeyFrame;
            mBaseKeyFrameTime = keyframeTime;
            mBaseKeyFrameAnimationName = baseAnimName;
        }
    }
    //-----------------------------------------------------------------------
    auto Animation::getUseBaseKeyFrame() const noexcept -> bool
    {
        return mUseBaseKeyFrame;
    }
    //-----------------------------------------------------------------------
    auto Animation::getBaseKeyFrameTime() const -> Real
    {
        return mBaseKeyFrameTime;
    }
    //-----------------------------------------------------------------------
    auto Animation::getBaseKeyFrameAnimationName() const noexcept -> std::string_view
    {
        return mBaseKeyFrameAnimationName;
    }
    //-----------------------------------------------------------------------
    void Animation::_applyBaseKeyFrame()
    {
        if (mUseBaseKeyFrame)
        {
            Animation* baseAnim = this;
            if (!mBaseKeyFrameAnimationName.empty() && mContainer)
                baseAnim = mContainer->getAnimation(mBaseKeyFrameAnimationName);
            
            if (baseAnim)
            {
                for (auto const& [key, track] : mNodeTrackList)
                {
                    NodeAnimationTrack* baseTrack;
                    if (baseAnim == this)
                        baseTrack = track;
                    else
                        baseTrack = baseAnim->getNodeTrack(track->getHandle());
                    
                    TransformKeyFrame kf(baseTrack, mBaseKeyFrameTime);
                    baseTrack->getInterpolatedKeyFrame(baseAnim->_getTimeIndex(mBaseKeyFrameTime), &kf);
                    track->_applyBaseKeyFrame(&kf);
                }
                
                for (auto const& [key, track] : mVertexTrackList)
                {
                    if (track->getAnimationType() == VertexAnimationType::POSE)
                    {
                        VertexAnimationTrack* baseTrack;
                        if (baseAnim == this)
                            baseTrack = track;
                        else
                            baseTrack = baseAnim->getVertexTrack(track->getHandle());
                        
                        VertexPoseKeyFrame kf(baseTrack, mBaseKeyFrameTime);
                        baseTrack->getInterpolatedKeyFrame(baseAnim->_getTimeIndex(mBaseKeyFrameTime), &kf);
                        track->_applyBaseKeyFrame(&kf);
                        
                    }
                }
                
            }
            
            // Re-base has been done, this is a one-way translation
            mUseBaseKeyFrame = false;
        }
        
    }
    //-----------------------------------------------------------------------
    void Animation::_notifyContainer(AnimationContainer* c)
    {
        mContainer = c;
    }
    //-----------------------------------------------------------------------
    auto Animation::getContainer() noexcept -> AnimationContainer*
    {
        return mContainer;
    }
    

}
