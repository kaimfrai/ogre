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

import :Config;
import :Exception;
import :HardwareBufferManager;
import :HardwareIndexBuffer;
import :HardwareVertexBuffer;
import :Root;
import :VertexIndexData;

import <algorithm>;
import <iterator>;
import <list>;
import <map>;
import <memory>;
import <set>;
import <utility>;

namespace Ogre {
    //-----------------------------------------------------------------------
    VertexData::VertexData(HardwareBufferManagerBase* mgr)
    {
        mMgr = mgr ? mgr : HardwareBufferManager::getSingletonPtr();
        vertexBufferBinding = mMgr->createVertexBufferBinding();
        vertexDeclaration = mMgr->createVertexDeclaration();
        mDeleteDclBinding = true;
        vertexCount = 0;
        vertexStart = 0;
        hwAnimDataItemsUsed = 0;

    }
    //---------------------------------------------------------------------
    VertexData::VertexData(VertexDeclaration* dcl, VertexBufferBinding* bind)
    {
        // this is a fallback rather than actively used
        mMgr = HardwareBufferManager::getSingletonPtr();
        vertexDeclaration = dcl;
        vertexBufferBinding = bind;
        mDeleteDclBinding = false;
        vertexCount = 0;
        vertexStart = 0;
        hwAnimDataItemsUsed = 0;
    }
    //-----------------------------------------------------------------------
    VertexData::~VertexData()
    {
        if (mDeleteDclBinding)
        {
            mMgr->destroyVertexBufferBinding(vertexBufferBinding);
            mMgr->destroyVertexDeclaration(vertexDeclaration);
        }
    }
    //-----------------------------------------------------------------------
    auto VertexData::clone(bool copyData, HardwareBufferManagerBase* mgr) const -> VertexData*
    {
        HardwareBufferManagerBase* pManager = mgr ? mgr : mMgr;

        auto* dest = new VertexData(mgr);

        // Copy vertex buffers in turn
        const VertexBufferBinding::VertexBufferBindingMap& bindings = 
            this->vertexBufferBinding->getBindings();
        for (auto const& [key, srcbuf]: bindings)
        {
            HardwareVertexBufferSharedPtr dstBuf;
            if (copyData)
            {
                // create new buffer with the same settings
                dstBuf = pManager->createVertexBuffer(
                        srcbuf->getVertexSize(), srcbuf->getNumVertices(), srcbuf->getUsage(),
                        srcbuf->hasShadowBuffer());

                // copy data
                dstBuf->copyData(*srcbuf, 0, 0, srcbuf->getSizeInBytes(), true);
            }
            else
            {
                // don't copy, point at existing buffer
                dstBuf = srcbuf;
            }

            // Copy binding
            dest->vertexBufferBinding->setBinding(key, dstBuf);
        }

        // Basic vertex info
        dest->vertexStart = this->vertexStart;
        dest->vertexCount = this->vertexCount;
        // Copy elements
        const VertexDeclaration::VertexElementList elems = 
            this->vertexDeclaration->getElements();
        for (const auto & elem : elems)
        {
            dest->vertexDeclaration->addElement(
                elem.getSource(),
                elem.getOffset(),
                elem.getType(),
                elem.getSemantic(),
                elem.getIndex() );
        }

        // Copy reference to hardware shadow buffer, no matter whether copy data or not
        dest->hardwareShadowVolWBuffer = hardwareShadowVolWBuffer;

        // copy anim data
        dest->hwAnimationDataList = hwAnimationDataList;
        dest->hwAnimDataItemsUsed = hwAnimDataItemsUsed;

        
        return dest;
    }
    //-----------------------------------------------------------------------
    void VertexData::prepareForShadowVolume()
    {
        /* NOTE
        I would dearly, dearly love to just use a 4D position buffer in order to 
        store the extra 'w' value I need to differentiate between extruded and 
        non-extruded sections of the buffer, so that vertex programs could use that.
        Hey, it works fine for GL. However, D3D9 in it's infinite stupidity, does not
        support 4d position vertices in the fixed-function pipeline. If you use them, 
        you just see nothing. Since we can't know whether the application is going to use
        fixed function or vertex programs, we have to stick to 3d position vertices and
        store the 'w' in a separate 1D texture coordinate buffer, which is only used
        when rendering the shadow.
        */

        // Upfront, lets check whether we have vertex program capability
        RenderSystem* rend = Root::getSingleton().getRenderSystem();
        bool useVertexPrograms = rend;


        // Look for a position element
        const VertexElement* posElem = vertexDeclaration->findElementBySemantic(VertexElementSemantic::POSITION);
        if (posElem)
        {
            size_t v;
            unsigned short posOldSource = posElem->getSource();

            HardwareVertexBufferSharedPtr vbuf = vertexBufferBinding->getBuffer(posOldSource);
            bool wasSharedBuffer = false;
            // Are there other elements in the buffer except for the position?
            if (vbuf->getVertexSize() > posElem->getSize())
            {
                // We need to create another buffer to contain the remaining elements
                // Most drivers don't like gaps in the declaration, and in any case it's waste
                wasSharedBuffer = true;
            }
            HardwareVertexBufferSharedPtr newPosBuffer, newRemainderBuffer;
            if (wasSharedBuffer)
            {
                newRemainderBuffer = vbuf->getManager()->createVertexBuffer(
                    vbuf->getVertexSize() - posElem->getSize(), vbuf->getNumVertices(), vbuf->getUsage(),
                    vbuf->hasShadowBuffer());
            }
            // Allocate new position buffer, will be FLOAT3 and 2x the size
            size_t oldVertexCount = vbuf->getNumVertices();
            size_t newVertexCount = oldVertexCount * 2;
            newPosBuffer = vbuf->getManager()->createVertexBuffer(
                VertexElement::getTypeSize(VertexElementType::FLOAT3), newVertexCount, vbuf->getUsage(), 
                vbuf->hasShadowBuffer());

            // Iterate over the old buffer, copying the appropriate elements and initialising the rest
            float* pSrc;
            auto *pBaseSrc = static_cast<unsigned char*>(
                vbuf->lock(HardwareBuffer::LockOptions::READ_ONLY));
            // Point first destination pointer at the start of the new position buffer,
            // the other one half way along
            auto *pDest = static_cast<float*>(newPosBuffer->lock(HardwareBuffer::LockOptions::DISCARD));
            float* pDest2 = pDest + oldVertexCount * 3; 

            // Precalculate any dimensions of vertex areas outside the position
            size_t prePosVertexSize = 0;
            unsigned char *pBaseDestRem = nullptr;
            if (wasSharedBuffer)
            {
                size_t postPosVertexSize, postPosVertexOffset;
                pBaseDestRem = static_cast<unsigned char*>(
                    newRemainderBuffer->lock(HardwareBuffer::LockOptions::DISCARD));
                prePosVertexSize = posElem->getOffset();
                postPosVertexOffset = prePosVertexSize + posElem->getSize();
                postPosVertexSize = vbuf->getVertexSize() - postPosVertexOffset;
                // the 2 separate bits together should be the same size as the remainder buffer vertex
                assert (newRemainderBuffer->getVertexSize() == prePosVertexSize + postPosVertexSize);

                // Iterate over the vertices
                for (v = 0; v < oldVertexCount; ++v)
                {
                    // Copy position, into both buffers
                    posElem->baseVertexPointerToElement(pBaseSrc, &pSrc);
                    *pDest++ = *pDest2++ = *pSrc++;
                    *pDest++ = *pDest2++ = *pSrc++;
                    *pDest++ = *pDest2++ = *pSrc++;

                    // now deal with any other elements 
                    // Basically we just memcpy the vertex excluding the position
                    if (prePosVertexSize > 0)
                        memcpy(pBaseDestRem, pBaseSrc, prePosVertexSize);
                    if (postPosVertexSize > 0)
                        memcpy(pBaseDestRem + prePosVertexSize, 
                            pBaseSrc + postPosVertexOffset, postPosVertexSize);
                    pBaseDestRem += newRemainderBuffer->getVertexSize();

                    pBaseSrc += vbuf->getVertexSize();

                } // next vertex
            }
            else
            {
                // Unshared buffer, can block copy the whole thing
                memcpy(pDest, pBaseSrc, vbuf->getSizeInBytes());
                memcpy(pDest2, pBaseSrc, vbuf->getSizeInBytes());
            }

            vbuf->unlock();
            newPosBuffer->unlock();
            if (wasSharedBuffer)
                newRemainderBuffer->unlock();

            // At this stage, he original vertex buffer is going to be destroyed
            // So we should force the deallocation of any temporary copies
            vbuf->getManager()->_forceReleaseBufferCopies(vbuf);

            if (useVertexPrograms)
            {
                // Now it's time to set up the w buffer
                hardwareShadowVolWBuffer = vbuf->getManager()->createVertexBuffer(
                    sizeof(float), newVertexCount, HardwareBuffer::STATIC_WRITE_ONLY, false);
                // Fill the first half with 1.0, second half with 0.0
                pDest = static_cast<float*>(
                    hardwareShadowVolWBuffer->lock(HardwareBuffer::LockOptions::DISCARD));
                for (v = 0; v < oldVertexCount; ++v)
                {
                    *pDest++ = 1.0f;
                }
                for (v = 0; v < oldVertexCount; ++v)
                {
                    *pDest++ = 0.0f;
                }
                hardwareShadowVolWBuffer->unlock();
            }

            unsigned short newPosBufferSource; 
            if (wasSharedBuffer)
            {
                // Get the a new buffer binding index
                newPosBufferSource= vertexBufferBinding->getNextIndex();
                // Re-bind the old index to the remainder buffer
                vertexBufferBinding->setBinding(posOldSource, newRemainderBuffer);
            }
            else
            {
                // We can just re-use the same source idex for the new position buffer
                newPosBufferSource = posOldSource;
            }
            // Bind the new position buffer
            vertexBufferBinding->setBinding(newPosBufferSource, newPosBuffer);

            // Now, alter the vertex declaration to change the position source
            // and the offsets of elements using the same buffer
            for(unsigned short idx = 0;
                auto const& elemi : vertexDeclaration->getElements())
            {
                if (&elemi == posElem)
                {
                    // Modify position to point at new position buffer
                    vertexDeclaration->modifyElement(
                        idx, 
                        newPosBufferSource, // new source buffer
                        0, // no offset now
                        VertexElementType::FLOAT3, 
                        VertexElementSemantic::POSITION);
                }
                else if (wasSharedBuffer &&
                    elemi.getSource() == posOldSource &&
                    elemi.getOffset() > prePosVertexSize )
                {
                    // This element came after position, remove the position's
                    // size
                    vertexDeclaration->modifyElement(
                        idx, 
                        posOldSource, // same old source
                        elemi.getOffset() - posElem->getSize(), // less offset now
                        elemi.getType(),
                        elemi.getSemantic(),
                        elemi.getIndex());

                }

                ++idx;
            }


            // Note that we don't change vertexCount, because the other buffer(s) are still the same
            // size after all


        }
    }
    //-----------------------------------------------------------------------
    void VertexData::reorganiseBuffers(VertexDeclaration* newDeclaration, 
        const BufferUsageList& bufferUsages, HardwareBufferManagerBase* mgr)
    {
        HardwareBufferManagerBase* pManager = mgr ? mgr : mMgr;
        // Firstly, close up any gaps in the buffer sources which might have arisen
        newDeclaration->closeGapsInSource();

        // Build up a list of both old and new elements in each buffer
        std::vector<void*> oldBufferLocks;
        std::vector<size_t> oldBufferVertexSizes;
        std::vector<void*> newBufferLocks;
        std::vector<size_t> newBufferVertexSizes;
        VertexBufferBinding* newBinding = pManager->createVertexBufferBinding();
        const VertexBufferBinding::VertexBufferBindingMap& oldBindingMap = vertexBufferBinding->getBindings();

        // Pre-allocate old buffer locks
        if (!oldBindingMap.empty())
        {
            size_t count = oldBindingMap.rbegin()->first + 1;
            oldBufferLocks.resize(count);
            oldBufferVertexSizes.resize(count);
        }

        bool useShadowBuffer = false;

        // Lock all the old buffers for reading
        for (auto const& [key, value] : oldBindingMap)
        {
            assert(value->getNumVertices() >= vertexCount);

            oldBufferVertexSizes[key] =
                value->getVertexSize();
            oldBufferLocks[key] =
                value->lock(
                    HardwareBuffer::LockOptions::READ_ONLY);

            useShadowBuffer |= value->hasShadowBuffer();
        }
        
        // Create new buffers and lock all for writing
        for (unsigned short buf = 0; !newDeclaration->findElementsBySource(buf).empty(); ++buf)
        {
            size_t vertexSize = newDeclaration->getVertexSize(buf);

            HardwareVertexBufferSharedPtr vbuf = 
                pManager->createVertexBuffer(
                    vertexSize,
                    vertexCount, 
                    bufferUsages[buf], useShadowBuffer);
            newBinding->setBinding(buf, vbuf);

            newBufferVertexSizes.push_back(vertexSize);
            newBufferLocks.push_back(
                vbuf->lock(HardwareBuffer::LockOptions::DISCARD));
        }

        // Map from new to old elements
        using NewToOldElementMap = std::map<const VertexElement *, const VertexElement *>;
        NewToOldElementMap newToOldElementMap;
        const VertexDeclaration::VertexElementList& newElemList = newDeclaration->getElements();
        for (const auto & ei : newElemList)
        {
            // Find corresponding old element
            const VertexElement* oldElem = 
                vertexDeclaration->findElementBySemantic(
                    ei.getSemantic(), ei.getIndex());
            if (!oldElem)
            {
                // Error, cannot create new elements with this method
                OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, 
                    "Element not found in old vertex declaration", 
                    "VertexData::reorganiseBuffers");
            }
            newToOldElementMap[&ei] = oldElem;
        }
        // Now iterate over the new buffers, pulling data out of the old ones
        // For each vertex
        for (size_t v = 0; v < vertexCount; ++v)
        {
            // For each (new) element
            for (const auto & ei : newElemList)
            {
                const VertexElement* newElem = &ei;
                auto noi = newToOldElementMap.find(newElem);
                const VertexElement* oldElem = noi->second;
                unsigned short oldBufferNo = oldElem->getSource();
                unsigned short newBufferNo = newElem->getSource();
                void* pSrcBase = static_cast<void*>(
                    static_cast<unsigned char*>(oldBufferLocks[oldBufferNo])
                    + v * oldBufferVertexSizes[oldBufferNo]);
                void* pDstBase = static_cast<void*>(
                    static_cast<unsigned char*>(newBufferLocks[newBufferNo])
                    + v * newBufferVertexSizes[newBufferNo]);
                void *pSrc, *pDst;
                oldElem->baseVertexPointerToElement(pSrcBase, &pSrc);
                newElem->baseVertexPointerToElement(pDstBase, &pDst);
                
                memcpy(pDst, pSrc, newElem->getSize());
                
            }
        }

        // Unlock all buffers
        for (const auto & itBinding : oldBindingMap)
        {
            itBinding.second->unlock();
        }
        for (unsigned short buf = 0; buf < newBinding->getBufferCount(); ++buf)
        {
            newBinding->getBuffer(buf)->unlock();
        }

        // Delete old binding & declaration
        if (mDeleteDclBinding)
        {
            pManager->destroyVertexBufferBinding(vertexBufferBinding);
            pManager->destroyVertexDeclaration(vertexDeclaration);
        }

        // Assign new binding and declaration
        vertexDeclaration = newDeclaration;
        vertexBufferBinding = newBinding;       
        // after this is complete, new manager should be used
        mMgr = pManager;
        mDeleteDclBinding = true; // because we created these through a manager

    }
    //-----------------------------------------------------------------------
    void VertexData::reorganiseBuffers(VertexDeclaration* newDeclaration, HardwareBufferManagerBase* mgr)
    {
        // Derive the buffer usages from looking at where the source has come
        // from
        BufferUsageList usages;
        for (unsigned short b = 0; b <= newDeclaration->getMaxSource(); ++b)
        {
            VertexDeclaration::VertexElementList destElems = newDeclaration->findElementsBySource(b);
            // Initialise with most restrictive version
            auto final = HardwareBuffer::STATIC_WRITE_ONLY;
            for (VertexElement& destelem : destElems)
            {
                // get source
                const VertexElement* srcelem =
                    vertexDeclaration->findElementBySemantic(
                        destelem.getSemantic(), destelem.getIndex());
                // get buffer
                HardwareVertexBufferSharedPtr srcbuf = 
                    vertexBufferBinding->getBuffer(srcelem->getSource());
                // improve flexibility only
                if (!!(srcbuf->getUsage() & HardwareBuffer::DYNAMIC))
                {
                    // remove static
                    final &= ~HardwareBuffer::STATIC;
                    // add dynamic
                    final |= HardwareBuffer::DYNAMIC;
                }
                if (!(srcbuf->getUsage() & HardwareBufferUsage::DETAIL_WRITE_ONLY))
                {
                    // remove write only
                    final &= ~HardwareBufferUsage::DETAIL_WRITE_ONLY;
                }
            }
            usages.push_back(final);
        }
        // Call specific method
        reorganiseBuffers(newDeclaration, usages, mgr);

    }
    //-----------------------------------------------------------------------
    void VertexData::closeGapsInBindings()
    {
        if (!vertexBufferBinding->hasGaps())
            return;

        // Check for error first
        const VertexDeclaration::VertexElementList& allelems = 
            vertexDeclaration->getElements();
        for (const auto & elem : allelems)
        {
            if (!vertexBufferBinding->isBufferBound(elem.getSource()))
            {
                OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                    "No buffer is bound to that element source.",
                    "VertexData::closeGapsInBindings");
            }
        }

        // Close gaps in the vertex buffer bindings
        VertexBufferBinding::BindingIndexMap bindingIndexMap;
        vertexBufferBinding->closeGaps(bindingIndexMap);

        // Modify vertex elements to reference to new buffer index
        for (unsigned short elemIndex = 0;
             const VertexElement& elem : allelems)
        {
            auto it =
                bindingIndexMap.find(elem.getSource());
            assert(it != bindingIndexMap.end());
            ushort targetSource = it->second;
            if (elem.getSource() != targetSource)
            {
                vertexDeclaration->modifyElement(elemIndex, 
                    targetSource, elem.getOffset(), elem.getType(), 
                    elem.getSemantic(), elem.getIndex());
            }
            ++elemIndex;
        }
    }
    //-----------------------------------------------------------------------
    void VertexData::removeUnusedBuffers()
    {
        std::set<ushort> usedBuffers;

        // Collect used buffers
        const VertexDeclaration::VertexElementList& allelems = 
            vertexDeclaration->getElements();
        for (const auto & elem : allelems)
        {
            usedBuffers.insert(elem.getSource());
        }

        // Unset unused buffer bindings
        ushort count = vertexBufferBinding->getLastBoundIndex();
        for (ushort index = 0; index < count; ++index)
        {
            if (usedBuffers.find(index) == usedBuffers.end() &&
                vertexBufferBinding->isBufferBound(index))
            {
                vertexBufferBinding->unsetBinding(index);
            }
        }

        // Close gaps
        closeGapsInBindings();
    }
    //-----------------------------------------------------------------------
    void VertexData::convertPackedColour(VertexElementType, VertexElementType destType)
    {
        OgreAssert(destType == VertexElementType::UBYTE4_NORM, "Not supported");

        const VertexBufferBinding::VertexBufferBindingMap& bindMap = 
            vertexBufferBinding->getBindings();
        for (auto const& [key, value] : bindMap)
        {
            VertexDeclaration::VertexElementList elems = 
                vertexDeclaration->findElementsBySource(key);
            bool conversionNeeded = false;
            for (auto & elem : elems)
            {
                if (elem.getType() == VertexElementType::_DETAIL_SWAP_RB)
                {
                    conversionNeeded = true;
                }
            }

            if (conversionNeeded)
            {
                void* pBase = value->lock(HardwareBuffer::LockOptions::NORMAL);

                for (size_t v = 0; v < value->getNumVertices(); ++v)
                {

                    for (auto & elem : elems)
                    {
                        if (elem.getType() == VertexElementType::_DETAIL_SWAP_RB)
                        {
                            uint32* pRGBA;
                            elem.baseVertexPointerToElement(pBase, &pRGBA);
                            VertexElement::convertColourValue(VertexElementType::_DETAIL_SWAP_RB, destType, pRGBA);
                        }
                    }
                    pBase = static_cast<void*>(
                        static_cast<char*>(pBase) + value->getVertexSize());
                }
                value->unlock();

                // Modify the elements to reflect the changed type
                const VertexDeclaration::VertexElementList& allelems = 
                    vertexDeclaration->getElements();
                unsigned short elemIndex = 0;
                for (const auto & elem : allelems)
                {
                    if (elem.getType() == VertexElementType::_DETAIL_SWAP_RB)
                    {
                        vertexDeclaration->modifyElement(elemIndex, 
                            elem.getSource(), elem.getOffset(), destType, 
                            elem.getSemantic(), elem.getIndex());
                    }
                    ++elemIndex;
                }

            }


        } // each buffer


    }
    //-----------------------------------------------------------------------
    auto VertexData::allocateHardwareAnimationElements(ushort count, bool animateNormals) -> ushort
    {
        // Find first free texture coord set
        unsigned short texCoord = vertexDeclaration->getNextFreeTextureCoordinate();
        auto freeCount = (ushort)(OGRE_MAX_TEXTURE_COORD_SETS - texCoord);
        if (animateNormals)
            // we need 2x the texture coords, round down
            freeCount /= 2;
        
        unsigned short supportedCount = std::min(freeCount, count);
        
        // Increase to correct size
        for (size_t c = hwAnimationDataList.size(); c < supportedCount; ++c)
        {
            // Create a new 3D texture coordinate set
            HardwareAnimationData data;
            data.targetBufferIndex = vertexBufferBinding->getNextIndex();
            vertexDeclaration->addElement(data.targetBufferIndex, 0, VertexElementType::FLOAT3, VertexElementSemantic::TEXTURE_COORDINATES, texCoord++);
            if (animateNormals)
                    vertexDeclaration->addElement(data.targetBufferIndex, sizeof(float)*3, VertexElementType::FLOAT3, VertexElementSemantic::TEXTURE_COORDINATES, texCoord++);

            hwAnimationDataList.push_back(data);
            // Vertex buffer will not be bound yet, we expect this to be done by the
            // caller when it becomes appropriate (e.g. through a VertexAnimationTrack)
        }
        
        return supportedCount;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    IndexData::IndexData()
    {
        indexCount = 0;
        indexStart = 0;
        
    }
    //-----------------------------------------------------------------------
    IndexData::~IndexData()
    = default;
    //-----------------------------------------------------------------------
    auto IndexData::clone(bool copyData, HardwareBufferManagerBase* mgr) const -> IndexData*
    {
        HardwareBufferManagerBase* pManager = mgr ? mgr : HardwareBufferManager::getSingletonPtr();
        auto* dest = new IndexData();
        if (indexBuffer.get())
        {
            if (copyData)
            {
                dest->indexBuffer = pManager->createIndexBuffer(indexBuffer->getType(), indexBuffer->getNumIndexes(),
                    indexBuffer->getUsage(), indexBuffer->hasShadowBuffer());
                dest->indexBuffer->copyData(*indexBuffer, 0, 0, indexBuffer->getSizeInBytes(), true);
            }
            else
            {
                dest->indexBuffer = indexBuffer;
            }
        }
        dest->indexCount = indexCount;
        dest->indexStart = indexStart;
        return dest;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // Local Utility class for vertex cache optimizer
    class Triangle
    {
    public:
        enum EdgeMatchType {
            AB, BC, CA, ANY, NONE
        };

        uint32 a, b, c;     

        inline Triangle()
        = default;

        inline Triangle( uint32 ta, uint32 tb, uint32 tc ) 
            : a( ta ), b( tb ), c( tc )
        {
        }

        inline Triangle( uint32 t[3] )
            : a( t[0] ), b( t[1] ), c( t[2] )
        {
        }

        inline Triangle( const Triangle& t )
             
        = default;

        inline auto operator=(const Triangle& rhs) -> Triangle& = default;

        [[nodiscard]] inline auto sharesEdge(const Triangle& t) const -> bool
        {
            return( (a == t.a && b == t.c) ||
                    (a == t.b && b == t.a) ||
                    (a == t.c && b == t.b) ||
                    (b == t.a && c == t.c) ||
                    (b == t.b && c == t.a) ||
                    (b == t.c && c == t.b) ||
                    (c == t.a && a == t.c) ||
                    (c == t.b && a == t.a) ||
                    (c == t.c && a == t.b) );
        }

        [[nodiscard]] inline auto sharesEdge(const uint32 ea, const uint32 eb, const Triangle& t) const -> bool
        {
            return( (ea == t.a && eb == t.c) ||
                    (ea == t.b && eb == t.a) ||
                    (ea == t.c && eb == t.b) ); 
        }

        [[nodiscard]] inline auto sharesEdge(const EdgeMatchType edge, const Triangle& t) const -> bool
        {
            if (edge == AB)
                return sharesEdge(a, b, t);
            else if (edge == BC)
                return sharesEdge(b, c, t);
            else if (edge == CA)
                return sharesEdge(c, a, t);
            else
                return (edge == ANY) == sharesEdge(t);
        }

        [[nodiscard]] inline auto endoSharedEdge(const Triangle& t) const -> EdgeMatchType
        {
            if (sharesEdge(a, b, t)) return AB;
            if (sharesEdge(b, c, t)) return BC;
            if (sharesEdge(c, a, t)) return CA;
            return NONE;
        }

        [[nodiscard]] inline auto exoSharedEdge(const Triangle& t) const -> EdgeMatchType
        {
            return t.endoSharedEdge(*this);
        }

        inline void shiftClockwise()
        {
            uint32 t = a;
            a = c;
            c = b;
            b = t;
        }

        inline void shiftCounterClockwise()
        {
            uint32 t = a;
            a = b;
            b = c;
            c = t;
        }
    };
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void IndexData::optimiseVertexCacheTriList()
    {
        if (indexBuffer->isLocked()) return;

        void *buffer = indexBuffer->lock(HardwareBuffer::LockOptions::NORMAL);

        Triangle* triangles;

        size_t nIndexes = indexCount;
        size_t nTriangles = nIndexes / 3;
        size_t i, j;
        uint16 *source = nullptr;

        if (indexBuffer->getType() == HardwareIndexBuffer::IndexType::_16BIT)
        {
            triangles = new Triangle[nTriangles];
            source = (uint16 *)buffer;
            auto *dest = (uint32 *)triangles;
            for (i = 0; i < nIndexes; ++i) dest[i] = source[i];
        }
        else
            triangles = static_cast<Triangle*>(buffer);

        // sort triangles based on shared edges
        auto *destlist = new uint32[nTriangles];
        auto *visited = new unsigned char[nTriangles];

        for (i = 0; i < nTriangles; ++i) visited[i] = 0;

        uint32 start = 0, ti = 0, destcount = 0;

        bool found = false;
        for (i = 0; i < nTriangles; ++i)
        {
            if (found)
                found = false;
            else
            {
                while (visited[start++]);
                ti = start - 1;
            }

            destlist[destcount++] = ti;
            visited[ti] = 1;

            for (j = start; j < nTriangles; ++j)
            {
                if (visited[j]) continue;
                
                if (triangles[ti].sharesEdge(triangles[j]))
                {
                    found = true;
                    ti = static_cast<uint32>(j);
                    break;
                }
            }
        }

        if (indexBuffer->getType() == HardwareIndexBuffer::IndexType::_16BIT)
        {
            // reorder the indexbuffer
            j = 0;
            for (i = 0; i < nTriangles; ++i)
            {
                Triangle *t = &triangles[destlist[i]];
                if(source)
                {
                    source[j++] = (uint16)t->a;
                    source[j++] = (uint16)t->b;
                    source[j++] = (uint16)t->c;
                }
            }
            delete [] triangles;
        }
        else
        {
            auto *reflist = new uint32[nTriangles];

            // fill the referencebuffer
            for (i = 0; i < nTriangles; ++i)
                reflist[destlist[i]] = static_cast<uint32>(i);
            
            // reorder the indexbuffer
            for (i = 0; i < nTriangles; ++i)
            {
                j = destlist[i];
                if (i == j) continue; // do not move triangle

                // swap triangles

                Triangle t = triangles[i];
                triangles[i] = triangles[j];
                triangles[j] = t;

                // change reference
                destlist[reflist[i]] = static_cast<uint32>(j);
                // destlist[i] = i; // not needed, it will not be used
            }

            delete[] reflist;
        }

        delete[] destlist;
        delete[] visited;
                    
        indexBuffer->unlock();
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void VertexCacheProfiler::profile(const HardwareIndexBufferSharedPtr& indexBuffer)
    {
        if (indexBuffer->isLocked()) return;

        auto *shortbuffer = (uint16 *)indexBuffer->lock(HardwareBuffer::LockOptions::READ_ONLY);

        if (indexBuffer->getType() == HardwareIndexBuffer::IndexType::_16BIT)
            for (unsigned int i = 0; i < indexBuffer->getNumIndexes(); ++i)
                inCache(shortbuffer[i]);
        else
        {
            auto *buffer = (uint32 *)shortbuffer;
            for (unsigned int i = 0; i < indexBuffer->getNumIndexes(); ++i)
                inCache(buffer[i]);
        }

        indexBuffer->unlock();
    }

    //-----------------------------------------------------------------------
    auto VertexCacheProfiler::inCache(unsigned int index) -> bool
    {
        for (unsigned int i = 0; i < buffersize; ++i)
        {
            if (index == cache[i])
            {
                hit++;
                return true;
            }
        }

        miss++;
        cache[tail++] = index;
        tail %= size;

        if (buffersize < size) buffersize++;

        return false;
    }
    

}
