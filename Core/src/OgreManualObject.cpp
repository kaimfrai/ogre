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
#include <cstring>

module Ogre.Core;

import :AxisAlignedBox;
import :ColourValue;
import :Common;
import :EdgeListBuilder;
import :Exception;
import :HardwareBuffer;
import :HardwareBufferManager;
import :HardwareIndexBuffer;
import :HardwareVertexBuffer;
import :Light;
import :Log;
import :LogManager;
import :ManualObject;
import :Material;
import :MaterialManager;
import :Math;
import :Matrix3;
import :Matrix4;
import :Mesh;
import :MeshManager;
import :MovableObject;
import :Node;
import :Pass;
import :Platform;
import :Prerequisites;
import :RenderOperation;
import :RenderQueue;
import :Renderable;
import :ShadowCaster;
import :SharedPtr;
import :SubMesh;
import :Technique;
import :Vector;
import :VertexIndexData;

import <algorithm>;
import <list>;
import <memory>;
import <string>;
import <utility>;
import <vector>;

namespace Ogre {
#define TEMP_INITIAL_SIZE 50
#define TEMP_VERTEXSIZE_GUESS sizeof(float) * 12
#define TEMP_INITIAL_VERTEX_SIZE TEMP_VERTEXSIZE_GUESS * TEMP_INITIAL_SIZE
#define TEMP_INITIAL_INDEX_SIZE sizeof(uint32) * TEMP_INITIAL_SIZE
    //-----------------------------------------------------------------------------
ManualObject::ManualObject(std::string_view name)
    : MovableObject(name), 
      mTempVertexSize(TEMP_INITIAL_VERTEX_SIZE), 
      mTempIndexSize(TEMP_INITIAL_INDEX_SIZE) 
{
    }
    //-----------------------------------------------------------------------------
    ManualObject::~ManualObject()
    {
        clear();
    }
    //-----------------------------------------------------------------------------
    void ManualObject::clear()
    {
        resetTempAreas();
        for (auto & i : mSectionList)
        {
            delete i;
        }
        mSectionList.clear();
        mRadius = 0;
        mAABB.setNull();
        delete mEdgeList;
        mEdgeList = nullptr;
        mAnyIndexed = false;

        clearShadowRenderableList(mShadowRenderables);
    }
    //-----------------------------------------------------------------------------
    void ManualObject::resetTempAreas()
    {
        delete[] mTempVertexBuffer;
        delete[] mTempIndexBuffer;
        mTempVertexBuffer = nullptr;
        mTempIndexBuffer = nullptr;
        mTempVertexSize = TEMP_INITIAL_VERTEX_SIZE;
        mTempIndexSize = TEMP_INITIAL_INDEX_SIZE;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::resizeTempVertexBufferIfNeeded(size_t numVerts)
    {
        // Calculate byte size
        // Use decl if we know it by now, otherwise default size to pos/norm/texcoord*2
        size_t newSize = (mFirstVertex ? TEMP_VERTEXSIZE_GUESS : mDeclSize) * numVerts;

        if(newSize <= mTempVertexSize && mTempVertexBuffer) {
            return;
        }

        // init or increase to at least double current
        newSize = std::max(newSize, mTempVertexBuffer ? mTempVertexSize*2 : mTempVertexSize);

        // copy old data
        char* tmp = mTempVertexBuffer;
        mTempVertexBuffer = new char[newSize];
        if (tmp)
        {
            memcpy(mTempVertexBuffer, tmp, mTempVertexSize);
            // delete old buffer
            delete[] tmp;
        }
        mTempVertexSize = newSize;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::resizeTempIndexBufferIfNeeded(size_t numInds)
    {
        size_t newSize = numInds * sizeof(uint32);

        if(newSize <= mTempIndexSize && mTempIndexBuffer) {
            return;
        }

        // init or increase to at least double current
        newSize = std::max(newSize, mTempIndexBuffer ? mTempIndexSize*2 : mTempIndexSize);

        numInds = newSize / sizeof(uint32);
        uint32* tmp = mTempIndexBuffer;
        mTempIndexBuffer = new uint32[numInds];
        if (tmp)
        {
            memcpy(mTempIndexBuffer, tmp, mTempIndexSize);
            delete[] tmp;
        }
        mTempIndexSize = newSize;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::estimateVertexCount(size_t vcount)
    {
        resizeTempVertexBufferIfNeeded(vcount);
        mEstVertexCount = vcount;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::estimateIndexCount(size_t icount)
    {
        resizeTempIndexBufferIfNeeded(icount);
        mEstIndexCount = icount;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::begin(std::string_view materialName,
        RenderOperation::OperationType opType, std::string_view groupName)
    {
        OgreAssert(!mCurrentSection, "You cannot call begin() again until after you call end()");

        // Check that a valid material was provided
        MaterialPtr material = MaterialManager::getSingleton().getByName(materialName, groupName);

        if(!material)
        {
            LogManager::getSingleton().logError(
                ::std::format("Can't assign material {}"
                " to the ManualObject {} because this "
                "Material does not exist in group {}"
                ". Have you forgotten to define it in a "
                ".material script?", materialName, mName, groupName));

            material = MaterialManager::getSingleton().getDefaultMaterial();
        }

        mCurrentSection = new ManualObjectSection(this, material, opType);
        mCurrentUpdating = false;
        mCurrentSection->setUseIdentityProjection(mUseIdentityProjection);
        mCurrentSection->setUseIdentityView(mUseIdentityView);
        mSectionList.push_back(mCurrentSection);
        mFirstVertex = true;
        mDeclSize = 0;
        mTexCoordIndex = 0;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::begin(const MaterialPtr& mat, RenderOperation::OperationType opType)
    {
      OgreAssert(!mCurrentSection, "You cannot call begin() again until after you call end()");

      if (mat)
      {
          mCurrentSection = new ManualObjectSection(this, mat, opType);
      }
      else
      {
          LogManager::getSingleton().logMessage("Can't assign null material", LogMessageLevel::Critical);
          const MaterialPtr defaultMat = MaterialManager::getSingleton().getDefaultMaterial();
          mCurrentSection = new ManualObjectSection(this, defaultMat, opType);
      }

      mCurrentUpdating = false;
      mCurrentSection->setUseIdentityProjection(mUseIdentityProjection);
      mCurrentSection->setUseIdentityView(mUseIdentityView);
      mSectionList.push_back(mCurrentSection);
      mFirstVertex = true;
      mDeclSize = 0;
      mTexCoordIndex = 0;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::beginUpdate(size_t sectionIndex)
    {
        OgreAssert(!mCurrentSection, "You cannot call begin() again until after you call end()");
        mCurrentSection = mSectionList.at(sectionIndex);
        mCurrentUpdating = true;
        mFirstVertex = true;
        mTexCoordIndex = 0;
        // reset vertex & index count
        RenderOperation* rop = mCurrentSection->getRenderOperation();
        rop->vertexData->vertexCount = 0;
        if (rop->indexData)
            rop->indexData->indexCount = 0;
        rop->useIndexes = false;
        mDeclSize = rop->vertexData->vertexDeclaration->getVertexSize(0);
    }
    //-----------------------------------------------------------------------------
    void ManualObject::declareElement(VertexElementType t, VertexElementSemantic s)
    {
        // defining declaration
        ushort idx = s == VertexElementSemantic::TEXTURE_COORDINATES ? mTexCoordIndex : 0;
        mDeclSize += mCurrentSection->getRenderOperation()
                         ->vertexData->vertexDeclaration->addElement(0, mDeclSize, t, s, idx)
                         .getSize();
    }
    //-----------------------------------------------------------------------------
    auto ManualObject::getCurrentVertexCount() const -> size_t
    {
        if (!mCurrentSection)
            return 0;
        
        RenderOperation* rop = mCurrentSection->getRenderOperation();

        // There's an unfinished vertex being defined, so include it in count
        if (mTempVertexPending)
            return rop->vertexData->vertexCount + 1;
        else
            return rop->vertexData->vertexCount;
        
    }
    //-----------------------------------------------------------------------------
    auto ManualObject::getCurrentIndexCount() const -> size_t
    {
        if (!mCurrentSection)
            return 0;

        RenderOperation* rop = mCurrentSection->getRenderOperation();
        if (rop->indexData)
            return rop->indexData->indexCount;
        else
            return 0;

    }
    //-----------------------------------------------------------------------------
    void ManualObject::copyTempVertexToBuffer()
    {
        mTempVertexPending = false;
        RenderOperation* rop = mCurrentSection->getRenderOperation();
        if (rop->vertexData->vertexCount == 0 && !mCurrentUpdating)
        {
            // first vertex, autoorganise decl
            VertexDeclaration* oldDcl = rop->vertexData->vertexDeclaration;
            rop->vertexData->vertexDeclaration =
                oldDcl->getAutoOrganisedDeclaration(false, false, false);
            HardwareBufferManager::getSingleton().destroyVertexDeclaration(oldDcl);
        }
        resizeTempVertexBufferIfNeeded(++rop->vertexData->vertexCount);

        // get base pointer
        char* pBase = mTempVertexBuffer + (mDeclSize * (rop->vertexData->vertexCount-1));
        const VertexDeclaration::VertexElementList& elemList =
            rop->vertexData->vertexDeclaration->getElements();
        for (const auto & elem : elemList)
        {
            float* pFloat = nullptr;
            RGBA* pRGBA = nullptr;
            switch(elem.getType())
            {
            using enum VertexElementType;
            case FLOAT1:
            case FLOAT2:
            case FLOAT3:
            case FLOAT4:
                OgreAssert(elem.getSemantic() != VertexElementSemantic::DIFFUSE, "must use VertexElementType::COLOUR");
                elem.baseVertexPointerToElement(pBase, &pFloat);
                break;
            case UBYTE4_NORM:
                OgreAssert(elem.getSemantic() == VertexElementSemantic::DIFFUSE, "must use VertexElementSemantic::DIFFUSE");
                elem.baseVertexPointerToElement(pBase, &pRGBA);
                break;
            default:
                OgreAssert(false, "invalid element type");
                break;
            };

            unsigned short dims;
            switch(elem.getSemantic())
            {
            using enum VertexElementSemantic;
            case POSITION:
                *pFloat++ = mTempVertex.position.x;
                *pFloat++ = mTempVertex.position.y;
                *pFloat++ = mTempVertex.position.z;
                break;
            case NORMAL:
                *pFloat++ = mTempVertex.normal.x;
                *pFloat++ = mTempVertex.normal.y;
                *pFloat++ = mTempVertex.normal.z;
                break;
            case TANGENT:
                *pFloat++ = mTempVertex.tangent.x;
                *pFloat++ = mTempVertex.tangent.y;
                *pFloat++ = mTempVertex.tangent.z;
                break;
            case TEXTURE_COORDINATES:
                dims = VertexElement::getTypeCount(elem.getType());
                for (ushort t = 0; t < dims; ++t)
                    *pFloat++ = mTempVertex.texCoord[elem.getIndex()][t];
                break;
            case DIFFUSE:
                *pRGBA++ = mTempVertex.colour.getAsABGR();
                break;
            default:
                OgreAssert(false, "invalid semantic");
                break;
            };

        }

    }
    //-----------------------------------------------------------------------------
    auto ManualObject::end() -> ManualObject::ManualObjectSection*
    {
        OgreAssert(mCurrentSection, "You cannot call end() until after you call begin()");
        if (mTempVertexPending)
        {
            // bake current vertex
            copyTempVertexToBuffer();
        }

        // pointer that will be returned
        ManualObjectSection* result = nullptr;

        RenderOperation* rop = mCurrentSection->getRenderOperation();
        // Check for empty content
        if (rop->vertexData->vertexCount == 0 ||
            (rop->useIndexes && rop->indexData->indexCount == 0))
        {
            // You're wasting my time sonny
            if (mCurrentUpdating)
            {
                // Can't just undo / remove since may be in the middle
                // Just allow counts to be 0, will not be issued to renderer

                // return the finished section (though it has zero vertices)
                result = mCurrentSection;
            }
            else
            {
                // First creation, can really undo
                // Has already been added to section list end, so remove
                mSectionList.pop_back();
                delete mCurrentSection;

            }
        }
        else // not an empty section
        {

            // Bake the real buffers
            HardwareVertexBufferSharedPtr vbuf;
            // Check buffer sizes
            bool vbufNeedsCreating = true;
            bool ibufNeedsCreating = rop->useIndexes;
            // Work out if we require 16 or 32-bit index buffers
            HardwareIndexBuffer::IndexType indexType = mCurrentSection->get32BitIndices()?  
                HardwareIndexBuffer::IndexType::_32BIT : HardwareIndexBuffer::IndexType::_16BIT;
            if (mCurrentUpdating)
            {
                // May be able to reuse buffers, check sizes
                vbuf = rop->vertexData->vertexBufferBinding->getBuffer(0);
                if (vbuf->getNumVertices() >= rop->vertexData->vertexCount)
                    vbufNeedsCreating = false;

                if (rop->useIndexes)
                {
                    if ((rop->indexData->indexBuffer->getNumIndexes() >= rop->indexData->indexCount) &&
                        (indexType == rop->indexData->indexBuffer->getType()))
                        ibufNeedsCreating = false;
                }

            }
            if (vbufNeedsCreating)
            {
                // Make the vertex buffer larger if estimated vertex count higher
                // to allow for user-configured growth area
                size_t vertexCount = std::max(rop->vertexData->vertexCount, 
                    mEstVertexCount);
                vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(mDeclSize, vertexCount,
                                                                                mBufferUsage);
                rop->vertexData->vertexBufferBinding->setBinding(0, vbuf);
            }
            if (ibufNeedsCreating)
            {
                // Make the index buffer larger if estimated index count higher
                // to allow for user-configured growth area
                size_t indexCount = std::max(rop->indexData->indexCount, mEstIndexCount);
                rop->indexData->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
                    indexType, indexCount, mBufferUsage);
            }
            // Write vertex data
            vbuf->writeData(
                0, rop->vertexData->vertexCount * vbuf->getVertexSize(), 
                mTempVertexBuffer, true);
            // Write index data
            if(rop->useIndexes)
            {
                if (HardwareIndexBuffer::IndexType::_32BIT == indexType)
                {
                    // direct copy from the mTempIndexBuffer
                    rop->indexData->indexBuffer->writeData(
                        0, 
                        rop->indexData->indexCount 
                            * rop->indexData->indexBuffer->getIndexSize(),
                        mTempIndexBuffer, true);
                }
                else //(HardwareIndexBuffer::IndexType::_16BIT == indexType)
                {
                    HardwareBufferLockGuard indexLock(rop->indexData->indexBuffer, HardwareBuffer::LockOptions::DISCARD);
                    auto* pIdx = static_cast<uint16*>(indexLock.pData);
                    uint32* pSrc = mTempIndexBuffer;
                    for (size_t i = 0; i < rop->indexData->indexCount; i++)
                    {
                        *pIdx++ = static_cast<uint16>(*pSrc++);
                    }
                }
            }

            // return the finished section
            result = mCurrentSection;

        } // empty section check

        mCurrentSection = nullptr;
        resetTempAreas();

        // Tell parent if present
        if (mParentNode)
        {
            mParentNode->needUpdate();
        }

        // will return the finished section or NULL if
        // the section was empty (i.e. zero vertices/indices)
        return result;
    }
    //-----------------------------------------------------------------------------
    auto ManualObject::convertToMesh(std::string_view meshName, std::string_view groupName) -> MeshPtr
    {
        OgreAssert(!mCurrentSection, "You cannot call convertToMesh() whilst you are in the middle of "
                                     "defining the object; call end() first.");
        OgreAssert(!mSectionList.empty(), "No data defined to convert to a mesh.");

        MeshPtr m = MeshManager::getSingleton().createManual(meshName, groupName);

        for (auto sec : mSectionList)
        {
            SubMesh* sm = m->createSubMesh();
            sec->convertToSubMesh(sm);
            sm->setMaterial(sec->getMaterial());
        }
        // update bounds
        m->_setBounds(mAABB);
        m->_setBoundingSphereRadius(mRadius);

        m->load();

        return m;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::setUseIdentityProjection(bool useIdentityProjection)
    {
        // Set existing
        for (auto & i : mSectionList)
        {
            i->setUseIdentityProjection(useIdentityProjection);
        }
        
        // Save setting for future sections
        mUseIdentityProjection = useIdentityProjection;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::setUseIdentityView(bool useIdentityView)
    {
        // Set existing
        for (auto & i : mSectionList)
        {
            i->setUseIdentityView(useIdentityView);
        }

        // Save setting for future sections
        mUseIdentityView = useIdentityView;
    }
    //-----------------------------------------------------------------------------
    auto ManualObject::getMovableType() const noexcept -> std::string_view
    {
        return ManualObjectFactory::FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::_updateRenderQueue(RenderQueue* queue)
    {
        // To be used when order of creation must be kept while rendering
        unsigned short priority = queue->getDefaultRenderablePriority();

        for (auto & i : mSectionList)
        {
            // Skip empty sections (only happens if non-empty first, then updated)
            RenderOperation* rop = i->getRenderOperation();
            if (rop->vertexData->vertexCount == 0 ||
                (rop->useIndexes && rop->indexData->indexCount == 0))
                continue;
            
            if (mRenderQueuePrioritySet)
            {
                assert(mRenderQueueIDSet == true);
                queue->addRenderable(i, mRenderQueueID, mRenderQueuePriority);
            }
            else if (mRenderQueueIDSet)
                queue->addRenderable(i, mRenderQueueID, mKeepDeclarationOrder ? priority++ : queue->getDefaultRenderablePriority());
            else
                queue->addRenderable(i, queue->getDefaultQueueGroup(), mKeepDeclarationOrder ? priority++ : queue->getDefaultRenderablePriority());
        }
    }
    //-----------------------------------------------------------------------------
    void ManualObject::visitRenderables(Renderable::Visitor* visitor, 
        bool debugRenderables)
    {
        for (auto & i : mSectionList)
        {
            visitor->visit(i, 0, false);
        }

    }
    //-----------------------------------------------------------------------------
    auto ManualObject::getEdgeList() noexcept -> EdgeData*
    {
        // Build on demand
        if (!mEdgeList && mAnyIndexed)
        {
            EdgeListBuilder eb;
            size_t vertexSet = 0;
            bool anyBuilt = false;
            for (auto & i : mSectionList)
            {
                RenderOperation* rop = i->getRenderOperation();
                // Only indexed triangle geometry supported for stencil shadows
                if (rop->useIndexes && rop->indexData->indexCount != 0 && 
                    (rop->operationType == RenderOperation::OperationType::TRIANGLE_FAN ||
                     rop->operationType == RenderOperation::OperationType::TRIANGLE_LIST ||
                     rop->operationType == RenderOperation::OperationType::TRIANGLE_STRIP))
                {
                    eb.addVertexData(rop->vertexData);
                    eb.addIndexData(rop->indexData, vertexSet++);
                    anyBuilt = true;
                }
            }

            if (anyBuilt)
                mEdgeList = eb.build();

        }
        return mEdgeList;
    }
    //-----------------------------------------------------------------------------
    auto ManualObject::getShadowVolumeRenderableList(
        const Light* light, const HardwareIndexBufferPtr& indexBuffer, size_t& indexBufferUsedSize,
        float extrusionDistance, ShadowRenderableFlags flags) -> const ShadowRenderableList&
    {
        EdgeData* edgeList = getEdgeList();
        if (!edgeList)
        {
            return mShadowRenderables;
        }

        // Calculate the object space light details
        Vector4 lightPos = light->getAs4DVector();
        Affine3 world2Obj = mParentNode->_getFullTransform().inverse();
        lightPos = world2Obj * lightPos;
        Matrix3 world2Obj3x3 = world2Obj.linear();
        extrusionDistance *= Math::Sqrt(std::min(std::min(world2Obj3x3.GetColumn(0).squaredLength(), world2Obj3x3.GetColumn(1).squaredLength()), world2Obj3x3.GetColumn(2).squaredLength()));

        // Init shadow renderable list if required (only allow indexed)
        bool init = mShadowRenderables.empty() && mAnyIndexed;
        bool extrude = !!(flags & ShadowRenderableFlags::EXTRUDE_IN_SOFTWARE);

        if (init)
            mShadowRenderables.resize(edgeList->edgeGroups.size());

        auto egi = edgeList->edgeGroups.begin();
        auto seci = mSectionList.begin();
        for (auto & mShadowRenderable : mShadowRenderables)
        {
            // Skip non-indexed geometry
            if (!(*seci)->getRenderOperation()->useIndexes)
            {
                continue;
            }

            if (init)
            {
                // Create a new renderable, create a separate light cap if
                // we're using a vertex program (either for this model, or
                // for extruding the shadow volume) since otherwise we can
                // get depth-fighting on the light cap
                MaterialPtr mat = (*seci)->getMaterial();
                mat->load();
                bool vertexProgram = false;
                Technique* t = mat->getBestTechnique(0, *seci);
                for (auto pass : t->getPasses())
                {
                    if (pass->hasVertexProgram())
                    {
                        vertexProgram = true;
                        break;
                    }
                }
                mShadowRenderable = new ShadowRenderable(this, indexBuffer, egi->vertexData,
                                                vertexProgram || !extrude);
            }
            // Extrude vertices in software if required
            if (extrude)
            {
                extrudeVertices(mShadowRenderable->getPositionBuffer(), egi->vertexData->vertexCount, lightPos,
                                extrusionDistance);
            }

            ++egi;
            ++seci;
        }
        // Calc triangle light facing
        updateEdgeListLightFacing(edgeList, lightPos);

        // Generate indexes and update renderables
        generateShadowVolume(edgeList, indexBuffer, indexBufferUsedSize, light, mShadowRenderables, flags);

        return mShadowRenderables;
    }
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    ManualObject::ManualObjectSection::ManualObjectSection(ManualObject* parent,
        std::string_view materialName, RenderOperation::OperationType opType, std::string_view groupName)
        : mParent(parent), mMaterialName(materialName), mGroupName(groupName), m32BitIndices(false)
    {
        mRenderOperation.operationType = opType;
        // default to no indexes unless we're told
        mRenderOperation.useIndexes = false;
        mRenderOperation.useGlobalInstancingVertexBufferIsAvailable = false;
        mRenderOperation.vertexData = new VertexData();
        mRenderOperation.vertexData->vertexCount = 0;
    }
    ManualObject::ManualObjectSection::ManualObjectSection(ManualObject* parent,
        const MaterialPtr& mat, RenderOperation::OperationType opType)
        : mParent(parent), mMaterial(mat), m32BitIndices(false)
    {
        assert(mMaterial);
        mMaterialName = mMaterial->getName();
        mGroupName = mMaterial->getGroup();

        mRenderOperation.operationType = opType;
        mRenderOperation.useIndexes = false;
        mRenderOperation.useGlobalInstancingVertexBufferIsAvailable = false;
        mRenderOperation.vertexData = new VertexData();
        mRenderOperation.vertexData->vertexCount = 0;
    }
    //-----------------------------------------------------------------------------
    ManualObject::ManualObjectSection::~ManualObjectSection()
    {
        delete mRenderOperation.vertexData;
        delete mRenderOperation.indexData; // ok to delete 0
    }
    //-----------------------------------------------------------------------------
    auto ManualObject::ManualObjectSection::getRenderOperation() noexcept -> RenderOperation*
    {
        return &mRenderOperation;
    }
    //-----------------------------------------------------------------------------
    auto ManualObject::ManualObjectSection::getMaterial() const noexcept -> const MaterialPtr&
    {
        if (!mMaterial)
        {
            mMaterial = static_pointer_cast<Material>(MaterialManager::getSingleton().load(mMaterialName, mGroupName));
        }
        return mMaterial;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::ManualObjectSection::setMaterialName(std::string_view name,
        std::string_view groupName /* = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME */)
    {
        if (mMaterialName != name || mGroupName != groupName)
        {
            mMaterialName = name;
            mGroupName = groupName;
            mMaterial.reset();
        }
    }
    //-----------------------------------------------------------------------------
    void ManualObject::ManualObjectSection::setMaterial(const MaterialPtr& mat)
    {
        assert(mat);
        mMaterial = mat;
        mMaterialName = mat->getName();
        mGroupName = mat->getGroup();
    }
    //-----------------------------------------------------------------------------
    void ManualObject::ManualObjectSection::getRenderOperation(RenderOperation& op)
    {
        // direct copy
        op = mRenderOperation;
    }
    //-----------------------------------------------------------------------------
    void ManualObject::ManualObjectSection::getWorldTransforms(Matrix4* xform) const
    {
        xform[0] = mParent->_getParentNodeFullTransform();
    }
    //-----------------------------------------------------------------------------
    auto ManualObject::ManualObjectSection::getSquaredViewDepth(const Ogre::Camera *cam) const -> Real
    {
        Node* n = mParent->getParentNode();
        return n ? n->getSquaredViewDepth(cam) : 0;
    }
    //-----------------------------------------------------------------------------
    auto ManualObject::ManualObjectSection::getLights() const noexcept -> const LightList&
    {
        return mParent->queryLights();
    }
    //-----------------------------------------------------------------------------
    void ManualObject::ManualObjectSection::convertToSubMesh(SubMesh* sm) const
    {
        sm->useSharedVertices = false;
        sm->operationType = mRenderOperation.operationType;
        // Copy vertex data; replicate buffers too
        sm->vertexData.reset(mRenderOperation.vertexData->clone(true));

        // Copy index data; replicate buffers too; delete the default, old one to avoid memory leaks

        // check if index data is present
        if (mRenderOperation.indexData)
        {
            // Copy index data; replicate buffers too; delete the default, old one to avoid memory leaks
            sm->indexData.reset(mRenderOperation.indexData->clone(true));
        }
    }
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    std::string_view const constinit ManualObjectFactory::FACTORY_TYPE_NAME = "ManualObject";
    //-----------------------------------------------------------------------------
    auto ManualObjectFactory::getType() const noexcept -> std::string_view
    {
        return FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------------
    auto ManualObjectFactory::createInstanceImpl(
        std::string_view name, const NameValuePairList* params) -> MovableObject*
    {
        return new ManualObject(name);
    }
}
