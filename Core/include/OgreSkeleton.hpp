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

#include <cstddef>

export module Ogre.Core:Skeleton;

export import :Animation;
export import :IteratorWrapper;
export import :Platform;
export import :Prerequisites;
export import :Resource;
export import :SharedPtr;
export import :StringVector;

export import <algorithm>;
export import <map>;
export import <set>;
export import <string>;
export import <string_view>;
export import <utility>;
export import <vector>;

export
namespace Ogre {
struct Affine3;
class AnimationStateSet;
class Bone;
class ResourceManager;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Animation
    *  @{
    */

    /**  */
    enum class SkeletonAnimationBlendMode : uint8 {
        /// Animations are applied by calculating a weighted average of all animations
        AVERAGE = 0,
        /// Animations are applied by calculating a weighted cumulative total
        CUMULATIVE = 1
    };

auto constexpr inline OGRE_MAX_NUM_BONES = 256;

    
    struct LinkedSkeletonAnimationSource;

    /** A collection of Bone objects used to animate a skinned mesh.
    @remarks
        Skeletal animation works by having a collection of 'bones' which are 
        actually just joints with a position and orientation, arranged in a tree structure.
        For example, the wrist joint is a child of the elbow joint, which in turn is a
        child of the shoulder joint. Rotating the shoulder automatically moves the elbow
        and wrist as well due to this hierarchy.
    @par
        So how does this animate a mesh? Well every vertex in a mesh is assigned to one or more
        bones which affects it's position when the bone is moved. If a vertex is assigned to 
        more than one bone, then weights must be assigned to determine how much each bone affects
        the vertex (actually a weight of 1.0 is used for single bone assignments). 
        Weighted vertex assignments are especially useful around the joints themselves
        to avoid 'pinching' of the mesh in this region. 
    @par
        Therefore by moving the skeleton using preset animations, we can animate the mesh. The
        advantage of using skeletal animation is that you store less animation data, especially
        as vertex counts increase. In addition, you are able to blend multiple animations together
        (e.g. walking and looking around, running and shooting) and provide smooth transitions
        between animations without incurring as much of an overhead as would be involved if you
        did this on the core vertex data.
    @par
        Skeleton definitions are loaded from datafiles, namely the .skeleton file format. They
        are loaded on demand, especially when referenced by a Mesh.
    */
    class Skeleton : public Resource, public AnimationContainer
    {
        friend class SkeletonInstance;
    private:
        /// Internal constructor for use by SkeletonInstance only
        Skeleton();

    public:
        /** Constructor, don't call directly, use SkeletonManager.
        @remarks
            On creation, a Skeleton has a no bones, you should create them and link
            them together appropriately. 
        */
        Skeleton(ResourceManager* creator, std::string_view name, ResourceHandle handle,
            std::string_view group, bool isManual = false, ManualResourceLoader* loader = nullptr);
        ~Skeleton() override;


        /** Creates a brand new Bone owned by this Skeleton. 
        @remarks
            This method creates an unattached new Bone for this skeleton.
            Unless this is to be a root bone (there may be more than one of 
            these), you must attach it to another Bone in the skeleton using addChild for it to be any use.
            For this reason you will likely be better off creating child bones using the
            Bone::createChild method instead, once you have created the root bone. 
        @par
            Note that this method automatically generates a handle for the bone, which you
            can retrieve using Bone::getHandle. If you wish the new Bone to have a specific
            handle, use the alternate form of this method which takes a handle as a parameter,
            although you should note the restrictions.
        */
        virtual auto createBone() -> Bone*;

        /** Creates a brand new Bone owned by this Skeleton. 
        @remarks
            This method creates an unattached new Bone for this skeleton and assigns it a 
            specific handle. Unless this is to be a root bone (there may be more than one of 
            these), you must attach it to another Bone in the skeleton using addChild for it to be any use. 
            For this reason you will likely be better off creating child bones using the
            Bone::createChild method instead, once you have created a root bone. 
        @param handle The handle to give to this new bone - must be unique within this skeleton. 
            You should also ensure that all bone handles are eventually contiguous (this is to simplify
            their compilation into an indexed array of transformation matrices). For this reason
            it is advised that you use the simpler createBone method which automatically assigns a
            sequential handle starting from 0.
        */
        virtual auto createBone(unsigned short handle) -> Bone*;

        /** Creates a brand new Bone owned by this Skeleton. 
        @remarks
            This method creates an unattached new Bone for this skeleton and assigns it a 
            specific name.Unless this is to be a root bone (there may be more than one of 
            these), you must attach it to another Bone in the skeleton using addChild for it to be any use.
            For this reason you will likely be better off creating child bones using the
            Bone::createChild method instead, once you have created the root bone. 
        @param name The name to give to this new bone - must be unique within this skeleton. 
            Note that the way OGRE looks up bones is via a numeric handle, so if you name a
            Bone this way it will be given an automatic sequential handle. The name is just
            for your convenience, although it is recommended that you only use the handle to 
            retrieve the bone in performance-critical code.
        */
        virtual auto createBone(std::string_view name) -> Bone*;

        /** Creates a brand new Bone owned by this Skeleton. 
        @remarks
            This method creates an unattached new Bone for this skeleton and assigns it a 
            specific name and handle. Unless this is to be a root bone (there may be more than one of 
            these), you must attach it to another Bone in the skeleton using addChild for it to be any use.
            For this reason you will likely be better off creating child bones using the
            Bone::createChild method instead, once you have created the root bone. 
        @param name The name to give to this new bone - must be unique within this skeleton. 
        @param handle The handle to give to this new bone - must be unique within this skeleton. 
        */
        virtual auto createBone(std::string_view name, unsigned short handle) -> Bone*;

        /** Returns the number of bones in this skeleton. */
        virtual auto getNumBones() const noexcept -> unsigned short;

        using BoneList = std::vector<Bone *>;
        using BoneIterator = VectorIterator<BoneList>;

        /** Get the root bones in the skeleton, ie those with no parents

            The system derives the root bone the first time you ask for it. The root bone is the
            only bone in the skeleton which has no parent. The system locates it by taking the
            first bone in the list and going up the bone tree until there are no more parents,
            and saves this top bone as the root. If you are building the skeleton manually using
            createBone then you must ensure there is only one bone which is not a child of
            another bone, otherwise your skeleton will not work properly. If you use createBone
            only once, and then use Bone::createChild from then on, then inherently the first
            bone you create will by default be the root.
        */
        auto getRootBones() const noexcept -> const BoneList&;

        /// Get all the bones in the skeleton
        auto getBones() const noexcept -> const BoneList& {
            return mBoneList;
        }

        /** Gets a bone by it's handle. */
        virtual auto getBone(unsigned short handle) const -> Bone*;

        /** Gets a bone by it's name. */
        virtual auto getBone(std::string_view name) const -> Bone*;

        /** Returns whether this skeleton contains the named bone. */
        virtual auto hasBone(std::string_view name) const -> bool;

        /** Sets the current position / orientation to be the 'binding pose' i.e. the layout in which 
            bones were originally bound to a mesh.
        */
        virtual void setBindingPose();

        /** Resets the position and orientation of all bones in this skeleton to their original binding position.
        @remarks
            A skeleton is bound to a mesh in a binding pose. Bone positions are then modified from this
            position during animation. This method returns all the bones to their original position and
            orientation.
        @param resetManualBones If set to true, causes the state of manual bones to be reset
            too, which is normally not done to allow the manual state to persist even 
            when keyframe animation is applied.
        */
        virtual void reset(bool resetManualBones = false);

        /** Creates a new Animation object for animating this skeleton. 
        @param name The name of this animation
        @param length The length of the animation in seconds
        */
        auto createAnimation(std::string_view name, Real length) -> Animation* override;

        /** Returns the named Animation object. 
        @remarks
            Will pick up animations in linked skeletons 
            (@see addLinkedSkeletonAnimationSource). 
        @param name The name of the animation
        @param linker Optional pointer to a pointer to the linked skeleton animation
            where this is coming from.
        */
        virtual auto getAnimation(std::string_view name, 
            const LinkedSkeletonAnimationSource** linker) const -> Animation*;

        /** Returns the named Animation object.
         @remarks
             Will pick up animations in linked skeletons 
             (@see addLinkedSkeletonAnimationSource). 
         @param name The name of the animation
         */
        auto getAnimation(std::string_view name) const -> Animation* override;

        /// Internal accessor for animations (returns null if animation does not exist)
        virtual auto _getAnimationImpl(std::string_view name, 
            const LinkedSkeletonAnimationSource** linker = nullptr) const -> Animation*;


        /** Returns whether this skeleton contains the named animation. */
        auto hasAnimation(std::string_view name) const -> bool override;

        /** Removes an Animation from this skeleton. */
        void removeAnimation(std::string_view name) override;

        /** Changes the state of the skeleton to reflect the application of the passed in collection of animations.
        @remarks
            Animating a skeleton involves both interpolating between keyframes of a specific animation,
            and blending between the animations themselves. Calling this method sets the state of
            the skeleton so that it reflects the combination of all the passed in animations, at the
            time index specified for each, using the weights specified. Note that the weights between 
            animations do not have to sum to 1.0, because some animations may affect only subsets
            of the skeleton. If the weights exceed 1.0 for the same area of the skeleton, the 
            movement will just be exaggerated.
        */
        virtual void setAnimationState(const AnimationStateSet& animSet);


        /** Initialise an animation set suitable for use with this skeleton. 
        @remarks
            Only recommended for use inside the engine, not by applications.
        */
        virtual void _initAnimationState(AnimationStateSet* animSet);

        /** Refresh an animation set suitable for use with this skeleton. 
        @remarks
            Only recommended for use inside the engine, not by applications.
        */
        virtual void _refreshAnimationState(AnimationStateSet* animSet);

        /** Populates the passed in array with the bone matrices based on the current position.
        @remarks
            Internal use only. The array pointed to by the passed in pointer must
            be at least as large as the number of bones.
            Assumes animation has already been updated.
        */
        virtual void _getBoneMatrices(Affine3* pMatrices);

        /** Gets the number of animations on this skeleton. */
        auto getNumAnimations() const noexcept -> unsigned short override;

        /** Gets a single animation by index. 
        @remarks
            Will NOT pick up animations in linked skeletons 
            (@see addLinkedSkeletonAnimationSource).
        */
        auto getAnimation(unsigned short index) const -> Animation* override;


        /** Gets the animation blending mode which this skeleton will use. */
        virtual auto getBlendMode() const -> SkeletonAnimationBlendMode;
        /** Sets the animation blending mode this skeleton will use. */
        virtual void setBlendMode(SkeletonAnimationBlendMode state);

        /// Updates all the derived transforms in the skeleton
        virtual void _updateTransforms();

        /** Optimise all of this skeleton's animations.
        @see Animation::optimise
        @param
            preservingIdentityNodeTracks If true, don't destroy identity node tracks.
        */
        virtual void optimiseAllAnimations(bool preservingIdentityNodeTracks = false);

        /** Allows you to use the animations from another Skeleton object to animate
            this skeleton.
        @remarks
            If you have skeletons of identical structure (that means identically
            named bones with identical handles, and with the same hierarchy), but
            slightly different proportions or binding poses, you can re-use animations
            from one in the other. Because animations are actually stored as
            changes to bones from their bind positions, it's possible to use the
            same animation data for different skeletons, provided the skeletal
            structure matches and the 'deltas' stored in the keyframes apply
            equally well to the other skeletons bind position (so they must be
            roughly similar, but don't have to be identical). You can use the 
            'scale' option to adjust the translation and scale keyframes where
            there are large differences in size between the skeletons.
        @note
            This method takes a skeleton name, rather than a more specific 
            animation name, for two reasons; firstly it allows some validation 
            of compatibility of skeletal structure, and secondly skeletons are
            the unit of loading. Linking a skeleton to another in this way means
            that the linkee will be prevented from being destroyed until the 
            linker is destroyed.

            You cannot set up cyclic relationships, e.g. SkeletonA uses SkeletonB's
            animations, and SkeletonB uses SkeletonA's animations. This is because
            it would set up a circular dependency which would prevent proper 
            unloading - make one of the skeletons the 'master' in this case.
        @param skelName Name of the skeleton to link animations from. This 
            skeleton will be loaded immediately if this skeleton is already 
            loaded, otherwise it will be loaded when this skeleton is.
        @param scale A scale factor to apply to translation and scaling elements
            of the keyframes in the other skeleton when applying the animations
            to this one. Compensates for skeleton size differences.
        */
        virtual void addLinkedSkeletonAnimationSource(std::string_view skelName, 
            Real scale = 1.0f);
        /// Remove all links to other skeletons for the purposes of sharing animation
        virtual void removeAllLinkedSkeletonAnimationSources();
        
        using LinkedSkeletonAnimSourceList = std::vector<LinkedSkeletonAnimationSource>;
        using LinkedSkeletonAnimSourceIterator = ConstVectorIterator<LinkedSkeletonAnimSourceList>;
        /// Get the linked skeletons used as animation sources
        virtual auto getLinkedSkeletonAnimationSources() const noexcept -> const LinkedSkeletonAnimSourceList&
        {
            return mLinkedSkeletonAnimSourceList;
        }

        /// Internal method for marking the manual bones as dirty
        virtual void _notifyManualBonesDirty();
        /// Internal method for notifying that a bone is manual
        virtual void _notifyManualBoneStateChange(Bone* bone);

        /// Have manual bones been modified since the skeleton was last updated?
        virtual auto getManualBonesDirty() const noexcept -> bool { return mManualBonesDirty; }
        /// Are there any manually controlled bones?
        virtual auto hasManualBones() const -> bool { return !mManualBones.empty(); }

        /// Map to translate bone handle from one skeleton to another skeleton.
        using BoneHandleMap = std::vector<ushort>;

        /** Merge animations from another Skeleton object into this skeleton.
        @remarks
            This function allow merge two structures compatible skeletons. The
            'compatible' here means identically bones will have same hierarchy,
            but skeletons are not necessary to have same number of bones (if
            number bones of source skeleton's more than this skeleton, they will
            copied as is, except that duplicate names are disallowed; and in the
            case of bones missing in source skeleton, nothing happen for those
            bones).
        @par
            There are also unnecessary to have same binding poses, this function
            will adjust keyframes of the source skeleton to match this skeleton
            automatically.
        @par
            It's useful for exporting skeleton animations separately. i.e. export
            mesh and 'master' skeleton at the same time, and then other animations
            will export separately (even if used completely difference binding
            pose), finally, merge separately exported animations into 'master'
            skeleton.
        @param
            source Pointer to source skeleton. It'll keep unmodified.
        @param
            boneHandleMap A map to translate identically bone's handle from source
            skeleton to this skeleton. If mapped bone handle doesn't exists in this
            skeleton, it'll created. You can populate bone handle map manually, or
            use predefined functions build bone handle map for you. (@see 
            _buildMapBoneByHandle, _buildMapBoneByName)
        @param
            animations A list name of animations to merge, if empty, all animations
            of source skeleton are used to merge. Note that the animation names
            must not presented in this skeleton, and will NOT pick up animations
            in linked skeletons (@see addLinkedSkeletonAnimationSource).
        */
        virtual void _mergeSkeletonAnimations(const Skeleton* source,
            const BoneHandleMap& boneHandleMap,
            const StringVector& animations = StringVector());

        /** Build the bone handle map to use with Skeleton::_mergeSkeletonAnimations.
        @remarks
            Identically bones are determine by handle.
        */
        virtual void _buildMapBoneByHandle(const Skeleton* source,
            BoneHandleMap& boneHandleMap) const;

        /** Build the bone handle map to use with Skeleton::_mergeSkeletonAnimations.
        @remarks
            Identically bones are determine by name.
        */
        virtual void _buildMapBoneByName(const Skeleton* source,
            BoneHandleMap& boneHandleMap) const;

    protected:
        /// Storage of animations, lookup by name
        using AnimationList = std::map<std::string, Animation*, std::less<>>;
        AnimationList mAnimationsList;
    private:
        /// Lookup by bone name
        using BoneListByName = std::map<std::string_view, Bone *>;
        BoneListByName mBoneListByName;

        /// Pointer to root bones (can now have multiple roots)
        mutable BoneList mRootBones;
        using BoneSet = std::set<Bone *>;
        /// Manual bones
        BoneSet mManualBones;

        /// List of references to other skeletons to use animations from 
        mutable LinkedSkeletonAnimSourceList mLinkedSkeletonAnimSourceList;

        /// Bone automatic handles
        unsigned short mNextAutoHandle;
        SkeletonAnimationBlendMode mBlendState;
        /// Manual bones dirty?
        bool mManualBonesDirty;
        /// Storage of bones, indexed by bone handle
        BoneList mBoneList;

        /** Internal method which parses the bones to derive the root bone. 
        @remarks
            Must be const because called in getRootBone but mRootBone is mutable
            since lazy-updated.
        */
        void deriveRootBone() const;

        /// Debugging method
        void _dumpContents(std::string_view filename);

        void loadImpl() override {}
        void unloadImpl() override { unprepareImpl(); }

        void prepareImpl() override;
        void unprepareImpl() override;
        /// @copydoc Resource::calculateSize
        auto calculateSize() const -> size_t override;

    };

    /// Link to another skeleton to share animations
    struct LinkedSkeletonAnimationSource
    {
        std::string_view skeletonName;
        Real scale;
        SkeletonPtr pSkeleton;
    };
    /** @} */
    /** @} */

}
