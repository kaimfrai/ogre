/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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
#include <cassert>
#include <memory>

#include "OgreException.hpp"
#include "OgreHardwarePixelBuffer.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderTexture.hpp"
#include "OgreRoot.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreTexture.hpp"

namespace Ogre 
{
  
    //-----------------------------------------------------------------------------    
    HardwarePixelBuffer::HardwarePixelBuffer(uint32 width, uint32 height, uint32 depth,
            PixelFormat format,
            HardwareBuffer::Usage usage, bool useSystemMemory, bool useShadowBuffer):
        HardwareBuffer(usage, useSystemMemory, useShadowBuffer),
         mWidth(width), mHeight(height), mDepth(depth),
        mFormat(format)
    {
        // Default
        mRowPitch = mWidth;
        mSlicePitch = mHeight*mWidth;
        mSizeInBytes = PixelUtil::getMemorySize(mWidth, mHeight, mDepth, mFormat);
    }
    
    //-----------------------------------------------------------------------------    
    HardwarePixelBuffer::~HardwarePixelBuffer()
    {
        if (!!(mUsage & TextureUsage::RENDERTARGET))
        {
            // Delete all render targets that are not yet deleted via _clearSliceRTT because the rendertarget
            // was deleted by the user.
            for (auto rt : mSliceTRT)
            {
                if (rt)
                    Root::getSingleton().getRenderSystem()->destroyRenderTarget(rt->getName());
            }
        }
    }
    
    //-----------------------------------------------------------------------------    
    auto HardwarePixelBuffer::lock(size_t offset, size_t length, LockOptions options) -> void*
    {
        OgreAssert(!isLocked(), "already locked");
        OgreAssert(offset == 0 && length == mSizeInBytes, "must lock box or entire buffer");
        
        Box myBox(0, 0, 0, mWidth, mHeight, mDepth);
        const PixelBox &rv = lock(myBox, options);
        return rv.data;
    }
    
    //-----------------------------------------------------------------------------    
    auto HardwarePixelBuffer::lock(const Box& lockBox, LockOptions options) -> const PixelBox&
    {
        if (mShadowBuffer)
        {
            if (options != LockOptions::READ_ONLY)
            {
                // we have to assume a read / write lock so we use the shadow buffer
                // and tag for sync on unlock()
                mShadowUpdated = true;
            }

            mCurrentLock = static_cast<HardwarePixelBuffer*>(mShadowBuffer.get())->lock(lockBox, options);
        }
        else
        {
            mCurrentLockOptions = options;
            mLockedBox = lockBox;
            // Lock the real buffer if there is no shadow buffer 
            mCurrentLock = lockImpl(lockBox, options);
            mIsLocked = true;
        }

        return mCurrentLock;
    }
    
    //-----------------------------------------------------------------------------    
    auto HardwarePixelBuffer::getCurrentLock() noexcept -> const PixelBox& 
    { 
        OgreAssert(isLocked(), "buffer not locked");
        return mCurrentLock; 
    }
    
    //-----------------------------------------------------------------------------    
    /// Internal implementation of lock()
    auto HardwarePixelBuffer::lockImpl(size_t offset, size_t length, LockOptions options) -> void*
    {
        OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, "lockImpl(offset,length) is not valid for PixelBuffers and should never be called",
            "HardwarePixelBuffer::lockImpl");
    }

    //-----------------------------------------------------------------------------    

    void HardwarePixelBuffer::blit(const HardwarePixelBufferSharedPtr &src, const Box &srcBox, const Box &dstBox)
    {
        if(isLocked() || src->isLocked())
        {
            OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, "Source and destination buffer may not be locked!");
        }
        if(src.get() == this)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Source must not be the same object");
        }

        LockOptions method = LockOptions::WRITE_ONLY;
        if (dstBox.left == 0 && dstBox.top == 0 && dstBox.front == 0 && dstBox.right == mWidth &&
            dstBox.bottom == mHeight && dstBox.back == mDepth)
            // Entire buffer -- we can discard the previous contents
            method = LockOptions::DISCARD;

        src->blitToMemory(srcBox, lock(dstBox, method));
        unlock();
    }
    //-----------------------------------------------------------------------------       
    void HardwarePixelBuffer::blit(const HardwarePixelBufferSharedPtr &src)
    {
        blit(src, Box(src->getSize()), Box(getSize()));
    }
    //-----------------------------------------------------------------------------    
    void HardwarePixelBuffer::readData(size_t offset, size_t length, void* pDest)
    {
        // allow easy full buffer reads
        if (offset == 0 && length == mSizeInBytes)
        {
            Box box(0, 0, 0, mWidth, mHeight, mDepth);
            blitToMemory(box, PixelBox(box, mFormat, pDest));
            return;
        }

        // TODO
        OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED,
                "Reading a byte range is not implemented. Use blitToMemory.",
                "HardwarePixelBuffer::readData");
    }
    //-----------------------------------------------------------------------------    

    void HardwarePixelBuffer::writeData(size_t offset, size_t length, const void* pSource,
            bool discardWholeBuffer)
    {
        // allow easy full buffer updates
        if (offset == 0 && length == mSizeInBytes)
        {
            Box box(0, 0, 0, mWidth, mHeight, mDepth);
            // we know pSource will not be written to
            blitFromMemory(PixelBox(box, mFormat, const_cast<void*>(pSource)), box);
            return;
        }

        // TODO
        OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED,
                "Writing a byte range is not implemented. Use blitFromMemory.",
                "HardwarePixelBuffer::writeData");
    }
    //-----------------------------------------------------------------------------    
    
    auto HardwarePixelBuffer::getRenderTarget(size_t zoffset) -> RenderTexture *
    {
        assert(mUsage & TextureUsage::RENDERTARGET);
        return mSliceTRT.at(zoffset);
    }
    //-----------------------------------------------------------------------------    

    void HardwarePixelBuffer::_clearSliceRTT(size_t zoffset)
    {
        if(zoffset < mSliceTRT.size())
            mSliceTRT[zoffset] = nullptr;
    }

}
