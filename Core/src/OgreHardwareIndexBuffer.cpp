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
module Ogre.Core;

import :DefaultHardwareBufferManager;
import :Exception;
import :HardwareIndexBuffer;
import :RenderSystem;
import :RenderSystemCapabilities;
import :Root;

import <memory>;

namespace Ogre {

    //-----------------------------------------------------------------------------
    HardwareIndexBuffer::HardwareIndexBuffer(HardwareBufferManagerBase* mgr, IndexType idxType, 
        size_t numIndexes, HardwareBuffer::Usage usage, 
        bool useSystemMemory, bool useShadowBuffer) 
        : HardwareBuffer(usage, useSystemMemory, useShadowBuffer)
        , mIndexType(idxType)
        , mMgr(mgr)
        , mNumIndexes(numIndexes)
    {
        // Calculate the size of the indexes
        mIndexSize = indexSize(idxType);
        mSizeInBytes = mIndexSize * mNumIndexes;

        if (idxType == IndexType::_32BIT && Root::getSingletonPtr() && Root::getSingleton().getRenderSystem())
        {
            if (!Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(Capabilities::_32BIT_INDEX))
            {
                OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, "32 bit index buffers are not supported");
            }
        }

        // Create a shadow buffer if required
        if (useShadowBuffer)
        {
            mShadowBuffer = std::make_unique<DefaultHardwareBuffer>(mSizeInBytes);
        }
    }

    HardwareIndexBuffer::HardwareIndexBuffer(HardwareBufferManagerBase* mgr, IndexType idxType,
                                             size_t numIndexes, HardwareBuffer* delegate)
        : HardwareIndexBuffer(mgr, idxType, numIndexes, delegate->getUsage(), delegate->isSystemMemory(),
                              false)
    {
        mDelegate.reset(delegate);
    }

    //-----------------------------------------------------------------------------
    HardwareIndexBuffer::~HardwareIndexBuffer()
    = default;

}
