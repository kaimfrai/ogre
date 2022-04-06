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

#ifndef OGRE_CORE_DEFAULTHARDWAREBUFFERMANAGER_H
#define OGRE_CORE_DEFAULTHARDWAREBUFFERMANAGER_H

#include <cstddef>
#include <memory>

#include "OgreHardwareBuffer.hpp"
#include "OgreHardwareBufferManager.hpp"
#include "OgreHardwareIndexBuffer.hpp"
#include "OgreHardwareVertexBuffer.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSharedPtr.hpp"

namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */

    /// Specialisation of HardwareBuffer for emulation
    class DefaultHardwareBuffer : public HardwareBuffer
    {
    private:
        unsigned char* mData;
        auto lockImpl(size_t offset, size_t length, LockOptions options) -> void* override;
        void unlockImpl() override;
    public:
        DefaultHardwareBuffer(size_t sizeInBytes);
        ~DefaultHardwareBuffer() override;
        void readData(size_t offset, size_t length, void* pDest) override;
        void writeData(size_t offset, size_t length, const void* pSource, bool discardWholeBuffer = false) override;
    };

    typedef DefaultHardwareBuffer DefaultHardwareUniformBuffer;

    class DefaultHardwareVertexBuffer : public HardwareVertexBuffer
    {
    public:
        DefaultHardwareVertexBuffer(size_t vertexSize, size_t numVertices, Usage = HBU_CPU_ONLY)
            : HardwareVertexBuffer(nullptr, vertexSize, numVertices,
                                   new DefaultHardwareBuffer(vertexSize * numVertices))
        {
        }
    };

    class DefaultHardwareIndexBuffer : public HardwareIndexBuffer
    {
    public:
        DefaultHardwareIndexBuffer(IndexType idxType, size_t numIndexes, Usage = HBU_CPU_ONLY)
            : HardwareIndexBuffer(nullptr, idxType, numIndexes,
                                  new DefaultHardwareBuffer(indexSize(idxType) * numIndexes))
        {
        }
    };

    /** Specialisation of HardwareBufferManagerBase to emulate hardware buffers.
    @remarks
        You might want to instantiate this class if you want to utilise
        classes like MeshSerializer without having initialised the 
        rendering system (which is required to create a 'real' hardware
        buffer manager).
    */
    class DefaultHardwareBufferManagerBase : public HardwareBufferManagerBase
    {
    public:
        DefaultHardwareBufferManagerBase();
        ~DefaultHardwareBufferManagerBase() override;
        /// Creates a vertex buffer
        auto 
            createVertexBuffer(size_t vertexSize, size_t numVerts, 
                HardwareBuffer::Usage usage, bool useShadowBuffer = false) -> HardwareVertexBufferSharedPtr override;
        /// Create a hardware index buffer
        auto 
            createIndexBuffer(HardwareIndexBuffer::IndexType itype, size_t numIndexes, 
                HardwareBuffer::Usage usage, bool useShadowBuffer = false) -> HardwareIndexBufferSharedPtr override;
        /// Create a hardware uniform buffer
        auto createUniformBuffer(size_t sizeBytes, HardwareBufferUsage = HBU_CPU_ONLY,
                                              bool = false) -> HardwareBufferPtr override
        {
            return std::make_shared<DefaultHardwareBuffer>(sizeBytes);
        }
    };

    /// DefaultHardwareBufferManager as a Singleton
    class DefaultHardwareBufferManager : public HardwareBufferManager
    {
        std::unique_ptr<HardwareBufferManagerBase> mImpl;
    public:
        DefaultHardwareBufferManager() : mImpl(new DefaultHardwareBufferManagerBase()) {}
        ~DefaultHardwareBufferManager() override
        {
            // have to do this before mImpl is gone
            destroyAllDeclarations();
            destroyAllBindings();
        }

        auto
            createVertexBuffer(size_t vertexSize, size_t numVerts, HardwareBuffer::Usage usage,
            bool useShadowBuffer = false) -> HardwareVertexBufferSharedPtr override
        {
            return mImpl->createVertexBuffer(vertexSize, numVerts, usage, useShadowBuffer);
        }

        auto
            createIndexBuffer(HardwareIndexBuffer::IndexType itype, size_t numIndexes,
            HardwareBuffer::Usage usage, bool useShadowBuffer = false) -> HardwareIndexBufferSharedPtr override
        {
            return mImpl->createIndexBuffer(itype, numIndexes, usage, useShadowBuffer);
        }

        auto createRenderToVertexBuffer() -> RenderToVertexBufferSharedPtr override
        {
            return mImpl->createRenderToVertexBuffer();
        }

        auto createUniformBuffer(size_t sizeBytes, HardwareBufferUsage usage,
                                              bool useShadowBuffer) -> HardwareBufferPtr override
        {
            return mImpl->createUniformBuffer(sizeBytes, usage, useShadowBuffer);
        }
    };

    /** @} */
    /** @} */

}

#endif
