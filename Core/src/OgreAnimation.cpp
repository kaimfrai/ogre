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
#include <algorithm>
#include <cmath>
#include <iterator>
#include <list>
#include <string>
#include <utility>

#include "OgreAnimation.hpp"
#include "OgreBone.hpp"
#include "OgreEntity.hpp"
#include "OgreException.hpp"
#include "OgreKeyFrame.hpp"
#include "OgreMesh.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreSkeleton.hpp"
#include "OgreStringConverter.hpp"
#include "OgreSubEntity.hpp"

namespace Ogre {
class Node;
class VertexData;

    Animation::InterpolationMode Animation::msDefaultInterpolationMode = Animation::IM_LINEAR;
    Animation::RotationInterpolationMode 
        Animation::msDefaultRotationInterpolationMode = Animation::RIM_LINEAR;
    //---------------------------------------------------------------------
    Animation::Animation(const String& name, Real length)
        : mName(name)
        , mLength(length)
        , mInterpolationMode(msDefaultInterpolationMode)
        , mRotationInterpolationMode(msDefaultRotationInterpolationMode)
        , mKeyFrameTimesDirty(false)
        , mUseBaseKeyFrame(false)
        , mBaseKeyFrameTime(0.0f)
        , mBaseKeyFrameAnimationName(BLANKSTRING)
        , mContainer(0)
    {
    }
    //---------------------------------------------------------------------
    Animation::~Animation()
    {
        destroyAllTracks();
    }
    //---------------------------------------------------------------------
    Real Animation::getLength() const
    {
        return mLength;
    }
    //---------------------------------------------------------------------
    void Animation::setLength(Real len)
    {
        mLength = len;
    }
    //---------------------------------------------------------------------
    NodeAnimationTrack* Animation::createNodeTrack(unsigned short handle)
    {
        if (hasNodeTrack(handle))
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
                "Node track with the specified handle " +
                StringConverter::toString(handle) + " already exists",
                "Animation::createNodeTrack");
        }

        NodeAnimationTrack* ret = new NodeAnimationTrack(this, handle);

        mNodeTrackList[handle] = ret;
        return ret;
    }
    //---------------------------------------------------------------------
    NodeAnimationTrack* Animation::createNodeTrack(unsigned short handle, Node* node)
    {
        NodeAnimationTrack* ret = createNodeTrack(handle);

        ret->setAssociatedNode(node);

        return ret;
    }
    //---------------------------------------------------------------------
    unsigned short Animation::getNumNodeTracks() const
    {
        return (unsigned short)mNodeTrackList.size();
    }
    //---------------------------------------------------------------------
    bool Animation::hasNodeTrack(unsigned short handle) const
    {
        return (mNodeTrackList.find(handle) != mNodeTrackList.end());
    }
    //---------------------------------------------------------------------
    NodeAnimationTrack* Animation::getNodeTrack(unsigned short handle) const
    {
        NodeTrackList::const_iterator i = mNodeTrackList.find(handle);

        if (i == mNodeTrackList.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
                "Cannot find node track with the specified handle " +
                StringConverter::toString(handle),
                "Animation::getNodeTrack");
        }

        return i->second;

    }
    //---------------------------------------------------------------------
    void Animation::destroyNodeTrack(unsigned short handle)
    {
        NodeTrackList::iterator i = mNodeTrackList.find(handle);

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
        NodeTrackList::iterator i;
        for (i = mNodeTrackList.begin(); i != mNodeTrackList.end(); ++i)
        {
            delete i->second;
        }
        mNodeTrackList.clear();
        _keyFrameListChanged();
    }
    //---------------------------------------------------------------------
    NumericAnimationTrack* Animation::createNumericTrack(unsigned short handle)
    {
        if (hasNumericTrack(handle))
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
                "Numeric track with the specified handle " +
                StringConverter::toString(handle) + " already exists",
                "Animation::createNumericTrack");
        }

        NumericAnimationTrack* ret = new NumericAnimationTrack(this, handle);

        mNumericTrackList[handle] = ret;
        return ret;
    }
    //---------------------------------------------------------------------
    NumericAnimationTrack* Animation::createNumericTrack(unsigned short handle, 
        const AnimableValuePtr& anim)
    {
        NumericAnimationTrack* ret = createNumericTrack(handle);

        ret->setAssociatedAnimable(anim);

        return ret;
    }
    //---------------------------------------------------------------------
    unsigned short Animation::getNumNumericTracks() const
    {
        return (unsigned short)mNumericTrackList.size();
    }
    //---------------------------------------------------------------------
    bool Animation::hasNumericTrack(unsigned short handle) const
    {
        return (mNumericTrackList.find(handle) != mNumericTrackList.end());
    }
    //---------------------------------------------------------------------
    NumericAnimationTrack* Animation::getNumericTrack(unsigned short handle) const
    {
        NumericTrackList::const_iterator i = mNumericTrackList.find(handle);

        if (i == mNumericTrackList.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
                "Cannot find numeric track with the specified handle " +
                StringConverter::toString(handle),
                "Animation::getNumericTrack");
        }

        return i->second;

    }
    //---------------------------------------------------------------------
    void Animation::destroyNumericTrack(unsigned short handle)
    {
        NumericTrackList::iterator i = mNumericTrackList.find(handle);

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
        NumericTrackList::iterator i;
        for (i = mNumericTrackList.begin(); i != mNumericTrackList.end(); ++i)
        {
            delete i->second;
        }
        mNumericTrackList.clear();
        _keyFrameListChanged();
    }
    //---------------------------------------------------------------------
    VertexAnimationTrack* Animation::createVertexTrack(unsigned short handle, 
        VertexAnimationType animType)
    {
        if (hasVertexTrack(handle))
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
                "Vertex track with the specified handle " +
                StringConverter::toString(handle) + " already exists",
                "Animation::createVertexTrack");
        }

        VertexAnimationTrack* ret = new VertexAnimationTrack(this, handle, animType);

        mVertexTrackList[handle] = ret;
        return ret;

    }
    //---------------------------------------------------------------------
    VertexAnimationTrack* Animation::createVertexTrack(unsigned short handle, 
        VertexData* data, VertexAnimationType animType)
    {
        VertexAnimationTrack* ret = createVertexTrack(handle, animType);

        ret->setAssociatedVertexData(data);

        return ret;
    }
    //---------------------------------------------------------------------
    unsigned short Animation::getNumVertexTracks() const
    {
        return (unsigned short)mVertexTrackList.size();
    }
    //---------------------------------------------------------------------
    bool Animation::hasVertexTrack(unsigned short handle) const
    {
        return (mVertexTrackList.find(handle) != mVertexTrackList.end());
    }
    //---------------------------------------------------------------------
    VertexAnimationTrack* Animation::getVertexTrack(unsigned short handle) const
    {
        VertexTrackList::const_iterator i = mVertexTrackList.find(handle);

        if (i == mVertexTrackList.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
                "Cannot find vertex track with the specified handle " +
                StringConverter::toString(handle),
                "Animation::getVertexTrack");
        }

        return i->second;

    }
    //---------------------------------------------------------------------
    void Animation::destroyVertexTrack(unsigned short handle)
    {
        VertexTrackList::iterator i = mVertexTrackList.find(handle);

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
        VertexTrackList::iterator i;
        for (i = mVertexTrackList.begin(); i != mVertexTrackList.end(); ++i)
        {
            delete  i->second;
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
    const String& Animation::getName() const
    {
        return mName;
    }
    //---------------------------------------------------------------------
    void Animation::apply(Real timePos, Real weight, Real scale)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
        TimeIndex timeIndex = _getTimeIndex(timePos);

        NodeTrackList::iterator i;
        for (i = mNodeTrackList.begin(); i != mNodeTrackList.end(); ++i)
        {
            i->second->apply(timeIndex, weight, scale);
        }
        NumericTrackList::iterator j;
        for (j = mNumericTrackList.begin(); j != mNumericTrackList.end(); ++j)
        {
            j->second->apply(timeIndex, weight, scale);
        }
        VertexTrackList::iterator k;
        for (k = mVertexTrackList.begin(); k != mVertexTrackList.end(); ++k)
        {
            k->second->apply(timeIndex, weight, scale);
        }

    }
    //---------------------------------------------------------------------
    void Animation::applyToNode(Node* node, Real timePos, Real weight, Real scale)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
        TimeIndex timeIndex = _getTimeIndex(timePos);

        NodeTrackList::iterator i;
        for (i = mNodeTrackList.begin(); i != mNodeTrackList.end(); ++i)
        {
            i->second->applyToNode(node, timeIndex, weight, scale);
        }
    }
    //---------------------------------------------------------------------
    void Animation::apply(Skeleton* skel, Real timePos, Real weight, 
        Real scale)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
        TimeIndex timeIndex = _getTimeIndex(timePos);

        NodeTrackList::iterator i;
        for (i = mNodeTrackList.begin(); i != mNodeTrackList.end(); ++i)
        {
            // get bone to apply to 
            Bone* b = skel->getBone(i->first);
            i->second->applyToNode(b, timeIndex, weight, scale);
        }


    }
    //---------------------------------------------------------------------
    void Animation::apply(Skeleton* skel, Real timePos, float weight,
      const AnimationState::BoneBlendMask* blendMask, Real scale)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
      TimeIndex timeIndex = _getTimeIndex(timePos);

      NodeTrackList::iterator i;
      for (i = mNodeTrackList.begin(); i != mNodeTrackList.end(); ++i)
      {
        // get bone to apply to 
        Bone* b = skel->getBone(i->first);
        i->second->applyToNode(b, timeIndex, (*blendMask)[b->getHandle()] * weight, scale);
      }
    }
    //---------------------------------------------------------------------
    void Animation::apply(Entity* entity, Real timePos, Real weight, 
        bool software, bool hardware)
    {
        _applyBaseKeyFrame();

        // Calculate time index for fast keyframe search
        TimeIndex timeIndex = _getTimeIndex(timePos);

        VertexTrackList::iterator i;
        for (i = mVertexTrackList.begin(); i != mVertexTrackList.end(); ++i)
        {
            unsigned short handle = i->first;
            VertexAnimationTrack* track = i->second;

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
                track->setTargetMode(VertexAnimationTrack::TM_SOFTWARE);
                track->applyToVertexData(swVertexData, timeIndex, weight, 
                    &(entity->getMesh()->getPoseList()));
            }
            if (hardware)
            {
                track->setTargetMode(VertexAnimationTrack::TM_HARDWARE);
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

        NumericTrackList::iterator j;
        for (j = mNumericTrackList.begin(); j != mNumericTrackList.end(); ++j)
        {
            j->second->applyToAnimable(anim, timePos, weight, scale);
        }
   }
    //---------------------------------------------------------------------
    void Animation::applyToVertexData(VertexData* data, Real timePos, Real weight)
    {
        _applyBaseKeyFrame();
        
        // Calculate time index for fast keyframe search
        TimeIndex timeIndex = _getTimeIndex(timePos);

        VertexTrackList::iterator k;
        for (k = mVertexTrackList.begin(); k != mVertexTrackList.end(); ++k)
        {
            k->second->applyToVertexData(data, timeIndex, weight);
        }
    }
    //---------------------------------------------------------------------
    void Animation::setInterpolationMode(InterpolationMode im)
    {
        mInterpolationMode = im;
    }
    //---------------------------------------------------------------------
    Animation::InterpolationMode Animation::getInterpolationMode() const
    {
        return mInterpolationMode;
    }
    //---------------------------------------------------------------------
    void Animation::setDefaultInterpolationMode(InterpolationMode im)
    {
        msDefaultInterpolationMode = im;
    }
    //---------------------------------------------------------------------
    Animation::InterpolationMode Animation::getDefaultInterpolationMode()
    {
        return msDefaultInterpolationMode;
    }
    //---------------------------------------------------------------------
    const Animation::NodeTrackList& Animation::_getNodeTrackList() const
    {
        return mNodeTrackList;

    }
    //---------------------------------------------------------------------
    const Animation::NumericTrackList& Animation::_getNumericTrackList() const
    {
        return mNumericTrackList;
    }
    //---------------------------------------------------------------------
    const Animation::VertexTrackList& Animation::_getVertexTrackList() const
    {
        return mVertexTrackList;
    }
    //---------------------------------------------------------------------
    void Animation::setRotationInterpolationMode(RotationInterpolationMode im)
    {
        mRotationInterpolationMode = im;
    }
    //---------------------------------------------------------------------
    Animation::RotationInterpolationMode Animation::getRotationInterpolationMode() const
    {
        return mRotationInterpolationMode;
    }
    //---------------------------------------------------------------------
    void Animation::setDefaultRotationInterpolationMode(RotationInterpolationMode im)
    {
        msDefaultRotationInterpolationMode = im;
    }
    //---------------------------------------------------------------------
    Animation::RotationInterpolationMode Animation::getDefaultRotationInterpolationMode()
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
        NodeTrackList::const_iterator i, iend;
        iend = mNodeTrackList.end();
        for (i = mNodeTrackList.begin(); i != iend; ++i)
        {
            const NodeAnimationTrack* track = i->second;
            if (track->hasNonZeroKeyFrames())
            {
                tracks.erase(i->first);
            }
        }
    }
    //-----------------------------------------------------------------------
    void Animation::_destroyNodeTracks(const TrackHandleList& tracks)
    {
        TrackHandleList::const_iterator t, tend;
        tend = tracks.end();
        for (t = tracks.begin(); t != tend; ++t)
        {
            destroyNodeTrack(*t);
        }
    }
    //-----------------------------------------------------------------------
    void Animation::optimiseNodeTracks(bool discardIdentityTracks)
    {
        // Iterate over the node tracks and identify those with no useful keyframes
        std::list<unsigned short> tracksToDestroy;
        NodeTrackList::iterator i;
        for (i = mNodeTrackList.begin(); i != mNodeTrackList.end(); ++i)
        {
            NodeAnimationTrack* track = i->second;
            if (discardIdentityTracks && !track->hasNonZeroKeyFrames())
            {
                // mark the entire track for destruction
                tracksToDestroy.push_back(i->first);
            }
            else
            {
                track->optimise();
            }

        }

        // Now destroy the tracks we marked for death
        for(std::list<unsigned short>::iterator h = tracksToDestroy.begin();
            h != tracksToDestroy.end(); ++h)
        {
            destroyNodeTrack(*h);
        }
    }
    //-----------------------------------------------------------------------
    void Animation::optimiseVertexTracks()
    {
        // Iterate over the node tracks and identify those with no useful keyframes
        std::list<unsigned short> tracksToDestroy;
        VertexTrackList::iterator i;
        for (i = mVertexTrackList.begin(); i != mVertexTrackList.end(); ++i)
        {
            VertexAnimationTrack* track = i->second;
            if (!track->hasNonZeroKeyFrames())
            {
                // mark the entire track for destruction
                tracksToDestroy.push_back(i->first);
            }
            else
            {
                track->optimise();
            }

        }

        // Now destroy the tracks we marked for death
        for(std::list<unsigned short>::iterator h = tracksToDestroy.begin();
            h != tracksToDestroy.end(); ++h)
        {
            destroyVertexTrack(*h);
        }

    }
    //-----------------------------------------------------------------------
    Animation* Animation::clone(const String& newName) const
    {
        Animation* newAnim = new Animation(newName, mLength);
        newAnim->mInterpolationMode = mInterpolationMode;
        newAnim->mRotationInterpolationMode = mRotationInterpolationMode;
        
        // Clone all tracks
        for (NodeTrackList::const_iterator i = mNodeTrackList.begin();
            i != mNodeTrackList.end(); ++i)
        {
            i->second->_clone(newAnim);
        }
        for (NumericTrackList::const_iterator i = mNumericTrackList.begin();
            i != mNumericTrackList.end(); ++i)
        {
            i->second->_clone(newAnim);
        }
        for (VertexTrackList::const_iterator i = mVertexTrackList.begin();
            i != mVertexTrackList.end(); ++i)
        {
            i->second->_clone(newAnim);
        }

        newAnim->_keyFrameListChanged();
        return newAnim;

    }
    //-----------------------------------------------------------------------
    TimeIndex Animation::_getTimeIndex(Real timePos) const
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
        KeyFrameTimeList::iterator it =
            std::lower_bound(mKeyFrameTimes.begin(), mKeyFrameTimes.end(), timePos);

        return TimeIndex(timePos, static_cast<uint>(std::distance(mKeyFrameTimes.begin(), it)));
    }
    //-----------------------------------------------------------------------
    void Animation::buildKeyFrameTimeList() const
    {
        NodeTrackList::const_iterator i;
        NumericTrackList::const_iterator j;
        VertexTrackList::const_iterator k;

        // Clear old keyframe times
        mKeyFrameTimes.clear();

        // Collect all keyframe times from each track
        for (i = mNodeTrackList.begin(); i != mNodeTrackList.end(); ++i)
        {
            i->second->_collectKeyFrameTimes(mKeyFrameTimes);
        }
        for (j = mNumericTrackList.begin(); j != mNumericTrackList.end(); ++j)
        {
            j->second->_collectKeyFrameTimes(mKeyFrameTimes);
        }
        for (k = mVertexTrackList.begin(); k != mVertexTrackList.end(); ++k)
        {
            k->second->_collectKeyFrameTimes(mKeyFrameTimes);
        }

        // Build global index to local index map for each track
        for (i = mNodeTrackList.begin(); i != mNodeTrackList.end(); ++i)
        {
            i->second->_buildKeyFrameIndexMap(mKeyFrameTimes);
        }
        for (j = mNumericTrackList.begin(); j != mNumericTrackList.end(); ++j)
        {
            j->second->_buildKeyFrameIndexMap(mKeyFrameTimes);
        }
        for (k = mVertexTrackList.begin(); k != mVertexTrackList.end(); ++k)
        {
            k->second->_buildKeyFrameIndexMap(mKeyFrameTimes);
        }

        // Reset dirty flag
        mKeyFrameTimesDirty = false;
    }
    //-----------------------------------------------------------------------
    void Animation::setUseBaseKeyFrame(bool useBaseKeyFrame, Real keyframeTime, const String& baseAnimName)
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
    bool Animation::getUseBaseKeyFrame() const
    {
        return mUseBaseKeyFrame;
    }
    //-----------------------------------------------------------------------
    Real Animation::getBaseKeyFrameTime() const
    {
        return mBaseKeyFrameTime;
    }
    //-----------------------------------------------------------------------
    const String& Animation::getBaseKeyFrameAnimationName() const
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
                for (NodeTrackList::iterator i = mNodeTrackList.begin(); i != mNodeTrackList.end(); ++i)
                {
                    NodeAnimationTrack* track = i->second;
                    
                    NodeAnimationTrack* baseTrack;
                    if (baseAnim == this)
                        baseTrack = track;
                    else
                        baseTrack = baseAnim->getNodeTrack(track->getHandle());
                    
                    TransformKeyFrame kf(baseTrack, mBaseKeyFrameTime);
                    baseTrack->getInterpolatedKeyFrame(baseAnim->_getTimeIndex(mBaseKeyFrameTime), &kf);
                    track->_applyBaseKeyFrame(&kf);
                }
                
                for (VertexTrackList::iterator i = mVertexTrackList.begin(); i != mVertexTrackList.end(); ++i)
                {
                    VertexAnimationTrack* track = i->second;
                    
                    if (track->getAnimationType() == VAT_POSE)
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
    AnimationContainer* Animation::getContainer()
    {
        return mContainer;
    }
    

}


