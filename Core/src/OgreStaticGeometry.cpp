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
#include <cstring>
#include <memory>
#include <ostream>
#include <utility>

#include "OgreCamera.hpp"
#include "OgreEdgeListBuilder.hpp"
#include "OgreEntity.hpp"
#include "OgreException.hpp"
#include "OgreHardwareBuffer.hpp"
#include "OgreHardwareBufferManager.hpp"
#include "OgreHardwareIndexBuffer.hpp"
#include "OgreHardwareVertexBuffer.hpp"
#include "OgreLight.hpp"
#include "OgreLodStrategy.hpp"
#include "OgreLogManager.hpp"
#include "OgreMath.hpp"
#include "OgreMatrix3.hpp"
#include "OgreMatrix4.hpp"
#include "OgreNode.hpp"
#include "OgrePass.hpp"
#include "OgreRenderOperation.hpp"
#include "OgreRenderQueue.hpp"
#include "OgreRoot.hpp"
#include "OgreSceneManager.hpp"
#include "OgreSceneNode.hpp"
#include "OgreStaticGeometry.hpp"
#include "OgreSubEntity.hpp"
#include "OgreSubMesh.hpp"
#include "OgreTechnique.hpp"
#include "OgreVertexIndexData.hpp"

namespace Ogre {

    #define REGION_RANGE 1024
    #define REGION_HALF_RANGE 512
    #define REGION_MAX_INDEX 511
    #define REGION_MIN_INDEX -512

    //--------------------------------------------------------------------------
    StaticGeometry::StaticGeometry(SceneManager* owner, std::string_view name):
        mOwner(owner),
        mName(name),
        
        mRegionDimensions(Vector3(1000,1000,1000)),
        mHalfRegionDimensions(Vector3(500,500,500)),
        mOrigin(Vector3(0,0,0)),
        
        mVisibilityFlags(Ogre::MovableObject::getDefaultVisibilityFlags())
    {
    }
    //--------------------------------------------------------------------------
    StaticGeometry::~StaticGeometry()
    {
        reset();
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::getRegion(const AxisAlignedBox& bounds,
        bool autoCreate) -> StaticGeometry::Region*
    {
        if (bounds.isNull())
            return nullptr;

        // Get the region which has the largest overlapping volume
        const Vector3 min = bounds.getMinimum();
        const Vector3 max = bounds.getMaximum();

        // Get the min and max region indexes
        ushort minx, miny, minz;
        ushort maxx, maxy, maxz;
        getRegionIndexes(min, minx, miny, minz);
        getRegionIndexes(max, maxx, maxy, maxz);
        Real maxVolume = 0.0f;
        ushort finalx = 0, finaly = 0, finalz = 0;
        for (ushort x = minx; x <= maxx; ++x)
        {
            for (ushort y = miny; y <= maxy; ++y)
            {
                for (ushort z = minz; z <= maxz; ++z)
                {
                    Real vol = getVolumeIntersection(bounds, x, y, z);
                    if (vol > maxVolume)
                    {
                        maxVolume = vol;
                        finalx = x;
                        finaly = y;
                        finalz = z;
                    }

                }
            }
        }

        assert(maxVolume > 0.0f &&
            "Static geometry: Problem determining closest volume match!");

        return getRegion(finalx, finaly, finalz, autoCreate);

    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::getVolumeIntersection(const AxisAlignedBox& box,
        ushort x, ushort y, ushort z) -> Real
    {
        // Get bounds of indexed region
        AxisAlignedBox regionBounds = getRegionBounds(x, y, z);
        AxisAlignedBox intersectBox = regionBounds.intersection(box);
        // return a 'volume' which ignores zero dimensions
        // since we only use this for relative comparisons of the same bounds
        // this will still be internally consistent
        Vector3 boxdiff = box.getMaximum() - box.getMinimum();
        Vector3 intersectDiff = intersectBox.getMaximum() - intersectBox.getMinimum();

        return (boxdiff.x == 0 ? 1 : intersectDiff.x) *
            (boxdiff.y == 0 ? 1 : intersectDiff.y) *
            (boxdiff.z == 0 ? 1 : intersectDiff.z);

    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::getRegionBounds(ushort x, ushort y, ushort z) -> AxisAlignedBox
    {
        Vector3 min(
            ((Real)x - REGION_HALF_RANGE) * mRegionDimensions.x + mOrigin.x,
            ((Real)y - REGION_HALF_RANGE) * mRegionDimensions.y + mOrigin.y,
            ((Real)z - REGION_HALF_RANGE) * mRegionDimensions.z + mOrigin.z
            );
        Vector3 max = min + mRegionDimensions;
        return { min, max };
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::getRegionCentre(ushort x, ushort y, ushort z) -> Vector3
    {
        return {
            ((Real)x - REGION_HALF_RANGE) * mRegionDimensions.x + mOrigin.x
                + mHalfRegionDimensions.x,
            ((Real)y - REGION_HALF_RANGE) * mRegionDimensions.y + mOrigin.y
                + mHalfRegionDimensions.y,
            ((Real)z - REGION_HALF_RANGE) * mRegionDimensions.z + mOrigin.z
                + mHalfRegionDimensions.z
            };
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::getRegion(
            ushort x, ushort y, ushort z, bool autoCreate) -> StaticGeometry::Region*
    {
        uint32 index = packIndex(x, y, z);
        Region* ret = getRegion(index);
        if (!ret && autoCreate)
        {
            // Make a name
            StringStream str;
            str << mName << ":" << index;
            // Calculate the region centre
            Vector3 centre = getRegionCentre(x, y, z);
            ret = new Region(this, str.str(), mOwner, index, centre);
            mOwner->injectMovableObject(ret);
            ret->setVisible(mVisible);
            ret->setCastShadows(mCastShadows);
            if (mRenderQueueIDSet)
            {
                ret->setRenderQueueGroup(mRenderQueueID);
            }
            mRegionMap[index] = ret;
        }
        return ret;
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::getRegion(uint32 index) -> StaticGeometry::Region*
    {
        auto i = mRegionMap.find(index);
        if (i != mRegionMap.end())
        {
            return i->second;
        }
        else
        {
            return nullptr;
        }

    }
    //--------------------------------------------------------------------------
    void StaticGeometry::getRegionIndexes(const Vector3& point,
        ushort& x, ushort& y, ushort& z)
    {
        // Scale the point into multiples of region and adjust for origin
        Vector3 scaledPoint = (point - mOrigin) / mRegionDimensions;

        // Round down to 'bottom left' point which represents the cell index
        int ix = Math::IFloor(scaledPoint.x);
        int iy = Math::IFloor(scaledPoint.y);
        int iz = Math::IFloor(scaledPoint.z);

        // Check bounds
        if (ix < REGION_MIN_INDEX || ix > REGION_MAX_INDEX
            || iy < REGION_MIN_INDEX || iy > REGION_MAX_INDEX
            || iz < REGION_MIN_INDEX || iz > REGION_MAX_INDEX)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                "Point out of bounds",
                "StaticGeometry::getRegionIndexes");
        }
        // Adjust for the fact that we use unsigned values for simplicity
        // (requires less faffing about for negatives give 10-bit packing
        x = static_cast<ushort>(ix + REGION_HALF_RANGE);
        y = static_cast<ushort>(iy + REGION_HALF_RANGE);
        z = static_cast<ushort>(iz + REGION_HALF_RANGE);


    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::packIndex(ushort x, ushort y, ushort z) -> uint32
    {
        return x + (y << 10) + (z << 20);
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::getRegion(const Vector3& point,
        bool autoCreate) -> StaticGeometry::Region*
    {
        ushort x, y, z;
        getRegionIndexes(point, x, y, z);
        return getRegion(x, y, z, autoCreate);
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::calculateBounds(VertexData* vertexData,
        const Vector3& position, const Quaternion& orientation,
        const Vector3& scale) -> AxisAlignedBox
    {
        const VertexElement* posElem =
            vertexData->vertexDeclaration->findElementBySemantic(
                VertexElementSemantic::POSITION);
        HardwareVertexBufferSharedPtr vbuf =
            vertexData->vertexBufferBinding->getBuffer(posElem->getSource());
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::READ_ONLY);
        auto* vertex = static_cast<unsigned char*>(vbufLock.pData);
        float* pFloat;

        Vector3 min = Vector3::ZERO, max = Vector3::UNIT_SCALE;
        bool first = true;

        for(size_t j = 0; j < vertexData->vertexCount; ++j, vertex += vbuf->getVertexSize())
        {
            posElem->baseVertexPointerToElement(vertex, &pFloat);

            Vector3 pt;

            pt.x = (*pFloat++);
            pt.y = (*pFloat++);
            pt.z = (*pFloat++);
            // Transform to world (scale, rotate, translate)
            pt = (orientation * (pt * scale)) + position;
            if (first)
            {
                min = max = pt;
                first = false;
            }
            else
            {
                min.makeFloor(pt);
                max.makeCeil(pt);
            }

        }
        return { min, max };
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::addEntity(Entity* ent, const Vector3& position,
        const Quaternion& orientation, const Vector3& scale)
    {
        const MeshPtr& msh = ent->getMesh();
        // Validate
        if (msh->hasManualLodLevel())
        {
            LogManager::getSingleton().logWarning(std::format("(StaticGeometry): Manual LOD is not supported. "
                                                  "Using only highest LOD level for mesh {}",
                                                  msh->getName()));
        }

        AxisAlignedBox sharedWorldBounds;
        // queue this entities submeshes and choice of material
        // also build the lists of geometry to be used for the source of lods
        for (uint i = 0; i < ent->getNumSubEntities(); ++i)
        {
            SubEntity* se = ent->getSubEntity(i);
            auto* q = new QueuedSubMesh();

            // Get the geometry for this SubMesh
            q->submesh = se->getSubMesh();
            q->material = se->getMaterial();
            q->geometryLodList = determineGeometry(q->submesh);
            q->orientation = orientation;
            q->position = position;
            q->scale = scale;
            // Determine the bounds based on the highest LOD
            q->worldBounds = calculateBounds(
                (*q->geometryLodList)[0].vertexData,
                    position, orientation, scale);

            mQueuedSubMeshes.push_back(q);
        }
    }
    //--------------------------------------------------------------------------
    auto
    StaticGeometry::determineGeometry(SubMesh* sm) -> StaticGeometry::SubMeshLodGeometryLinkList*
    {
        OgreAssert(sm->indexData->indexBuffer, "currently only works with indexed geometry");
        // First, determine if we've already seen this submesh before
        auto i =
            mSubMeshGeometryLookup.find(sm);
        if (i != mSubMeshGeometryLookup.end())
        {
            return i->second;
        }
        // Otherwise, we have to create a new one
        auto* lodList = new SubMeshLodGeometryLinkList();
        mSubMeshGeometryLookup[sm] = lodList;
        ushort numLods = sm->parent->hasManualLodLevel() ? 1 :
            sm->parent->getNumLodLevels();
        lodList->resize(numLods);
        for (ushort lod = 0; lod < numLods; ++lod)
        {
            SubMeshLodGeometryLink& geomLink = (*lodList)[lod];
            IndexData *lodIndexData;
            if (lod == 0)
            {
                lodIndexData = sm->indexData.get();
            }
            else
            {
                lodIndexData = sm->mLodFaceList[lod - 1];
            }
            // Can use the original mesh geometry?
            if (sm->useSharedVertices)
            {
                if (sm->parent->getNumSubMeshes() == 1)
                {
                    // Ok, this is actually our own anyway
                    geomLink.vertexData = sm->parent->sharedVertexData;
                    geomLink.indexData = lodIndexData;
                }
                else
                {
                    // We have to split it
                    splitGeometry(sm->parent->sharedVertexData,
                        lodIndexData, &geomLink);
                }
            }
            else
            {
                if (lod == 0)
                {
                    // Ok, we can use the existing geometry; should be in full
                    // use by just this SubMesh
                    geomLink.vertexData = sm->vertexData.get();
                    geomLink.indexData = sm->indexData.get();
                }
                else
                {
                    // We have to split it
                    splitGeometry(sm->vertexData.get(),
                        lodIndexData, &geomLink);
                }
            }
            assert (geomLink.vertexData->vertexStart == 0 &&
                "Cannot use vertexStart > 0 on indexed geometry due to "
                "rendersystem incompatibilities - see the docs!");
        }


        return lodList;
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::splitGeometry(VertexData* vd, IndexData* id,
            StaticGeometry::SubMeshLodGeometryLink* targetGeomLink)
    {
        // Firstly we need to scan to see how many vertices are being used
        // and while we're at it, build the remap we can use later
        bool use32bitIndexes =
            id->indexBuffer->getType() == HardwareIndexBuffer::IndexType::_32BIT;
        IndexRemap indexRemap;
        HardwareBufferLockGuard indexLock(id->indexBuffer,
                                          id->indexStart * id->indexBuffer->getIndexSize(), 
                                          id->indexCount * id->indexBuffer->getIndexSize(), 
                                          HardwareBuffer::LockOptions::READ_ONLY);
        if (use32bitIndexes)
        {
            buildIndexRemap(static_cast<uint32*>(indexLock.pData), id->indexCount, indexRemap);
        }
        else
        {
            buildIndexRemap(static_cast<uint16*>(indexLock.pData), id->indexCount, indexRemap);
        }
        indexLock.unlock();
        if (indexRemap.size() == vd->vertexCount)
        {
            // ha, complete usage after all
            targetGeomLink->vertexData = vd;
            targetGeomLink->indexData = id;
            return;
        }


        // Create the new vertex data records
        targetGeomLink->vertexData = vd->clone(false);
        // Convenience
        VertexData* newvd = targetGeomLink->vertexData;
        //IndexData* newid = targetGeomLink->indexData;
        // Update the vertex count
        newvd->vertexCount = indexRemap.size();

        size_t numvbufs = vd->vertexBufferBinding->getBufferCount();
        // Copy buffers from old to new
        for (unsigned short b = 0; b < numvbufs; ++b)
        {
            // Lock old buffer
            HardwareVertexBufferSharedPtr oldBuf =
                vd->vertexBufferBinding->getBuffer(b);
            // Create new buffer
            HardwareVertexBufferSharedPtr newBuf =
                HardwareBufferManager::getSingleton().createVertexBuffer(
                    oldBuf->getVertexSize(),
                    indexRemap.size(),
                    HardwareBuffer::STATIC);
            // rebind
            newvd->vertexBufferBinding->setBinding(b, newBuf);

            // Copy all the elements of the buffer across, by iterating over
            // the IndexRemap which describes how to move the old vertices
            // to the new ones. By nature of the map the remap is in order of
            // indexes in the old buffer, but note that we're not guaranteed to
            // address every vertex (which is kinda why we're here)
            HardwareBufferLockGuard oldBufLock(oldBuf, HardwareBuffer::LockOptions::READ_ONLY);
            HardwareBufferLockGuard newBufLock(newBuf, HardwareBuffer::LockOptions::DISCARD);
            size_t vertexSize = oldBuf->getVertexSize();
            // Buffers should be the same size
            assert (vertexSize == newBuf->getVertexSize());

            for (auto& r : indexRemap)
            {
                assert (r.first < oldBuf->getNumVertices());
                assert (r.second < newBuf->getNumVertices());

                uchar* pSrc = static_cast<uchar*>(oldBufLock.pData) + r.first * vertexSize;
                uchar* pDst = static_cast<uchar*>(newBufLock.pData) + r.second * vertexSize;
                memcpy(pDst, pSrc, vertexSize);
            }
        }

        // Now create a new index buffer
        HardwareIndexBufferSharedPtr ibuf =
            HardwareBufferManager::getSingleton().createIndexBuffer(
                id->indexBuffer->getType(), id->indexCount,
                HardwareBuffer::STATIC);

        HardwareBufferLockGuard srcIndexLock(id->indexBuffer,
                                             id->indexStart * id->indexBuffer->getIndexSize(), 
                                             id->indexCount * id->indexBuffer->getIndexSize(), 
                                             HardwareBuffer::LockOptions::READ_ONLY);
        HardwareBufferLockGuard dstIndexLock(ibuf, HardwareBuffer::LockOptions::DISCARD);
        if (use32bitIndexes)
        {
            auto *pSrc32 = static_cast<uint32*>(srcIndexLock.pData);
            auto *pDst32 = static_cast<uint32*>(dstIndexLock.pData);
            remapIndexes(pSrc32, pDst32, indexRemap, id->indexCount);
        }
        else
        {
            auto *pSrc16 = static_cast<uint16*>(srcIndexLock.pData);
            auto *pDst16 = static_cast<uint16*>(dstIndexLock.pData);
            remapIndexes(pSrc16, pDst16, indexRemap, id->indexCount);
        }
        srcIndexLock.unlock();
        dstIndexLock.unlock();

        targetGeomLink->indexData = new IndexData();
        targetGeomLink->indexData->indexStart = 0;
        targetGeomLink->indexData->indexCount = id->indexCount;
        targetGeomLink->indexData->indexBuffer = ibuf;

        // Store optimised geometry for deallocation later
        auto *optGeom = new OptimisedSubMeshGeometry();
        optGeom->indexData.reset(targetGeomLink->indexData);
        optGeom->vertexData.reset(targetGeomLink->vertexData);
        mOptimisedSubMeshGeometryList.push_back(optGeom);
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::addSceneNode(const SceneNode* node)
    {
        for (auto mobj : node->getAttachedObjects())
        {
            if (mobj->getMovableType() == "Entity")
            {
                addEntity(static_cast<Entity*>(mobj),
                    node->_getDerivedPosition(),
                    node->_getDerivedOrientation(),
                    node->_getDerivedScale());
            }
        }
        // Iterate through all the child-nodes
        for (auto c : node->getChildren())
        {
            // Add this subnode and its children...
            addSceneNode( static_cast<const SceneNode*>(c) );
        }
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::build()
    {
        // Make sure there's nothing from previous builds
        destroy();

        // Firstly allocate meshes to regions
        for (auto qsm : mQueuedSubMeshes)
        {
            Region* region = getRegion(qsm->worldBounds, true);
            region->assign(qsm);
        }
        bool stencilShadows = false;
        if (mCastShadows && mOwner->isShadowTechniqueStencilBased())
        {
            stencilShadows = true;
        }

        // Now tell each region to build itself
        for (auto & ri : mRegionMap)
        {
            ri.second->build(stencilShadows);
            
            // Set the visibility flags on these regions
            ri.second->setVisibilityFlags(mVisibilityFlags);
        }

    }
    //--------------------------------------------------------------------------
    void StaticGeometry::destroy()
    {
        // delete the regions
        for (auto & i : mRegionMap)
        {
            mOwner->extractMovableObject(i.second);
            delete i.second;
        }
        mRegionMap.clear();
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::reset()
    {
        destroy();
        for (auto & mQueuedSubMeshe : mQueuedSubMeshes)
        {
            delete mQueuedSubMeshe;
        }
        mQueuedSubMeshes.clear();
        // Delete precached geoemtry lists
        for (auto & l : mSubMeshGeometryLookup)
        {
            delete l.second;
        }
        mSubMeshGeometryLookup.clear();
        // Delete optimised geometry
        for (auto & o : mOptimisedSubMeshGeometryList)
        {
            delete o;
        }
        mOptimisedSubMeshGeometryList.clear();

    }
    //--------------------------------------------------------------------------
    void StaticGeometry::setVisible(bool visible)
    {
        mVisible = visible;
        // tell any existing regions
        for (auto & ri : mRegionMap)
        {
            ri.second->setVisible(visible);
        }
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::setCastShadows(bool castShadows)
    {
        mCastShadows = castShadows;
        // tell any existing regions
        for (auto & ri : mRegionMap)
        {
            ri.second->setCastShadows(castShadows);
        }

    }
    //--------------------------------------------------------------------------
    void StaticGeometry::setRenderQueueGroup(RenderQueueGroupID queueID)
    {
        assert(queueID <= RenderQueueGroupID::MAX && "Render queue out of range!");
        mRenderQueueIDSet = true;
        mRenderQueueID = queueID;
        // tell any existing regions
        for (auto & ri : mRegionMap)
        {
            ri.second->setRenderQueueGroup(queueID);
        }
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::getRenderQueueGroup() const noexcept -> RenderQueueGroupID
    {
        return mRenderQueueID;
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::setVisibilityFlags(QueryTypeMask flags)
    {
        mVisibilityFlags = flags;
        for (auto & ri : mRegionMap)
        {
            ri.second->setVisibilityFlags(flags);
        }
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::getVisibilityFlags() const noexcept -> QueryTypeMask
    {
        if(mRegionMap.empty())
            return MovableObject::getDefaultVisibilityFlags();

        auto ri = mRegionMap.begin();
        return ri->second->getVisibilityFlags();
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::dump(std::string_view filename) const
    {
        std::ofstream of(filename.data());
        of << "Static Geometry Report for " << mName << std::endl;
        of << "-------------------------------------------------" << std::endl;
        of << "Number of queued submeshes: " << mQueuedSubMeshes.size() << std::endl;
        of << "Number of regions: " << mRegionMap.size() << std::endl;
        of << "Region dimensions: " << mRegionDimensions << std::endl;
        of << "Origin: " << mOrigin << std::endl;
        of << "Max distance: " << mUpperDistance << std::endl;
        of << "Casts shadows?: " << mCastShadows << std::endl;
        of << std::endl;
        for (auto ri : mRegionMap)
        {
            ri.second->dump(of);
        }
        of << "-------------------------------------------------" << std::endl;
    }
    //---------------------------------------------------------------------
    void StaticGeometry::visitRenderables(Renderable::Visitor* visitor, 
        bool debugRenderables)
    {
        for (auto & ri : mRegionMap)
        {
            ri.second->visitRenderables(visitor, debugRenderables);
        }

    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    StaticGeometry::Region::Region(StaticGeometry* parent, std::string_view name,
        SceneManager* mgr, uint32 regionID, const Vector3& centre)
        : MovableObject(name), mParent(parent),
        mRegionID(regionID), mCentre(centre) 
    {
        mManager = mgr;
    }
    //--------------------------------------------------------------------------
    StaticGeometry::Region::~Region()
    {
        if (mParentNode)
        {
            mManager->destroySceneNode(static_cast<SceneNode*>(mParentNode));
            mParentNode = nullptr;
        }

        // no need to delete queued meshes, these are managed in StaticGeometry

    }
    //-----------------------------------------------------------------------
    void StaticGeometry::Region::_releaseManualHardwareResources()
    {
        for (auto & i : mLodBucketList)
        {
            clearShadowRenderableList(i->getShadowRenderableList());
        }
    }
    //-----------------------------------------------------------------------
    void StaticGeometry::Region::_restoreManualHardwareResources()
    {
        // shadow renderables are lazy initialized
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::Region::getTypeFlags() const noexcept -> QueryTypeMask
    {
        return QueryTypeMask::STATICGEOMETRY;
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::Region::assign(QueuedSubMesh* qmesh)
    {
        mQueuedSubMeshes.push_back(qmesh);

        // Set/check LOD strategy
        const LodStrategy *lodStrategy = qmesh->submesh->parent->getLodStrategy();
        if (mLodStrategy == nullptr)
        {
            mLodStrategy = lodStrategy;

            // First LOD mandatory, and always from base LOD value
            mLodValues.push_back(mLodStrategy->getBaseValue());
        }
        else
        {
            if (mLodStrategy != lodStrategy)
                OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Lod strategies do not match",
                    "StaticGeometry::Region::assign");
        }

        // update LOD values
        ushort lodLevels = qmesh->submesh->parent->getNumLodLevels();
        assert(qmesh->geometryLodList->size() == lodLevels);

        while(mLodValues.size() < lodLevels)
        {
            mLodValues.push_back(0.0f);
        }
        // Make sure LOD levels are max of all at the requested level
        for (ushort lod = 1; lod < lodLevels; ++lod)
        {
            const MeshLodUsage& meshLod =
                qmesh->submesh->parent->getLodLevel(lod);
            mLodValues[lod] = std::max(mLodValues[lod],
                meshLod.value);
        }

        // update bounds
        // Transform world bounds relative to our centre
        AxisAlignedBox localBounds(
            qmesh->worldBounds.getMinimum() - mCentre,
            qmesh->worldBounds.getMaximum() - mCentre);
        mAABB.merge(localBounds);
        mBoundingRadius = Math::boundingRadiusFromAABB(mAABB);

    }
    //--------------------------------------------------------------------------
    void StaticGeometry::Region::build(bool stencilShadows)
    {
        // Create a node
        mManager->getRootSceneNode()->createChildSceneNode(mCentre)->attachObject(this);
        // We need to create enough LOD buckets to deal with the highest LOD
        // we encountered in all the meshes queued
        for (ushort lod = 0; lod < mLodValues.size(); ++lod)
        {
            mLodBucketList.push_back(::std::make_unique<LODBucket>(this, lod, mLodValues[lod]));
            auto* lodBucket = mLodBucketList.back().get();
            // Now iterate over the meshes and assign to LODs
            // LOD bucket will pick the right LOD to use
            for (auto & mQueuedSubMeshe : mQueuedSubMeshes)
            {
                lodBucket->assign(mQueuedSubMeshe, lod);
            }
            // now build
            lodBucket->build(stencilShadows);
        }



    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::Region::getMovableType() const noexcept -> std::string_view
    {
        static String sType = "StaticGeometry";
        return sType;
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::Region::_notifyCurrentCamera(Camera* cam)
    {
        // Set camera
        mCamera = cam;

        // Cache squared view depth for use by GeometryBucket
        mSquaredViewDepth = mParentNode->getSquaredViewDepth(cam->getLodCamera());

        // No LOD strategy set yet, skip (this indicates that there are no submeshes)
        if (mLodStrategy == nullptr)
            return;

        // Sanity check
        assert(!mLodValues.empty());

        // Calculate LOD value
        Real lodValue = mLodStrategy->getValue(this, cam);

        // Store LOD value for this strategy
        mLodValue = lodValue;

        // Get LOD index
        mCurrentLod = mLodStrategy->getIndex(lodValue, mLodValues);
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::Region::getBoundingBox() const noexcept -> const AxisAlignedBox&
    {
        return mAABB;
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::Region::getBoundingRadius() const -> Real
    {
        return mBoundingRadius;
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::Region::_updateRenderQueue(RenderQueue* queue)
    {
        mLodBucketList[mCurrentLod]->addRenderables(queue, mRenderQueueID,
            mLodValue);
    }
    //---------------------------------------------------------------------
    void StaticGeometry::Region::visitRenderables(Renderable::Visitor* visitor, 
        bool debugRenderables)
    {
        for (auto & i : mLodBucketList)
        {
            i->visitRenderables(visitor, debugRenderables);
        }

    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::Region::isVisible() const noexcept -> bool
    {
        if(!mVisible || mBeyondFarDistance)
            return false;

        SceneManager* sm = Root::getSingleton()._getCurrentSceneManager();
        if (sm && !(mVisibilityFlags & sm->_getCombinedVisibilityMask()))
            return false;

        return true;
    }

    //---------------------------------------------------------------------
    auto StaticGeometry::Region::getShadowVolumeRenderableList(
        const Light* light, const HardwareIndexBufferPtr& indexBuffer, size_t& indexBufferUsedSize,
        float extrusionDistance, ShadowRenderableFlags flags) -> const ShadowRenderableList&
    {
        // Calculate the object space light details
        Vector4 lightPos = light->getAs4DVector();
        Affine3 world2Obj = mParentNode->_getFullTransform().inverse();
        lightPos = world2Obj * lightPos;
        Matrix3 world2Obj3x3 = world2Obj.linear();
        extrusionDistance *= Math::Sqrt(std::min(std::min(world2Obj3x3.GetColumn(0).squaredLength(), world2Obj3x3.GetColumn(1).squaredLength()), world2Obj3x3.GetColumn(2).squaredLength()));

        // per-LOD shadow lists & edge data
        mLodBucketList[mCurrentLod]->updateShadowRenderables(lightPos, indexBuffer,
                                                             extrusionDistance, flags);

        EdgeData* edgeList = mLodBucketList[mCurrentLod]->getEdgeList();
        ShadowRenderableList& shadowRendList = mLodBucketList[mCurrentLod]->getShadowRenderableList();

        // Calc triangle light facing
        updateEdgeListLightFacing(edgeList, lightPos);

        // Generate indexes and update renderables
        generateShadowVolume(edgeList, indexBuffer, indexBufferUsedSize, light, shadowRendList, flags);

        return shadowRendList;

    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::Region::getEdgeList() noexcept -> EdgeData*
    {
        return mLodBucketList[mCurrentLod]->getEdgeList();
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::Region::dump(std::ofstream& of) const
    {
        of << "Region " << mRegionID << std::endl;
        of << "--------------------------" << std::endl;
        of << "Centre: " << mCentre << std::endl;
        of << "Local AABB: " << mAABB << std::endl;
        of << "Bounding radius: " << mBoundingRadius << std::endl;
        of << "Number of LODs: " << mLodBucketList.size() << std::endl;

        for (const auto & i : mLodBucketList)
        {
            i->dump(of);
        }
        of << "--------------------------" << std::endl;
    }
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    StaticGeometry::LODBucket::LODBucket(Region* parent, unsigned short lod,
        Real lodValue)
        : mParent(parent), mLod(lod), mLodValue(lodValue) 
    {
    }
    //--------------------------------------------------------------------------
    StaticGeometry::LODBucket::~LODBucket()
    {
        ShadowCaster::clearShadowRenderableList(mShadowRenderables);

        // no need to delete queued meshes, these are managed in StaticGeometry
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::LODBucket::assign(QueuedSubMesh* qmesh, ushort atLod)
    {
        mQueuedGeometryList.push_back(::std::make_unique<QueuedGeometry>());
        auto* q = mQueuedGeometryList.back().get();
        q->position = qmesh->position;
        q->orientation = qmesh->orientation;
        q->scale = qmesh->scale;
        if (qmesh->geometryLodList->size() > atLod)
        {
            // This submesh has enough lods, use the right one
            q->geometry = &(*qmesh->geometryLodList)[atLod];
        }
        else
        {
            // Not enough lods, use the lowest one we have
            q->geometry =
                &(*qmesh->geometryLodList)[qmesh->geometryLodList->size() - 1];
        }
        // Locate a material bucket
        MaterialBucket* mbucket = nullptr;
        auto m =
            mMaterialBucketMap.find(qmesh->material->getName());
        if (m != mMaterialBucketMap.end())
        {
            mbucket = m->second.get();
        }
        else
        {
            mbucket = new MaterialBucket(this, qmesh->material);
            mMaterialBucketMap[qmesh->material->getName()].reset(mbucket);
        }
        mbucket->assign(q);
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::LODBucket::build(bool stencilShadows)
    {

        EdgeListBuilder eb;
        size_t vertexSet = 0;

        // Just pass this on to child buckets
        for (auto const& [key, mat] : mMaterialBucketMap)
        {
            mat->build(stencilShadows);

            if (stencilShadows)
            {
                // Check if we have vertex programs here
                Technique* t = mat->getMaterial()->getBestTechnique();
                if (t)
                {
                    Pass* p = t->getPass(0);
                    if (p)
                    {
                        if (p->hasVertexProgram())
                        {
                            mVertexProgramInUse = true;
                        }
                    }
                }

                for (auto const& geom : mat->getGeometryList())
                {
                    // Check we're dealing with 16-bit indexes here
                    // Since stencil shadows can only deal with 16-bit
                    // More than that and stencil is probably too CPU-heavy
                    // in any case
                    assert(geom->getIndexData()->indexBuffer->getType()
                        == HardwareIndexBuffer::IndexType::_16BIT &&
                        "Only 16-bit indexes allowed when using stencil shadows");
                    eb.addVertexData(geom->getVertexData());
                    eb.addIndexData(geom->getIndexData(), vertexSet++);
                }

            }
        }

        if (stencilShadows)
        {
            mEdgeList.reset(eb.build());
        }
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::LODBucket::addRenderables(RenderQueue* queue,
        RenderQueueGroupID group, Real lodValue)
    {
        // Just pass this on to child buckets
        for (auto & i : mMaterialBucketMap)
        {
            i.second->addRenderables(queue, group, lodValue);
        }
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::LODBucket::dump(std::ofstream& of) const
    {
        of << "LOD Bucket " << mLod << std::endl;
        of << "------------------" << std::endl;
        of << "LOD Value: " << mLodValue << std::endl;
        of << "Number of Materials: " << mMaterialBucketMap.size() << std::endl;
        for (const auto & i : mMaterialBucketMap)
        {
            i.second->dump(of);
        }
        of << "------------------" << std::endl;

    }
    //---------------------------------------------------------------------
    void StaticGeometry::LODBucket::visitRenderables(Renderable::Visitor* visitor, 
        bool debugRenderables)
    {
        for (auto & i : mMaterialBucketMap)
        {
            i.second->visitRenderables(visitor, debugRenderables);
        }

    }
    //---------------------------------------------------------------------
    void StaticGeometry::LODBucket::updateShadowRenderables(const Vector4& lightPos,
                                                            const HardwareIndexBufferPtr& indexBuffer,
                                                            Real extrusionDistance, ShadowRenderableFlags flags)
    {
        assert(indexBuffer->getType() == HardwareIndexBuffer::IndexType::_16BIT &&
               "Only 16-bit indexes supported for now");

        // We need to search the edge list for silhouette edges
        OgreAssert(mEdgeList, "You enabled stencil shadows after the build process!");

        // Init shadow renderable list if required
        bool init = mShadowRenderables.empty();
        bool extrude = !!(flags & ShadowRenderableFlags::EXTRUDE_IN_SOFTWARE);

        if (init)
            mShadowRenderables.resize(mEdgeList->edgeGroups.size());

        auto egi = mEdgeList->edgeGroups.begin();
        for (auto & mShadowRenderable : mShadowRenderables)
        {
            if (init)
            {
                // Create a new renderable, create a separate light cap if
                // we're using a vertex program (either for this model, or
                // for extruding the shadow volume) since otherwise we can
                // get depth-fighting on the light cap

                mShadowRenderable = new ShadowRenderable(mParent, indexBuffer, egi->vertexData,
                                                mVertexProgramInUse || !extrude);
            }
            // Extrude vertices in software if required
            if (extrude)
            {
                mParent->extrudeVertices(mShadowRenderable->getPositionBuffer(), egi->vertexData->vertexCount, lightPos,
                                         extrusionDistance);
            }
            ++egi;
        }
    }
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    StaticGeometry::MaterialBucket::MaterialBucket(LODBucket* parent,
        const MaterialPtr& material)
        : mParent(parent)
        , mMaterial(material)
         
    {
    }
    //--------------------------------------------------------------------------
    static auto getHash(StaticGeometry::SubMeshLodGeometryLink* geom) -> uint32
    {
        // Formulate an identifying string for the geometry format
        // Must take into account the vertex declaration and the index type
        // Format is:
        // Index type
        // Vertex element (repeating)
        //   source
        //   semantic
        //   type
        uint32 hash = HashCombine(0, geom->indexData->indexBuffer->getType());
        const auto& elemList = geom->vertexData->vertexDeclaration->getElements();
        for (const VertexElement& elem : elemList)
        {
            hash = HashCombine(hash, elem.getSource());
            hash = HashCombine(hash, elem.getSemantic());
            hash = HashCombine(hash, elem.getType());
        }

        return hash;
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::MaterialBucket::assign(QueuedGeometry* qgeom)
    {
        // Look up any current geometry
        uint32 hash = getHash(qgeom->geometry);
        auto gi = mCurrentGeometryMap.find(hash);
        bool newBucket = true;
        if (gi != mCurrentGeometryMap.end())
        {
            // Found existing geometry, try to assign
            newBucket = !gi->second->assign(qgeom);
            // Note that this bucket will be replaced as the 'current'
            // for this hash string below since it's out of space
        }
        // Do we need to create a new one?
        if (newBucket)
        {
            // Add to main list
            mGeometryBucketList.push_back(::std::make_unique<GeometryBucket>(this, qgeom->geometry->vertexData, qgeom->geometry->indexData));
            auto* gbucket = mGeometryBucketList.back().get();
            // Also index in 'current' list
            mCurrentGeometryMap[hash] = gbucket;
            if (!gbucket->assign(qgeom))
            {
                OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR,
                    "Somehow we couldn't fit the requested geometry even in a "
                    "brand new GeometryBucket!! Must be a bug, please report.",
                    "StaticGeometry::MaterialBucket::assign");
            }
        }
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::MaterialBucket::build(bool stencilShadows)
    {
        mTechnique = nullptr;
        mMaterial->load();
        // tell the geometry buckets to build
        for (auto const& gb : mGeometryBucketList)
        {
            gb->build(stencilShadows);
        }
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::MaterialBucket::addRenderables(RenderQueue* queue,
        RenderQueueGroupID group, Real lodValue)
    {
        // Get region
        Region *region = mParent->getParent();

        // Get material LOD strategy
        const LodStrategy *materialLodStrategy = mMaterial->getLodStrategy();

        // If material strategy doesn't match, recompute LOD value with correct strategy
        if (materialLodStrategy != region->mLodStrategy)
            lodValue = materialLodStrategy->getValue(region, region->mCamera);

        // Determine the current material technique
        mTechnique = mMaterial->getBestTechnique(
            mMaterial->getLodIndex(lodValue));
        for (auto & i : mGeometryBucketList)
        {
            queue->addRenderable(i.get(), group);
        }

    }
    void StaticGeometry::MaterialBucket::_setMaterial(const MaterialPtr& material)
    {
        OgreAssert(material, "NULL pointer");
        mMaterial = material;
        mMaterial->load();
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::MaterialBucket::dump(std::ofstream& of) const
    {
        of << "Material Bucket " << getMaterialName() << std::endl;
        of << "--------------------------------------------------" << std::endl;
        of << "Geometry buckets: " << mGeometryBucketList.size() << std::endl;
        for (const auto & i : mGeometryBucketList)
        {
            i->dump(of);
        }
        of << "--------------------------------------------------" << std::endl;

    }
    //---------------------------------------------------------------------
    void StaticGeometry::MaterialBucket::visitRenderables(Renderable::Visitor* visitor, 
        bool debugRenderables)
    {
        for (auto & i : mGeometryBucketList)
        {
            visitor->visit(i.get(), mParent->getLod(), false);
        }

    }
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    StaticGeometry::GeometryBucket::GeometryBucket(MaterialBucket* parent, const VertexData* vData,
                                                   const IndexData* iData)
        : Renderable(), mParent(parent)
    {
        // Clone the structure from the example
        mVertexData.reset(vData->clone(false));
        mIndexData.reset(iData->clone(false));
        mVertexData->vertexCount = 0;
        mVertexData->vertexStart = 0;
        mIndexData->indexCount = 0;
        mIndexData->indexStart = 0;
        // Derive the max vertices
        if (iData->indexBuffer->getType() == HardwareIndexBuffer::IndexType::_32BIT)
        {
            mMaxVertexIndex = 0xFFFFFFFF;
        }
        else
        {
            mMaxVertexIndex = 0xFFFF;
        }

        // Check to see if we have blend indices / blend weights
        // remove them if so, they can try to blend non-existent bones!
        const VertexElement* blendIndices =
            mVertexData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::BLEND_INDICES);
        const VertexElement* blendWeights =
            mVertexData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::BLEND_WEIGHTS);
        if (blendIndices && blendWeights)
        {
            assert(blendIndices->getSource() == blendWeights->getSource()
                && "Blend indices and weights should be in the same buffer");
            // Get the source
            ushort source = blendIndices->getSource();
            assert(blendIndices->getSize() + blendWeights->getSize() ==
                mVertexData->vertexBufferBinding->getBuffer(source)->getVertexSize()
                && "Blend indices and blend buffers should have buffer to themselves!");
            // Unset the buffer
            mVertexData->vertexBufferBinding->unsetBinding(source);
            // Remove the elements
            mVertexData->vertexDeclaration->removeElement(VertexElementSemantic::BLEND_INDICES);
            mVertexData->vertexDeclaration->removeElement(VertexElementSemantic::BLEND_WEIGHTS);
            // Close gaps in bindings for effective and safely
            mVertexData->closeGapsInBindings();
        }


    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::GeometryBucket::getMaterial() const noexcept -> const MaterialPtr&
    {
        return mParent->getMaterial();
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::GeometryBucket::getTechnique() const noexcept -> Technique*
    {
        return mParent->getCurrentTechnique();
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::GeometryBucket::getRenderOperation(RenderOperation& op)
    {
        op.indexData = mIndexData.get();
        op.operationType = RenderOperation::OperationType::TRIANGLE_LIST;
        op.srcRenderable = this;
        op.useIndexes = true;
        op.vertexData = mVertexData.get();
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::GeometryBucket::getWorldTransforms(Matrix4* xform) const
    {
        // Should be the identity transform, but lets allow transformation of the
        // nodes the regions are attached to for kicks
        *xform = mParent->getParent()->getParent()->_getParentNodeFullTransform();
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::GeometryBucket::getSquaredViewDepth(const Camera* cam) const -> Real
    {
        const Region *region = mParent->getParent()->getParent();
        if (cam == region->mCamera)
            return region->mSquaredViewDepth;
        else
            return region->getParentNode()->getSquaredViewDepth(cam->getLodCamera());
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::GeometryBucket::getLights() const noexcept -> const LightList&
    {
        return mParent->getParent()->getParent()->queryLights();
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::GeometryBucket::getCastsShadows() const noexcept -> bool
    {
        return mParent->getParent()->getParent()->getCastShadows();
    }
    //--------------------------------------------------------------------------
    auto StaticGeometry::GeometryBucket::assign(QueuedGeometry* qgeom) -> bool
    {
        // Do we have enough space?
        // -2 first to avoid overflow (-1 to adjust count to index, -1 to ensure
        // no overflow at 32 bits and use >= instead of >)
        if ((mVertexData->vertexCount - 2 + qgeom->geometry->vertexData->vertexCount)
            >= mMaxVertexIndex)
        {
            return false;
        }

        mQueuedGeometry.emplace_back(qgeom);
        mVertexData->vertexCount += qgeom->geometry->vertexData->vertexCount;
        mIndexData->indexCount += qgeom->geometry->indexData->indexCount;

        return true;
    }
    //--------------------------------------------------------------------------
    template<typename T>
    static void copyIndexes(const T* src, T* dst, size_t count, uint32 indexOffset)
    {
        if (indexOffset == 0)
        {
            memcpy(dst, src, sizeof(T) * count);
        }
        else
        {
            while(count--)
            {
                *dst++ = static_cast<T>(*src++ + indexOffset);
            }
        }
    }
    void StaticGeometry::GeometryBucket::build(bool stencilShadows)
    {
        // Need to double the vertex count for the position buffer
        // if we're doing stencil shadows
        OgreAssert(!stencilShadows || mVertexData->vertexCount * 2 <= mMaxVertexIndex,
                   "Index range exceeded when using stencil shadows, consider reducing your region size or "
                   "reducing poly count");

        // Ok, here's where we transfer the vertices and indexes to the shared
        // buffers
        // Shortcuts
        VertexDeclaration* dcl = mVertexData->vertexDeclaration;
        VertexBufferBinding* binds = mVertexData->vertexBufferBinding;

        // create index buffer, and lock
        auto indexType = mIndexData->indexBuffer->getType();
        mIndexData->indexBuffer = HardwareBufferManager::getSingleton()
            .createIndexBuffer(indexType, mIndexData->indexCount,
                HardwareBuffer::STATIC_WRITE_ONLY);
        HardwareBufferLockGuard dstIndexLock(mIndexData->indexBuffer, HardwareBuffer::LockOptions::DISCARD);
        auto* p32Dest = static_cast<uint32*>(dstIndexLock.pData);
        auto* p16Dest = static_cast<uint16*>(dstIndexLock.pData);
        // create all vertex buffers, and lock
        ushort b;

        std::vector<uchar*> destBufferLocks;
        std::vector<VertexDeclaration::VertexElementList> bufferElements;
        for (b = 0; b < binds->getBufferCount(); ++b)
        {
            HardwareVertexBufferSharedPtr vbuf =
                HardwareBufferManager::getSingleton().createVertexBuffer(
                    dcl->getVertexSize(b),
                    mVertexData->vertexCount,
                    HardwareBuffer::STATIC_WRITE_ONLY);
            binds->setBinding(b, vbuf);
            auto* pLock = static_cast<uchar*>(
                vbuf->lock(HardwareBuffer::LockOptions::DISCARD));
            destBufferLocks.push_back(pLock);
            // Pre-cache vertex elements per buffer
            bufferElements.push_back(dcl->findElementsBySource(b));
        }


        // Iterate over the geometry items
        uint32 indexOffset = 0;
        Vector3 regionCentre = mParent->getParent()->getParent()->getCentre();
        for (auto geom : mQueuedGeometry)
        {
            // Copy indexes across with offset
            IndexData* srcIdxData = geom->geometry->indexData;
            HardwareBufferLockGuard srcIdxLock(srcIdxData->indexBuffer,
                                               srcIdxData->indexStart * srcIdxData->indexBuffer->getIndexSize(), 
                                               srcIdxData->indexCount * srcIdxData->indexBuffer->getIndexSize(),
                                               HardwareBuffer::LockOptions::READ_ONLY);
            if (indexType == HardwareIndexBuffer::IndexType::_32BIT)
            {
                auto* pSrc = static_cast<uint32*>(srcIdxLock.pData);
                copyIndexes(pSrc, p32Dest, srcIdxData->indexCount, indexOffset);
                p32Dest += srcIdxData->indexCount;
            }
            else
            {
                // Lock source indexes
                auto* pSrc = static_cast<uint16*>(srcIdxLock.pData);
                copyIndexes(pSrc, p16Dest, srcIdxData->indexCount, indexOffset);
                p16Dest += srcIdxData->indexCount;
            }
            srcIdxLock.unlock();

            // Now deal with vertex buffers
            // we can rely on buffer counts / formats being the same
            VertexData* srcVData = geom->geometry->vertexData;
            VertexBufferBinding* srcBinds = srcVData->vertexBufferBinding;
            for (b = 0; b < binds->getBufferCount(); ++b)
            {
                // lock source
                HardwareVertexBufferSharedPtr srcBuf = srcBinds->getBuffer(b);
                HardwareBufferLockGuard srcBufLock(srcBuf, HardwareBuffer::LockOptions::READ_ONLY);
                auto* pSrcBase = static_cast<uchar*>(srcBufLock.pData);
                // Get buffer lock pointer, we'll update this later
                uchar* pDstBase = destBufferLocks[b];
                size_t bufInc = srcBuf->getVertexSize();

                // Iterate over vertices
                float *pSrcReal, *pDstReal;
                Vector3 tmp;
                for (size_t v = 0; v < srcVData->vertexCount; ++v)
                {
                    // Iterate over vertex elements
                    VertexDeclaration::VertexElementList& elems =
                        bufferElements[b];
                    for (auto & elem : elems)
                    {
                        elem.baseVertexPointerToElement(pSrcBase, &pSrcReal);
                        elem.baseVertexPointerToElement(pDstBase, &pDstReal);
                        switch (elem.getSemantic())
                        {
                        case VertexElementSemantic::POSITION:
                            tmp.x = *pSrcReal++;
                            tmp.y = *pSrcReal++;
                            tmp.z = *pSrcReal++;
                            // transform
                            tmp = (geom->orientation * (tmp * geom->scale)) +
                                geom->position;
                            // Adjust for region centre
                            tmp -= regionCentre;
                            *pDstReal++ = tmp.x;
                            *pDstReal++ = tmp.y;
                            *pDstReal++ = tmp.z;
                            break;
                        case VertexElementSemantic::NORMAL:
                        case VertexElementSemantic::TANGENT:
                        case VertexElementSemantic::BINORMAL:
                            tmp.x = *pSrcReal++;
                            tmp.y = *pSrcReal++;
                            tmp.z = *pSrcReal++;
                            // scale (invert)
                            tmp = tmp / geom->scale;
                            tmp.normalise();
                            // rotation
                            tmp = geom->orientation * tmp;
                            *pDstReal++ = tmp.x;
                            *pDstReal++ = tmp.y;
                            *pDstReal++ = tmp.z;
                            // copy parity for tangent.
                            if (elem.getType() == Ogre::VertexElementType::FLOAT4)
                                *pDstReal = *pSrcReal;
                            break;
                        default:
                            // just raw copy
                            memcpy(pDstReal, pSrcReal,
                                    VertexElement::getTypeSize(elem.getType()));
                            break;
                        };

                    }

                    // Increment both pointers
                    pDstBase += bufInc;
                    pSrcBase += bufInc;

                }

                // Update pointer
                destBufferLocks[b] = pDstBase;
            }

            indexOffset += geom->geometry->vertexData->vertexCount;
        }

        // Unlock everything
        dstIndexLock.unlock();
        for (b = 0; b < binds->getBufferCount(); ++b)
        {
            binds->getBuffer(b)->unlock();
        }

        if (stencilShadows)
        {
            mVertexData->prepareForShadowVolume();
        }
    }
    //--------------------------------------------------------------------------
    void StaticGeometry::GeometryBucket::dump(std::ofstream& of) const
    {
        of << "Geometry Bucket" << std::endl;
        of << "---------------" << std::endl;
        of << "Geometry items: " << mQueuedGeometry.size() << std::endl;
        of << "Vertex count: " << mVertexData->vertexCount << std::endl;
        of << "Index count: " << mIndexData->indexCount << std::endl;
        of << "---------------" << std::endl;

    }
    //--------------------------------------------------------------------------
    String StaticGeometryFactory::FACTORY_TYPE_NAME = "StaticGeometry";
}

