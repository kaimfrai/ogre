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

import :AnimationTrack;
import :Camera;
import :Entity;
import :HardwareVertexBuffer;
import :LogManager;
import :Material;
import :MaterialManager;
import :Math;
import :Matrix4;
import :Mesh;
import :Node;
import :RenderOperation;
import :SubEntity;
import :SubMesh;
import :Vector;
import :VertexIndexData;

import <algorithm>;
import <limits>;
import <span>;
import <string>;
import <vector>;

namespace Ogre {
    //-----------------------------------------------------------------------
    SubEntity::SubEntity (Entity* parent, SubMesh* subMeshBasis)
        : Renderable(), mParentEntity(parent),
        mSubMesh(subMeshBasis) 
    {
        mVisible = true;
        mRenderQueueID = {};
        mRenderQueueIDSet = false;
        mRenderQueuePrioritySet = false;
        mSkelAnimVertexData = nullptr;
        mVertexAnimationAppliedThisFrame = false;
        mHardwarePoseCount = 0;
        mIndexStart = 0;
        mIndexEnd = 0;
        setMaterial(MaterialManager::getSingleton().getDefaultMaterial());
    }
    SubEntity::~SubEntity() = default; // ensure unique_ptr destructors are in cpp
    //-----------------------------------------------------------------------
    auto SubEntity::getSubMesh() noexcept -> SubMesh*
    {
        return mSubMesh;
    }
    //-----------------------------------------------------------------------
    auto SubEntity::getMaterialName() const noexcept -> std::string_view
    {
        return mMaterialPtr ? mMaterialPtr->getName() : BLANKSTRING;
    }
    //-----------------------------------------------------------------------
    void SubEntity::setMaterialName( std::string_view name, std::string_view groupName /* = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME */)
    {
        MaterialPtr material = MaterialManager::getSingleton().getByName(name, groupName);

        if( !material )
        {
            LogManager::getSingleton().logError(
                ::std::format("Can't assign material '"
                    "' to SubEntity of '{}' because this "
                    "Material does not exist in group '{}'. Have you forgotten to define it in a {}"
                    ".material script?",
                    name, mParentEntity->getName(), groupName ));

            material = MaterialManager::getSingleton().getDefaultMaterial();
        }

        setMaterial( material );
    }
    //-----------------------------------------------------------------------
    void SubEntity::setMaterial( const MaterialPtr& material )
    {
        mMaterialPtr = material;
        
        if (!mMaterialPtr)
        {
            LogManager::getSingleton().logError(
                ::std::format("Can't assign nullptr material "
                "to SubEntity of '{}'. Falling back to default", mParentEntity->getName() ));
            
            mMaterialPtr = MaterialManager::getSingleton().getDefaultMaterial();
        }
        
        // Ensure new material loaded (will not load again if already loaded)
        mMaterialPtr->load();

        // tell parent to reconsider material vertex processing options
        mParentEntity->reevaluateVertexProcessing();
    }
    //-----------------------------------------------------------------------
    auto SubEntity::getTechnique() const noexcept -> Technique*
    {
        return mMaterialPtr->getBestTechnique(mMaterialLodIndex, this);
    }
    //-----------------------------------------------------------------------
    void SubEntity::getRenderOperation(RenderOperation& op)
    {
        // Use LOD
        mSubMesh->_getRenderOperation(op, mParentEntity->mMeshLodIndex);
        // Deal with any vertex data overrides
        op.vertexData = getVertexDataForBinding();

        // If we use custom index position the client is responsible to set meaningful values 
        if(mIndexStart != mIndexEnd)
        {
            op.indexData->indexStart = mIndexStart;
            op.indexData->indexCount = mIndexEnd;
        }
    }
    //-----------------------------------------------------------------------
    void SubEntity::setIndexDataStartIndex(size_t start_index)
    {
        if(start_index < mSubMesh->indexData->indexCount)
            mIndexStart = start_index;
    }
    //-----------------------------------------------------------------------
    auto SubEntity::getIndexDataStartIndex() const -> size_t
    {
        return mIndexStart;
    }
    //-----------------------------------------------------------------------
    void SubEntity::setIndexDataEndIndex(size_t end_index)
    {
        if(end_index > 0 && end_index <= mSubMesh->indexData->indexCount)
            mIndexEnd = end_index;
    }
    //-----------------------------------------------------------------------
    auto SubEntity::getIndexDataEndIndex() const -> size_t
    {
        return mIndexEnd;
    }
    //-----------------------------------------------------------------------
    void SubEntity::resetIndexDataStartEndIndex()
    {
        mIndexStart = 0;
        mIndexEnd = 0;
    }
    //-----------------------------------------------------------------------
    auto SubEntity::getVertexDataForBinding() noexcept -> VertexData*
    {
        if (mSubMesh->useSharedVertices)
        {
            return mParentEntity->getVertexDataForBinding();
        }
        else
        {
            Entity::VertexDataBindChoice c = 
                mParentEntity->chooseVertexDataForBinding(
                    mSubMesh->getVertexAnimationType() != VertexAnimationType::NONE);
            using enum Entity::VertexDataBindChoice;
            switch(c)
            {
            case ORIGINAL:
                return mSubMesh->vertexData.get();
            case HARDWARE_MORPH:
                return mHardwareVertexAnimVertexData.get();
            case SOFTWARE_MORPH:
                return mSoftwareVertexAnimVertexData.get();
            case SOFTWARE_SKELETAL:
                return mSkelAnimVertexData.get();
            };
            // keep compiler happy
            return mSubMesh->vertexData.get();

        }
    }
    //-----------------------------------------------------------------------
    void SubEntity::getWorldTransforms(Matrix4* xform) const
    {
        if (!mParentEntity->mNumBoneMatrices ||
            !mParentEntity->isHardwareAnimationEnabled())
        {
            // No skeletal animation, or software skinning
            *xform = mParentEntity->_getParentNodeFullTransform();
        }
        else
        {
            // Hardware skinning, pass all actually used matrices
            const Mesh::IndexMap& indexMap = mSubMesh->useSharedVertices ?
                mSubMesh->parent->sharedBlendIndexToBoneIndexMap : mSubMesh->blendIndexToBoneIndexMap;
            assert(indexMap.size() <= mParentEntity->mNumBoneMatrices);

            if (mParentEntity->_isSkeletonAnimated())
            {
                // Bones, use cached matrices built when Entity::_updateRenderQueue was called
                assert(mParentEntity->mBoneWorldMatrices);

                for (auto const& it : indexMap)
                {
                    *xform = mParentEntity->mBoneWorldMatrices[it];
                    ++xform;
                }
            }
            else
            {
                // All animations disabled, use parent entity world transform only
                std::ranges::fill(std::span{xform, indexMap.size()}, mParentEntity->_getParentNodeFullTransform());
            }
        }
    }
    //-----------------------------------------------------------------------
    auto SubEntity::getNumWorldTransforms() const noexcept -> unsigned short
    {
        if (!mParentEntity->mNumBoneMatrices ||
            !mParentEntity->isHardwareAnimationEnabled())
        {
            // No skeletal animation, or software skinning
            return 1;
        }
        else
        {
            // Hardware skinning, pass all actually used matrices
            const Mesh::IndexMap& indexMap = mSubMesh->useSharedVertices ?
                mSubMesh->parent->sharedBlendIndexToBoneIndexMap : mSubMesh->blendIndexToBoneIndexMap;
            assert(indexMap.size() <= mParentEntity->mNumBoneMatrices);

            return static_cast<unsigned short>(indexMap.size());
        }
    }
    //-----------------------------------------------------------------------
    auto SubEntity::getSquaredViewDepth(const Camera* cam) const -> Real
    {
        // First of all, check the cached value
        // NB this is manually invalidated by parent each _notifyCurrentCamera call
        // Done this here rather than there since we only need this for transparent objects
        if (mCachedCamera == cam)
            return mCachedCameraDist;

        Node* n = mParentEntity->getParentNode();
        assert(n);
        Real dist;
        if (!mSubMesh->extremityPoints.empty())
        {
            bool euclidean = cam->getSortMode() == SortMode::Distance;
            Vector3 zAxis = cam->getDerivedDirection();
            const Vector3 &cp = cam->getDerivedPosition();
            const Affine3 &l2w = mParentEntity->_getParentNodeFullTransform();
            dist = std::numeric_limits<Real>::infinity();
            for (const Vector3& v : mSubMesh->extremityPoints)
            {
                Vector3 diff = l2w * v - cp;
                Real d = euclidean ? diff.squaredLength() : Math::Sqr(zAxis.dotProduct(diff));

                dist = std::min(d, dist);
            }
        }
        else
            dist = n->getSquaredViewDepth(cam);

        mCachedCameraDist = dist;
        mCachedCamera = cam;

        return dist;
    }
    //-----------------------------------------------------------------------
    auto SubEntity::getLights() const noexcept -> const LightList&
    {
        return mParentEntity->queryLights();
    }
    //-----------------------------------------------------------------------
    void SubEntity::setVisible(bool visible)
    {
        mVisible = visible;
    }
    //-----------------------------------------------------------------------
    void SubEntity::prepareTempBlendBuffers()
    {
        if (mSubMesh->useSharedVertices)
            return;

        mSkelAnimVertexData.reset();
        mSoftwareVertexAnimVertexData.reset();
        mHardwareVertexAnimVertexData.reset();

        if (!mSubMesh->useSharedVertices)
        {
            if (mSubMesh->getVertexAnimationType() != VertexAnimationType::NONE)
            {
                // Create temporary vertex blend info
                // Prepare temp vertex data if needed
                // Clone without copying data, don't remove any blending info
                // (since if we skeletally animate too, we need it)
                mSoftwareVertexAnimVertexData.reset(mSubMesh->vertexData->clone(false));
                mParentEntity->extractTempBufferInfo(mSoftwareVertexAnimVertexData.get(), &mTempVertexAnimInfo);

                // Also clone for hardware usage, don't remove blend info since we'll
                // need it if we also hardware skeletally animate
                mHardwareVertexAnimVertexData.reset(mSubMesh->vertexData->clone(false));
            }

            if (mParentEntity->hasSkeleton())
            {
                // Create temporary vertex blend info
                // Prepare temp vertex data if needed
                // Clone without copying data, remove blending info
                // (since blend is performed in software)
                mSkelAnimVertexData.reset(
                    mParentEntity->cloneVertexDataRemoveBlendInfo(mSubMesh->vertexData.get()));
                mParentEntity->extractTempBufferInfo(mSkelAnimVertexData.get(), &mTempSkelAnimInfo);

            }
        }
    }
    //-----------------------------------------------------------------------
    auto SubEntity::getCastsShadows() const noexcept -> bool
    {
        return mParentEntity->getCastShadows();
    }
    //-----------------------------------------------------------------------
    auto SubEntity::_getSkelAnimVertexData() -> VertexData* 
    {
        assert (mSkelAnimVertexData && "Not software skinned or has no dedicated geometry!");
        return mSkelAnimVertexData.get();
    }
    //-----------------------------------------------------------------------
    auto SubEntity::_getSoftwareVertexAnimVertexData() -> VertexData*
    {
        assert (mSoftwareVertexAnimVertexData && "Not vertex animated or has no dedicated geometry!");
        return mSoftwareVertexAnimVertexData.get();
    }
    //-----------------------------------------------------------------------
    auto SubEntity::_getHardwareVertexAnimVertexData() -> VertexData*
    {
        assert (mHardwareVertexAnimVertexData && "Not vertex animated or has no dedicated geometry!");
        return mHardwareVertexAnimVertexData.get();
    }
    //-----------------------------------------------------------------------
    auto SubEntity::_getSkelAnimTempBufferInfo() -> TempBlendedBufferInfo* 
    {
        return &mTempSkelAnimInfo;
    }
    //-----------------------------------------------------------------------
    auto SubEntity::_getVertexAnimTempBufferInfo() -> TempBlendedBufferInfo* 
    {
        return &mTempVertexAnimInfo;
    }
    //-----------------------------------------------------------------------
    void SubEntity::_updateCustomGpuParameter(
        const GpuProgramParameters::AutoConstantEntry& constantEntry,
        GpuProgramParameters* params) const
    {
        if (constantEntry.paramType == GpuProgramParameters::AutoConstantType::ANIMATION_PARAMETRIC)
        {
            // Set up to 4 values, or up to limit of hardware animation entries
            // Pack into 4-element constants offset based on constant data index
            // If there are more than 4 entries, this will be called more than once
            Vector4 val{0.0f,0.0f,0.0f,0.0f};
            const auto& vd = mHardwareVertexAnimVertexData ? mHardwareVertexAnimVertexData : mParentEntity->mHardwareVertexAnimVertexData;
            
            size_t animIndex = constantEntry.data * 4;
            for (size_t i = 0; i < 4 && 
                animIndex < vd->hwAnimationDataList.size();
                ++i, ++animIndex)
            {
                val[i] = 
                    vd->hwAnimationDataList[animIndex].parametric;
            }
            // set the parametric morph value
            params->_writeRawConstant(constantEntry.physicalIndex, val);
        }
        else
        {
            // default
            return Renderable::_updateCustomGpuParameter(constantEntry, params);
        }
    }
    //-----------------------------------------------------------------------------
    void SubEntity::_markBuffersUnusedForAnimation()
    {
        mVertexAnimationAppliedThisFrame = false;
    }
    //-----------------------------------------------------------------------------
    void SubEntity::_markBuffersUsedForAnimation()
    {
        mVertexAnimationAppliedThisFrame = true;
    }
    //-----------------------------------------------------------------------------
    void SubEntity::_restoreBuffersForUnusedAnimation(bool hardwareAnimation)
    {
        // Rebind original positions if:
        //  We didn't apply any animation and 
        //    We're morph animated (hardware binds keyframe, software is missing)
        //    or we're pose animated and software (hardware is fine, still bound)
        if (mSubMesh->getVertexAnimationType() != VertexAnimationType::NONE && 
            !mSubMesh->useSharedVertices && 
            !mVertexAnimationAppliedThisFrame &&
            (!hardwareAnimation || mSubMesh->getVertexAnimationType() == VertexAnimationType::MORPH))
        {
            // Note, VertexElementSemantic::POSITION is specified here but if normals are included in animation
            // then these will be re-bound too (buffers must be shared)
            const VertexElement* srcPosElem = 
                mSubMesh->vertexData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::POSITION);
            HardwareVertexBufferSharedPtr srcBuf = 
                mSubMesh->vertexData->vertexBufferBinding->getBuffer(
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
        if (!mSubMesh->useSharedVertices && hardwareAnimation 
            && mSubMesh->getVertexAnimationType() == VertexAnimationType::POSE)
        {
            mParentEntity->bindMissingHardwarePoseBuffers(
                mSubMesh->vertexData.get(), mHardwareVertexAnimVertexData.get());
        }

    }
    //-----------------------------------------------------------------------
    void SubEntity::setRenderQueueGroup(RenderQueueGroupID queueID)
    {
        mRenderQueueIDSet = true;
        mRenderQueueID = queueID;
    }
    //-----------------------------------------------------------------------
    void SubEntity::setRenderQueueGroupAndPriority(RenderQueueGroupID queueID, ushort priority)
    {
        setRenderQueueGroup(queueID);
        mRenderQueuePrioritySet = true;
        mRenderQueuePriority = priority;
    }
    //-----------------------------------------------------------------------
}
