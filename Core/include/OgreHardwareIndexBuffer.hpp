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
#ifndef OGRE_CORE_HARDWAREINDEXBUFFER_H
#define OGRE_CORE_HARDWAREINDEXBUFFER_H

#include <cstddef>

#include "OgreHardwareBuffer.hpp"
#include "OgrePlatform.hpp"

namespace Ogre {
    class HardwareBufferManagerBase;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */
    /** Specialisation of HardwareBuffer for vertex index buffers, still abstract. */
    class HardwareIndexBuffer : public HardwareBuffer
    {
        public:
            enum IndexType : uint8 {
                IT_16BIT,
                IT_32BIT
            };

        private:
            IndexType mIndexType;
            uint8 mIndexSize;
            HardwareBufferManagerBase* mMgr;
            size_t mNumIndexes;
        public:
            /// Should be called by HardwareBufferManager
            HardwareIndexBuffer(HardwareBufferManagerBase* mgr, IndexType idxType, size_t numIndexes,
                                Usage usage, bool useSystemMemory, bool useShadowBuffer);
            HardwareIndexBuffer(HardwareBufferManagerBase* mgr, IndexType idxType, size_t numIndexes,
                                HardwareBuffer* delegate);
            ~HardwareIndexBuffer() override;
            /// Return the manager of this buffer, if any
            [[nodiscard]] HardwareBufferManagerBase* getManager() const noexcept { return mMgr; }
            /// Get the type of indexes used in this buffer
            [[nodiscard]] IndexType getType() const noexcept { return mIndexType; }
            /// Get the number of indexes in this buffer
            [[nodiscard]] size_t getNumIndexes() const noexcept { return mNumIndexes; }
            /// Get the size in bytes of each index
            [[nodiscard]] uint8 getIndexSize() const noexcept { return mIndexSize; }

            static size_t indexSize(IndexType type) { return type == IT_16BIT ? sizeof(uint16) : sizeof(uint32); }

            // NB subclasses should override lock, unlock, readData, writeData
    };

    /** @} */
    /** @} */
}
#endif

