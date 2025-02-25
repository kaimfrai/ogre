/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

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
#include <cstring>

module Ogre.Core;

import :AlignedAllocator;
import :Animation;
import :AnimationState;
import :AnimationTrack;
import :Bone;
import :Camera;
import :EdgeListBuilder;
import :Entity;
import :Exception;
import :GpuProgram;
import :HardwareBuffer;
import :HardwareIndexBuffer;
import :HardwareVertexBuffer;
import :Light;
import :LodListener;
import :LodStrategy;
import :Log;
import :LogManager;
import :Material;
import :MaterialManager;
import :Math;
import :Matrix3;
import :Mesh;
import :MeshManager;
import :Node;
import :OptimisedUtil;
import :Pass;
import :RenderOperation;
import :RenderQueue;
import :Root;
import :SceneManager;
import :SceneNode;
import :Skeleton;
import :SkeletonInstance;
import :StringConverter;
import :SubEntity;
import :SubMesh;
import :TagPoint;
import :Technique;
import :VertexIndexData;

import <algorithm>;
import <limits>;
import <list>;
import <map>;
import <span>;
import <string>;

namespace Ogre {
    //-----------------------------------------------------------------------
    Entity::Entity ()
        : mAnimationState(nullptr),
          mTempSkelAnimInfo(),
          mTempVertexAnimInfo(),
          mVertexAnimationAppliedThisFrame(false),
          mPreparedForShadowVolumes(false),
          mDisplaySkeleton(false),
          mCurrentHWAnimationState(false),
          mSkipAnimStateUpdates(false),
          mAlwaysUpdateMainSkeleton(false),
          mUpdateBoundingBoxFromSkeleton(false),
          mVertexProgramInUse(false),
          mInitialised(false),
          mHardwarePoseCount(0),
          mNumBoneMatrices(0),
          mBoneWorldMatrices(nullptr),
          mBoneMatrices(nullptr),
          mFrameAnimationLastUpdated(std::numeric_limits<unsigned long>::max()),
          mFrameBonesLastUpdated(nullptr),
          mSharedSkeletonEntities(nullptr),
        mSoftwareAnimationRequests(0),
        mSoftwareAnimationNormalsRequests(0),
        mMeshLodIndex(0),
        mMeshLodFactorTransformed(1.0f),
        mMinMeshLodIndex(99),
        mMaxMeshLodIndex(0),        // Backwards, remember low value = high detail
        mMaterialLodFactor(1.0f),
        mMaterialLodFactorTransformed(1.0f),
        mMinMaterialLodIndex(99),
        mMaxMaterialLodIndex(0),        // Backwards, remember low value = high detail
        mSkeletonInstance(nullptr),
        mLastParentXform(Affine3::ZERO),
        mMeshStateCount(0),
        mFullBoundingBox()
    {
    }
    //-----------------------------------------------------------------------
    Entity::Entity( std::string_view name, const MeshPtr& mesh) : Entity()
    {
        mName = name;
        mMesh = mesh;
        _initialise();
    }
    //-----------------------------------------------------------------------
    void Entity::loadingComplete(Resource* res)
    {
        if (res == mMesh.get())
        {
            // mesh loading has finished, we can construct ourselves now
            _initialise();
        }
    }
    //-----------------------------------------------------------------------
    void Entity::_initialise(bool forceReinitialise)
    {
        if (forceReinitialise)
            _deinitialise();

        if (mInitialised)
            return;

        if (mMesh->isBackgroundLoaded() && !mMesh->isLoaded())
        {
            // register for a callback when mesh is finished loading
            // do this before asking for load to happen to avoid race
            mMesh->addListener(this);
        }
        
        // On-demand load
        mMesh->load();
        // If loading failed, or deferred loading isn't done yet, defer
        // Will get a callback in the case of deferred loading
        // Skeletons are cascade-loaded so no issues there
        if (!mMesh->isLoaded())
            return;

        // Is mesh skeletally animated?
        if (mMesh->hasSkeleton() && mMesh->getSkeleton())
        {
            mSkeletonInstance = new SkeletonInstance(mMesh->getSkeleton());
            mSkeletonInstance->load();
            // if mUpdateBoundingBoxFromSkeleton was turned on before the mesh was loaded, and mesh hasn't computed the boneBoundingRadius yet,
            if ( mUpdateBoundingBoxFromSkeleton && mMesh->getBoneBoundingRadius() == Real(0))
            {
                mMesh->_computeBoneBoundingRadius();
            }
        }

        // Build main subentity list
        buildSubEntityList(mMesh, &mSubEntityList);

        // Check if mesh is using manual LOD
        if (mMesh->hasManualLodLevel())
        {
            ushort i, numLod;
            numLod = mMesh->getNumLodLevels();
            // NB skip LOD 0 which is the original
            for (i = 1; i < numLod; ++i)
            {
                const MeshLodUsage& usage = mMesh->getLodLevel(i);
                Entity* lodEnt;
                if(!usage.manualName.empty()){
                    // Disabled to prevent recursion when a.mesh has manualLod to b.mesh and b.mesh has manualLod to a.mesh.
                    OgreAssert(usage.manualMesh->getNumLodLevels() == 1, "Manual Lod Mesh can't have Lod levels!");

                    if(usage.manualMesh->getNumLodLevels() != 1) {
                        // To prevent crash in release builds, we will remove Lod levels.
                        usage.manualMesh->removeLodLevels();
                    }

                    // Manually create entity
                    lodEnt = new Entity(::std::format("{}Lod{}", mName, i),
                        usage.manualMesh);
                } else {
                    // Autogenerated lod uses original entity
                    lodEnt = this;
                }
                mLodEntityList.push_back(lodEnt);
            }
        }

        // Initialise the AnimationState, if Mesh has animation
        if (hasSkeleton())
        {
            mFrameBonesLastUpdated = new unsigned long[1];
            *mFrameBonesLastUpdated = std::numeric_limits<unsigned long>::max();
            mNumBoneMatrices = mSkeletonInstance->getNumBones();
            mBoneMatrices = static_cast<Affine3*>(::Ogre::AlignedMemory::allocate(sizeof(Affine3) * mNumBoneMatrices));
        }
        if (hasSkeleton() || hasVertexAnimation())
        {
            mAnimationState = new AnimationStateSet();
            mMesh->_initAnimationState(mAnimationState);
            prepareTempBlendBuffers();
        }

        reevaluateVertexProcessing();
        
        // Update of bounds of the parent SceneNode, if Entity already attached
        // this can happen if Mesh is loaded in background or after reinitialisation
        if( mParentNode )
        {
            getParentSceneNode()->needUpdate();
        }

        mInitialised = true;
        mMeshStateCount = mMesh->getStateCount();
    }
    //-----------------------------------------------------------------------
    void Entity::_deinitialise()
    {
        if (!mInitialised)
            return;

        // Delete submeshes
        for (auto & i : mSubEntityList)
        {
            // Delete SubEntity
            delete i;
            i = 0;
        }
        mSubEntityList.clear();

        // Delete LOD entities
        for (auto & li : mLodEntityList)
        {
            if(li != this) {
                // Delete
                delete li;
                li = 0;
            }
        }
        mLodEntityList.clear();

        // Delete shadow renderables
        clearShadowRenderableList(mShadowRenderables);

        // Detach all child objects, do this manually to avoid needUpdate() call
        // which can fail because of deleted items
        detachAllObjectsImpl();

        if (mSkeletonInstance) {
            ::Ogre::AlignedMemory::deallocate(mBoneWorldMatrices);
            mBoneWorldMatrices = nullptr;

            if (mSharedSkeletonEntities) {
                mSharedSkeletonEntities->erase(this);
                if (mSharedSkeletonEntities->size() == 1)
                {
                    (*mSharedSkeletonEntities->begin())->stopSharingSkeletonInstance();
                }
                // Should never occur, just in case
                else if (mSharedSkeletonEntities->empty())
                {
                    delete mSharedSkeletonEntities; mSharedSkeletonEntities = nullptr;

                    delete[] mFrameBonesLastUpdated; mFrameBonesLastUpdated = nullptr;
                    delete mSkeletonInstance; mSkeletonInstance = nullptr;
                    ::Ogre::AlignedMemory::deallocate(mBoneMatrices); mBoneMatrices = nullptr;
                    delete mAnimationState; mAnimationState = nullptr;
                }
            } else {
                delete[] mFrameBonesLastUpdated; mFrameBonesLastUpdated = nullptr;
                delete mSkeletonInstance; mSkeletonInstance = nullptr;
                ::Ogre::AlignedMemory::deallocate(mBoneMatrices); mBoneMatrices = nullptr;
                delete mAnimationState; mAnimationState = nullptr;
            }
        }
        else
        {
            //Non-skeletally animated objects don't share the mAnimationState. Always delete.
            //See https://ogre3d.atlassian.net/browse/OGRE-504
            delete mAnimationState;
            mAnimationState = nullptr;
        }

        mSkelAnimVertexData.reset();
        mSoftwareVertexAnimVertexData.reset();
        mHardwareVertexAnimVertexData.reset();

        mInitialised = false;
    }
    //-----------------------------------------------------------------------
    Entity::~Entity()
    {
        _deinitialise();
        // Unregister our listener
        mMesh->removeListener(this);
    }
    //-----------------------------------------------------------------------
    void Entity::_releaseManualHardwareResources()
    {
        clearShadowRenderableList(mShadowRenderables);
    }
    //-----------------------------------------------------------------------
    void Entity::_restoreManualHardwareResources()
    {
        // mShadowRenderables are lazy initialized
    }
    //-----------------------------------------------------------------------
    auto Entity::hasVertexAnimation() const -> bool
    {
        return mMesh->hasVertexAnimation();
    }
    //-----------------------------------------------------------------------
    auto Entity::getMesh() const noexcept -> const MeshPtr&
    {
        return mMesh;
    }
    //-----------------------------------------------------------------------
    auto Entity::getSubEntity(std::string_view name) const -> SubEntity*
    {
        ushort index = mMesh->_getSubMeshIndex(name);
        return getSubEntity(index);
    }
    //-----------------------------------------------------------------------
    auto Entity::clone( std::string_view newName) const -> Entity*
    {
        OgreAssert(mManager, "Cannot clone an Entity that wasn't created through a SceneManager");
        Entity* newEnt = mManager->createEntity(newName, getMesh()->getName() );

        if (mInitialised)
        {
            // Copy material settings
            for (unsigned int n = 0;
                auto const& i : mSubEntityList)
            {
                newEnt->getSubEntity(n)->setMaterialName(i->getMaterialName());
                ++n;
            }
            if (mAnimationState)
            {
                delete newEnt->mAnimationState;
                newEnt->mAnimationState = new AnimationStateSet(*mAnimationState);
            }
        }

        return newEnt;
    }
    //-----------------------------------------------------------------------
    void Entity::setMaterialName( std::string_view name, std::string_view groupName /* = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME */)
    {
        // Set for all subentities
        for (auto & i : mSubEntityList)
        {
            i->setMaterialName(name, groupName);
        }

    }
    //-----------------------------------------------------------------------
    void Entity::setMaterial( const MaterialPtr& material )
    {
        // Set for all subentities
        for (auto & i : mSubEntityList)
        {
            i->setMaterial(material);
        }
    }
    //-----------------------------------------------------------------------
    void Entity::_notifyCurrentCamera(Camera* cam)
    {
        MovableObject::_notifyCurrentCamera(cam);

        // Calculate the LOD
        if (mParentNode)
        {
            // Get mesh lod strategy
            const LodStrategy *meshStrategy = mMesh->getLodStrategy();
            // Get the appropriate LOD value
            Real lodValue = meshStrategy->getValue(this, cam);
            // Bias the LOD value
            Real biasedMeshLodValue = lodValue * mMeshLodFactorTransformed;


            // Get the index at this biased depth
            ushort newMeshLodIndex = mMesh->getLodIndex(biasedMeshLodValue);
            // Apply maximum detail restriction (remember lower = higher detail, higher = lower detail)
            newMeshLodIndex = Math::Clamp(newMeshLodIndex, mMaxMeshLodIndex, mMinMeshLodIndex);

            // Construct event object
            EntityMeshLodChangedEvent evt;
            evt.entity = this;
            evt.camera = cam;
            evt.lodValue = biasedMeshLodValue;
            evt.previousLodIndex = mMeshLodIndex;
            evt.newLodIndex = newMeshLodIndex;

            // Notify LOD event listeners
            cam->getSceneManager()->_notifyEntityMeshLodChanged(evt);

            // Change LOD index
            mMeshLodIndex = evt.newLodIndex;

            // Now do material LOD
            lodValue *= mMaterialLodFactorTransformed;


            for (auto & i : mSubEntityList)
            {
                // Get sub-entity material
                const MaterialPtr& material = i->getMaterial();
                
                // Get material LOD strategy
                const LodStrategy *materialStrategy = material->getLodStrategy();
                
                // Recalculate LOD value if strategies do not match
                Real biasedMaterialLodValue;
                if (meshStrategy == materialStrategy)
                    biasedMaterialLodValue = lodValue;
                else
                    biasedMaterialLodValue = materialStrategy->getValue(this, cam) * materialStrategy->transformBias(mMaterialLodFactor);

                // Get the index at this biased depth
                unsigned short idx = material->getLodIndex(biasedMaterialLodValue);
                // Apply maximum detail restriction (remember lower = higher detail, higher = lower detail)
                idx = Math::Clamp(idx, mMaxMeshLodIndex, mMinMeshLodIndex);

                // Construct event object
                EntityMaterialLodChangedEvent subEntEvt;
                subEntEvt.subEntity = i;
                subEntEvt.camera = cam;
                subEntEvt.lodValue = biasedMaterialLodValue;
                subEntEvt.previousLodIndex = i->mMaterialLodIndex;
                subEntEvt.newLodIndex = idx;

                // Notify LOD event listeners
                cam->getSceneManager()->_notifyEntityMaterialLodChanged(subEntEvt);

                // Change LOD index
                i->mMaterialLodIndex = subEntEvt.newLodIndex;

                // Also invalidate any camera distance cache
                i->_invalidateCameraCache ();
            }


        }
        // Notify any child objects
        for(auto child : mChildObjectList)
        {
            child->_notifyCurrentCamera(cam);
        }
    }
    //-----------------------------------------------------------------------
    void Entity::setUpdateBoundingBoxFromSkeleton(bool update)
    {
        mUpdateBoundingBoxFromSkeleton = update;
        if (mMesh->isLoaded() && mMesh->getBoneBoundingRadius() == Real(0))
        {
            mMesh->_computeBoneBoundingRadius();
        }
    }
    //-----------------------------------------------------------------------
    auto Entity::getBoundingBox() const noexcept -> const AxisAlignedBox&
    {
        // Get from Mesh
        if (mMesh->isLoaded())
        {
            if ( mUpdateBoundingBoxFromSkeleton && hasSkeleton() )
            {
                // get from skeleton
                // self bounding box without children
                AxisAlignedBox bbox;
                bbox.setNull();
                Real maxScale = Real(0);
                bool boneHasVerts[ OGRE_MAX_NUM_BONES ];
                uint16 numBones = mSkeletonInstance->getNumBones();
                for (size_t iBone = 0; iBone < numBones; ++iBone)
                {
                    boneHasVerts[ iBone ] = false;
                }
                // for each bone that has vertices weighted to it,
                for (unsigned long iBone : mMesh->sharedBlendIndexToBoneIndexMap)
                {
                    // record which bones have vertices assigned
                    boneHasVerts[ iBone ] = true;
                }
                // for each submesh,
                for (uint16 iSubMesh = 0; iSubMesh < mMesh->getNumSubMeshes(); ++iSubMesh)
                {
                    SubMesh* submesh = mMesh->getSubMesh( iSubMesh );
                    // if the submesh has own vertices,
                    if ( ! submesh->useSharedVertices )
                    {
                        // record which bones have vertices assigned
                        for (unsigned long iBone : submesh->blendIndexToBoneIndexMap)
                        {
                            boneHasVerts[ iBone ] = true;
                        }
                    }
                }
                // for each bone that has vertices weighted to it,
                for (uint16 iBone = 0; iBone < numBones; ++iBone)
                {
                    if ( boneHasVerts[ iBone ] )
                    {
                        const Bone* bone = mSkeletonInstance->getBone( iBone );
                        Vector3 scaleVec = bone->_getDerivedScale();
                        Real scale = std::max( std::max( Math::Abs(scaleVec.x), Math::Abs(scaleVec.y)), Math::Abs(scaleVec.z) );
                        maxScale = std::max( maxScale, scale );
                        // only include bones that aren't scaled to zero
                        if (scale > Real(0))
                        {
                            bbox.merge( bone->_getDerivedPosition() );
                        }
                    }
                }
                // unless all bones were scaled to zero,
                if (! bbox.isNull())
                {
                    // inflate the bounding box
                    float r = mMesh->getBoneBoundingRadius() * maxScale;  // adjust bone bounding radius by max scale of any bone
                    Vector3 expansion{r, r, r};
                    bbox.setExtents( bbox.getMinimum() - expansion, bbox.getMaximum() + expansion );
                }
                bbox.merge(getChildObjectsBoundingBox());
                // if bounding box has changed,
                if (bbox != mFullBoundingBox)
                {
                    mFullBoundingBox = bbox;
                    Node::queueNeedUpdate( mParentNode );  // inform the parent node to update its AABB (without this, changes to the bbox won't propagate to the scene node)
                }
            }
            else
            {
                // Get from Mesh
                mFullBoundingBox = mMesh->getBounds();
                mFullBoundingBox.merge(getChildObjectsBoundingBox());
            }
            // Don't scale here, this is taken into account when world BBox calculation is done
        }
        else
        {
            mFullBoundingBox.setNull();
        }

        return mFullBoundingBox;
    }
    //-----------------------------------------------------------------------
    auto Entity::getChildObjectsBoundingBox() const -> AxisAlignedBox
    {
        AxisAlignedBox aa_box;
        AxisAlignedBox full_aa_box;
        full_aa_box.setNull();

        for(auto child : mChildObjectList)
        {
            aa_box = child->getBoundingBox();
            auto* tp = static_cast<TagPoint*>(child->getParentNode());
            // Use transform local to skeleton since world xform comes later
            aa_box.transform(tp->_getFullLocalTransform());

            full_aa_box.merge(aa_box);
        }

        return full_aa_box;
    }
    //-----------------------------------------------------------------------
    auto Entity::getWorldBoundingBox(bool derive) const -> const AxisAlignedBox&
    {
        if (derive)
        {
            // derive child bounding boxes
            for(auto child : mChildObjectList)
            {
                child->getWorldBoundingBox(true);
            }
        }
        return MovableObject::getWorldBoundingBox(derive);
    }
    //-----------------------------------------------------------------------
    auto Entity::getWorldBoundingSphere(bool derive) const -> const Sphere&
    {
        if (derive)
        {
            // derive child bounding boxes
            for(auto child : mChildObjectList)
            {
                child->getWorldBoundingSphere(true);
            }
        }
        return MovableObject::getWorldBoundingSphere(derive);

    }
    //-----------------------------------------------------------------------
    void Entity::_updateRenderQueue(RenderQueue* queue)
    {
        // Do nothing if not initialised yet
        if (!mInitialised)
            return;

        // Check mesh state count, will be incremented if reloaded
        if (mMesh->getStateCount() != mMeshStateCount)
        {
            // force reinitialise
            _initialise(true);
        }

        Entity* displayEntity = this;

        // Check we're not using a manual LOD
        if (mMeshLodIndex > 0 && mMesh->hasManualLodLevel())
        {
            // Use alternate entity
            assert( static_cast< size_t >( mMeshLodIndex - 1 ) < mLodEntityList.size() &&
                    "No LOD EntityList - did you build the manual LODs after creating the entity?");
            // index - 1 as we skip index 0 (original LOD)
            displayEntity = mLodEntityList[mMeshLodIndex-1];

            if (displayEntity != this && hasSkeleton() && displayEntity->hasSkeleton())
            {
                // Copy the animation state set to lod entity, we assume the lod
                // entity only has a subset animation states
                AnimationStateSet* targetState = displayEntity->mAnimationState;
                if (mAnimationState != targetState) // only copy if LODs use different skeleton instances
                {
                    if (mAnimationState->getDirtyFrameNumber() != targetState->getDirtyFrameNumber()) // only copy if animation was updated
                        mAnimationState->copyMatchingState(targetState);
                }
            }
        }

        // Add each visible SubEntity to the queue
        for (auto & i : displayEntity->mSubEntityList)
        {
            if(i->isVisible())
            {
                // Order: first use subentity queue settings, if available
                //        if not then use entity queue settings, if available
                //        finally fall back on default queue settings
                if(i->isRenderQueuePrioritySet())
                {
                    assert(i->isRenderQueueGroupSet() == true);
                    queue->addRenderable(i, i->getRenderQueueGroup(), i->getRenderQueuePriority());
                }
                else if(i->isRenderQueueGroupSet())
                {
                    queue->addRenderable(i, i->getRenderQueueGroup());
                }
                else if (mRenderQueuePrioritySet)
                {
                    assert(mRenderQueueIDSet == true);
                    queue->addRenderable(i, mRenderQueueID, mRenderQueuePriority);
                }
                else if(mRenderQueueIDSet)
                {
                    queue->addRenderable(i, mRenderQueueID);
                }
                else
                {
                    queue->addRenderable(i);
                }
            }
        }
        if (getAlwaysUpdateMainSkeleton() && hasSkeleton() && (mMeshLodIndex > 0))
        {
            //check if an update was made
            if (cacheBoneMatrices())
            {
                getSkeleton()->_updateTransforms();
                //We will mark the skeleton as dirty. Otherwise, if in the same frame the entity will 
                //be rendered first with a low LOD and then with a high LOD the system wont know that
                //the bone matrices has changed and there for will not update the vertex buffers
                getSkeleton()->_notifyManualBonesDirty();
            }
        }

        // Since we know we're going to be rendered, take this opportunity to
        // update the animation
        if (displayEntity->hasSkeleton() || displayEntity->hasVertexAnimation())
        {
            displayEntity->updateAnimation();

            //--- pass this point,  we are sure that the transformation matrix of each bone and tagPoint have been updated
            for(auto child : mChildObjectList)
            {
                bool visible = child->isVisible();
                if (visible && (displayEntity != this))
                {
                    //Check if the bone exists in the current LOD

                    //The child is connected to a tagpoint which is connected to a bone
                    Bone* bone = static_cast<Bone*>(child->getParentNode()->getParent());
                    if (!displayEntity->getSkeleton()->hasBone(bone->getName()))
                    {
                        //Current LOD entity does not have the bone that the
                        //child is connected to. Do not display.
                        visible = false;
                    }
                }
                if (visible)
                {
                    child->_updateRenderQueue(queue);
                }   
            }
        }

        // HACK to display bones
        // This won't work if the entity is not centered at the origin
        // TODO work out a way to allow bones to be rendered when Entity not centered
        if (mDisplaySkeleton && hasSkeleton() && mManager && mManager->getDebugDrawer())
        {
            for (Bone* bone : mSkeletonInstance->getBones())
            {
                mManager->getDebugDrawer()->drawBone(bone);
            }
        }
    }
    //-----------------------------------------------------------------------
    auto Entity::getAnimationState(std::string_view name) const -> AnimationState*
    {
        OgreAssert(mAnimationState, "Entity is not animated");
        return mAnimationState->getAnimationState(name);
    }
    //-----------------------------------------------------------------------
    auto Entity::hasAnimationState(std::string_view name) const -> bool
    {
        return mAnimationState && mAnimationState->hasAnimationState(name);
    }
    //-----------------------------------------------------------------------
    auto Entity::getAllAnimationStates() const noexcept -> AnimationStateSet*
    {
        return mAnimationState;
    }
    //-----------------------------------------------------------------------
    auto Entity::getMovableType() const noexcept -> std::string_view
    {
        return EntityFactory::FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
    auto Entity::tempVertexAnimBuffersBound() const -> bool
    {
        // Do we still have temp buffers for software vertex animation bound?
        bool ret = true;
        if (mMesh->sharedVertexData && mMesh->getSharedVertexDataAnimationType() != VertexAnimationType::NONE)
        {
            ret = ret && mTempVertexAnimInfo.buffersCheckedOut(true, mMesh->getSharedVertexDataAnimationIncludesNormals());
        }
        for (auto sub : mSubEntityList)
        {
            if (!sub->getSubMesh()->useSharedVertices
                && sub->getSubMesh()->getVertexAnimationType() != VertexAnimationType::NONE)
            {
                ret = ret && sub->_getVertexAnimTempBufferInfo()->buffersCheckedOut(
                    true, sub->getSubMesh()->getVertexAnimationIncludesNormals());
            }
        }
        return ret;
    }
    //-----------------------------------------------------------------------
    auto Entity::tempSkelAnimBuffersBound(bool requestNormals) const -> bool
    {
        // Do we still have temp buffers for software skeleton animation bound?
        if (mSkelAnimVertexData)
        {
            if (!mTempSkelAnimInfo.buffersCheckedOut(true, requestNormals))
                return false;
        }
        for (auto sub : mSubEntityList)
        {
            if (sub->isVisible() && sub->mSkelAnimVertexData)
            {
                if (!sub->mTempSkelAnimInfo.buffersCheckedOut(true, requestNormals))
                    return false;
            }
        }
        return true;
    }
    //-----------------------------------------------------------------------
    void Entity::updateAnimation()
    {
        // Do nothing if not initialised yet
        if (!mInitialised)
            return;

        Root& root = Root::getSingleton();
        bool hwAnimation = isHardwareAnimationEnabled();
        bool isNeedUpdateHardwareAnim = hwAnimation && !mCurrentHWAnimationState;
        bool forcedSwAnimation = getSoftwareAnimationRequests()>0;
        bool forcedNormals = getSoftwareAnimationNormalsRequests()>0;
        bool stencilShadows = false;
        if (getCastShadows() && hasEdgeList() && root._getCurrentSceneManager())
            stencilShadows =  root._getCurrentSceneManager()->isShadowTechniqueStencilBased();
        bool softwareAnimation = !hwAnimation || stencilShadows || forcedSwAnimation;
        // Blend normals in s/w only if we're not using h/w animation,
        // since shadows only require positions
        bool blendNormals = !hwAnimation || forcedNormals;
        // Animation dirty if animation state modified or manual bones modified
        bool animationDirty =
            (mFrameAnimationLastUpdated != mAnimationState->getDirtyFrameNumber()) ||
            (hasSkeleton() && getSkeleton()->getManualBonesDirty());
        
        //update the current hardware animation state
        mCurrentHWAnimationState = hwAnimation;

        // We only do these tasks if animation is dirty
        // Or, if we're using a skeleton and manual bones have been moved
        // Or, if we're using software animation and temp buffers are unbound
        if (animationDirty ||
            (softwareAnimation && hasVertexAnimation() && !tempVertexAnimBuffersBound()) ||
            (softwareAnimation && hasSkeleton() && !tempSkelAnimBuffersBound(blendNormals)))
        {
            if (hasVertexAnimation())
            {
                if (softwareAnimation)
                {
                    // grab & bind temporary buffer for positions (& normals if they are included)
                    if (mSoftwareVertexAnimVertexData
                        && mMesh->getSharedVertexDataAnimationType() != VertexAnimationType::NONE)
                    {
                        bool useNormals = mMesh->getSharedVertexDataAnimationIncludesNormals();
                        mTempVertexAnimInfo.checkoutTempCopies(true, useNormals);
                        // NB we suppress hardware upload while doing blend if we're
                        // hardware animation, because the only reason for doing this
                        // is for shadow, which need only be uploaded then
                        mTempVertexAnimInfo.bindTempCopies(mSoftwareVertexAnimVertexData.get(),
                                                           hwAnimation);
                    }
                    for (auto se : mSubEntityList)
                    {
                        // Blend dedicated geometry
                        if (se->isVisible() && se->mSoftwareVertexAnimVertexData
                            && se->getSubMesh()->getVertexAnimationType() != VertexAnimationType::NONE)
                        {
                            bool useNormals = se->getSubMesh()->getVertexAnimationIncludesNormals();
                            se->mTempVertexAnimInfo.checkoutTempCopies(true, useNormals);
                            se->mTempVertexAnimInfo.bindTempCopies(se->mSoftwareVertexAnimVertexData.get(),
                                                                   hwAnimation);
                        }

                    }
                }
                applyVertexAnimation(hwAnimation, stencilShadows);
            }

            if (hasSkeleton())
            {
                cacheBoneMatrices();

                // Software blend?
                if (softwareAnimation)
                {
                    const Affine3* blendMatrices[256];

                    // Ok, we need to do a software blend
                    // Firstly, check out working vertex buffers
                    if (mSkelAnimVertexData)
                    {
                        // Blend shared geometry
                        // NB we suppress hardware upload while doing blend if we're
                        // hardware animation, because the only reason for doing this
                        // is for shadow, which need only be uploaded then
                        mTempSkelAnimInfo.checkoutTempCopies(true, blendNormals);
                        mTempSkelAnimInfo.bindTempCopies(mSkelAnimVertexData.get(),
                                                         hwAnimation);
                        // Prepare blend matrices, TODO: Move out of here
                        Mesh::prepareMatricesForVertexBlend(blendMatrices,
                                                            mBoneMatrices, mMesh->sharedBlendIndexToBoneIndexMap);
                        // Blend, taking source from either mesh data or morph data
                        Mesh::softwareVertexBlend(
                            (mMesh->getSharedVertexDataAnimationType() != VertexAnimationType::NONE) ?
                            mSoftwareVertexAnimVertexData.get() : mMesh->sharedVertexData,
                            mSkelAnimVertexData.get(),
                            blendMatrices, mMesh->sharedBlendIndexToBoneIndexMap.size(),
                            blendNormals);
                    }
                    for (auto se : mSubEntityList)
                    {
                        // Blend dedicated geometry
                        if (se->isVisible() && se->mSkelAnimVertexData)
                        {
                            se->mTempSkelAnimInfo.checkoutTempCopies(true, blendNormals);
                            se->mTempSkelAnimInfo.bindTempCopies(se->mSkelAnimVertexData.get(),
                                                                 hwAnimation);
                            // Prepare blend matrices, TODO: Move out of here
                            Mesh::prepareMatricesForVertexBlend(blendMatrices,
                                                                mBoneMatrices, se->mSubMesh->blendIndexToBoneIndexMap);
                            // Blend, taking source from either mesh data or morph data
                            Mesh::softwareVertexBlend(
                                (se->getSubMesh()->getVertexAnimationType() != VertexAnimationType::NONE)?
                                se->mSoftwareVertexAnimVertexData.get() : se->mSubMesh->vertexData.get(),
                                se->mSkelAnimVertexData.get(),
                                blendMatrices, se->mSubMesh->blendIndexToBoneIndexMap.size(),
                                blendNormals);
                        }

                    }

                }
            }

            // Trigger update of bounding box if necessary
            if (!mChildObjectList.empty())
                mParentNode->needUpdate();

            mFrameAnimationLastUpdated = mAnimationState->getDirtyFrameNumber();
        }

        // Need to update the child object's transforms when animation dirty
        // or parent node transform has altered.
        if (hasSkeleton() && 
            (isNeedUpdateHardwareAnim || 
             animationDirty || mLastParentXform != _getParentNodeFullTransform()))
        {
            // Cache last parent transform for next frame use too.
            mLastParentXform = _getParentNodeFullTransform();

            //--- Update the child object's transforms
            for(auto child : mChildObjectList)
            {
                child->getParentNode()->_update(true, true);
            }

            // Also calculate bone world matrices, since are used as replacement world matrices,
            // but only if it's used (when using hardware animation and skeleton animated).
            if (hwAnimation && _isSkeletonAnimated())
            {
                // Allocate bone world matrices on demand, for better memory footprint
                // when using software animation.
                if (!mBoneWorldMatrices)
                {
                    mBoneWorldMatrices =
                        static_cast<Affine3*>(::Ogre::AlignedMemory::allocate(sizeof(Affine3) * mNumBoneMatrices));
                    std::ranges::fill(std::span{mBoneWorldMatrices, mNumBoneMatrices}, Affine3::IDENTITY);
                }

                OptimisedUtil::getImplementation()->concatenateAffineMatrices(
                    mLastParentXform,
                    mBoneMatrices,
                    mBoneWorldMatrices,
                    mNumBoneMatrices);
            }
        }
    }
    //-----------------------------------------------------------------------
    auto Entity::initHardwareAnimationElements(VertexData* vdata,
                                                 ushort numberOfElements, bool animateNormals) -> ushort
    {
        ushort elemsSupported = numberOfElements;
        if (vdata->hwAnimationDataList.size() < numberOfElements)
        {
            elemsSupported = 
                vdata->allocateHardwareAnimationElements(numberOfElements, animateNormals);
        }
        // Initialise parametrics in case we don't use all of them
        for (auto & i : vdata->hwAnimationDataList)
        {
            i.parametric = 0.0f;
        }
        // reset used count
        vdata->hwAnimDataItemsUsed = 0;
                
        return elemsSupported;

    }
    //-----------------------------------------------------------------------
    void Entity::applyVertexAnimation(bool hardwareAnimation, bool stencilShadows)
    {
        const MeshPtr& msh = getMesh();
        bool swAnim = !hardwareAnimation || stencilShadows || (mSoftwareAnimationRequests>0);

        // make sure we have enough hardware animation elements to play with
        if (hardwareAnimation)
        {
            if (mHardwareVertexAnimVertexData
                && msh->getSharedVertexDataAnimationType() != VertexAnimationType::NONE)
            {
                ushort supportedCount =
                    initHardwareAnimationElements(mHardwareVertexAnimVertexData.get(),
                                                  (msh->getSharedVertexDataAnimationType() == VertexAnimationType::POSE)
                                                  ? mHardwarePoseCount : 1, 
                                                  msh->getSharedVertexDataAnimationIncludesNormals());
                
                if (msh->getSharedVertexDataAnimationType() == VertexAnimationType::POSE && 
                    supportedCount < mHardwarePoseCount)
                {
                    LogManager::getSingleton().stream() <<
                        "Vertex program assigned to Entity '" << mName << 
                        "' claimed to support " << mHardwarePoseCount << 
                        " morph/pose vertex sets, but in fact only " << supportedCount <<
                        " were able to be supported in the shared mesh data.";
                    mHardwarePoseCount = supportedCount;
                }
                    
}
            for (auto sub : mSubEntityList)
            {
                if (sub->getSubMesh()->getVertexAnimationType() != VertexAnimationType::NONE &&
                    !sub->getSubMesh()->useSharedVertices)
                {
                    ushort supportedCount = initHardwareAnimationElements(
                        sub->_getHardwareVertexAnimVertexData(),
                        (sub->getSubMesh()->getVertexAnimationType() == VertexAnimationType::POSE)
                        ? sub->mHardwarePoseCount : 1,
                        sub->getSubMesh()->getVertexAnimationIncludesNormals());

                    if (sub->getSubMesh()->getVertexAnimationType() == VertexAnimationType::POSE && 
                        supportedCount < sub->mHardwarePoseCount)
                    {
                        LogManager::getSingleton().stream() <<
                        "Vertex program assigned to SubEntity of '" << mName << 
                        "' claimed to support " << sub->mHardwarePoseCount << 
                        " morph/pose vertex sets, but in fact only " << supportedCount <<
                        " were able to be supported in the mesh data.";
                        sub->mHardwarePoseCount = supportedCount;
                    }
                    
                }
            }

        }
        else
        {
            // May be blending multiple poses in software
            // Suppress hardware upload of buffers
            // Note, we query position buffer here but it may also include normals
            if (mSoftwareVertexAnimVertexData &&
                mMesh->getSharedVertexDataAnimationType() == VertexAnimationType::POSE)
            {
                const VertexElement* elem = mSoftwareVertexAnimVertexData
                    ->vertexDeclaration->findElementBySemantic(VertexElementSemantic::POSITION);
                HardwareVertexBufferSharedPtr buf = mSoftwareVertexAnimVertexData
                    ->vertexBufferBinding->getBuffer(elem->getSource());
                buf->suppressHardwareUpdate(true);
                
                initialisePoseVertexData(mMesh->sharedVertexData, mSoftwareVertexAnimVertexData.get(),
                    mMesh->getSharedVertexDataAnimationIncludesNormals());
            }
            for (auto sub : mSubEntityList)
            {
                if (!sub->getSubMesh()->useSharedVertices &&
                    sub->getSubMesh()->getVertexAnimationType() == VertexAnimationType::POSE)
                {
                    VertexData* data = sub->_getSoftwareVertexAnimVertexData();
                    const VertexElement* elem = data->vertexDeclaration
                        ->findElementBySemantic(VertexElementSemantic::POSITION);
                    HardwareVertexBufferSharedPtr buf = data
                        ->vertexBufferBinding->getBuffer(elem->getSource());
                    buf->suppressHardwareUpdate(true);
                    // if we're animating normals, we need to start with zeros
                    initialisePoseVertexData(sub->getSubMesh()->vertexData.get(), data,
                        sub->getSubMesh()->getVertexAnimationIncludesNormals());
                }
            }
        }


        // Now apply the animation(s)
        // Note - you should only apply one morph animation to each set of vertex data
        // at once; if you do more, only the last one will actually apply
        markBuffersUnusedForAnimation();
        for (auto state : mAnimationState->getEnabledAnimationStates())
        {
            Animation* anim = msh->_getAnimationImpl(state->getAnimationName());
            if (anim)
            {
                anim->apply(this, state->getTimePosition(), state->getWeight(),
                    swAnim, hardwareAnimation);
            }
        }
        // Deal with cases where no animation applied
        restoreBuffersForUnusedAnimation(hardwareAnimation);

        // Unsuppress hardware upload if we suppressed it
        if (!hardwareAnimation)
        {
            if (mSoftwareVertexAnimVertexData &&
                msh->getSharedVertexDataAnimationType() == VertexAnimationType::POSE)
            {
                // if we're animating normals, if pose influence < 1 need to use the base mesh
                if (mMesh->getSharedVertexDataAnimationIncludesNormals())
                    finalisePoseNormals(mMesh->sharedVertexData, mSoftwareVertexAnimVertexData.get());
            
                const VertexElement* elem = mSoftwareVertexAnimVertexData
                    ->vertexDeclaration->findElementBySemantic(VertexElementSemantic::POSITION);
                HardwareVertexBufferSharedPtr buf = mSoftwareVertexAnimVertexData
                    ->vertexBufferBinding->getBuffer(elem->getSource());
                buf->suppressHardwareUpdate(false);
            }
            for (auto sub : mSubEntityList)
            {
                if (!sub->getSubMesh()->useSharedVertices &&
                    sub->getSubMesh()->getVertexAnimationType() == VertexAnimationType::POSE)
                {
                    VertexData* data = sub->_getSoftwareVertexAnimVertexData();
                    // if we're animating normals, if pose influence < 1 need to use the base mesh
                    if (sub->getSubMesh()->getVertexAnimationIncludesNormals())
                        finalisePoseNormals(sub->getSubMesh()->vertexData.get(), data);
                    
                    const VertexElement* elem = data->vertexDeclaration
                        ->findElementBySemantic(VertexElementSemantic::POSITION);
                    HardwareVertexBufferSharedPtr buf = data
                        ->vertexBufferBinding->getBuffer(elem->getSource());
                    buf->suppressHardwareUpdate(false);
                }
            }
        }

    }
    //-----------------------------------------------------------------------------
    void Entity::markBuffersUnusedForAnimation()
    {
        mVertexAnimationAppliedThisFrame = false;
        for (auto & i : mSubEntityList)
        {
            i->_markBuffersUnusedForAnimation();
        }
    }
    //-----------------------------------------------------------------------------
    void Entity::_markBuffersUsedForAnimation()
    {
        mVertexAnimationAppliedThisFrame = true;
        // no cascade
    }
    //-----------------------------------------------------------------------------
    void Entity::restoreBuffersForUnusedAnimation(bool hardwareAnimation)
    {
        // Rebind original positions if:
        //  We didn't apply any animation and
        //    We're morph animated (hardware binds keyframe, software is missing)
        //    or we're pose animated and software (hardware is fine, still bound)
        if (mMesh->sharedVertexData &&
            !mVertexAnimationAppliedThisFrame &&
            (!hardwareAnimation || mMesh->getSharedVertexDataAnimationType() == VertexAnimationType::MORPH))
        {
            // Note, VertexElementSemantic::POSITION is specified here but if normals are included in animation
            // then these will be re-bound too (buffers must be shared)
            const VertexElement* srcPosElem =
                mMesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::POSITION);
            HardwareVertexBufferSharedPtr srcBuf =
                mMesh->sharedVertexData->vertexBufferBinding->getBuffer(
                    srcPosElem->getSource());

            // Bind to software
            const VertexElement* destPosElem =
                mSoftwareVertexAnimVertexData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::POSITION);
            mSoftwareVertexAnimVertexData->vertexBufferBinding->setBinding(
                destPosElem->getSource(), srcBuf);
                
        }

        // rebind any missing hardware pose buffers
        // Caused by not having any animations enabled, or keyframes which reference
        // no poses
        if (mMesh->sharedVertexData && hardwareAnimation 
            && mMesh->getSharedVertexDataAnimationType() == VertexAnimationType::POSE)
        {
            bindMissingHardwarePoseBuffers(mMesh->sharedVertexData, mHardwareVertexAnimVertexData.get());
        }


        for (auto & i : mSubEntityList)
        {
            i->_restoreBuffersForUnusedAnimation(hardwareAnimation);
        }

    }
    //---------------------------------------------------------------------
    void Entity::bindMissingHardwarePoseBuffers(const VertexData* srcData, 
        VertexData* destData)
    {
        // For hardware pose animation, also make sure we've bound buffers to all the elements
        // required - if there are missing bindings for elements in use,
        // some rendersystems can complain because elements refer
        // to an unbound source.
        // Get the original position source, we'll use this to fill gaps
        const VertexElement* srcPosElem =
            srcData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::POSITION);
        HardwareVertexBufferSharedPtr srcBuf =
            srcData->vertexBufferBinding->getBuffer(
                srcPosElem->getSource());

        for (auto i = destData->hwAnimationDataList.begin();
            i != destData->hwAnimationDataList.end(); ++i)
        {
            const VertexData::HardwareAnimationData& animData = *i;
            if (!destData->vertexBufferBinding->isBufferBound(
                animData.targetBufferIndex))
            {
                // Bind to a safe default
                destData->vertexBufferBinding->setBinding(
                    animData.targetBufferIndex, srcBuf);
            }
        }

    }
    //-----------------------------------------------------------------------
    void Entity::initialisePoseVertexData(const VertexData* srcData, 
        VertexData* destData, bool animateNormals)
    {
    
        // First time through for a piece of pose animated vertex data
        // We need to copy the original position values to the temp accumulator
        const VertexElement* origelem = 
            srcData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::POSITION);
        const VertexElement* destelem = 
            destData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::POSITION);
        HardwareVertexBufferSharedPtr origBuffer = 
            srcData->vertexBufferBinding->getBuffer(origelem->getSource());
        HardwareVertexBufferSharedPtr destBuffer = 
            destData->vertexBufferBinding->getBuffer(destelem->getSource());
        destBuffer->copyData(*origBuffer.get(), 0, 0, destBuffer->getSizeInBytes(), true);
    
        // If normals are included in animation, we want to reset the normals to zero
        if (animateNormals)
        {
            const VertexElement* normElem =
                destData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::NORMAL);
                
            if (normElem)
            {
                HardwareVertexBufferSharedPtr buf = 
                    destData->vertexBufferBinding->getBuffer(normElem->getSource());
                HardwareBufferLockGuard vertexLock(buf, HardwareBuffer::LockOptions::NORMAL);
                char* pBase = static_cast<char*>(vertexLock.pData) + destData->vertexStart * buf->getVertexSize();
                
                for (size_t v = 0; v < destData->vertexCount; ++v)
                {
                    float* pNorm;
                    normElem->baseVertexPointerToElement(pBase, &pNorm);
                    *pNorm++ = 0.0f;
                    *pNorm++ = 0.0f;
                    *pNorm++ = 0.0f;
                    
                    pBase += buf->getVertexSize();
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void Entity::finalisePoseNormals(const VertexData* srcData, VertexData* destData)
    {
        const VertexElement* destNormElem =
            destData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::NORMAL);
        const VertexElement* srcNormElem =
            srcData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::NORMAL);
            
        if (destNormElem && srcNormElem)
        {
            auto srcbuf = srcData->vertexBufferBinding->getBuffer(srcNormElem->getSource());
            auto dstbuf = destData->vertexBufferBinding->getBuffer(destNormElem->getSource());

            HardwareBufferLockGuard dstLock(dstbuf, HardwareBuffer::LockOptions::NORMAL);
            char* pDstBase = static_cast<char*>(dstLock.pData) + destData->vertexStart * dstbuf->getVertexSize();
            char* pSrcBase = pDstBase;

            HardwareBufferLockGuard srcLock;
            if (srcbuf != dstbuf)
            {
                srcLock.lock(srcbuf, HardwareBuffer::LockOptions::READ_ONLY);
                pSrcBase = static_cast<char*>(srcLock.pData) + srcData->vertexStart * srcbuf->getVertexSize();
            }
            // The goal here is to detect the length of the vertices, and to apply
            // the base mesh vertex normal at one minus that length; this deals with 
            // any individual vertices which were either not affected by any pose, or
            // were not affected to a complete extent
            // We also normalise every normal to deal with over-weighting
            for (size_t v = 0; v < destData->vertexCount; ++v)
            {
                float* pDstNorm;
                destNormElem->baseVertexPointerToElement(pDstBase, &pDstNorm);
                Vector3 norm{pDstNorm[0], pDstNorm[1], pDstNorm[2]};
                Real len = norm.length();
                if (len + 1e-4f < 1.0f)
                {
                    // Poses did not completely fill in this normal
                    // Apply base mesh
                    float baseWeight = 1.0f - (float)len;
                    float* pSrcNorm;
                    srcNormElem->baseVertexPointerToElement(pSrcBase, &pSrcNorm);
                    norm.x += *pSrcNorm++ * baseWeight;
                    norm.y += *pSrcNorm++ * baseWeight;
                    norm.z += *pSrcNorm++ * baseWeight;
                }
                norm.normalise();
                
                *pDstNorm++ = (float)norm.x;
                *pDstNorm++ = (float)norm.y;
                *pDstNorm++ = (float)norm.z;
                
                pDstBase += dstbuf->getVertexSize();
                pSrcBase += dstbuf->getVertexSize();
            }
        }
    }
    //-----------------------------------------------------------------------
    void Entity::_updateAnimation()
    {
        // Externally visible method
        if (hasSkeleton() || hasVertexAnimation())
        {
            updateAnimation();
        }
    }
    //-----------------------------------------------------------------------
    auto Entity::_isAnimated() const -> bool
    {
        return (mAnimationState && mAnimationState->hasEnabledAnimationState()) ||
               (getSkeleton() && getSkeleton()->hasManualBones());
    }
    //-----------------------------------------------------------------------
    auto Entity::_isSkeletonAnimated() const -> bool
    {
        return getSkeleton() &&
            (mAnimationState->hasEnabledAnimationState() || getSkeleton()->hasManualBones());
    }
    //-----------------------------------------------------------------------
    auto Entity::_getSkelAnimVertexData() const -> VertexData*
    {
        assert (mSkelAnimVertexData && "Not software skinned or has no shared vertex data!");
        return mSkelAnimVertexData.get();
    }
    //-----------------------------------------------------------------------
    auto Entity::_getSoftwareVertexAnimVertexData() const -> VertexData*
    {
        assert (mSoftwareVertexAnimVertexData && "Not vertex animated or has no shared vertex data!");
        return mSoftwareVertexAnimVertexData.get();
    }
    //-----------------------------------------------------------------------
    auto Entity::_getHardwareVertexAnimVertexData() const -> VertexData*
    {
        assert (mHardwareVertexAnimVertexData && "Not vertex animated or has no shared vertex data!");
        return mHardwareVertexAnimVertexData.get();
    }
    //-----------------------------------------------------------------------
    auto Entity::_getSkelAnimTempBufferInfo() -> TempBlendedBufferInfo*
    {
        return &mTempSkelAnimInfo;
    }
    //-----------------------------------------------------------------------
    auto Entity::_getVertexAnimTempBufferInfo() -> TempBlendedBufferInfo*
    {
        return &mTempVertexAnimInfo;
    }
    //-----------------------------------------------------------------------
    auto Entity::cacheBoneMatrices() -> bool
    {
        Root& root = Root::getSingleton();
        unsigned long currentFrameNumber = root.getNextFrameNumber();
        if ((*mFrameBonesLastUpdated != currentFrameNumber) ||
            (hasSkeleton() && getSkeleton()->getManualBonesDirty()))
        {
            if ((!mSkipAnimStateUpdates) && (*mFrameBonesLastUpdated != currentFrameNumber))
                mSkeletonInstance->setAnimationState(*mAnimationState);
            mSkeletonInstance->_getBoneMatrices(mBoneMatrices);
            *mFrameBonesLastUpdated  = currentFrameNumber;

            return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    void Entity::setDisplaySkeleton(bool display)
    {
        mDisplaySkeleton = display;
    }
    //-----------------------------------------------------------------------
    auto Entity::getDisplaySkeleton() const noexcept -> bool
    {
        return mDisplaySkeleton;
    }
    //-----------------------------------------------------------------------
    auto Entity::getNumManualLodLevels() const -> size_t
    {
        return mLodEntityList.size();
    }

    //-----------------------------------------------------------------------
    auto Entity::getManualLodLevel(size_t index) const -> Entity*
    {
        assert(index < mLodEntityList.size());

        return mLodEntityList[index];
    }

    //-----------------------------------------------------------------------
    void Entity::setMeshLodBias(Real factor, ushort maxDetailIndex, ushort minDetailIndex)
    {
        mMeshLodFactorTransformed = mMesh->getLodStrategy()->transformBias(factor);
        mMaxMeshLodIndex = maxDetailIndex;
        mMinMeshLodIndex = minDetailIndex;
    }
    //-----------------------------------------------------------------------
    void Entity::setMaterialLodBias(Real factor, ushort maxDetailIndex, ushort minDetailIndex)
    {
        mMaterialLodFactor = factor;
        mMaterialLodFactorTransformed = mMesh->getLodStrategy()->transformBias(factor);
        mMaxMaterialLodIndex = maxDetailIndex;
        mMinMaterialLodIndex = minDetailIndex;
    }
    //-----------------------------------------------------------------------
    void Entity::buildSubEntityList(MeshPtr& mesh, SubEntityList* sublist)
    {
        // Create SubEntities
        size_t i, numSubMeshes;

        numSubMeshes = mesh->getNumSubMeshes();
        for (i = 0; i < numSubMeshes; ++i)
        {
            SubMesh* subMesh = mesh->getSubMesh(i);
            auto* subEnt = new SubEntity(this, subMesh);
            if (subMesh->getMaterial())
                subEnt->setMaterial(subMesh->getMaterial());
            sublist->push_back(subEnt);
        }
    }
    //-----------------------------------------------------------------------
    void Entity::setPolygonModeOverrideable(bool overrideable)
    {
        for (auto & i : mSubEntityList)
        {
            i->setPolygonModeOverrideable(overrideable);
        }
    }

    //-----------------------------------------------------------------------

    struct MovableObjectNameExists {
        std::string_view name;
        auto operator()(const MovableObject* mo) -> bool {
            return mo->getName() == name;
        }
    };

    auto Entity::attachObjectToBone(std::string_view boneName, MovableObject *pMovable, const Quaternion &offsetOrientation, const Vector3 &offsetPosition) -> TagPoint*
    {
        MovableObjectNameExists pred = {pMovable->getName()};
        auto it = std::ranges::find_if(mChildObjectList, pred);
        if (it != mChildObjectList.end())
        {
            OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM,
                ::std::format("An object with the name {} already attached", pMovable->getName() ),
                "Entity::attachObjectToBone");
        }
        OgreAssert(!pMovable->isAttached(), "Object already attached to a sceneNode or a Bone");
        OgreAssert(hasSkeleton(), "This entity's mesh has no skeleton to attach object to");
        Bone* bone = mSkeletonInstance->getBone(boneName);
        if (!bone)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, ::std::format("Cannot locate bone named {}", boneName),
                "Entity::attachObjectToBone");
        }

        TagPoint *tp = mSkeletonInstance->createTagPointOnBone(
            bone, offsetOrientation, offsetPosition);
        tp->setParentEntity(this);
        tp->setChildObject(pMovable);

        attachObjectImpl(pMovable, tp);

        // Trigger update of bounding box if necessary
        if (mParentNode)
            mParentNode->needUpdate();

        return tp;
    }

    //-----------------------------------------------------------------------
    void Entity::attachObjectImpl(MovableObject *pObject, TagPoint *pAttachingPoint)
    {
        assert(std::ranges::find_if(mChildObjectList,
                            MovableObjectNameExists{pObject->getName()}) == mChildObjectList.end());

        mChildObjectList.push_back(pObject);
        pObject->_notifyAttached(pAttachingPoint, true);
    }

    //-----------------------------------------------------------------------
    auto Entity::detachObjectFromBone(std::string_view name) -> MovableObject*
    {
        MovableObjectNameExists pred = {name};
        auto it = std::ranges::find_if(mChildObjectList, pred);

        if (it == mChildObjectList.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("No child object entry found named {}", name),
                "Entity::detachObjectFromBone");
        }
        detachObjectImpl(*it);
        std::swap(*it, mChildObjectList.back());
        mChildObjectList.pop_back();

        // Trigger update of bounding box if necessary
        if (mParentNode)
            mParentNode->needUpdate();

        return *it;
    }
    //-----------------------------------------------------------------------
    void Entity::detachObjectFromBone(MovableObject* obj)
    {
        for (auto i = mChildObjectList.begin(); i != mChildObjectList.end(); ++i)
        {
            if (*i == obj)
            {
                detachObjectImpl(obj);
                std::swap(*i, mChildObjectList.back());
                mChildObjectList.pop_back();

                // Trigger update of bounding box if necessary
                if (mParentNode)
                    mParentNode->needUpdate();
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    void Entity::detachAllObjectsFromBone()
    {
        detachAllObjectsImpl();

        // Trigger update of bounding box if necessary
        if (mParentNode)
            mParentNode->needUpdate();
    }
    //-----------------------------------------------------------------------
    void Entity::detachObjectImpl(MovableObject* pObject)
    {
        auto* tp = static_cast<TagPoint*>(pObject->getParentNode());

        // free the TagPoint so we can reuse it later
        mSkeletonInstance->freeTagPoint(tp);

        pObject->_notifyAttached((TagPoint*)nullptr);
    }
    //-----------------------------------------------------------------------
    void Entity::detachAllObjectsImpl()
    {
        for (auto child : mChildObjectList)
        {
            detachObjectImpl(child);
        }
        mChildObjectList.clear();
    }

    //-----------------------------------------------------------------------
    auto Entity::getBoundingRadius() const -> Real
    {
        return mMesh->getBoundingSphereRadius();
    }
    //-----------------------------------------------------------------------
    void Entity::prepareTempBlendBuffers()
    {
        mSkelAnimVertexData.reset();
        mSoftwareVertexAnimVertexData.reset();
        mHardwareVertexAnimVertexData.reset();

        if (hasVertexAnimation())
        {
            // Shared data
            if (mMesh->sharedVertexData
                && mMesh->getSharedVertexDataAnimationType() != VertexAnimationType::NONE)
            {
                // Create temporary vertex blend info
                // Prepare temp vertex data if needed
                // Clone without copying data, don't remove any blending info
                // (since if we skeletally animate too, we need it)
                mSoftwareVertexAnimVertexData.reset(mMesh->sharedVertexData->clone(false));
                extractTempBufferInfo(mSoftwareVertexAnimVertexData.get(), &mTempVertexAnimInfo);

                // Also clone for hardware usage, don't remove blend info since we'll
                // need it if we also hardware skeletally animate
                mHardwareVertexAnimVertexData.reset(mMesh->sharedVertexData->clone(false));
            }
        }

        if (hasSkeleton())
        {
            // Shared data
            if (mMesh->sharedVertexData)
            {
                // Create temporary vertex blend info
                // Prepare temp vertex data if needed
                // Clone without copying data, remove blending info
                // (since blend is performed in software)
                mSkelAnimVertexData.reset(
                    cloneVertexDataRemoveBlendInfo(mMesh->sharedVertexData));
                extractTempBufferInfo(mSkelAnimVertexData.get(), &mTempSkelAnimInfo);
            }

        }

        // Do SubEntities
        for (auto s : mSubEntityList)
        {
            s->prepareTempBlendBuffers();
        }

        // It's prepared for shadow volumes only if mesh has been prepared for shadow volumes.
        mPreparedForShadowVolumes = mMesh->isPreparedForShadowVolumes();
    }
    //-----------------------------------------------------------------------
    void Entity::extractTempBufferInfo(VertexData* sourceData, TempBlendedBufferInfo* info)
    {
        info->extractFrom(sourceData);
    }
    //-----------------------------------------------------------------------
    auto Entity::cloneVertexDataRemoveBlendInfo(const VertexData* source) -> VertexData*
    {
        // Clone without copying data
        VertexData* ret = source->clone(false);
        bool removeIndices = Ogre::Root::getSingleton().isBlendIndicesGpuRedundant();
        bool removeWeights = Ogre::Root::getSingleton().isBlendWeightsGpuRedundant();
         
        unsigned short safeSource = 0xFFFF;
        const VertexElement* blendIndexElem =
            source->vertexDeclaration->findElementBySemantic(VertexElementSemantic::BLEND_INDICES);
        if (blendIndexElem)
        {
            //save the source in order to prevent the next stage from unbinding it.
            safeSource = blendIndexElem->getSource();
            if (removeIndices)
            {
                // Remove buffer reference
                ret->vertexBufferBinding->unsetBinding(blendIndexElem->getSource());
            }
        }
        if (removeWeights)
        {
            // Remove blend weights
            const VertexElement* blendWeightElem =
                source->vertexDeclaration->findElementBySemantic(VertexElementSemantic::BLEND_WEIGHTS);
            if (blendWeightElem &&
                blendWeightElem->getSource() != safeSource)
            {
                // Remove buffer reference
                ret->vertexBufferBinding->unsetBinding(blendWeightElem->getSource());
            }
        }

        // remove elements from declaration
        if (removeIndices)
            ret->vertexDeclaration->removeElement(VertexElementSemantic::BLEND_INDICES);
        if (removeWeights)
            ret->vertexDeclaration->removeElement(VertexElementSemantic::BLEND_WEIGHTS);

        // Close gaps in bindings for effective and safely
        if (removeWeights || removeIndices)
            ret->closeGapsInBindings();

        return ret;
    }
    //-----------------------------------------------------------------------
    auto Entity::getEdgeList() noexcept -> EdgeData*
    {
        // Get from Mesh
        return mMesh->getEdgeList(mMeshLodIndex);
    }
    //-----------------------------------------------------------------------
    auto Entity::isHardwareAnimationEnabled() noexcept -> bool
    {
        //find whether the entity has hardware animation for the current active sceme
        unsigned short schemeIndex = MaterialManager::getSingleton()._getActiveSchemeIndex();
        for(const auto& p : mSchemeHardwareAnim)
        {
            if(p.first == schemeIndex) return p.second;
        }
        bool ret = calcVertexProcessing();
        mSchemeHardwareAnim.emplace_back(schemeIndex, ret);
        return ret;
    }

    //-----------------------------------------------------------------------
    void Entity::reevaluateVertexProcessing()
    {
        //clear the cache so that the values will be reevaluated
        mSchemeHardwareAnim.clear();
    }
    //-----------------------------------------------------------------------
    auto Entity::calcVertexProcessing() -> bool
    {
        // init
        bool hasHardwareAnimation = false;
        bool firstPass = true;

        for (auto sub : mSubEntityList)
        {
            const MaterialPtr& m = sub->getMaterial();
            // Make sure it's loaded
            m->load();
            Technique* t = m->getBestTechnique(0, sub);
            if (!t)
            {
                // No supported techniques
                continue;
            }
            if (t->getNumPasses() == 0)
            {
                // No passes, invalid
                continue;
            }
            Pass* p = t->getPass(0);
            if (p->hasVertexProgram())
            {
                if (mVertexProgramInUse == false)
                {
                    // If one material uses a vertex program, set this flag
                    // Causes some special processing like forcing a separate light cap
                    mVertexProgramInUse = true;
                    
                    // If shadow renderables already created create their light caps
                    for (auto & mShadowRenderable : mShadowRenderables)
                    {
                        static_cast<EntityShadowRenderable*>(mShadowRenderable)->_createSeparateLightCap();
                    }
                }

                if (hasSkeleton())
                {
                    // All materials must support skinning for us to consider using
                    // hardware animation - if one fails we use software
                    if (firstPass)
                    {
                        hasHardwareAnimation = p->getVertexProgram()->isSkeletalAnimationIncluded();
                        firstPass = false;
                    }
                    else
                    {
                        hasHardwareAnimation = hasHardwareAnimation &&
                            p->getVertexProgram()->isSkeletalAnimationIncluded();
                    }
                }

                VertexAnimationType animType = VertexAnimationType::NONE;
                if (sub->getSubMesh()->useSharedVertices)
                {
                    animType = mMesh->getSharedVertexDataAnimationType();
                }
                else
                {
                    animType = sub->getSubMesh()->getVertexAnimationType();
                }
                if (animType == VertexAnimationType::MORPH)
                {
                    // All materials must support morph animation for us to consider using
                    // hardware animation - if one fails we use software
                    if (firstPass)
                    {
                        hasHardwareAnimation = p->getVertexProgram()->isMorphAnimationIncluded();
                        firstPass = false;
                    }
                    else
                    {
                        hasHardwareAnimation = hasHardwareAnimation &&
                            p->getVertexProgram()->isMorphAnimationIncluded();
                    }
                }
                else if (animType == VertexAnimationType::POSE)
                {
                    // All materials must support pose animation for us to consider using
                    // hardware animation - if one fails we use software
                    if (firstPass)
                    {
                        hasHardwareAnimation = p->getVertexProgram()->isPoseAnimationIncluded();
                        if (sub->getSubMesh()->useSharedVertices)
                            mHardwarePoseCount = p->getVertexProgram()->getNumberOfPosesIncluded();
                        else
                            sub->mHardwarePoseCount = p->getVertexProgram()->getNumberOfPosesIncluded();
                        firstPass = false;
                    }
                    else
                    {
                        hasHardwareAnimation = hasHardwareAnimation &&
                            p->getVertexProgram()->isPoseAnimationIncluded();
                        if (sub->getSubMesh()->useSharedVertices)
                            mHardwarePoseCount = std::max(mHardwarePoseCount,
                                p->getVertexProgram()->getNumberOfPosesIncluded());
                        else
                            sub->mHardwarePoseCount = std::max(sub->mHardwarePoseCount,
                                p->getVertexProgram()->getNumberOfPosesIncluded());
                    }
                }

            }
        }

        // Should be force update of animation if they exists, due reevaluate
        // vertex processing might switchs between hardware/software animation,
        // and then we'll end with NULL or incorrect mBoneWorldMatrices, or
        // incorrect blended software animation buffers.
        if (mAnimationState)
        {
            mFrameAnimationLastUpdated = mAnimationState->getDirtyFrameNumber() - 1;
        }

        return hasHardwareAnimation;
    }

    //-----------------------------------------------------------------------
    auto Entity::_getMeshLodFactorTransformed() const -> Real
    {
        return mMeshLodFactorTransformed;
    }
    //-----------------------------------------------------------------------
    auto
    Entity::getShadowVolumeRenderableList(const Light* light, const HardwareIndexBufferPtr& indexBuffer,
                                          size_t& indexBufferUsedSize, float extrusionDistance, ShadowRenderableFlags flags) -> const ShadowRenderableList&
    {
        assert(indexBuffer->getType() == HardwareIndexBuffer::IndexType::_16BIT &&
               "Only 16-bit indexes supported for now");

        // Check mesh state count, will be incremented if reloaded
        if (mMesh->getStateCount() != mMeshStateCount)
        {
            // force reinitialise
            _initialise(true);
        }

        // Potentially delegate to LOD entity
        if (mMesh->hasManualLodLevel() && mMeshLodIndex > 0)
        {
            // Use alternate entity
            assert( static_cast< size_t >( mMeshLodIndex - 1 ) < mLodEntityList.size() &&
                "No LOD EntityList - did you build the manual LODs after creating the entity?");

            Entity* requiredEntity = mLodEntityList[mMeshLodIndex-1];
            if (requiredEntity != this) {
                // delegate, we're using manual LOD and not the top lod index
                if (hasSkeleton() && mLodEntityList[mMeshLodIndex-1]->hasSkeleton())
                {
                    // Copy the animation state set to lod entity, we assume the lod
                    // entity only has a subset animation states
                    AnimationStateSet* targetState = mLodEntityList[mMeshLodIndex-1]->mAnimationState;
                    if (mAnimationState != targetState) // only copy if lods have different skeleton instances
                    {
                        if (mAnimationState && mAnimationState->getDirtyFrameNumber() != targetState->getDirtyFrameNumber()) // only copy if animation was updated
                            mAnimationState->copyMatchingState(targetState);
                    }
                }
                return mLodEntityList[mMeshLodIndex - 1]->getShadowVolumeRenderableList(
                    light, indexBuffer, indexBufferUsedSize, extrusionDistance, flags);
            }
        }

        // Prepare temp buffers if required
        if (!mPreparedForShadowVolumes)
        {
            mMesh->prepareForShadowVolume();
            // reset frame last updated to force update of animations if they exist
            if (mAnimationState)
                mFrameAnimationLastUpdated = mAnimationState->getDirtyFrameNumber() - 1;
            // re-prepare buffers
            prepareTempBlendBuffers();
        }


        bool hasAnimation = (hasSkeleton() || hasVertexAnimation());

        // Update any animation
        if (hasAnimation)
        {
            updateAnimation();
        }

        // Calculate the object space light details
        Vector4 lightPos = light->getAs4DVector();
        Affine3 world2Obj = mParentNode->_getFullTransform().inverse();
        lightPos = world2Obj * lightPos;
        Matrix3 world2Obj3x3 = world2Obj.linear();
        extrusionDistance *= Math::Sqrt(std::min(std::min(world2Obj3x3.GetColumn(0).squaredLength(), world2Obj3x3.GetColumn(1).squaredLength()), world2Obj3x3.GetColumn(2).squaredLength()));

        // We need to search the edge list for silhouette edges
        EdgeData* edgeList = getEdgeList();

        if (!edgeList)
        {
            // we can't get an edge list for some reason, return blank
            // really we shouldn't be able to get here, but this is a safeguard
            return mShadowRenderables;
        }

        // Init shadow renderable list if required
        bool init = mShadowRenderables.empty();

        if (init)
            mShadowRenderables.resize(edgeList->edgeGroups.size());

        bool isAnimated = hasAnimation;
        bool updatedSharedGeomNormals = false;
        bool extrude = !!(flags & ShadowRenderableFlags::EXTRUDE_IN_SOFTWARE);
        auto egi = edgeList->edgeGroups.begin();
        for (auto & mShadowRenderable : mShadowRenderables)
        {
            const VertexData *pVertData;
            if (isAnimated)
            {
                // Use temp buffers
                pVertData = findBlendedVertexData(egi->vertexData);
            }
            else
            {
                pVertData = egi->vertexData;
            }
            if (init)
            {
                // Try to find corresponding SubEntity; this allows the
                // linkage of visibility between ShadowRenderable and SubEntity
                SubEntity* subent = findSubEntityForVertexData(egi->vertexData);
                // Create a new renderable, create a separate light cap if
                // we're using a vertex program (either for this model, or
                // for extruding the shadow volume) since otherwise we can
                // get depth-fighting on the light cap

                mShadowRenderable = new EntityShadowRenderable(this, indexBuffer, pVertData,
                                                      mVertexProgramInUse || !extrude, subent);
            }
            else
            {
                // If we have animation, we have no guarantee that the position
                // buffer we used last frame is the same one we used last frame
                // since a temporary buffer is requested each frame
                // therefore, we need to update the EntityShadowRenderable
                // with the current position buffer
                static_cast<EntityShadowRenderable*>(mShadowRenderable)->rebindPositionBuffer(pVertData, hasAnimation);

            }
            // Get shadow renderable
            HardwareVertexBufferSharedPtr esrPositionBuffer = mShadowRenderable->getPositionBuffer();
            // For animated entities we need to recalculate the face normals
            if (hasAnimation)
            {
                if (egi->vertexData != mMesh->sharedVertexData || !updatedSharedGeomNormals)
                {
                    // recalculate face normals
                    edgeList->updateFaceNormals(egi->vertexSet, esrPositionBuffer);
                    // If we're not extruding in software we still need to update
                    // the latter part of the buffer (the hardware extruded part)
                    // with the latest animated positions
                    if (!extrude)
                    {
                        // Lock, we'll be locking the (suppressed hardware update) shadow buffer
                        HardwareBufferLockGuard posLock(esrPositionBuffer, HardwareBuffer::LockOptions::NORMAL);
                        auto* pSrc = static_cast<float*>(posLock.pData);
                        float* pDest = pSrc + (egi->vertexData->vertexCount * 3);
                        memcpy(pDest, pSrc, sizeof(float) * 3 * egi->vertexData->vertexCount);
                    }
                    if (egi->vertexData == mMesh->sharedVertexData)
                    {
                        updatedSharedGeomNormals = true;
                    }
                }
            }
            // Extrude vertices in software if required
            if (extrude)
            {
                extrudeVertices(esrPositionBuffer,
                    egi->vertexData->vertexCount,
                    lightPos, extrusionDistance);

            }
            // Stop suppressing hardware update now, if we were
            esrPositionBuffer->suppressHardwareUpdate(false);

            ++egi;
        }
        // Calc triangle light facing
        updateEdgeListLightFacing(edgeList, lightPos);

        // Generate indexes and update renderables
        generateShadowVolume(edgeList, indexBuffer, indexBufferUsedSize, light, mShadowRenderables, flags);

        return mShadowRenderables;
    }
    //-----------------------------------------------------------------------
    auto Entity::findBlendedVertexData(const VertexData* orig) -> const VertexData*
    {
        bool skel = hasSkeleton();

        if (orig == mMesh->sharedVertexData)
        {
            return skel? mSkelAnimVertexData.get() : mSoftwareVertexAnimVertexData.get();
        }
        for (auto se : mSubEntityList)
        {
            if (orig == se->getSubMesh()->vertexData.get())
            {
                return skel? se->_getSkelAnimVertexData() : se->_getSoftwareVertexAnimVertexData();
            }
        }
        // None found
        OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
            "Cannot find blended version of the vertex data specified.",
            "Entity::findBlendedVertexData");
    }
    //-----------------------------------------------------------------------
    auto Entity::findSubEntityForVertexData(const VertexData* orig) -> SubEntity*
    {
        if (orig == mMesh->sharedVertexData)
        {
            return nullptr;
        }

        for (auto se : mSubEntityList)
        {
            if (orig == se->getSubMesh()->vertexData.get())
            {
                return se;
            }
        }

        // None found
        return nullptr;
    }
    //-----------------------------------------------------------------------
    void Entity::addSoftwareAnimationRequest(bool normalsAlso)
    {
        mSoftwareAnimationRequests++;
        if (normalsAlso) {
            mSoftwareAnimationNormalsRequests++;
        }
    }
    //-----------------------------------------------------------------------
    void Entity::removeSoftwareAnimationRequest(bool normalsAlso)
    {
        if (mSoftwareAnimationRequests == 0 ||
            (normalsAlso && mSoftwareAnimationNormalsRequests == 0))
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "Attempt to remove nonexistent request.",
                        "Entity::removeSoftwareAnimationRequest");
        }
        mSoftwareAnimationRequests--;
        if (normalsAlso) {
            mSoftwareAnimationNormalsRequests--;
        }
    }
    //-----------------------------------------------------------------------
    void Entity::_notifyAttached(Node* parent, bool isTagPoint)
    {
        MovableObject::_notifyAttached(parent, isTagPoint);

        // Also notify LOD entities
        for (auto & i : mLodEntityList)
        {
            if(i != this)
                i->_notifyAttached(parent, isTagPoint);
        }
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    Entity::EntityShadowRenderable::EntityShadowRenderable(MovableObject* parent,
                                                           const HardwareIndexBufferSharedPtr& indexBuffer,
                                                           const VertexData* vertexData,
                                                           bool createSeparateLightCap, SubEntity* subent,
                                                           bool isLightCap)
        : ShadowRenderable(parent, indexBuffer, vertexData, false, isLightCap), mSubEntity(subent)
    {
        // Save link to vertex data
        mCurrentVertexData = vertexData;
        mOriginalPosBufferBinding =
            vertexData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::POSITION)->getSource();
        if (!isLightCap && createSeparateLightCap) // we passed createSeparateLightCap=false to parent
        {
            _createSeparateLightCap();
        }
    }

    //-----------------------------------------------------------------------
    void Entity::EntityShadowRenderable::_createSeparateLightCap()
    {
        if (mLightCap == nullptr)
        {
            // Create child light cap
            mLightCap = ::std::make_unique<EntityShadowRenderable>(mParent,
                mRenderOp.indexData->indexBuffer, mCurrentVertexData, false, mSubEntity, true);
        }   
    }
    //-----------------------------------------------------------------------
    void Entity::EntityShadowRenderable::rebindPositionBuffer(const VertexData* vertexData, bool force)
    {
        if (force || mCurrentVertexData != vertexData)
        {
            mCurrentVertexData = vertexData;
            mPositionBuffer = mCurrentVertexData->vertexBufferBinding->getBuffer(
                mOriginalPosBufferBinding);
            mRenderOp.vertexData->vertexBufferBinding->setBinding(0, mPositionBuffer);
            if (mLightCap)
            {
                static_cast<EntityShadowRenderable*>(mLightCap.get())->rebindPositionBuffer(vertexData, force);
            }
        }
    }
    //-----------------------------------------------------------------------
    auto Entity::EntityShadowRenderable::isVisible() const noexcept -> bool
    {
        if (mSubEntity)
        {
            return mSubEntity->isVisible();
        }
        else
        {
            return ShadowRenderable::isVisible();
        }
    }
    //-----------------------------------------------------------------------
    void Entity::setRenderQueueGroup(RenderQueueGroupID queueID)
    {
        MovableObject::setRenderQueueGroup(queueID);

        // Set render queue for all manual LOD entities
        if (mMesh->hasManualLodLevel())
        {
            for (auto & li : mLodEntityList)
            {
                if(li != this)
                    li->setRenderQueueGroup(queueID);
            }
        }
    }
    //-----------------------------------------------------------------------
    void Entity::setRenderQueueGroupAndPriority(RenderQueueGroupID queueID, ushort priority)
    {
        MovableObject::setRenderQueueGroupAndPriority(queueID, priority);

        // Set render queue for all manual LOD entities
        if (mMesh->hasManualLodLevel())
        {
            for (auto & li : mLodEntityList)
            {
                if(li != this)
                    li->setRenderQueueGroupAndPriority(queueID, priority);
            }
        }
    }
    //-----------------------------------------------------------------------
    void Entity::shareSkeletonInstanceWith(Entity* entity)
    {
        OgreAssert(entity->getMesh()->getSkeleton() == getMesh()->getSkeleton(),
                   "The supplied entity has a different skeleton");
        OgreAssert(mSkeletonInstance, "This entity has no skeleton");
        OgreAssert(!mSharedSkeletonEntities || !entity->mSharedSkeletonEntities,
                   "Both entities already shares their SkeletonInstances! At least one of the instances "
                   "must not share it's instance");

        //check if we already share our skeletoninstance, we don't want to delete it if so
        if (mSharedSkeletonEntities != nullptr)
        {
            entity->shareSkeletonInstanceWith(this);
        }
        else
        {
            delete mSkeletonInstance;
            ::Ogre::AlignedMemory::deallocate(mBoneMatrices);
            delete mAnimationState;

            delete[] mFrameBonesLastUpdated;
            mSkeletonInstance = entity->mSkeletonInstance;
            mNumBoneMatrices = entity->mNumBoneMatrices;
            mBoneMatrices = entity->mBoneMatrices;
            mAnimationState = entity->mAnimationState;
            mFrameBonesLastUpdated = entity->mFrameBonesLastUpdated;
            if (entity->mSharedSkeletonEntities == nullptr)
            {
                entity->mSharedSkeletonEntities = new EntitySet();
                entity->mSharedSkeletonEntities->insert(entity);
            }
            mSharedSkeletonEntities = entity->mSharedSkeletonEntities;
            mSharedSkeletonEntities->insert(this);
        }
    }
    //-----------------------------------------------------------------------
    void Entity::stopSharingSkeletonInstance()
    {
        OgreAssert(mSharedSkeletonEntities, "This entity is not sharing it's skeletoninstance");
        //check if there's no other than us sharing the skeleton instance
        if (mSharedSkeletonEntities->size() == 1)
        {
            //just reset
            delete mSharedSkeletonEntities;
            mSharedSkeletonEntities = nullptr;
        }
        else
        {
            mSkeletonInstance = new SkeletonInstance(mMesh->getSkeleton());
            mSkeletonInstance->load();
            mAnimationState = new AnimationStateSet();
            mMesh->_initAnimationState(mAnimationState);
            mFrameBonesLastUpdated = new unsigned long[1];
            *mFrameBonesLastUpdated = std::numeric_limits<unsigned long>::max();
            mNumBoneMatrices = mSkeletonInstance->getNumBones();
            mBoneMatrices = static_cast<Affine3*>(::Ogre::AlignedMemory::allocate(sizeof(Affine3) * mNumBoneMatrices));

            mSharedSkeletonEntities->erase(this);
            if (mSharedSkeletonEntities->size() == 1)
            {
                (*mSharedSkeletonEntities->begin())->stopSharingSkeletonInstance();
            }
            mSharedSkeletonEntities = nullptr;
        }
    }
    //-----------------------------------------------------------------------
    void Entity::refreshAvailableAnimationState()
    {
        mMesh->_refreshAnimationState(mAnimationState);
    }
    //-----------------------------------------------------------------------
    auto Entity::getTypeFlags() const noexcept -> QueryTypeMask
    {
        return QueryTypeMask::ENTITY;
    }
    //-----------------------------------------------------------------------
    auto Entity::getVertexDataForBinding() noexcept -> VertexData*
    {
        Entity::VertexDataBindChoice c =
            chooseVertexDataForBinding(mMesh->getSharedVertexDataAnimationType() != VertexAnimationType::NONE);
        using enum VertexDataBindChoice;
        switch(c)
        {
        case ORIGINAL:
            return mMesh->sharedVertexData;
        case HARDWARE_MORPH:
            return mHardwareVertexAnimVertexData.get();
        case SOFTWARE_MORPH:
            return mSoftwareVertexAnimVertexData.get();
        case SOFTWARE_SKELETAL:
            return mSkelAnimVertexData.get();
        };
        // keep compiler happy
        return mMesh->sharedVertexData;
    }
    //-----------------------------------------------------------------------
    auto Entity::chooseVertexDataForBinding(bool vertexAnim) -> Entity::VertexDataBindChoice
    {
        if (hasSkeleton())
        {
            if (!isHardwareAnimationEnabled())
            {
                // all software skeletal binds same vertex data
                // may be a 2-stage s/w transform including morph earlier though
                return VertexDataBindChoice::SOFTWARE_SKELETAL;
            }
            else if (vertexAnim)
            {
                // hardware morph animation
                return VertexDataBindChoice::HARDWARE_MORPH;
            }
            else
            {
                // hardware skeletal, no morphing
                return VertexDataBindChoice::ORIGINAL;
            }
        }
        else if (vertexAnim)
        {
            // morph only, no skeletal
            if (isHardwareAnimationEnabled())
            {
                return VertexDataBindChoice::HARDWARE_MORPH;
            }
            else
            {
                return VertexDataBindChoice::SOFTWARE_MORPH;
            }

        }
        else
        {
            return VertexDataBindChoice::ORIGINAL;
        }

    }
    //---------------------------------------------------------------------
    void Entity::visitRenderables(Renderable::Visitor* visitor, 
        bool debugRenderables)
    {
        // Visit each SubEntity
        for (auto & i : mSubEntityList)
        {
            visitor->visit(i, 0, false);
        }

        // if manual LOD is in use, visit those too
        ushort lodi = 1;
        for (auto e = mLodEntityList.begin(); 
            e != mLodEntityList.end(); ++e, ++lodi)
        {
            if(*e != this) {
                size_t nsub = (*e)->getNumSubEntities();
                for (uint s = 0; s < nsub; ++s)
                {
                    visitor->visit((*e)->getSubEntity(s), lodi, false);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string_view const constinit EntityFactory::FACTORY_TYPE_NAME = "Entity";
    //-----------------------------------------------------------------------
    auto EntityFactory::getType() const noexcept -> std::string_view
    {
        return FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
    auto EntityFactory::createInstanceImpl( std::string_view name,
        const NameValuePairList* params) -> MovableObject*
    {
        // must have mesh parameter
        MeshPtr pMesh;
        if (params != nullptr)
        {
            String groupName{ ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME };

            NameValuePairList::const_iterator ni;

            ni = params->find("resourceGroup");
            if (ni != params->end())
            {
                groupName = ni->second;
            }

            ni = params->find("mesh");
            if (ni != params->end())
            {
                // Get mesh (load if required)
                pMesh = MeshManager::getSingleton().load(
                    ni->second,
                    // autodetect group location
                    groupName );
            }

        }
        if (!pMesh)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                "'mesh' parameter required when constructing an Entity.",
                "EntityFactory::createInstance");
        }

        return new Entity(name, pMesh);

    }
}
