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
#ifndef OGRE_CORE_INSTANCEDENTITY_H
#define OGRE_CORE_INSTANCEDENTITY_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

#include "OgreMatrix4.hpp"
#include "OgreMovableObject.hpp"
#include "OgreNode.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreQuaternion.hpp"
#include "OgreRenderable.hpp"
#include "OgreVector.hpp"

namespace Ogre
{
    class NameGenerator;
class AnimationState;
class AnimationStateSet;
class AxisAlignedBox;
class Camera;
class InstanceBatch;
class RenderQueue;
class SkeletonInstance;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */

    /** @see InstanceBatch to understand how instancing works.

        Instanced entities work in a very similar way as how an Entity works, as this interface
        tries to mimic it as most as possible to make the transition between Entity and InstancedEntity
        as straightforward and simple as possible.
        There are a couple inherent limitations, for example setRenderQueueGroup only works on
        the InstanceBatch level, not the individual instance. This limits Z sorting for alpha blending
        quite significantly
        An InstancedEntity won't appear in scene until a SceneNode is attached to them. Once the
        InstancedEntity is no longer needed, call InstanceBatch::removeInstancedEntity to put them
        back into a queue so the batch can return it back again when a new instance is requested.
        @par
        Internally, instanced entities that are not visible (i.e. by setting setVisible(false) or
        when they don't have a SceneNode attached to them) a Zero world matrix is sent to the vertex shader
        which in turn causes a zero area triangle.
        This obviously means no Vertex shader benefit, but saves a bit of pixel shader processing power.
        Also this means this object won't be considered when sizing the InstanceBatch's bounding box.
        @par
        Each instance has an instance ID unique within the batch, which matches the ID sent to the vertex
        shader by some techniques (like InstanceBatchShader).
        @par
        Differences between Entity and InstancedEntity:
        * Setting RenderQueueGroup and other Renderable stuff works at InstanceBatch level, not
        InstancedEntity. This is obviously a limitation from instancing in general, not this particular
        implementation

        @remarks
            Design discussion webpage
        @author
            Matias N. Goldberg ("dark_sylinc")
        @version
            1.0
     */
    class InstancedEntity : public MovableObject
    {
        friend class InstanceBatch;
        friend class InstanceBatchShader;
        friend class InstanceBatchHW;
        friend class InstanceBatchHW_VTF;
        friend class BaseInstanceBatchVTF;
    private:
        typedef TransformBase<3, float>        Matrix3x4f;
        uint16 mInstanceId; //Note it may change after defragmenting!
        bool mInUse;
        InstanceBatch *mBatchOwner;

        AnimationStateSet *mAnimationState;
        SkeletonInstance *mSkeletonInstance;
        Affine3 *mBoneMatrices;  //Local space
        Affine3 *mBoneWorldMatrices; //World space
        unsigned long mFrameAnimationLastUpdated;

        InstancedEntity* mSharedTransformEntity;    //When not null, another InstancedEntity controls the skeleton
                                                
        /** Used in conjunction with bone matrix lookup. Tells the number of the transform for
            as arranged in the vertex texture */
        uint16 mTransformLookupNumber;

        /// Stores the master when we're the slave, store our slaves when we're the master
        typedef std::vector<InstancedEntity*> InstancedEntityVec;
        InstancedEntityVec mSharingPartners;

        //////////////////////////////////////////////////////////////////////////
        // Parameters used for local transformation offset information
        // The 
        //////////////////////////////////////////////////////////////////////////

        /// Object position
        Vector3 mPosition;
        Vector3 mDerivedLocalPosition;
        /// Object orientation
        Quaternion mOrientation;
        /// Object scale
        Vector3 mScale;
        /// The maximum absolute scale for all dimension
        Real mMaxScaleLocal;
        /// Full world transform
        Affine3 mFullLocalTransform;
        /// Tells if mFullTransform needs an updated
        bool mNeedTransformUpdate;
        /// Tells if the animation world transform needs an update
        bool mNeedAnimTransformUpdate;
        /// Tells whether to use the local transform parameters
        bool mUseLocalTransform;


        /// Returns number of matrices written to transform, assumes transform has enough space
        auto getTransforms( Matrix4 *xform ) const -> size_t;
        /// Returns number of 32-bit values written
        auto getTransforms3x4( Matrix3x4f *xform ) const -> size_t;

        /// Returns true if this InstancedObject is visible to the current camera
        auto findVisible( Camera *camera ) const -> bool;

        /// Creates/destroys our own skeleton, also tells slaves to unlink if we're destroying
        void createSkeletonInstance();
        void destroySkeletonInstance();

        /// When this entity is a slave, stopSharingTransform delegates to this function.
        /// nofityMaster = false is used to prevent iterator invalidation in specific cases.
        void stopSharingTransformAsSlave( bool notifyMaster );

        /// Just unlinks, and tells our master we're no longer sharing
        void unlinkTransform( bool notifyMaster=true );

        /// Called when a slave has unlinked from us
        void notifyUnlink( const InstancedEntity *slave );

        /// Mark the transformation matrixes as dirty
        inline void markTransformDirty();

        /// Incremented count for next name extension
        static NameGenerator msNameGenerator;

    public:
        InstancedEntity( InstanceBatch *batchOwner, uint32 instanceID, InstancedEntity* sharedTransformEntity = nullptr);
        ~InstancedEntity() override;

        /** Shares the entire transformation with another InstancedEntity. This is useful when a mesh
            has more than one submeshes, therefore creating multiple InstanceManagers (one for each
            submesh). With this function, sharing makes the skeleton to be shared (less memory) and
            updated once (performance optimization).
            Note that one InstancedEntity (i.e. submesh 0) must be chosen as "master" which will share
            with the other instanced entities (i.e. submeshes 1-N) which are called "slaves"
            @par
            Requirements to share trasnformations:
                * Both InstancedEntities must have use the same skeleton
                * An InstancedEntity can't be both "master" and "slave" at the same time
            @remarks
            Sharing does nothing if the original mesh doesn't have a skeleton
            When an InstancedEntity is removed (@see InstanceBatch::removeInstancedEntity), it stops
            sharing the transform. If the instanced entity was the master one, all it's slaves stop
            sharing and start having their own transform too.
            @param slave The InstancedEntity that should share with us and become our slave
            @return true if successfully shared (may fail if they aren't skeletally animated)
        */
        auto shareTransformWith( InstancedEntity *slave ) -> bool;

        /** @see shareTransformWith
            Stops sharing the transform if this is a slave, and notifies the master we're no longer
            a slave.
            If this is a master, tells all it's slave to stop sharing
            @remarks
            This function is automatically called in InstanceBatch::removeInstancedEntity
        */
        void stopSharingTransform();

        auto _getOwner() const -> InstanceBatch* { return mBatchOwner; }

        auto getMovableType() const -> const String& override;

        auto getBoundingBox() const -> const AxisAlignedBox& override;
        auto getBoundingRadius() const -> Real override;

        /** This is used by our batch owner to get the closest entity's depth, returns infinity
            when not attached to a scene node */
        auto getSquaredViewDepth( const Camera* cam ) const -> Real;

        /// Overridden so we can tell the InstanceBatch it needs to update it's bounds
        void _notifyMoved() override;
        void _notifyAttached( Node* parent, bool isTagPoint = false ) override;

        /// Do nothing, InstanceBatch takes care of this.
        void _updateRenderQueue( RenderQueue* queue ) override   {}
        void visitRenderables( Renderable::Visitor* visitor, bool debugRenderables = false ) override {}

        /** @see Entity::hasSkeleton */
        auto hasSkeleton() const -> bool { return mSkeletonInstance != nullptr; }
        /** @see Entity::getSkeleton */
        auto getSkeleton() const -> SkeletonInstance* { return mSkeletonInstance; }

        /** @see Entity::getAnimationState */
        auto getAnimationState(const String& name) const -> AnimationState*;
        /** @see Entity::getAllAnimationStates */
        auto getAllAnimationStates() const -> AnimationStateSet*;

        /** Called by InstanceBatch in <i>his</i> _updateRenderQueue to tell us we need
            to calculate our bone matrices.
            @remarks Assumes it has a skeleton (mSkeletonInstance != 0)
            @return true if something was actually updated
        */
        virtual auto _updateAnimation() -> bool;

        /** Sets the transformation look up number */
        void setTransformLookupNumber(uint16 num) { mTransformLookupNumber = num;}

        /** Retrieve the position */
        auto getPosition() const -> const Vector3& { return mPosition; }
        /** Set the position or the offset from the parent node if a parent node exists */ 
        void setPosition(const Vector3& position, bool doUpdate = true);

        /** Retrieve the orientation */
        auto getOrientation() const -> const Quaternion& { return mOrientation; }
        /** Set the orientation or the offset from the parent node if a parent node exists */
        void setOrientation(const Quaternion& orientation, bool doUpdate = true);

        /** Retrieve the local scale */ 
        auto getScale() const -> const Vector3& { return mScale; }
        /** Set the  scale or the offset from the parent node if a parent node exists  */ 
        void setScale(const Vector3& scale, bool doUpdate = true);

        /** Returns the maximum derived scale coefficient among the xyz values */
        auto getMaxScaleCoef() const -> Real;

        /** Update the world transform and derived values */
        void updateTransforms();

        /** Tells if the entity is in use. */
        auto isInUse() const -> bool { return mInUse; }
        /** Sets whether the entity is in use. */
        void setInUse(bool used);

        /** Returns the world transform of the instanced entity including local transform */
        auto _getParentNodeFullTransform() const -> const Affine3& override {
            assert((!mNeedTransformUpdate || !mUseLocalTransform) && "Transform data should be updated at this point");
            return mUseLocalTransform ? mFullLocalTransform :
                mParentNode ? mParentNode->_getFullTransform() : Affine3::IDENTITY;
        }

        /** Returns the derived position of the instanced entity including local transform */
        auto _getDerivedPosition() const -> const Vector3& {
            assert((!mNeedTransformUpdate || !mUseLocalTransform) && "Transform data should be updated at this point");
            return mUseLocalTransform ? mDerivedLocalPosition :
                mParentNode ? mParentNode->_getDerivedPosition() : Vector3::ZERO;
        }

        /** @copydoc MovableObject::isInScene */
        auto isInScene() const -> bool override
        {
            //We assume that the instanced entity is in the scene if it is in use
            //It is in the scene whether it has a parent node or not
            return mInUse;
        }

        /** Sets the custom parameter for this instance @see InstanceManager::setNumCustomParams
            Because not all techniques support custom params, and some users may not need it while
            using millions of InstancedEntities, the params have been detached from InstancedEntity
            and stored in it's InstanceBatch instead, to reduce memory overhead.
        @remarks
            If this function is never called, all instances default to Vector4::ZERO. Watch out!
            If you destroy an instanced entity and then create it again (remember! Instanced entities
            are pre-allocated) it's custom param will contain the old value when it was destroyed.
        @param idx of the param. In the range [0; InstanceManager::getNumCustomParams())
        @param newParam New parameter
        */
        void setCustomParam( unsigned char idx, const Vector4 &newParam );
        auto getCustomParam( unsigned char idx ) -> const Vector4&;
    };
}

#endif
