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
module Ogre.Core:HardwareBufferManager.Obj;

import :Exception;
import :HardwareBuffer;
import :HardwareBufferManager;
import :HardwareVertexBuffer;
import :Log;
import :LogManager;
import :Prerequisites;
import :SharedPtr;
import :Singleton;
import :VertexIndexData;

import <cassert>;
import <cstddef>;
import <list>;
import <map>;
import <memory>;
import <ostream>;
import <set>;
import <utility>;

namespace Ogre {

    //-----------------------------------------------------------------------
    template<> HardwareBufferManager* Singleton<HardwareBufferManager>::msSingleton = nullptr;
    auto HardwareBufferManager::getSingletonPtr() -> HardwareBufferManager*
    {
        return msSingleton;
    }
    auto HardwareBufferManager::getSingleton() -> HardwareBufferManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //---------------------------------------------------------------------
    HardwareBufferManager::HardwareBufferManager()
    = default;
    //---------------------------------------------------------------------
    HardwareBufferManager::~HardwareBufferManager()
    = default;
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    // Free temporary vertex buffers every 5 minutes on 100fps
    const size_t HardwareBufferManagerBase::UNDER_USED_FRAME_THRESHOLD = 30000;
    const size_t HardwareBufferManagerBase::EXPIRED_DELAY_FRAME_THRESHOLD = 5;
    //-----------------------------------------------------------------------
    HardwareBufferManagerBase::HardwareBufferManagerBase()
         
    = default;
    //-----------------------------------------------------------------------
    HardwareBufferManagerBase::~HardwareBufferManagerBase()
    {
        // Destroy everything
        destroyAllDeclarations();
        destroyAllBindings();
        // No need to destroy main buffers - they will be destroyed by removal of bindings

        // No need to destroy temp buffers - they will be destroyed automatically.
    }
    //-----------------------------------------------------------------------
    auto HardwareBufferManagerBase::createVertexDeclaration() -> VertexDeclaration*
    {
        VertexDeclaration* decl = createVertexDeclarationImpl();
        mVertexDeclarations.insert(decl);
        return decl;
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::destroyVertexDeclaration(VertexDeclaration* decl)
    {
        OgreAssertDbg(mVertexDeclarations.find(decl) != mVertexDeclarations.end(), "unknown decl");
        mVertexDeclarations.erase(decl);
        destroyVertexDeclarationImpl(decl);
    }
    //-----------------------------------------------------------------------
    auto HardwareBufferManagerBase::createVertexBufferBinding() -> VertexBufferBinding*
    {
        VertexBufferBinding* ret = createVertexBufferBindingImpl();
        mVertexBufferBindings.insert(ret);
        return ret;
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::destroyVertexBufferBinding(VertexBufferBinding* binding)
    {
        OgreAssertDbg(mVertexBufferBindings.find(binding) != mVertexBufferBindings.end(),
                      "unknown binding");
        mVertexBufferBindings.erase(binding);
        destroyVertexBufferBindingImpl(binding);
    }
    //-----------------------------------------------------------------------
    auto HardwareBufferManagerBase::createVertexDeclarationImpl() -> VertexDeclaration*
    {
        return new VertexDeclaration();
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::destroyVertexDeclarationImpl(VertexDeclaration* decl)
    {
        delete decl;
    }
    //-----------------------------------------------------------------------
    auto HardwareBufferManagerBase::createVertexBufferBindingImpl() -> VertexBufferBinding*
    {
        return new VertexBufferBinding();
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::destroyVertexBufferBindingImpl(VertexBufferBinding* binding)
    {
        delete binding;
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::destroyAllDeclarations()
    {
        VertexDeclarationList::iterator decl;
        for (decl = mVertexDeclarations.begin(); decl != mVertexDeclarations.end(); ++decl)
        {
            destroyVertexDeclarationImpl(*decl);
        }
        mVertexDeclarations.clear();
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::destroyAllBindings()
    {
        VertexBufferBindingList::iterator bind;
        for (bind = mVertexBufferBindings.begin(); bind != mVertexBufferBindings.end(); ++bind)
        {
            destroyVertexBufferBindingImpl(*bind);
        }
        mVertexBufferBindings.clear();
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::registerVertexBufferSourceAndCopy(
            const HardwareVertexBufferSharedPtr& sourceBuffer,
            const HardwareVertexBufferSharedPtr& copy)
    {
        // Add copy to free temporary vertex buffers
        mFreeTempVertexBufferMap.emplace(sourceBuffer.get(), copy);
    }
    //-----------------------------------------------------------------------
    auto 
    HardwareBufferManagerBase::allocateVertexBufferCopy(
        const HardwareVertexBufferSharedPtr& sourceBuffer, 
        BufferLicenseType licenseType, HardwareBufferLicensee* licensee,
        bool copyData) -> HardwareVertexBufferSharedPtr
    {
        HardwareVertexBufferSharedPtr vbuf;

        // Locate existing buffer copy in temporary vertex buffers
        auto i =
            mFreeTempVertexBufferMap.find(sourceBuffer.get());
        if (i == mFreeTempVertexBufferMap.end())
        {
            // copy buffer, use shadow buffer and make dynamic
            vbuf = makeBufferCopy(
                sourceBuffer,
                HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE,
                true);
        }
        else
        {
            // Allocate existing copy
            vbuf = i->second;
            mFreeTempVertexBufferMap.erase(i);
        }

        // Copy data?
        if (copyData)
        {
            vbuf->copyData(*(sourceBuffer.get()), 0, 0, sourceBuffer->getSizeInBytes(), true);
        }

        // Insert copy into licensee list
        mTempVertexBufferLicenses.emplace(
                vbuf.get(),
                VertexBufferLicense(sourceBuffer.get(), licenseType, EXPIRED_DELAY_FRAME_THRESHOLD, vbuf, licensee));
        return vbuf;
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::releaseVertexBufferCopy(
        const HardwareVertexBufferSharedPtr& bufferCopy)
    {
        auto i =
            mTempVertexBufferLicenses.find(bufferCopy.get());
        if (i != mTempVertexBufferLicenses.end())
        {
            const VertexBufferLicense& vbl = i->second;

            vbl.licensee->licenseExpired(vbl.buffer.get());

            mFreeTempVertexBufferMap.emplace(vbl.originalBufferPtr, vbl.buffer);
            mTempVertexBufferLicenses.erase(i);
        }
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::touchVertexBufferCopy(
            const HardwareVertexBufferSharedPtr& bufferCopy)
    {
        auto i =
            mTempVertexBufferLicenses.find(bufferCopy.get());
        if (i != mTempVertexBufferLicenses.end())
        {
            VertexBufferLicense& vbl = i->second;
            assert(vbl.licenseType == BLT_AUTOMATIC_RELEASE);

            vbl.expiredDelay = EXPIRED_DELAY_FRAME_THRESHOLD;
        }
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::_freeUnusedBufferCopies()
    {
        size_t numFreed = 0;

        // Free unused temporary buffers
        FreeTemporaryVertexBufferMap::iterator i;
        i = mFreeTempVertexBufferMap.begin();
        while (i != mFreeTempVertexBufferMap.end())
        {
            auto icur = i++;
            // Free the temporary buffer that referenced by ourself only.
            // TODO: Some temporary buffers are bound to vertex buffer bindings
            // but not checked out, need to sort out method to unbind them.
            if (icur->second.use_count() <= 1)
            {
                ++numFreed;
                mFreeTempVertexBufferMap.erase(icur);
            }
        }

        StringStream str;
        if (numFreed)
        {
            str << "HardwareBufferManager: Freed " << numFreed << " unused temporary vertex buffers.";
        }
        else
        {
            str << "HardwareBufferManager: No unused temporary vertex buffers found.";
        }
        LogManager::getSingleton().logMessage(str.str(), LML_TRIVIAL);
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::_releaseBufferCopies(bool forceFreeUnused)
    {
        size_t numUnused = mFreeTempVertexBufferMap.size();
        size_t numUsed = mTempVertexBufferLicenses.size();

        // Erase the copies which are automatic licensed out
        TemporaryVertexBufferLicenseMap::iterator i;
        i = mTempVertexBufferLicenses.begin(); 
        while (i != mTempVertexBufferLicenses.end()) 
        {
            auto icur = i++;
            VertexBufferLicense& vbl = icur->second;
            if (vbl.licenseType == BLT_AUTOMATIC_RELEASE &&
                (forceFreeUnused || --vbl.expiredDelay <= 0))
            {
                vbl.licensee->licenseExpired(vbl.buffer.get());

                mFreeTempVertexBufferMap.emplace(vbl.originalBufferPtr, vbl.buffer);
                mTempVertexBufferLicenses.erase(icur);
            }
        }

        // Check whether or not free unused temporary vertex buffers.
        if (forceFreeUnused)
        {
            _freeUnusedBufferCopies();
            mUnderUsedFrameCount = 0;
        }
        else
        {
            if (numUsed < numUnused)
            {
                // Free temporary vertex buffers if too many unused for a long time.
                // Do overall temporary vertex buffers instead of per source buffer
                // to avoid overhead.
                ++mUnderUsedFrameCount;
                if (mUnderUsedFrameCount >= UNDER_USED_FRAME_THRESHOLD)
                {
                    _freeUnusedBufferCopies();
                    mUnderUsedFrameCount = 0;
                }
            }
            else
            {
                mUnderUsedFrameCount = 0;
            }
        }
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::_forceReleaseBufferCopies(
        const HardwareVertexBufferSharedPtr& sourceBuffer)
    {
        _forceReleaseBufferCopies(sourceBuffer.get());
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::_forceReleaseBufferCopies(
        HardwareVertexBuffer* sourceBuffer)
    {
        // Erase the copies which are licensed out
        TemporaryVertexBufferLicenseMap::iterator i;
        i = mTempVertexBufferLicenses.begin();
        while (i != mTempVertexBufferLicenses.end()) 
        {
            auto icur = i++;
            const VertexBufferLicense& vbl = icur->second;
            if (vbl.originalBufferPtr == sourceBuffer)
            {
                // Just tell the owner that this is being released
                vbl.licensee->licenseExpired(vbl.buffer.get());

                mTempVertexBufferLicenses.erase(icur);
            }
        }

        // Erase the free copies
        //
        // Why we need this unusual code? It's for resolve reenter problem.
        //
        // Using mFreeTempVertexBufferMap.erase(sourceBuffer) directly will
        // cause reenter into here because vertex buffer destroyed notify.
        // In most time there are no problem. But when sourceBuffer is the
        // last item of the mFreeTempVertexBufferMap, some STL multimap
        // implementation (VC and STLport) will call to clear(), which will
        // causing intermediate state of mFreeTempVertexBufferMap, in that
        // time destroyed notify back to here cause illegal accessing in
        // the end.
        //
        // For safely reason, use following code to resolve reenter problem.
        //
        using _Iter = FreeTemporaryVertexBufferMap::iterator;
        std::pair<_Iter, _Iter> range = mFreeTempVertexBufferMap.equal_range(sourceBuffer);
        if (range.first != range.second)
        {
            std::list<HardwareVertexBufferSharedPtr> holdForDelayDestroy;
            for (auto it = range.first; it != range.second; ++it)
            {
                if (it->second.use_count() <= 1)
                {
                    holdForDelayDestroy.push_back(it->second);
                }
            }

            mFreeTempVertexBufferMap.erase(range.first, range.second);

            // holdForDelayDestroy will destroy auto.
        }
    }
    //-----------------------------------------------------------------------
    void HardwareBufferManagerBase::_notifyVertexBufferDestroyed(HardwareVertexBuffer* buf)
    {
        auto i = mVertexBuffers.find(buf);
        if (i != mVertexBuffers.end())
        {
            // release vertex buffer copies
            mVertexBuffers.erase(i);
            _forceReleaseBufferCopies(buf);
        }
    }
    auto HardwareBufferManagerBase::createRenderToVertexBuffer() -> RenderToVertexBufferSharedPtr
    {
        OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR, "not supported by RenderSystem");
    }
    auto HardwareBufferManagerBase::createUniformBuffer(size_t sizeBytes,
                                                                     HardwareBufferUsage usage,
                                                                     bool useShadowBuffer) -> HardwareBufferPtr
    {
        OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR, "not supported by RenderSystem");
    }
    //-----------------------------------------------------------------------
    auto 
    HardwareBufferManagerBase::makeBufferCopy(
        const HardwareVertexBufferSharedPtr& source,
        HardwareBuffer::Usage usage, bool useShadowBuffer) -> HardwareVertexBufferSharedPtr
    {
        return this->createVertexBuffer(
            source->getVertexSize(), 
            source->getNumVertices(),
            usage, useShadowBuffer);
    }
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    TempBlendedBufferInfo::~TempBlendedBufferInfo()
    {
        // check that temp buffers have been released
        if (destPositionBuffer)
            destPositionBuffer->getManager()->releaseVertexBufferCopy(destPositionBuffer);
        if (destNormalBuffer)
            destNormalBuffer->getManager()->releaseVertexBufferCopy(destNormalBuffer);

    }
    //-----------------------------------------------------------------------------
    void TempBlendedBufferInfo::extractFrom(const VertexData* sourceData)
    {
        // Release old buffer copies first
        if (destPositionBuffer)
        {
            destPositionBuffer->getManager()->releaseVertexBufferCopy(destPositionBuffer);
            assert(!destPositionBuffer);
        }
        if (destNormalBuffer)
        {
            destNormalBuffer->getManager()->releaseVertexBufferCopy(destNormalBuffer);
            assert(!destNormalBuffer);
        }

        VertexDeclaration* decl = sourceData->vertexDeclaration;
        VertexBufferBinding* bind = sourceData->vertexBufferBinding;
        const VertexElement *posElem = decl->findElementBySemantic(VES_POSITION);
        const VertexElement *normElem = decl->findElementBySemantic(VES_NORMAL);

        assert(posElem && "Positions are required");

        posBindIndex = posElem->getSource();
        srcPositionBuffer = bind->getBuffer(posBindIndex);

        if (!normElem)
        {
            posNormalShareBuffer = false;
            srcNormalBuffer.reset();
        }
        else
        {
            normBindIndex = normElem->getSource();
            if (normBindIndex == posBindIndex)
            {
                posNormalShareBuffer = true;
                srcNormalBuffer.reset();
            }
            else
            {
                posNormalShareBuffer = false;
                srcNormalBuffer = bind->getBuffer(normBindIndex);
            }
        }
    }
    //-----------------------------------------------------------------------------
    void TempBlendedBufferInfo::checkoutTempCopies(bool positions, bool normals)
    {
        bindPositions = positions;
        bindNormals = normals;

        if (positions && !destPositionBuffer)
        {
            destPositionBuffer = srcPositionBuffer->getManager()->allocateVertexBufferCopy(srcPositionBuffer, 
                HardwareBufferManagerBase::BLT_AUTOMATIC_RELEASE, this);
        }
        if (normals && !posNormalShareBuffer && srcNormalBuffer && !destNormalBuffer)
        {
            destNormalBuffer = srcNormalBuffer->getManager()->allocateVertexBufferCopy(srcNormalBuffer, 
                HardwareBufferManagerBase::BLT_AUTOMATIC_RELEASE, this);
        }
    }
    //-----------------------------------------------------------------------------
    auto TempBlendedBufferInfo::buffersCheckedOut(bool positions, bool normals) const -> bool
    {
        if (positions || (normals && posNormalShareBuffer))
        {
            if (!destPositionBuffer)
                return false;

            destPositionBuffer->getManager()->touchVertexBufferCopy(destPositionBuffer);
        }

        if (normals && !posNormalShareBuffer)
        {
            if (!destNormalBuffer)
                return false;

            destNormalBuffer->getManager()->touchVertexBufferCopy(destNormalBuffer);
        }

        return true;
    }
    //-----------------------------------------------------------------------------
    void TempBlendedBufferInfo::bindTempCopies(VertexData* targetData, bool suppressHardwareUpload)
    {
        this->destPositionBuffer->suppressHardwareUpdate(suppressHardwareUpload);
        targetData->vertexBufferBinding->setBinding(
            this->posBindIndex, this->destPositionBuffer);
        if (bindNormals && !posNormalShareBuffer && destNormalBuffer)
        {
            this->destNormalBuffer->suppressHardwareUpdate(suppressHardwareUpload);
            targetData->vertexBufferBinding->setBinding(
                this->normBindIndex, this->destNormalBuffer);
        }
    }
    //-----------------------------------------------------------------------------
    void TempBlendedBufferInfo::licenseExpired(HardwareBuffer* buffer)
    {
        assert(buffer == destPositionBuffer.get()
            || buffer == destNormalBuffer.get());

        if (buffer == destPositionBuffer.get())
            destPositionBuffer.reset();
        if (buffer == destNormalBuffer.get())
            destNormalBuffer.reset();

    }

}
