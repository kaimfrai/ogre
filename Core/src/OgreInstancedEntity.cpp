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
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include "OgreAlignedAllocator.hpp"
#include "OgreAnimationState.hpp"
#include "OgreCamera.hpp"
#include "OgreException.hpp"
#include "OgreInstanceBatch.hpp"
#include "OgreInstancedEntity.hpp"
#include "OgreMath.hpp"
#include "OgreMesh.hpp"
#include "OgreNameGenerator.hpp"
#include "OgreOptimisedUtil.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreSkeletonInstance.hpp"
#include "OgreSphere.hpp"
#include "OgreStringConverter.hpp"

namespace Ogre
{
class AxisAlignedBox;

    NameGenerator InstancedEntity::msNameGenerator("");

    InstancedEntity::InstancedEntity( InstanceBatch *batchOwner, uint32 instanceID, InstancedEntity* sharedTransformEntity ) :
                MovableObject(),
                mInstanceId( instanceID ),
                
                mBatchOwner( batchOwner ),
                
                mFrameAnimationLastUpdated(std::numeric_limits<unsigned long>::max() - 1),
                
                mTransformLookupNumber(instanceID),
                mPosition(Vector3::ZERO),
                mDerivedLocalPosition(Vector3::ZERO),
                mOrientation(Quaternion::IDENTITY),
                mScale(Vector3::UNIT_SCALE)
                

    
    {
        //Use a static name generator to ensure this name stays unique (which may not happen
        //otherwise due to reparenting when defragmenting)
        mName = ::std::format("{}/InstancedEntity_{}/{}",
                    batchOwner->getName(),
                    mInstanceId,
                    msNameGenerator.generate());

        if (sharedTransformEntity)
        {
            sharedTransformEntity->shareTransformWith(this);
        }
        else
        {
            createSkeletonInstance();
        }
        updateTransforms();
    }

    InstancedEntity::~InstancedEntity()
    {
        unlinkTransform();
        destroySkeletonInstance();
    }

    bool InstancedEntity::shareTransformWith( InstancedEntity *slave )
    {
        if( !this->mBatchOwner->_getMeshRef()->hasSkeleton() ||
            !this->mBatchOwner->_getMeshRef()->getSkeleton() ||
            !this->mBatchOwner->_supportsSkeletalAnimation() )
        {
            return false;
        }

        if( this->mSharedTransformEntity  )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         ::std::format("Attempted to share '{}' transforms "
                            "with slave '{}' but '{}' is "
                            "already sharing. Hierarchical sharing not allowed.",
                            "InstancedEntity::shareTransformWith",
                            mName,
                            slave->mName,
                            mName));
            return false;
        }

        if( this->mBatchOwner->_getMeshRef()->getSkeleton() !=
            slave->mBatchOwner->_getMeshRef()->getSkeleton() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE, "Sharing transforms requires both instanced"
                                            " entities to have the same skeleton",
                                            "InstancedEntity::shareTransformWith" );
            return false;
        }

        slave->unlinkTransform();
        slave->destroySkeletonInstance();
        
        slave->mSkeletonInstance    = this->mSkeletonInstance;
        slave->mAnimationState      = this->mAnimationState;
        slave->mBoneMatrices        = this->mBoneMatrices;
        if (mBatchOwner->useBoneWorldMatrices())
        {
            slave->mBoneWorldMatrices   = this->mBoneWorldMatrices;
        }
        slave->mSharedTransformEntity = this;
        //The sharing partners are kept in the parent entity 
        this->mSharingPartners.push_back( slave );
        
        slave->mBatchOwner->_markTransformSharingDirty();

        return true;
    }
    //-----------------------------------------------------------------------
    void InstancedEntity::stopSharingTransform()
    {
        if( mSharedTransformEntity )
        {
            stopSharingTransformAsSlave( true );
        }
        else
        {
            //Tell the ones sharing skeleton with us to use their own
            for (auto const& itor : mSharingPartners)
            {
                itor->stopSharingTransformAsSlave( false );
            }
            mSharingPartners.clear();
        }
    }
    //-----------------------------------------------------------------------
    const String& InstancedEntity::getMovableType() const noexcept
    {
        static String sType = "InstancedEntity";
        return sType;
    }
    //-----------------------------------------------------------------------
    size_t InstancedEntity::getTransforms( Matrix4 *xform ) const
    {
        size_t retVal = 1;

        //When not attached, returns zero matrix to avoid rendering this one, not identity
        if( isVisible() && isInScene() )
        {
            if( !mSkeletonInstance )
            {
                *xform = mBatchOwner->useBoneWorldMatrices() ? 
                        _getParentNodeFullTransform() : Matrix4::IDENTITY;
            }
            else
            {
                Affine3* matrices = mBatchOwner->useBoneWorldMatrices() ? mBoneWorldMatrices : mBoneMatrices;
                const Mesh::IndexMap *indexMap = mBatchOwner->_getIndexToBoneMap();

                for (auto const& itor : *indexMap)
                    *xform++ = matrices[itor];

                retVal = indexMap->size();
            }
        }
        else
        {
            if( mSkeletonInstance )
                retVal = mBatchOwner->_getIndexToBoneMap()->size();

            std::ranges::fill(std::span{xform, retVal}, Matrix4::ZERO );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------
    size_t InstancedEntity::getTransforms3x4( Matrix3x4f *xform ) const
    {
        size_t retVal;
        //When not attached, returns zero matrix to avoid rendering this one, not identity
        if( isVisible() && isInScene() )
        {
            if( !mSkeletonInstance )
            {
                const Affine3& mat = mBatchOwner->useBoneWorldMatrices() ?
                    _getParentNodeFullTransform() : Affine3::IDENTITY;

                *xform = Matrix3x4f(mat[0]);
                retVal = 12;
            }
            else
            {
                Affine3* matrices = mBatchOwner->useBoneWorldMatrices() ? mBoneWorldMatrices : mBoneMatrices;
                const Mesh::IndexMap *indexMap = mBatchOwner->_getIndexToBoneMap();
                
                for(auto i : *indexMap)
                {
                    *xform++ = Matrix3x4f(matrices[i][0]);
                }

                retVal = indexMap->size() * 4 * 3;
            }
        }
        else
        {
            if( mSkeletonInstance )
                retVal = mBatchOwner->_getIndexToBoneMap()->size() * 3 * 4;
            else
                retVal = 12;
            
            std::ranges::fill(std::span{xform, retVal/12}, Matrix3x4f(Affine3::ZERO[0]) );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------
    bool InstancedEntity::findVisible( Camera *camera ) const
    {
        //Object is active
        bool retVal = isInScene();
        if (retVal) 
        {
            //check object is explicitly visible
            retVal = isVisible();

            //Object's bounding box is viewed by the camera
            if( retVal && camera )
                retVal = camera->isVisible(Sphere(_getDerivedPosition(),getBoundingRadius() * getMaxScaleCoef()));
        }

        return retVal;
    }
    //-----------------------------------------------------------------------
    void InstancedEntity::createSkeletonInstance()
    {
        //Is mesh skeletally animated?
        if( mBatchOwner->_getMeshRef()->hasSkeleton() &&
            mBatchOwner->_getMeshRef()->getSkeleton() &&
            mBatchOwner->_supportsSkeletalAnimation() )
        {
            mSkeletonInstance = new SkeletonInstance( mBatchOwner->_getMeshRef()->getSkeleton() );
            mSkeletonInstance->load();

            mBoneMatrices       = static_cast<Affine3*>(::Ogre::AlignedMemory::allocate(sizeof(Affine3) *
                                                                    mSkeletonInstance->getNumBones()));
            if (mBatchOwner->useBoneWorldMatrices())
            {
                mBoneWorldMatrices  = static_cast<Affine3*>(::Ogre::AlignedMemory::allocate(sizeof(Affine3) *
                                                                    mSkeletonInstance->getNumBones()));
                std::ranges::fill(std::span{mBoneWorldMatrices, mSkeletonInstance->getNumBones()}, Affine3::IDENTITY);
            }

            mAnimationState = new AnimationStateSet();
            mBatchOwner->_getMeshRef()->_initAnimationState( mAnimationState );
        }
    }
    //-----------------------------------------------------------------------
    void InstancedEntity::destroySkeletonInstance()
    {
        if( mSkeletonInstance )
        {
            //Tell the ones sharing skeleton with us to use their own
            //sharing partners will remove themselves from notifyUnlink
            while( mSharingPartners.empty() == false )
            {
                mSharingPartners.front()->stopSharingTransform();
            }
            mSharingPartners.clear();

            delete mSkeletonInstance;
            delete mAnimationState;
            ::Ogre::AlignedMemory::deallocate(mBoneMatrices);
            ::Ogre::AlignedMemory::deallocate(mBoneWorldMatrices);

            mSkeletonInstance   = nullptr;
            mAnimationState     = nullptr;
            mBoneMatrices       = nullptr;
            mBoneWorldMatrices  = nullptr;
        }
    }
    //-----------------------------------------------------------------------
    void InstancedEntity::stopSharingTransformAsSlave( bool notifyMaster )
    {
        unlinkTransform( notifyMaster );
        createSkeletonInstance();
    }
    //-----------------------------------------------------------------------
    void InstancedEntity::unlinkTransform( bool notifyMaster )
    {
        if( mSharedTransformEntity )
        {
            //Tell our master we're no longer his slave
            if( notifyMaster )
                mSharedTransformEntity->notifyUnlink( this );
            mBatchOwner->_markTransformSharingDirty();

            mSkeletonInstance   = nullptr;
            mAnimationState     = nullptr;
            mBoneMatrices       = nullptr;
            mBoneWorldMatrices  = nullptr;
            mSharedTransformEntity = nullptr;
        }
    }
    //-----------------------------------------------------------------------
    void InstancedEntity::notifyUnlink( const InstancedEntity *slave )
    {
        //Find the slave and remove it
        auto itor = mSharingPartners.begin();
        auto end  = mSharingPartners.end();
        while( itor != end )
        {
            if( *itor == slave )
            {
                std::swap(*itor,mSharingPartners.back());
                mSharingPartners.pop_back();
                break;
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------
    const AxisAlignedBox& InstancedEntity::getBoundingBox() const noexcept
    {
        //TODO: Add attached objects (TagPoints) to the bbox
        return mBatchOwner->_getMeshReference()->getBounds();
    }

    //-----------------------------------------------------------------------
    Real InstancedEntity::getBoundingRadius() const
    {
        return mBatchOwner->_getMeshReference()->getBoundingSphereRadius();
    }
    //-----------------------------------------------------------------------
    Real InstancedEntity::getSquaredViewDepth( const Camera* cam ) const
    {
        return _getDerivedPosition().squaredDistance(cam->getDerivedPosition());
    }
    //-----------------------------------------------------------------------
    void InstancedEntity::_notifyMoved()
    {
        markTransformDirty();
        MovableObject::_notifyMoved();
        updateTransforms();
    }

    //-----------------------------------------------------------------------
    void InstancedEntity::_notifyAttached( Node* parent, bool isTagPoint )
    {
        markTransformDirty();
        MovableObject::_notifyAttached( parent, isTagPoint );
        updateTransforms();
    }
    //-----------------------------------------------------------------------
    AnimationState* InstancedEntity::getAnimationState(const String& name) const
    {
        if (!mAnimationState)
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Entity is not animated",
                "InstancedEntity::getAnimationState");
        }

        return mAnimationState->getAnimationState(name);
    }
    //-----------------------------------------------------------------------
    AnimationStateSet* InstancedEntity::getAllAnimationStates() const noexcept
    {
        return mAnimationState;
    }
    //-----------------------------------------------------------------------
    bool InstancedEntity::_updateAnimation()
    {
        if (mSharedTransformEntity)
        {
            return mSharedTransformEntity->_updateAnimation();
        }
        else
        {
            const bool animationDirty =
                (mFrameAnimationLastUpdated != mAnimationState->getDirtyFrameNumber()) ||
                (mSkeletonInstance->getManualBonesDirty());

            if( animationDirty || (mNeedAnimTransformUpdate &&  mBatchOwner->useBoneWorldMatrices()))
            {
                mSkeletonInstance->setAnimationState( *mAnimationState );
                mSkeletonInstance->_getBoneMatrices( mBoneMatrices );

                // Cache last parent transform for next frame use too.
                if (mBatchOwner->useBoneWorldMatrices())
                {
                    OptimisedUtil::getImplementation()->concatenateAffineMatrices(
                                                    _getParentNodeFullTransform(),
                                                    mBoneMatrices,
                                                    mBoneWorldMatrices,
                                                    mSkeletonInstance->getNumBones() );
                    mNeedAnimTransformUpdate = false;
                }
                
                mFrameAnimationLastUpdated = mAnimationState->getDirtyFrameNumber();

                return true;
            }
        }

        return false;
    }

    //-----------------------------------------------------------------------
    void InstancedEntity::markTransformDirty()
    {
        mNeedTransformUpdate = true;
        mNeedAnimTransformUpdate = true; 
        mBatchOwner->_boundsDirty();
    }

    //---------------------------------------------------------------------------
    void InstancedEntity::setPosition(const Vector3& position, bool doUpdate) 
    { 
        mPosition = position; 
        mDerivedLocalPosition = position;
        mUseLocalTransform = true;
        markTransformDirty();
        if (doUpdate) updateTransforms();
    } 

    //---------------------------------------------------------------------------
    void InstancedEntity::setOrientation(const Quaternion& orientation, bool doUpdate) 
    { 
        mOrientation = orientation;  
        mUseLocalTransform = true;
        markTransformDirty();
        if (doUpdate) updateTransforms();
    } 

    //---------------------------------------------------------------------------
    void InstancedEntity::setScale(const Vector3& scale, bool doUpdate) 
    { 
        mScale = scale; 
        mMaxScaleLocal = std::max<Real>(std::max<Real>(
            Math::Abs(mScale.x), Math::Abs(mScale.y)), Math::Abs(mScale.z)); 
        mUseLocalTransform = true;
        markTransformDirty();
        if (doUpdate) updateTransforms();
    } 

    //---------------------------------------------------------------------------
    Real InstancedEntity::getMaxScaleCoef() const 
    { 
        return mMaxScaleLocal;
    }

    //---------------------------------------------------------------------------
    void InstancedEntity::updateTransforms()
    {
        if (mNeedTransformUpdate)
        {
            if (mUseLocalTransform)
            {
                if (mParentNode)
                {
                    const Vector3& parentPosition = mParentNode->_getDerivedPosition();
                    const Quaternion& parentOrientation = mParentNode->_getDerivedOrientation();
                    const Vector3& parentScale = mParentNode->_getDerivedScale();

                    Quaternion derivedOrientation = parentOrientation * mOrientation;
                    Vector3 derivedScale = parentScale * mScale;
                    mDerivedLocalPosition = parentOrientation * (parentScale * mPosition) + parentPosition;

                    mFullLocalTransform.makeTransform(mDerivedLocalPosition, derivedScale, derivedOrientation);
                }
                else
                {
                    mFullLocalTransform.makeTransform(mPosition,mScale,mOrientation);
                }
            } else {
                if (mParentNode) {
                    const Ogre::Vector3& parentScale = mParentNode->_getDerivedScale();
                    mMaxScaleLocal = std::max<Real>(std::max<Real>(
                            Math::Abs(parentScale.x), Math::Abs(parentScale.y)), Math::Abs(parentScale.z));
                }
            }
            mNeedTransformUpdate = false;
        }
    }

    //---------------------------------------------------------------------------
    void InstancedEntity::setInUse( bool used )
    {
        mInUse = used;
        //Remove the use of local transform if the object is deleted
        mUseLocalTransform &= used;
    }
    //---------------------------------------------------------------------------
    void InstancedEntity::setCustomParam( unsigned char idx, const Vector4 &newParam )
    {
        mBatchOwner->_setCustomParam( this, idx, newParam );
    }
    //---------------------------------------------------------------------------
    const Vector4& InstancedEntity::getCustomParam( unsigned char idx )
    {
        return mBatchOwner->_getCustomParam( this, idx );
    }
}
