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
#include <cassert>
#include <cstddef>
#include <vector>

#include "OgreBone.hpp"
#include "OgreNode.hpp"
#include "OgreSkeletonInstance.hpp"
#include "OgreTagPoint.hpp"


namespace Ogre {
class Animation;
class AnimationStateSet;

    //-------------------------------------------------------------------------
    SkeletonInstance::SkeletonInstance(const SkeletonPtr& masterCopy) 
        : Skeleton()
        , mSkeleton(masterCopy)
         
    {
    }
    //-------------------------------------------------------------------------
    SkeletonInstance::~SkeletonInstance()
    {
        // have to call this here rather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        // ...and calling it in Skeleton destructor does not unload
        // SkeletonInstance since it has seized to be by then.
        unload();
    }
    //-------------------------------------------------------------------------
    auto SkeletonInstance::getNumAnimations() const -> unsigned short
    {
        return mSkeleton->getNumAnimations();
    }
    //-------------------------------------------------------------------------
    auto SkeletonInstance::getAnimation(unsigned short index) const -> Animation*
    {
        return mSkeleton->getAnimation(index);
    }
    //-------------------------------------------------------------------------
    auto SkeletonInstance::createAnimation(const String& name, Real length) -> Animation*
    {
        return mSkeleton->createAnimation(name, length);
    }
    //-------------------------------------------------------------------------
    auto SkeletonInstance::getAnimation(const String& name, 
        const LinkedSkeletonAnimationSource** linker) const -> Animation*
    {
        return mSkeleton->getAnimation(name, linker);
    }
    //-------------------------------------------------------------------------
    auto SkeletonInstance::_getAnimationImpl(const String& name, 
        const LinkedSkeletonAnimationSource** linker) const -> Animation*
    {
        return mSkeleton->_getAnimationImpl(name, linker);
    }
    //-------------------------------------------------------------------------
    void SkeletonInstance::removeAnimation(const String& name)
    {
        mSkeleton->removeAnimation(name);
    }
    //-------------------------------------------------------------------------
    void SkeletonInstance::addLinkedSkeletonAnimationSource(const String& skelName, 
        Real scale)
    {
        mSkeleton->addLinkedSkeletonAnimationSource(skelName, scale);
    }
    //-------------------------------------------------------------------------
    void SkeletonInstance::removeAllLinkedSkeletonAnimationSources()
    {
        mSkeleton->removeAllLinkedSkeletonAnimationSources();
    }
    //-------------------------------------------------------------------------
    auto
    SkeletonInstance::getLinkedSkeletonAnimationSources() const -> const Skeleton::LinkedSkeletonAnimSourceList&
    {
        return mSkeleton->getLinkedSkeletonAnimationSources();
    }

    //-------------------------------------------------------------------------
    void SkeletonInstance::_initAnimationState(AnimationStateSet* animSet)
    {
        mSkeleton->_initAnimationState(animSet);
    }
    //-------------------------------------------------------------------------
    void SkeletonInstance::_refreshAnimationState(AnimationStateSet* animSet)
    {
        mSkeleton->_refreshAnimationState(animSet);
    }
    //-------------------------------------------------------------------------
    void SkeletonInstance::cloneBoneAndChildren(Bone* source, Bone* parent)
    {
        Bone* newBone;
        if (source->getName().empty())
        {
            newBone = createBone(source->getHandle());
        }
        else
        {
            newBone = createBone(source->getName(), source->getHandle());
        }
        if (parent == nullptr)
        {
            mRootBones.push_back(newBone);
        }
        else
        {
            parent->addChild(newBone);
        }
        newBone->setOrientation(source->getOrientation());
        newBone->setPosition(source->getPosition());
        newBone->setScale(source->getScale());

        // Process children
        for (auto c : source->getChildren())
        {
            cloneBoneAndChildren(static_cast<Bone*>(c), newBone);
        }
    }
    //-------------------------------------------------------------------------
    void SkeletonInstance::prepareImpl()
    {
        mNextAutoHandle = mSkeleton->mNextAutoHandle;
        mNextTagPointAutoHandle = 0;
        // construct self from master
        mBlendState = mSkeleton->mBlendState;
        // Copy bones
        BoneList::const_iterator i;
        for (i = mSkeleton->getRootBones().begin(); i != mSkeleton->getRootBones().end(); ++i)
        {
            Bone* b = *i;
            cloneBoneAndChildren(b, nullptr);
            b->_update(true, false);
        }
        setBindingPose();
    }
    //-------------------------------------------------------------------------
    void SkeletonInstance::unprepareImpl()
    {
        Skeleton::unprepareImpl();

        // destroy TagPoints
        for (auto tagPoint : mActiveTagPoints)
        {
            // Woohoo! The child object all the same attaching this skeleton instance, but is ok we can just
            // ignore it:
            //   1. The parent node of the tagPoint already deleted by Skeleton::unload(), nothing need to do now
            //   2. And the child object relationship already detached by Entity::~Entity()
            delete tagPoint;
        }
        mActiveTagPoints.clear();
        for (auto tagPoint : mFreeTagPoints)
        {
            delete tagPoint;
        }
        mFreeTagPoints.clear();
    }

    //-------------------------------------------------------------------------
    auto SkeletonInstance::createTagPointOnBone(Bone* bone,
        const Quaternion &offsetOrientation, 
        const Vector3 &offsetPosition) -> TagPoint*
    {
        TagPoint* ret;
        if (mFreeTagPoints.empty()) {
            ret = new TagPoint(mNextTagPointAutoHandle++, this);
            mActiveTagPoints.push_back(ret);
        } else {
            ret = mFreeTagPoints.front();
            mActiveTagPoints.splice(
                mActiveTagPoints.end(), mFreeTagPoints, mFreeTagPoints.begin());
            // Initial some members ensure identically behavior, avoiding potential bug.
            ret->setParentEntity(nullptr);
            ret->setChildObject(nullptr);
            ret->setInheritOrientation(true);
            ret->setInheritScale(true);
            ret->setInheritParentEntityOrientation(true);
            ret->setInheritParentEntityScale(true);
        }

        ret->setPosition(offsetPosition);
        ret->setOrientation(offsetOrientation);
        ret->setScale(Vector3::UNIT_SCALE);
        ret->setBindingPose();
        bone->addChild(ret);

        return ret;
    }
    //-------------------------------------------------------------------------
    void SkeletonInstance::freeTagPoint(TagPoint* tagPoint)
    {
        auto it =
            std::find(mActiveTagPoints.begin(), mActiveTagPoints.end(), tagPoint);
        assert(it != mActiveTagPoints.end());
        if (it != mActiveTagPoints.end())
        {
            if (tagPoint->getParent())
                tagPoint->getParent()->removeChild(tagPoint);

            mFreeTagPoints.splice(mFreeTagPoints.end(), mActiveTagPoints, it);
        }
    }
    //-------------------------------------------------------------------------
    auto SkeletonInstance::getName() const -> const String&
    {
        // delegate
        return mSkeleton->getName();
    }
    //-------------------------------------------------------------------------
    auto SkeletonInstance::getHandle() const -> ResourceHandle
    {
        // delegate
        return mSkeleton->getHandle();
    }
    //-------------------------------------------------------------------------
    auto SkeletonInstance::getGroup() const -> const String&
    {
        // delegate
        return mSkeleton->getGroup();
    }

}

