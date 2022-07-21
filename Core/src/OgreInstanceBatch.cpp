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

module Ogre.Core;

import :Camera;
import :Exception;
import :Frustum;
import :InstanceBatch;
import :InstanceManager;
import :InstancedEntity;
import :LodListener;
import :Material;
import :Math;
import :Node;
import :RenderQueue;
import :Root;
import :SceneManager;
import :SceneNode;
import :SubMesh;
import :VertexIndexData;

import <algorithm>;
import <iterator>;
import <limits>;
import <memory>;
import <utility>;

namespace Ogre
{
    InstanceBatch::InstanceBatch( InstanceManager *creator, MeshPtr &meshReference,
                                    const MaterialPtr &material, size_t instancesPerBatch,
                                    const Mesh::IndexMap *indexToBoneMap, std::string_view batchName ) :
                Renderable(),
                MovableObject(),
                mInstancesPerBatch( instancesPerBatch ),
                mCreator( creator ),
                mMaterial( material ),
                mMeshReference( meshReference ),
                mIndexToBoneMap( indexToBoneMap ),
                
                mCameraDistLastUpdateFrameNumber( std::numeric_limits<unsigned long>::max() )
                
    {
        assert( mInstancesPerBatch );

        //Force batch visibility to be always visible. The instanced entities
        //have individual visibility flags. If none matches the scene's current,
        //then this batch won't rendered.
        mVisibilityFlags = static_cast<QueryTypeMask>(std::numeric_limits<Ogre::uint32>::max());

        if( indexToBoneMap )
        {
            assert( !(meshReference->hasSkeleton() && indexToBoneMap->empty()) );
        }

        mFullBoundingBox.setExtents( -Vector3::ZERO, Vector3::ZERO );

        mName = batchName;
		if (mCreator != nullptr)
		{
		    mCustomParams.resize( mCreator->getNumCustomParams() * mInstancesPerBatch, Ogre::Vector4::ZERO );
	    }
		
	}

    InstanceBatch::~InstanceBatch()
    {
        deleteAllInstancedEntities();

        //Remove the parent scene node automatically
        SceneNode *sceneNode = getParentSceneNode();
        if( sceneNode )
        {
            sceneNode->detachAllObjects();
            sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );
        }

        if( mRemoveOwnVertexData )
            delete mRenderOperation.vertexData;
        if( mRemoveOwnIndexData )
            delete mRenderOperation.indexData;

    }

    void InstanceBatch::_setInstancesPerBatch( size_t instancesPerBatch )
    {
        OgreAssert(mInstancedEntities.empty(),
                   "Instances per batch can only be changed before building the batch");
        mInstancesPerBatch = instancesPerBatch;
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::checkSubMeshCompatibility( const SubMesh* baseSubMesh ) -> bool
    {
        OgreAssert(baseSubMesh->operationType == RenderOperation::OperationType::TRIANGLE_LIST,
                   "Only meshes with OperationType::TRIANGLE_LIST are supported");

        if( !mCustomParams.empty() && mCreator->getInstancingTechnique() != InstanceManager::HWInstancingBasic )
        {
            //Implementing this for ShaderBased is impossible. All other variants can be.
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Custom parameters not supported for this "
                                                        "technique. Do you dare implementing it?"
                                                        "See InstanceManager::setNumCustomParams "
                                                        "documentation.",
                        "InstanceBatch::checkSubMeshCompatibility");
        }

        return true;
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::_updateBounds()
    {
        mFullBoundingBox.setNull();

        Real maxScale = 0;
        for(auto const& ent : mInstancedEntities)
        {
            //Only increase the bounding box for those objects we know are in the scene
            if( ent->isInScene() )
            {
                maxScale = std::max(maxScale, ent->getMaxScaleCoef());
                mFullBoundingBox.merge( ent->_getDerivedPosition() );
            }
        }

        Real addToBound = maxScale * _getMeshReference()->getBoundingSphereRadius();
        mFullBoundingBox.setMaximum(mFullBoundingBox.getMaximum() + addToBound);
        mFullBoundingBox.setMinimum(mFullBoundingBox.getMinimum() - addToBound);


        mBoundingRadius = Math::boundingRadiusFromAABBCentered( mFullBoundingBox );
        if (mParentNode) {
            mParentNode->needUpdate();
        }
	mBoundsUpdated  = true;
        mBoundsDirty    = false;
    }

    //-----------------------------------------------------------------------
    void InstanceBatch::updateVisibility()
    {
        mVisible = false;

        for(auto const& ent : mInstancedEntities)
        {
            //Trick to force Ogre not to render us if none of our instances is visible
            //Because we do Camera::isVisible(), it is better if the SceneNode from the
            //InstancedEntity is not part of the scene graph (i.e. ultimate parent is root node)
            //to avoid unnecessary wasteful calculations
            mVisible |= ent->findVisible( mCurrentCamera );
            if (mVisible)
                break;
        }
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::createAllInstancedEntities()
    {
        mInstancedEntities.reserve( mInstancesPerBatch );
        mUnusedEntities.reserve( mInstancesPerBatch );

        for( size_t i=0; i<mInstancesPerBatch; ++i )
        {
            InstancedEntity *instance = generateInstancedEntity(i);
            mInstancedEntities.emplace_back( instance );
            mUnusedEntities.push_back( instance );
        }
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::generateInstancedEntity(size_t num) -> InstancedEntity*
    {
        return new InstancedEntity(this, static_cast<uint32>(num));
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::deleteAllInstancedEntities()
    {
        for(auto const& ent : mInstancedEntities)
        {
            if( ent->getParentSceneNode() )
                ent->getParentSceneNode()->detachObject( ent.get() );
        }
        mInstancedEntities.clear();
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::makeMatrixCameraRelative3x4( Matrix3x4f *mat3x4, size_t count )
    {
        const Vector3 &cameraRelativePosition = mCurrentCamera->getDerivedPosition();

        for( size_t i=0; i<count; i++ )
        {
            mat3x4[i].setTrans(mat3x4[i].getTrans() - Vector<3, float>(cameraRelativePosition));
        }
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::build( const SubMesh* baseSubMesh ) -> RenderOperation
    {
        if( checkSubMeshCompatibility( baseSubMesh ) )
        {
            //Only triangle list at the moment
            mRenderOperation.operationType  = RenderOperation::OperationType::TRIANGLE_LIST;
            mRenderOperation.srcRenderable  = this;
            mRenderOperation.useIndexes = true;
            setupVertices( baseSubMesh );
            setupIndices( baseSubMesh );

            createAllInstancedEntities();
        }

        return mRenderOperation;
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::buildFrom( const SubMesh *baseSubMesh, const RenderOperation &renderOperation )
    {
        mRenderOperation = renderOperation;
        createAllInstancedEntities();
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::createInstancedEntity() -> InstancedEntity*
    {
        InstancedEntity *retVal = nullptr;

        if( !mUnusedEntities.empty() )
        {
            retVal = mUnusedEntities.back();
            mUnusedEntities.pop_back();

            retVal->setInUse(true);
        }

        return retVal;
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::removeInstancedEntity( InstancedEntity *instancedEntity )
    {
        OgreAssert(instancedEntity->mBatchOwner == this,
                   "Trying to remove an InstancedEntity from scene created with a different InstanceBatch");
        OgreAssert(instancedEntity->isInUse(), "Trying to remove an InstancedEntity that is already removed");

        if( instancedEntity->getParentSceneNode() )
            instancedEntity->getParentSceneNode()->detachObject( instancedEntity );

        instancedEntity->setInUse(false);
        instancedEntity->stopSharingTransform();

        //Put it back into the queue
        mUnusedEntities.push_back( instancedEntity );
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::transferInstancedEntitiesInUse( InstancedEntityVec &outEntities,
                                                    CustomParamsVec &outParams )
    {
        for(auto& ent : mInstancedEntities)
        {
            if( ent->isInUse() )
            {
                outEntities.emplace_back( ::std::move(ent) );

                for( unsigned char i=0; i<mCreator->getNumCustomParams(); ++i )
                    outParams.push_back( _getCustomParam( ent.get(), i ) );
            }
        }

        mInstancedEntities.clear();
        mUnusedEntities.clear();
        mCustomParams.clear();
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::defragmentBatchNoCull( InstancedEntityVec &usedEntities,
                                                CustomParamsVec &usedParams )
    {
        size_t const maxInstancesToCopy = std::min( mInstancesPerBatch, usedEntities.size() );
        auto const last = ::std::make_move_iterator(usedEntities.end());
        auto const first = last- maxInstancesToCopy;

        //Copy from the back to front, into m_instancedEntities
        mInstancedEntities.insert( mInstancedEntities.begin(), first, last );
        //Remove them from the array
        usedEntities.resize( usedEntities.size() - maxInstancesToCopy );

        size_t const maxParamsToCopy = maxInstancesToCopy * mCreator->getNumCustomParams();
        auto const lastParams = ::std::make_move_iterator(usedParams.end());
        auto const firstParams = lastParams - maxParamsToCopy;

        mCustomParams.insert( mCustomParams.begin(), firstParams, lastParams );
        usedParams.resize( usedParams.size() - maxParamsToCopy );
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::defragmentBatchDoCull( InstancedEntityVec &usedEntities,
                                                CustomParamsVec &usedParams )
    {
        //Get the the entity closest to the minimum bbox edge and put into "first"

        Vector3 vMinPos = Vector3::ZERO, firstPos = Vector3::ZERO;
        InstancedEntity *first = nullptr;

        if( !usedEntities.empty() )
        {
            first      = usedEntities.begin()->get();
            firstPos   = first->_getDerivedPosition();
            vMinPos      = first->_getDerivedPosition();
        }

        for (auto const& itor : usedEntities)
        {
            const Vector3 &vPos      = itor->_getDerivedPosition();

            vMinPos.x = std::min( vMinPos.x, vPos.x );
            vMinPos.y = std::min( vMinPos.y, vPos.y );
            vMinPos.z = std::min( vMinPos.z, vPos.z );

            if( vMinPos.squaredDistance( vPos ) < vMinPos.squaredDistance( firstPos ) )
            {
                firstPos   = vPos;
            }
        }

        //Now collect entities closest to 'first'
        while( !usedEntities.empty() && mInstancedEntities.size() < mInstancesPerBatch )
        {
            auto closest   = usedEntities.begin();
            auto it         = usedEntities.begin();
            auto e          = usedEntities.end();

            Vector3 closestPos;
            closestPos = (*closest)->_getDerivedPosition();

            while( it != e )
            {
                const Vector3 &vPos   = (*it)->_getDerivedPosition();

                if( firstPos.squaredDistance( vPos ) < firstPos.squaredDistance( closestPos ) )
                {
                    closest      = it;
                    closestPos   = vPos;
                }

                ++it;
            }

            mInstancedEntities.push_back( ::std::move(*closest) );
            //Now the custom params
            const size_t idx = closest - usedEntities.begin();
            for( unsigned char i=0; i<mCreator->getNumCustomParams(); ++i )
            {
                mCustomParams.push_back( usedParams[idx * mCreator->getNumCustomParams() + i] );
            }

            //Remove 'closest' from usedEntities & usedParams using swap and pop_back trick
            if  (closest != usedEntities.end() - 1)
            {
                // no self move assignment!
                *closest = ::std::move(usedEntities.back());

                for( unsigned char i=1; i<=mCreator->getNumCustomParams(); ++i )
                {
                    usedParams[(idx + 1) * mCreator->getNumCustomParams() - i] = *(usedParams.end() - i);
                }
            }
            usedEntities.pop_back();
            usedParams.resize(usedParams.size() - mCreator->getNumCustomParams());
        }
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::_defragmentBatch( bool optimizeCulling, InstanceBatch::InstancedEntityVec &usedEntities,
                                            CustomParamsVec &usedParams )
    {
        if( !optimizeCulling )
            defragmentBatchNoCull( usedEntities, usedParams );
        else
            defragmentBatchDoCull( usedEntities, usedParams );

        //Reassign instance IDs and tell we're the new parent
        uint32 instanceId = 0;

        for(auto& ent : mInstancedEntities)
        {
            ent->mInstanceId = instanceId++;
            ent->mBatchOwner = this;
        }

        //Recreate unused entities, if there's left space in our container
        assert( mInstancesPerBatch >= mInstancedEntities.size());
        mInstancedEntities.reserve( mInstancesPerBatch );
        mUnusedEntities.reserve( mInstancesPerBatch );
        mCustomParams.reserve( mCreator->getNumCustomParams() * mInstancesPerBatch );
        for( size_t i=mInstancedEntities.size(); i<mInstancesPerBatch; ++i )
        {
            InstancedEntity *instance = generateInstancedEntity(i);
            mInstancedEntities.emplace_back( instance );
            mUnusedEntities.push_back( instance );
            mCustomParams.push_back( Ogre::Vector4::ZERO );
        }

        //We've potentially changed our bounds
        if( !isBatchUnused() )
            _boundsDirty();
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::_boundsDirty()
    {
        if( mCreator && !mBoundsDirty ) 
            mCreator->_addDirtyBatch( this );
        mBoundsDirty = true;
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::getMovableType() const noexcept -> std::string_view
    {
        static std::string_view const constexpr sType = "InstanceBatch";
        return sType;
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::_notifyCurrentCamera( Camera* cam )
    {
        mCurrentCamera = cam;

        //See DistanceLodStrategy::getValueImpl()
        //We use our own because our SceneNode is just filled with zeroes, and updating it
        //with real values is expensive, plus we would need to make sure it doesn't get to
        //the shader
		Real depth = Math::Sqrt(getSquaredViewDepth(cam)) - getBoundingRadius();          
        depth = std::max( depth, Real(0) );

        Real lodValue = depth * cam->_getLodBiasInverse();

        //Now calculate Material LOD
        /*const LodStrategy *materialStrategy = m_material->getLodStrategy();
        
        //Calculate LOD value for given strategy
        Real lodValue = materialStrategy->getValue( this, cam );*/

        //Get the index at this depth
        unsigned short idx = mMaterial->getLodIndex( lodValue );

        //TODO: Replace subEntity for MovableObject
        // Construct event object
        /*EntityMaterialLodChangedEvent subEntEvt;
        subEntEvt.subEntity = this;
        subEntEvt.camera = cam;
        subEntEvt.lodValue = lodValue;
        subEntEvt.previousLodIndex = m_materialLodIndex;
        subEntEvt.newLodIndex = idx;

        //Notify LOD event listeners
        cam->getSceneManager()->_notifyEntityMaterialLodChanged(subEntEvt);*/

        // Change LOD index
        mMaterialLodIndex = idx;

        mBeyondFarDistance = false;

        if (cam->getUseRenderingDistance() && mUpperDistance > 0)
        {
            if (depth > mUpperDistance)
                mBeyondFarDistance = true;
        }

        if (!mBeyondFarDistance && cam->getUseMinPixelSize() && mMinPixelSize > 0)
        {
            Real pixelRatio = cam->getPixelDisplayRatio();

            Ogre::Vector3 objBound =
                getBoundingBox().getSize() * getParentNode()->_getDerivedScale();
            objBound.x = Math::Sqr(objBound.x);
            objBound.y = Math::Sqr(objBound.y);
            objBound.z = Math::Sqr(objBound.z);
            float sqrObjMedianSize = std::max(
                std::max(std::min(objBound.x, objBound.y), std::min(objBound.x, objBound.z)),
                std::min(objBound.y, objBound.z));

            // If we have a perspective camera calculations are done relative to distance
            Real sqrDistance = 1;

            if (cam->getProjectionType() == ProjectionType::PERSPECTIVE)
                sqrDistance = getSquaredViewDepth(cam->getLodCamera()); // it's ok

            mBeyondFarDistance =
                sqrObjMedianSize < sqrDistance * Math::Sqr(pixelRatio * mMinPixelSize);
        }

        if (mParentNode)
        {
            MovableObjectLodChangedEvent evt;
            evt.movableObject = this;
            evt.camera = cam;

            cam->getSceneManager()->_notifyMovableObjectLodChanged(evt);
        }

        mRenderingDisabled = mListener && !mListener->objectRendering(this, cam);

        // MovableObject::_notifyCurrentCamera( cam ); // it does not suit
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::getBoundingBox() const noexcept -> const AxisAlignedBox&
    {
        return mFullBoundingBox;
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::getBoundingRadius() const -> Real
    {
        return mBoundingRadius;
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::getSquaredViewDepth( const Camera* cam ) const -> Real
    {
        unsigned long currentFrameNumber = Root::getSingleton().getNextFrameNumber();

        if (mCameraDistLastUpdateFrameNumber != currentFrameNumber || mCachedCamera != cam)
        {
            mCachedCameraDist =
                getBoundingBox().getCenter().squaredDistance(cam->getDerivedPosition());

            mCachedCamera = cam;
            mCameraDistLastUpdateFrameNumber = currentFrameNumber;
        }

        return mCachedCameraDist;
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::getLights( ) const noexcept -> const LightList&
    {
        return queryLights();
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::getTechnique( ) const noexcept -> Technique*
    {
        return mMaterial->getBestTechnique( mMaterialLodIndex, this );
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::_updateRenderQueue( RenderQueue* queue )
    {
        /*if( m_boundsDirty )
            _updateBounds();*/

        mDirtyAnimation = false;

        //Is at least one object in the scene?
        updateVisibility();

        if( mVisible )
        {
            if( mMeshReference->hasSkeleton() )
            {
                for(auto& ent : mInstancedEntities)
                {
                    mDirtyAnimation |= ent->_updateAnimation();
                }
            }

            queue->addRenderable( this, mRenderQueueID, mRenderQueuePriority );
        }

        //Reset visibility once we skipped addRenderable (which saves GPU time), because OGRE for some
        //reason stops updating our render queue afterwards, preventing us to recalculate visibility
        mVisible = true;
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::visitRenderables( Renderable::Visitor* visitor, bool debugRenderables )
    {
        visitor->visit( this, 0, false );
    }
    //-----------------------------------------------------------------------
    void InstanceBatch::_setCustomParam( InstancedEntity *instancedEntity, unsigned char idx,
                                         const Vector4 &newParam )
    {
        mCustomParams[instancedEntity->mInstanceId * mCreator->getNumCustomParams() + idx] = newParam;
    }
    //-----------------------------------------------------------------------
    auto InstanceBatch::_getCustomParam( InstancedEntity *instancedEntity, unsigned char idx ) -> const Vector4&
    {
        return mCustomParams[instancedEntity->mInstanceId * mCreator->getNumCustomParams() + idx];
    }
}
