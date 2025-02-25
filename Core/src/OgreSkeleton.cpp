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

#include <cassert>
#include <cstddef>

module Ogre.Core;

// Just for logging
import :Animation;
import :AnimationState;
import :AnimationTrack;
import :Bone;
import :Exception;
import :KeyFrame;
import :Log;
import :LogManager;
import :Math;
import :Prerequisites;
import :Quaternion;
import :Resource;
import :ResourceGroupManager;
import :ResourceManager;
import :SharedPtr;
import :Skeleton;
import :SkeletonManager;
import :SkeletonSerializer;
import :StringConverter;
import :StringVector;
import :Vector;

import <algorithm>;
import <format>;
import <iterator>;
import <list>;
import <map>;
import <memory>;
import <ostream>;
import <string>;
import <utility>;
import <vector>;

namespace Ogre {
    //---------------------------------------------------------------------
    Skeleton::Skeleton()
        : Resource(),
        mNextAutoHandle(0),
        mBlendState(SkeletonAnimationBlendMode::AVERAGE),
        mManualBonesDirty(false)
    {
    }
    //---------------------------------------------------------------------
    Skeleton::Skeleton(ResourceManager* creator, std::string_view name, ResourceHandle handle,
        std::string_view group, bool isManual, ManualResourceLoader* loader) 
        : Resource(creator, name, handle, group, isManual, loader), 
        mNextAutoHandle(0), mBlendState(SkeletonAnimationBlendMode::AVERAGE)
        // set animation blending to weighted, not cumulative
    {
        if (createParamDictionary("Skeleton"))
        {
            // no custom params
        }
    }
    //---------------------------------------------------------------------
    Skeleton::~Skeleton()
    {
        // have to call this here reather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        unload(); 
    }
    //---------------------------------------------------------------------
    void Skeleton::prepareImpl()
    {
        SkeletonSerializer serializer;

        if (getCreator()->getVerbose())
            LogManager::getSingleton().stream() << "Skeleton: Loading " << mName;

        DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(mName, mGroup, this);

        serializer.importSkeleton(stream, this);

        // Load any linked skeletons
        for (auto & i : mLinkedSkeletonAnimSourceList)
        {
            i.pSkeleton = static_pointer_cast<Skeleton>(
                SkeletonManager::getSingleton().prepare(i.skeletonName, mGroup));
        }
    }
    //---------------------------------------------------------------------
    void Skeleton::unprepareImpl()
    {
        // destroy bones
        for (auto & i : mBoneList)
        {
            delete i;
        }
        mBoneList.clear();
        mBoneListByName.clear();
        mRootBones.clear();
        mManualBones.clear();
        mManualBonesDirty = false;

        // Destroy animations
        for (auto & ai : mAnimationsList)
        {
            delete ai.second;
        }
        mAnimationsList.clear();

        // Remove all linked skeletons
        mLinkedSkeletonAnimSourceList.clear();
    }
    //---------------------------------------------------------------------
    auto Skeleton::createBone() -> Bone*
    {
        // use autohandle
        return createBone(mNextAutoHandle++);
    }
    //---------------------------------------------------------------------
    auto Skeleton::createBone(std::string_view name) -> Bone*
    {
        return createBone(name, mNextAutoHandle++);
    }
    //---------------------------------------------------------------------
    auto Skeleton::createBone(unsigned short handle) -> Bone*
    {
        OgreAssert(handle < OGRE_MAX_NUM_BONES, "Exceeded the maximum number of bones per skeleton");
        // Check handle not used
        if (handle < mBoneList.size() && mBoneList[handle] != nullptr)
        {
            OGRE_EXCEPT(
                ExceptionCodes::DUPLICATE_ITEM,
                ::std::format("A bone with the handle {} already exists", handle ),
                "Skeleton::createBone" );
        }
        Bone* ret = new Bone(handle, this);
        assert(mBoneListByName.find(ret->getName()) == mBoneListByName.end());
        if (mBoneList.size() <= handle)
        {
            mBoneList.resize(handle+1);
        }
        mBoneList[handle] = ret;
        mBoneListByName[ret->getName()] = ret;
        return ret;

    }
    //---------------------------------------------------------------------
    auto Skeleton::createBone(std::string_view name, unsigned short handle) -> Bone*
    {
        OgreAssert(handle < OGRE_MAX_NUM_BONES, "Exceeded the maximum number of bones per skeleton");
        // Check handle not used
        if (handle < mBoneList.size() && mBoneList[handle] != nullptr)
        {
            OGRE_EXCEPT(
                ExceptionCodes::DUPLICATE_ITEM,
                ::std::format("A bone with the handle {} already exists", handle ),
                "Skeleton::createBone" );
        }
        // Check name not used
        if (mBoneListByName.find(name) != mBoneListByName.end())
        {
            OGRE_EXCEPT(
                ExceptionCodes::DUPLICATE_ITEM,
                ::std::format("A bone with the name {} already exists", name),
                "Skeleton::createBone" );
        }
        Bone* ret = new Bone(name, handle, this);
        if (mBoneList.size() <= handle)
        {
            mBoneList.resize(handle+1);
        }
        mBoneList[handle] = ret;
        mBoneListByName[ret->getName()] = ret;
        return ret;
    }

    auto Skeleton::getRootBones() const noexcept -> const Skeleton::BoneList& {
        if (mRootBones.empty())
        {
            deriveRootBone();
        }

        return mRootBones;
    }

    //---------------------------------------------------------------------
    void Skeleton::setAnimationState(const AnimationStateSet& animSet)
    {
        /* 
        Algorithm:
          1. Reset all bone positions
          2. Iterate per AnimationState, if enabled get Animation and call Animation::apply
        */

        // Reset bones
        reset();

        Real weightFactor = 1.0f;
        if (mBlendState == SkeletonAnimationBlendMode::AVERAGE)
        {
            // Derive total weights so we can rebalance if > 1.0f
            Real totalWeights = 0.0f;
            for (auto animState : animSet.getEnabledAnimationStates())
            {
                // Make sure we have an anim to match implementation
                const LinkedSkeletonAnimationSource* linked = nullptr;
                if (_getAnimationImpl(animState->getAnimationName(), &linked))
                {
                    totalWeights += animState->getWeight();
                }
            }

            // Allow < 1.0f, allows fade out of all anims if required 
            if (totalWeights > 1.0f)
            {
                weightFactor = 1.0f / totalWeights;
            }
        }

        // Per enabled animation state
        for (auto animState : animSet.getEnabledAnimationStates())
        {
            const LinkedSkeletonAnimationSource* linked = nullptr;
            Animation* anim = _getAnimationImpl(animState->getAnimationName(), &linked);
            // tolerate state entries for animations we're not aware of
            if (anim)
            {
              if(animState->hasBlendMask())
              {
                anim->apply(this, animState->getTimePosition(), animState->getWeight() * weightFactor,
                  animState->getBlendMask(), linked ? linked->scale : 1.0f);
              }
              else
              {
                anim->apply(this, animState->getTimePosition(), 
                  animState->getWeight() * weightFactor, linked ? linked->scale : 1.0f);
              }
            }
        }


    }
    //---------------------------------------------------------------------
    void Skeleton::setBindingPose()
    {
        // Update the derived transforms
        _updateTransforms();


        for (auto & i : mBoneList)
        {            
            i->setBindingPose();
        }
    }
    //---------------------------------------------------------------------
    void Skeleton::reset(bool resetManualBones)
    {
        for (auto & i : mBoneList)
        {
            if(!i->isManuallyControlled() || resetManualBones)
                i->reset();
        }
    }
    //---------------------------------------------------------------------
    auto Skeleton::createAnimation(std::string_view name, Real length) -> Animation*
    {
        // Check name not used
        if (mAnimationsList.find(name) != mAnimationsList.end())
        {
            OGRE_EXCEPT(
                ExceptionCodes::DUPLICATE_ITEM,
                ::std::format("An animation with the name {} already exists", name ),
                "Skeleton::createAnimation");
        }

        auto* ret = new Animation(name, length);
        ret->_notifyContainer(this);

        // Add to list
        mAnimationsList[std::string{ name }] = ret;

        return ret;

    }
    //---------------------------------------------------------------------
    auto Skeleton::getAnimation(std::string_view name, 
        const LinkedSkeletonAnimationSource** linker) const -> Animation*
    {
        Animation* ret = _getAnimationImpl(name, linker);
        if (!ret)
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("No animation entry found named {}", name), 
                "Skeleton::getAnimation");
        }

        return ret;
    }
    //---------------------------------------------------------------------
    auto Skeleton::getAnimation(std::string_view name) const -> Animation*
    {
        return getAnimation(name, nullptr);
    }
    //---------------------------------------------------------------------
    auto Skeleton::hasAnimation(std::string_view name) const -> bool
    {
        return _getAnimationImpl(name) != nullptr;
    }
    //---------------------------------------------------------------------
    auto Skeleton::_getAnimationImpl(std::string_view name, 
        const LinkedSkeletonAnimationSource** linker) const -> Animation*
    {
        Animation* ret = nullptr;
        auto i = mAnimationsList.find(name);

        if (i == mAnimationsList.end())
        {
            for (auto it = mLinkedSkeletonAnimSourceList.begin();
                it != mLinkedSkeletonAnimSourceList.end() && !ret; ++it)
            {
                if (it->pSkeleton)
                {
                    ret = it->pSkeleton->_getAnimationImpl(name);
                    if (ret && linker)
                    {
                        *linker = &(*it);
                    }

                }
            }

        }
        else
        {
            if (linker)
                *linker = nullptr;
            ret = i->second;
        }

        return ret;

    }
    //---------------------------------------------------------------------
    void Skeleton::removeAnimation(std::string_view name)
    {
        auto i = mAnimationsList.find(name);

        if (i == mAnimationsList.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("No animation entry found named {}", name), 
            "Skeleton::getAnimation");
        }

        delete i->second;

        mAnimationsList.erase(i);

    }
    //-----------------------------------------------------------------------
    void Skeleton::_initAnimationState(AnimationStateSet* animSet)
    {
        animSet->removeAllAnimationStates();
           
        for (auto const& [key, anim] : mAnimationsList)
        {
            // Create animation at time index 0, default params mean this has weight 1 and is disabled
            std::string_view animName = anim->getName();
            animSet->createAnimationState(animName, 0.0, anim->getLength());
        }

        // Also iterate over linked animation
        for (auto & li : mLinkedSkeletonAnimSourceList)
        {
            if (li.pSkeleton)
            {
                li.pSkeleton->_refreshAnimationState(animSet);
            }
        }

    }
    //-----------------------------------------------------------------------
    void Skeleton::_refreshAnimationState(AnimationStateSet* animSet)
    {
        // Merge in any new animations
        for (auto const& [key, anim] : mAnimationsList)
        {
            // Create animation at time index 0, default params mean this has weight 1 and is disabled
            std::string_view animName = anim->getName();
            if (!animSet->hasAnimationState(animName))
            {
                animSet->createAnimationState(animName, 0.0, anim->getLength());
            }
            else
            {
                // Update length incase changed
                AnimationState* animState = animSet->getAnimationState(animName);
                animState->setLength(anim->getLength());
                animState->setTimePosition(std::min(anim->getLength(), animState->getTimePosition()));
            }
        }
        // Also iterate over linked animation
        for (auto & li : mLinkedSkeletonAnimSourceList)
        {
            if (li.pSkeleton)
            {
                li.pSkeleton->_refreshAnimationState(animSet);
            }
        }
    }
    //-----------------------------------------------------------------------
    void Skeleton::_notifyManualBonesDirty()
    {
        mManualBonesDirty = true;
    }
    //-----------------------------------------------------------------------
    void Skeleton::_notifyManualBoneStateChange(Bone* bone)
    {
        if (bone->isManuallyControlled())
            mManualBones.insert(bone);
        else
            mManualBones.erase(bone);
    }
    //-----------------------------------------------------------------------
    auto Skeleton::getNumBones() const noexcept -> unsigned short
    {
        return (unsigned short)mBoneList.size();
    }
    //-----------------------------------------------------------------------
    void Skeleton::_getBoneMatrices(Affine3* pMatrices)
    {
        // Update derived transforms
        _updateTransforms();

        /*
            Calculating the bone matrices
            -----------------------------
            Now that we have the derived scaling factors, orientations & positions in the
            Bone nodes, we have to compute the Matrix4 to apply to the vertices of a mesh.
            Because any modification of a vertex has to be relative to the bone, we must
            first reverse transform by the Bone's original derived position/orientation/scale,
            then transform by the new derived position/orientation/scale.
            Also note we combine scale as equivalent axes, no shearing.
        */

        for (auto pBone : mBoneList)
        {
            pBone->_getOffsetTransform(*pMatrices);
            pMatrices++;
        }

    }
    //---------------------------------------------------------------------
    auto Skeleton::getNumAnimations() const noexcept -> unsigned short
    {
        return (unsigned short)mAnimationsList.size();
    }
    //---------------------------------------------------------------------
    auto Skeleton::getAnimation(unsigned short index) const -> Animation*
    {
        // If you hit this assert, then the index is out of bounds.
        assert( index < mAnimationsList.size() );

        auto i = mAnimationsList.begin();

        std::advance(i, index);

        return i->second;
    }
    //---------------------------------------------------------------------
    auto Skeleton::getBone(unsigned short handle) const -> Bone*
    {
        assert(handle < mBoneList.size() && "Index out of bounds");
        return mBoneList[handle];
    }
    //---------------------------------------------------------------------
    auto Skeleton::getBone(std::string_view name) const -> Bone*
    {
        auto i = mBoneListByName.find(name);

        if (i == mBoneListByName.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("Bone named '{}' not found.", name ), 
                "Skeleton::getBone");
        }

        return i->second;

    }
    //---------------------------------------------------------------------
    auto Skeleton::hasBone(std::string_view name) const -> bool
    {   
        return mBoneListByName.find(name) != mBoneListByName.end();
    }
    //---------------------------------------------------------------------
    void Skeleton::deriveRootBone() const
    {
        // Start at the first bone and work up
        OgreAssert(!mBoneList.empty(), "Cannot derive root bone as this skeleton has no bones");

        mRootBones.clear();

        for (auto currentBone : mBoneList)
        {
            if (currentBone->getParent() == nullptr)
            {
                // This is a root
                mRootBones.push_back(currentBone);
            }
        }
    }
    //---------------------------------------------------------------------
    void Skeleton::_dumpContents(std::string_view filename)
    {
        std::ofstream of;

        Quaternion q;
        Radian angle;
        Vector3 axis;
        of.open(filename.data());

        of << "-= Debug output of skeleton " << mName << " =-" << std::endl << std::endl;
        of << "== Bones ==" << std::endl;
        of << "Number of bones: " << (unsigned int)mBoneList.size() << std::endl;
        
        for (auto bone : mBoneList)
        {
            of << "-- Bone " << bone->getHandle() << " --" << std::endl;
            of << "Position: " << bone->getPosition();
            q = bone->getOrientation();
            of << "Rotation: " << q;
            q.ToAngleAxis(angle, axis);
            of << " = " << angle.valueRadians() << " radians around axis " << axis << std::endl << std::endl;
        }

        of << "== Animations ==" << std::endl;
        of << "Number of animations: " << (unsigned int)mAnimationsList.size() << std::endl;

        for (auto const& [first, anim] : mAnimationsList)
        {
            of << "-- Animation '" << anim->getName() << "' (length " << anim->getLength() << ") --" << std::endl;
            of << "Number of tracks: " << anim->getNumNodeTracks() << std::endl;

            for (unsigned short ti = 0; ti < anim->getNumNodeTracks(); ++ti)
            {
                NodeAnimationTrack* track = anim->getNodeTrack(ti);
                of << "  -- AnimationTrack " << ti << " --" << std::endl;
                of << "  Affects bone: " << static_cast<Bone*>(track->getAssociatedNode())->getHandle() << std::endl;
                of << "  Number of keyframes: " << track->getNumKeyFrames() << std::endl;

                for (unsigned short ki = 0; ki < track->getNumKeyFrames(); ++ki)
                {
                    TransformKeyFrame* key = track->getNodeKeyFrame(ki);
                    of << "    -- KeyFrame " << ki << " --" << std::endl;
                    of << "    Time index: " << key->getTime(); 
                    of << "    Translation: " << key->getTranslate() << std::endl;
                    q = key->getRotation();
                    of << "    Rotation: " << q;
                    q.ToAngleAxis(angle, axis);
                    of << " = " << angle.valueRadians() << " radians around axis " << axis << std::endl;
                }

            }



        }

    }
    //---------------------------------------------------------------------
    auto Skeleton::getBlendMode() const -> SkeletonAnimationBlendMode
    {
        return mBlendState;
    }
    //---------------------------------------------------------------------
    void Skeleton::setBlendMode(SkeletonAnimationBlendMode state) 
    {
        mBlendState = state;
    }

    //---------------------------------------------------------------------
    void Skeleton::_updateTransforms()
    {
        for (auto & mRootBone : mRootBones)
        {
            mRootBone->_update(true, false);
        }
        mManualBonesDirty = false;
    }
    //---------------------------------------------------------------------
    void Skeleton::optimiseAllAnimations(bool preservingIdentityNodeTracks)
    {
        if (!preservingIdentityNodeTracks)
        {
            Animation::TrackHandleList tracksToDestroy;

            // Assume all node tracks are identity
            ushort numBones = getNumBones();
            for (ushort h = 0; h < numBones; ++h)
            {
                tracksToDestroy.insert(h);
            }

            // Collect identity node tracks for all animations
            for (auto & ai : mAnimationsList)
            {
                ai.second->_collectIdentityNodeTracks(tracksToDestroy);
            }

            // Destroy identity node tracks
            for (auto & ai : mAnimationsList)
            {
                ai.second->_destroyNodeTracks(tracksToDestroy);
            }
        }

        for (auto & ai : mAnimationsList)
        {
            // Don't discard identity node tracks here
            ai.second->optimise(false);
        }
    }
    //---------------------------------------------------------------------
    void Skeleton::addLinkedSkeletonAnimationSource(std::string_view skelName, 
        Real scale)
    {
        // Check not already linked
        for (auto & i : mLinkedSkeletonAnimSourceList)
        {
            if (skelName == i.skeletonName)
                return; // don't bother
        }

        if (isPrepared() || isLoaded())
        {
            // Load immediately
            SkeletonPtr skelPtr = static_pointer_cast<Skeleton>(
                SkeletonManager::getSingleton().prepare(skelName, mGroup));
            mLinkedSkeletonAnimSourceList.push_back(
                LinkedSkeletonAnimationSource{skelName, scale, skelPtr});

        }
        else
        {
            // Load later
            mLinkedSkeletonAnimSourceList.push_back(
                LinkedSkeletonAnimationSource{skelName, scale});
        }

    }
    //---------------------------------------------------------------------
    void Skeleton::removeAllLinkedSkeletonAnimationSources()
    {
        mLinkedSkeletonAnimSourceList.clear();
    }

    //---------------------------------------------------------------------
    struct DeltaTransform
    {
        Vector3 translate;
        Quaternion rotate;
        Vector3 scale;
        bool isIdentity;
    };
    void Skeleton::_mergeSkeletonAnimations(const Skeleton* src,
        const BoneHandleMap& boneHandleMap,
        const StringVector& animations)
    {
        ushort handle;

        ushort numSrcBones = src->getNumBones();
        ushort numDstBones = this->getNumBones();

        OgreAssert(
            boneHandleMap.size() == numSrcBones,
            "Number of bones in the bone handle map must equal to number of bones in the source skeleton");

        bool existsMissingBone = false;

        // Check source skeleton structures compatible with ourself (that means
        // identically bones with identical handles, and with same hierarchy, but
        // not necessary to have same number of bones and bone names).
        for (handle = 0; handle < numSrcBones; ++handle)
        {
            const Bone* srcBone = src->getBone(handle);
            ushort dstHandle = boneHandleMap[handle];

            // Does it exists in target skeleton?
            if (dstHandle < numDstBones)
            {
                Bone* destBone = this->getBone(dstHandle);

                // Check both bones have identical parent, or both are root bone.
                const Bone* srcParent = static_cast<Bone*>(srcBone->getParent());
                Bone* destParent = static_cast<Bone*>(destBone->getParent());
                if ((srcParent || destParent) &&
                    (!srcParent || !destParent ||
                     boneHandleMap[srcParent->getHandle()] != destParent->getHandle()))
                {
                    OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        ::std::format(
                        "Source skeleton incompatible with this skeleton: "
                        "difference hierarchy between bone '{}"
                        "' and '{}'.", srcBone->getName(), destBone->getName()),
                        "Skeleton::_mergeSkeletonAnimations");
                }
            }
            else
            {
                existsMissingBone = true;
            }
        }

        // Clone bones if need
        if (existsMissingBone)
        {
            // Create missing bones
            for (handle = 0; handle < numSrcBones; ++handle)
            {
                const Bone* srcBone = src->getBone(handle);
                ushort dstHandle = boneHandleMap[handle];

                // The bone is missing in target skeleton?
                if (dstHandle >= numDstBones)
                {
                    Bone* dstBone = this->createBone(srcBone->getName(), dstHandle);
                    // Sets initial transform
                    dstBone->setPosition(srcBone->getInitialPosition());
                    dstBone->setOrientation(srcBone->getInitialOrientation());
                    dstBone->setScale(srcBone->getInitialScale());
                    dstBone->setInitialState();
                }
            }

            // Link new bones to parent
            for (handle = 0; handle < numSrcBones; ++handle)
            {
                const Bone* srcBone = src->getBone(handle);
                ushort dstHandle = boneHandleMap[handle];

                // Is new bone?
                if (dstHandle >= numDstBones)
                {
                    const Bone* srcParent = static_cast<Bone*>(srcBone->getParent());
                    if (srcParent)
                    {
                        Bone* destParent = this->getBone(boneHandleMap[srcParent->getHandle()]);
                        Bone* dstBone = this->getBone(dstHandle);
                        destParent->addChild(dstBone);
                    }
                }
            }

            // Derive root bones in case it was changed
            this->deriveRootBone();

            // Reset binding pose for new bones
            this->reset(true);
            this->setBindingPose();
        }

        //
        // We need to adapt animations from source to target skeleton, but since source
        // and target skeleton bones bind transform might difference, so we need to alter
        // keyframes in source to suit to target skeleton.
        //
        // For any given animation time, formula:
        //
        //      LocalTransform = BindTransform * KeyFrame;
        //      DerivedTransform = ParentDerivedTransform * LocalTransform
        //
        // And all derived transforms should be keep identically after adapt to
        // target skeleton, Then:
        //
        //      DestDerivedTransform == SrcDerivedTransform
        //      DestParentDerivedTransform == SrcParentDerivedTransform
        // ==>
        //      DestLocalTransform = SrcLocalTransform
        // ==>
        //      DestBindTransform * DestKeyFrame = SrcBindTransform * SrcKeyFrame
        // ==>
        //      DestKeyFrame = inverse(DestBindTransform) * SrcBindTransform * SrcKeyFrame
        //
        // We define (inverse(DestBindTransform) * SrcBindTransform) as 'delta-transform' here.
        //

        // Calculate delta-transforms for all source bones.
        std::vector<DeltaTransform> deltaTransforms(numSrcBones);
        for (handle = 0; handle < numSrcBones; ++handle)
        {
            const Bone* srcBone = src->getBone(handle);
            DeltaTransform& deltaTransform = deltaTransforms[handle];
            ushort dstHandle = boneHandleMap[handle];

            if (dstHandle < numDstBones)
            {
                // Common bone, calculate delta-transform

                Bone* dstBone = this->getBone(dstHandle);

                deltaTransform.translate = srcBone->getInitialPosition() - dstBone->getInitialPosition();
                deltaTransform.rotate = dstBone->getInitialOrientation().Inverse() * srcBone->getInitialOrientation();
                deltaTransform.scale = srcBone->getInitialScale() / dstBone->getInitialScale();

                // Check whether or not delta-transform is identity
                const Real tolerance = 1e-3f;
                Vector3 axis;
                Radian angle;
                deltaTransform.rotate.ToAngleAxis(angle, axis);
                deltaTransform.isIdentity =
                    deltaTransform.translate.positionEquals(Vector3::ZERO, tolerance) &&
                    deltaTransform.scale.positionEquals(Vector3::UNIT_SCALE, tolerance) &&
                    Math::RealEqual(angle.valueRadians(), 0.0f, tolerance);
            }
            else
            {
                // New bone, the delta-transform is identity

                deltaTransform.translate = Vector3::ZERO;
                deltaTransform.rotate = Quaternion::IDENTITY;
                deltaTransform.scale = Vector3::UNIT_SCALE;
                deltaTransform.isIdentity = true;
            }
        }

        // Now copy animations

        ushort numAnimations;
        if (animations.empty())
            numAnimations = src->getNumAnimations();
        else
            numAnimations = static_cast<ushort>(animations.size());
        for (ushort i = 0; i < numAnimations; ++i)
        {
            const Animation* srcAnimation;
            if (animations.empty())
            {
                // Get animation of source skeleton by the given index
                srcAnimation = src->getAnimation(i);
            }
            else
            {
                // Get animation of source skeleton by the given name
                const LinkedSkeletonAnimationSource* linker;
                srcAnimation = src->_getAnimationImpl(animations[i], &linker);
                if (!srcAnimation || linker)
                {
                    OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                        ::std::format("No animation entry found named {}", animations[i]),
                        "Skeleton::_mergeSkeletonAnimations");
                }
            }

            // Create target animation
            Animation* dstAnimation = this->createAnimation(srcAnimation->getName(), srcAnimation->getLength());

            // Copy interpolation modes
            dstAnimation->setInterpolationMode(srcAnimation->getInterpolationMode());
            dstAnimation->setRotationInterpolationMode(srcAnimation->getRotationInterpolationMode());

            // Copy track for each bone
            for (handle = 0; handle < numSrcBones; ++handle)
            {
                const DeltaTransform& deltaTransform = deltaTransforms[handle];
                ushort dstHandle = boneHandleMap[handle];

                if (srcAnimation->hasNodeTrack(handle))
                {
                    // Clone track from source animation

                    const NodeAnimationTrack* srcTrack = srcAnimation->getNodeTrack(handle);
                    NodeAnimationTrack* dstTrack = dstAnimation->createNodeTrack(dstHandle, this->getBone(dstHandle));
                    dstTrack->setUseShortestRotationPath(srcTrack->getUseShortestRotationPath());

                    ushort numKeyFrames = srcTrack->getNumKeyFrames();
                    for (ushort k = 0; k < numKeyFrames; ++k)
                    {
                        const TransformKeyFrame* srcKeyFrame = srcTrack->getNodeKeyFrame(k);
                        TransformKeyFrame* dstKeyFrame = dstTrack->createNodeKeyFrame(srcKeyFrame->getTime());

                        // Adjust keyframes to match target binding pose
                        if (deltaTransform.isIdentity)
                        {
                            dstKeyFrame->setTranslate(srcKeyFrame->getTranslate());
                            dstKeyFrame->setRotation(srcKeyFrame->getRotation());
                            dstKeyFrame->setScale(srcKeyFrame->getScale());
                        }
                        else
                        {
                            dstKeyFrame->setTranslate(deltaTransform.translate + srcKeyFrame->getTranslate());
                            dstKeyFrame->setRotation(deltaTransform.rotate * srcKeyFrame->getRotation());
                            dstKeyFrame->setScale(deltaTransform.scale * srcKeyFrame->getScale());
                        }
                    }
                }
                else if (!deltaTransform.isIdentity)
                {
                    // Create 'static' track for this bone

                    NodeAnimationTrack* dstTrack = dstAnimation->createNodeTrack(dstHandle, this->getBone(dstHandle));
                    TransformKeyFrame* dstKeyFrame;

                    dstKeyFrame = dstTrack->createNodeKeyFrame(0);
                    dstKeyFrame->setTranslate(deltaTransform.translate);
                    dstKeyFrame->setRotation(deltaTransform.rotate);
                    dstKeyFrame->setScale(deltaTransform.scale);

                    dstKeyFrame = dstTrack->createNodeKeyFrame(dstAnimation->getLength());
                    dstKeyFrame->setTranslate(deltaTransform.translate);
                    dstKeyFrame->setRotation(deltaTransform.rotate);
                    dstKeyFrame->setScale(deltaTransform.scale);
                }
            }
        }
    }
    //---------------------------------------------------------------------
    void Skeleton::_buildMapBoneByHandle(const Skeleton* src,
        BoneHandleMap& boneHandleMap) const
    {
        ushort numSrcBones = src->getNumBones();
        boneHandleMap.resize(numSrcBones);

        for (ushort handle = 0; handle < numSrcBones; ++handle)
        {
            boneHandleMap[handle] = handle;
        }
    }
    //---------------------------------------------------------------------
    void Skeleton::_buildMapBoneByName(const Skeleton* src,
        BoneHandleMap& boneHandleMap) const
    {
        ushort numSrcBones = src->getNumBones();
        boneHandleMap.resize(numSrcBones);

        ushort newBoneHandle = this->getNumBones();
        for (ushort handle = 0; handle < numSrcBones; ++handle)
        {
            const Bone* srcBone = src->getBone(handle);
            auto i = this->mBoneListByName.find(srcBone->getName());
            if (i == mBoneListByName.end())
                boneHandleMap[handle] = newBoneHandle++;
            else
                boneHandleMap[handle] = i->second->getHandle();
        }
    }

    auto Skeleton::calculateSize() const -> size_t
    {
        size_t memSize = sizeof(*this);
        memSize += mBoneList.size() * sizeof(Bone);
        memSize += mRootBones.size() * sizeof(Bone);
        memSize += mBoneListByName.size() * (sizeof(String) + sizeof(Bone*));
        memSize += mAnimationsList.size() * (sizeof(String) + sizeof(Animation*));
        memSize += mManualBones.size() * sizeof(Bone*);
        memSize += mLinkedSkeletonAnimSourceList.size() * sizeof(LinkedSkeletonAnimationSource);

        return memSize;
    }
}
