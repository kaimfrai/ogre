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

#include "glad/glad.h"
#include <cassert>

module Ogre.RenderSystems.GL;

import :HardwareBuffer;
import :HardwareBufferManager;
import :RenderSystem;
import :RenderToVertexBuffer;

import Ogre.Core;

import <memory>;

namespace Ogre {
    //-----------------------------------------------------------------------
    // Scratch pool management (32 bit structure)
    struct GLScratchBufferAlloc
    {
        /// Size in bytes
        uint32 size: 31;
        /// Free? (pack with size)
        uint32 free: 1;
    };
    #define SCRATCH_POOL_SIZE 1 * 1024 * 1024
    //---------------------------------------------------------------------
    GLHardwareBufferManager::GLHardwareBufferManager()
        :  mMapBufferThreshold(OGRE_GL_DEFAULT_MAP_BUFFER_THRESHOLD)
    {
        mRenderSystem = static_cast<GLRenderSystem*>(Root::getSingleton().getRenderSystem());

        // Init scratch pool
        // TODO make it a configurable size?
        // 32-bit aligned buffer
        mScratchBufferPool = static_cast<char*>(::Ogre::AlignedMemory::allocate(SCRATCH_POOL_SIZE));
        auto* ptrAlloc = (GLScratchBufferAlloc*)mScratchBufferPool;
        ptrAlloc->size = SCRATCH_POOL_SIZE - sizeof(GLScratchBufferAlloc);
        ptrAlloc->free = 1;
        mMapBufferThreshold = 0;
    }
    //-----------------------------------------------------------------------
    GLHardwareBufferManager::~GLHardwareBufferManager()
    {
        destroyAllDeclarations();
        destroyAllBindings();

        ::Ogre::AlignedMemory::deallocate(mScratchBufferPool);
    }
    //-----------------------------------------------------------------------
    auto GLHardwareBufferManager::getStateCacheManager() noexcept -> GLStateCacheManager *
    {
        return mRenderSystem->_getStateCacheManager();
    }
    //-----------------------------------------------------------------------
    auto GLHardwareBufferManager::createVertexBuffer(
        size_t vertexSize, size_t numVerts, HardwareBuffer::Usage usage, bool useShadowBuffer) -> HardwareVertexBufferSharedPtr
    {
        auto impl = new GLHardwareVertexBuffer(GL_ARRAY_BUFFER, vertexSize * numVerts, usage, useShadowBuffer);
        auto buf = std::make_shared<HardwareVertexBuffer>(this, vertexSize, numVerts, impl);
        {
            mVertexBuffers.insert(buf.get());
        }
        return buf;
    }
    //-----------------------------------------------------------------------
    auto 
    GLHardwareBufferManager::createIndexBuffer(
        HardwareIndexBuffer::IndexType itype, size_t numIndexes, 
        HardwareBuffer::Usage usage, bool useShadowBuffer) -> HardwareIndexBufferSharedPtr
    {
        // Calculate the size of the indexes
        auto indexSize = HardwareIndexBuffer::indexSize(itype);
        auto impl = new GLHardwareBuffer(GL_ELEMENT_ARRAY_BUFFER, indexSize * numIndexes, usage, useShadowBuffer);

        return std::make_shared<HardwareIndexBuffer>(this, itype, numIndexes, impl);
    }
    //---------------------------------------------------------------------
    auto 
        GLHardwareBufferManager::createRenderToVertexBuffer() -> RenderToVertexBufferSharedPtr
    {
        return RenderToVertexBufferSharedPtr(new GLRenderToVertexBuffer);
    }
    //---------------------------------------------------------------------
    auto GLHardwareBufferManager::getGLUsage(HardwareBufferUsage usage) -> GLenum
    {
        return (usage == HardwareBufferUsage::GPU_TO_CPU) ? GL_STATIC_READ
                                         : (usage == HardwareBufferUsage::GPU_ONLY) ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
    }
    //---------------------------------------------------------------------
    auto GLHardwareBufferManager::getGLType(VertexElementType type) -> GLenum
    {
        using enum VertexElementType;
        switch(type)
        {
            case FLOAT1:
            case FLOAT2:
            case FLOAT3:
            case FLOAT4:
                return GL_FLOAT;
            case SHORT1:
            case SHORT2:
            case SHORT3:
            case SHORT4:
            case SHORT2_NORM:
            case SHORT4_NORM:
                return GL_SHORT;
            case UBYTE4:
            case UBYTE4_NORM:
                return GL_UNSIGNED_BYTE;
            case BYTE4:
            case BYTE4_NORM:
                return GL_BYTE;
            case USHORT2_NORM:
            case USHORT4_NORM:
                return GL_UNSIGNED_SHORT;
            default:
                return 0;
        };
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    auto GLHardwareBufferManager::allocateScratch(uint32 size) -> void*
    {
        // simple forward link search based on alloc sizes
        // not that fast but the list should never get that long since not many
        // locks at once (hopefully)

        // Alignment - round up the size to 32 bits
        // control blocks are 32 bits too so this packs nicely
        if (size % 4 != 0)
        {
            size += 4 - (size % 4);
        }

        uint32 bufferPos = 0;
        while (bufferPos < SCRATCH_POOL_SIZE)
        {
            auto* pNext = (GLScratchBufferAlloc*)(mScratchBufferPool + bufferPos);
            // Big enough?
            if (pNext->free && pNext->size >= size)
            {
                // split? And enough space for control block
                if(pNext->size > size + sizeof(GLScratchBufferAlloc))
                {
                    uint32 offset = (uint32)sizeof(GLScratchBufferAlloc) + size;

                    auto* pSplitAlloc = (GLScratchBufferAlloc*)
                        (mScratchBufferPool + bufferPos + offset);
                    pSplitAlloc->free = 1;
                    // split size is remainder minus new control block
                    pSplitAlloc->size = pNext->size - size - sizeof(GLScratchBufferAlloc);

                    // New size of current
                    pNext->size = size;
                }
                // allocate and return
                pNext->free = 0;

                // return pointer just after this control block (++ will do that for us)
                return ++pNext;

            }

            bufferPos += (uint32)sizeof(GLScratchBufferAlloc) + pNext->size;

        }

        // no available alloc
        return nullptr;

    }
    //---------------------------------------------------------------------
    void GLHardwareBufferManager::deallocateScratch(void* ptr)
    {
        // Simple linear search dealloc
        uint32 bufferPos = 0;
        GLScratchBufferAlloc* pLast = nullptr;
        while (bufferPos < SCRATCH_POOL_SIZE)
        {
            auto* pCurrent = (GLScratchBufferAlloc*)(mScratchBufferPool + bufferPos);
            
            // Pointers match?
            if ((mScratchBufferPool + bufferPos + sizeof(GLScratchBufferAlloc))
                == ptr)
            {
                // dealloc
                pCurrent->free = 1;
                
                // merge with previous
                if (pLast && pLast->free)
                {
                    // adjust buffer pos
                    bufferPos -= (pLast->size + (uint32)sizeof(GLScratchBufferAlloc));
                    // merge free space
                    pLast->size += pCurrent->size + sizeof(GLScratchBufferAlloc);
                    pCurrent = pLast;
                }

                // merge with next
                uint32 offset = bufferPos + pCurrent->size + (uint32)sizeof(GLScratchBufferAlloc);
                if (offset < SCRATCH_POOL_SIZE)
                {
                    auto* pNext = (GLScratchBufferAlloc*)(
                        mScratchBufferPool + offset);
                    if (pNext->free)
                    {
                        pCurrent->size += pNext->size + sizeof(GLScratchBufferAlloc);
                    }
                }

                // done
                return;
            }

            bufferPos += (uint32)sizeof(GLScratchBufferAlloc) + pCurrent->size;
            pLast = pCurrent;

        }

        // Should never get here unless there's a corruption
        assert (false && "Memory deallocation error");


    }
    //---------------------------------------------------------------------
    auto GLHardwareBufferManager::getGLMapBufferThreshold() const -> size_t
    {
        return mMapBufferThreshold;
    }
    //---------------------------------------------------------------------
    void GLHardwareBufferManager::setGLMapBufferThreshold( const size_t value )
    {
        mMapBufferThreshold = value;
    }
}
