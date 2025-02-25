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

export module Ogre.Core:HardwareBuffer;

// Precompiler options
export import :Exception;
export import :Prerequisites;

export
namespace Ogre {

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */
    /// Enums describing buffer usage
    enum class HardwareBufferUsage : uint8
    {
        /** Memory mappable on host and cached
         * @par Usage
         * results of some computations, e.g. screen capture
         */
        GPU_TO_CPU = 1,
        /** CPU (system) memory
         * This is the least optimal buffer setting.
         * @par Usage
         * Staging copy of resources used as transfer source.
         */
        CPU_ONLY = 2,
        /** Indicates the application will never read the contents of the buffer back,
        it will only ever write data. Locking a buffer with this flag will ALWAYS
        return a pointer to new, blank memory rather than the memory associated
        with the contents of the buffer; this avoids DMA stalls because you can
        write to a new memory area while the previous one is being used.

        However, you may read from it’s shadow buffer if you set one up
        */
        DETAIL_WRITE_ONLY = 4,
        /** Device-local GPU (video) memory. No need to be mappable on host.
         * This is the optimal buffer usage setting.
         * @par Usage
         * Resources transferred from host once (immutable) - e.g. most textures, vertex buffers
         */
        GPU_ONLY = HardwareBufferUsage::GPU_TO_CPU | HardwareBufferUsage::DETAIL_WRITE_ONLY,
        /** Mappable on host and preferably fast to access by GPU.
         * @par Usage
         * Resources written frequently by host (dynamic) - e.g. uniform buffers updated every frame
         */
        CPU_TO_GPU = HardwareBufferUsage::CPU_ONLY | HardwareBufferUsage::DETAIL_WRITE_ONLY,
    };

    auto constexpr operator compl(HardwareBufferUsage mask) -> HardwareBufferUsage
    {
        return static_cast<HardwareBufferUsage>
        (   compl
            std::to_underlying(mask)
        );
    }

    auto constexpr operator not(HardwareBufferUsage mask) -> bool
    {
        return not std::to_underlying(mask);
    }


    auto constexpr operator bitor(HardwareBufferUsage left, HardwareBufferUsage right) -> HardwareBufferUsage
    {
        return static_cast<HardwareBufferUsage>
        (   std::to_underlying(left)
        bitor
            std::to_underlying(right)
        );
    }

    auto constexpr operator |= (HardwareBufferUsage& left, HardwareBufferUsage right) -> HardwareBufferUsage&
    {
        return left = left bitor right;
    }

    auto constexpr operator bitand(HardwareBufferUsage left, HardwareBufferUsage right) -> HardwareBufferUsage
    {
        return static_cast<HardwareBufferUsage>
        (   std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }

    auto constexpr operator &= (HardwareBufferUsage& left, HardwareBufferUsage right) -> HardwareBufferUsage&
    {
        return left = left bitand right;
    }

    /** Abstract class defining common features of hardware buffers.
    @remarks
        A 'hardware buffer' is any area of memory held outside of core system ram,
        and in our case refers mostly to video ram, although in theory this class
        could be used with other memory areas such as sound card memory, custom
        coprocessor memory etc.
    @par
        This reflects the fact that memory held outside of main system RAM must 
        be interacted with in a more formal fashion in order to promote
        cooperative and optimal usage of the buffers between the various 
        processing units which manipulate them.
    @par
        This abstract class defines the core interface which is common to all
        buffers, whether it be vertex buffers, index buffers, texture memory
        or framebuffer memory etc.
    @par
        Buffers have the ability to be 'shadowed' in system memory, this is because
        the kinds of access allowed on hardware buffers is not always as flexible as
        that allowed for areas of system memory - for example it is often either 
        impossible, or extremely undesirable from a performance standpoint to read from
        a hardware buffer; when writing to hardware buffers, you should also write every
        byte and do it sequentially. In situations where this is too restrictive, 
        it is possible to create a hardware, write-only buffer (the most efficient kind) 
        and to back it with a system memory 'shadow' copy which can be read and updated arbitrarily.
        Ogre handles synchronising this buffer with the real hardware buffer (which should still be
        created with the HardwareBufferUsage::DYNAMIC flag if you intend to update it very frequently). Whilst this
        approach does have its own costs, such as increased memory overhead, these costs can 
        often be outweighed by the performance benefits of using a more hardware efficient buffer.
        You should look for the 'useShadowBuffer' parameter on the creation methods used to create
        the buffer of the type you require (see HardwareBufferManager) to enable this feature.
    */
    class HardwareBuffer : public BufferAlloc
    {

        public:
            using Usage = HardwareBufferUsage;
            /// Rather use Usage
            /// same as Usage::GPU_TO_CPU
            static auto constexpr STATIC = Usage::GPU_TO_CPU;
            /// same as Usage::CPU_ONLY
            static auto constexpr DYNAMIC = Usage::CPU_ONLY;
            /// @deprecated use Usage::DETAIL_WRITE_ONLY
            static auto constexpr WRITE_ONLY = Usage::DETAIL_WRITE_ONLY;
            /// @deprecated do not use
            static auto constexpr DISCARDABLE = static_cast<Usage>(8);
            /// same as #HardwareBufferUsage::GPU_ONLY
            static auto constexpr STATIC_WRITE_ONLY = Usage::GPU_ONLY;
            /// same as #HardwareBufferUsage::CPU_TO_GPU
            static auto constexpr DYNAMIC_WRITE_ONLY = Usage::CPU_TO_GPU;
            /// @deprecated do not use
            static auto constexpr DYNAMIC_WRITE_ONLY_DISCARDABLE = Usage::CPU_TO_GPU;

            /// Locking options
            enum class LockOptions : uint8
            {
                /** Normal mode, ie allows read/write and contents are preserved.
                 This kind of lock allows reading and writing from the buffer - it’s also the least
                 optimal because basically you’re telling the card you could be doing anything at
                 all. If you’re not using a shadow buffer, it requires the buffer to be transferred
                 from the card and back again. If you’re using a shadow buffer the effect is
                 minimal.
                 */
                NORMAL,
                /** Discards the <em>entire</em> buffer while locking.
                This means you are happy for the card to discard the entire current contents of the
                buffer. Implicitly this means you are not going to read the data - it also means
                that the card can avoid any stalls if the buffer is currently being rendered from,
                because it will actually give you an entirely different one. Use this wherever
                possible when you are locking a buffer which was not created with a shadow buffer.
                If you are using a shadow buffer it matters less, although with a shadow buffer it’s
                preferable to lock the entire buffer at once, because that allows the shadow buffer
                to use LockOptions::DISCARD when it uploads the updated contents to the real buffer.
                @note Only useful on buffers created with the HardwareBufferUsage::CPU_TO_GPU flag.
                */
                DISCARD,
                /** Lock the buffer for reading only. Not allowed in buffers which are created with
                HardwareBufferUsage::GPU_ONLY.
                Mandatory on static buffers, i.e. those created without the HardwareBufferUsage::DYNAMIC flag.
                */
                READ_ONLY,
                /** As LockOptions::WRITE_ONLY, except the application guarantees not to overwrite any
                region of the buffer which has already been used in this frame, can allow
                some optimisation on some APIs.
                @note Only useful on buffers with no shadow buffer.*/
                NO_OVERWRITE,
                /** Lock the buffer for writing only.*/
                WRITE_ONLY

            };
        protected:
            size_t mSizeInBytes{0};
            size_t mLockStart{0};
            size_t mLockSize{0};
            std::unique_ptr<HardwareBuffer> mDelegate;
            std::unique_ptr<HardwareBuffer> mShadowBuffer;
            bool mSystemMemory;
            bool mShadowUpdated{false};
            bool mSuppressHardwareUpdate{false};
            bool mIsLocked{false};
            Usage mUsage;
            
            /// Internal implementation of lock()
            virtual auto lockImpl(size_t offset, size_t length, LockOptions options) -> void*
            {
                return mDelegate->lock(offset, length, options);
            }
            /// Internal implementation of unlock()
            virtual void unlockImpl() { mDelegate->unlock(); }

        public:
            /// Constructor, to be called by HardwareBufferManager only
            HardwareBuffer(Usage usage, bool systemMemory, bool useShadowBuffer)
                :  mSystemMemory(systemMemory),
                   mUsage(usage)
            {
                // If use shadow buffer, upgrade to WRITE_ONLY on hardware side
                if (useShadowBuffer && usage == HardwareBufferUsage::CPU_ONLY)
                {
                    mUsage = HardwareBufferUsage::CPU_TO_GPU;
                }
                else if (useShadowBuffer && usage == HardwareBufferUsage::GPU_TO_CPU)
                {
                    mUsage = HardwareBufferUsage::GPU_ONLY;
                }
            }
            virtual ~HardwareBuffer() = default;
            /** Lock the buffer for (potentially) reading / writing.
            @param offset The byte offset from the start of the buffer to lock
            @param length The size of the area to lock, in bytes
            @param options Locking options
            @return Pointer to the locked memory
            */
            virtual auto lock(size_t offset, size_t length, LockOptions options) -> void*
            {
                OgreAssert(!isLocked(), "Cannot lock this buffer: it is already locked");
                OgreAssert((length + offset) <= mSizeInBytes, "Lock request out of bounds");

                void* ret = nullptr;
                if (mShadowBuffer)
                {
                    // we have to assume a read / write lock so we use the shadow buffer
                    // and tag for sync on unlock()
                    mShadowUpdated = (options != LockOptions::READ_ONLY);

                    ret = mShadowBuffer->lock(offset, length, options);
                }
                else
                {
                    mIsLocked = true;
                    // Lock the real buffer if there is no shadow buffer 
                    ret = lockImpl(offset, length, options);
                }
                mLockStart = offset;
                mLockSize = length;
                return ret;
            }

            /// @overload
            auto lock(LockOptions options) -> void*
            {
                return this->lock(0, mSizeInBytes, options);
            }
            /** Releases the lock on this buffer. 
            @remarks 
                Locking and unlocking a buffer can, in some rare circumstances such as 
                switching video modes whilst the buffer is locked, corrupt the 
                contents of a buffer. This is pretty rare, but if it occurs, 
                this method will throw an exception, meaning you
                must re-upload the data.
            @par
                Note that using the 'read' and 'write' forms of updating the buffer does not
                suffer from this problem, so if you want to be 100% sure your
                data will not be lost, use the 'read' and 'write' forms instead.
            */
            void unlock()
            {
                OgreAssert(isLocked(), "Cannot unlock this buffer: it is not locked");

                // If we used the shadow buffer this time...
                if (mShadowBuffer && mShadowBuffer->isLocked())
                {
                    mShadowBuffer->unlock();
                    // Potentially update the 'real' buffer from the shadow buffer
                    _updateFromShadow();
                }
                else
                {
                    // Otherwise, unlock the real one
                    unlockImpl();
                    mIsLocked = false;
                }

            }

            /** Reads data from the buffer and places it in the memory pointed to by pDest.
            @param offset The byte offset from the start of the buffer to read
            @param length The size of the area to read, in bytes
            @param pDest The area of memory in which to place the data, must be large enough to 
                accommodate the data!
            */
            virtual void readData(size_t offset, size_t length, void* pDest)
            {
                if (mShadowBuffer)
                {
                    mShadowBuffer->readData(offset, length, pDest);
                    return;
                }

                mDelegate->readData(offset, length, pDest);
            }
            /** Writes data to the buffer from an area of system memory; note that you must
                ensure that your buffer is big enough.
            @param offset The byte offset from the start of the buffer to start writing
            @param length The size of the data to write to, in bytes
            @param pSource The source of the data to be written
            @param discardWholeBuffer If true, this allows the driver to discard the entire buffer when writing,
                such that DMA stalls can be avoided; use if you can.
            */
            virtual void writeData(size_t offset, size_t length, const void* pSource,
                                   bool discardWholeBuffer = false)
            {
                // Update the shadow buffer
                if (mShadowBuffer)
                {
                    mShadowBuffer->writeData(offset, length, pSource, discardWholeBuffer);
                }

                mDelegate->writeData(offset, length, pSource, discardWholeBuffer);
            }

            /** Copy data from another buffer into this one.
            @remarks
                Note that the source buffer must not be created with the
                usage HardwareBufferUsage::WRITE_ONLY otherwise this will fail.
            @param srcBuffer The buffer from which to read the copied data
            @param srcOffset Offset in the source buffer at which to start reading
            @param dstOffset Offset in the destination buffer to start writing
            @param length Length of the data to copy, in bytes.
            @param discardWholeBuffer If true, will discard the entire contents of this buffer before copying
            */
            virtual void copyData(HardwareBuffer& srcBuffer, size_t srcOffset, 
                size_t dstOffset, size_t length, bool discardWholeBuffer = false)
            {
                if(mDelegate && !srcBuffer.isSystemMemory())
                {
                    // GPU copy
                    mDelegate->copyData(*srcBuffer.mDelegate, srcOffset, dstOffset, length, discardWholeBuffer);
                    return;
                }
                const void* srcData = srcBuffer.lock(srcOffset, length, LockOptions::READ_ONLY);
                this->writeData(dstOffset, length, srcData, discardWholeBuffer);
                srcBuffer.unlock();
            }

            /** Copy all data from another buffer into this one. 
            @remarks
                Normally these buffers should be of identical size, but if they're
                not, the routine will use the smallest of the two sizes.
            */
            void copyData(HardwareBuffer& srcBuffer)
            {
                size_t sz = std::min(getSizeInBytes(), srcBuffer.getSizeInBytes()); 
                copyData(srcBuffer, 0, 0, sz, true);
            }
            
            /// Updates the real buffer from the shadow buffer, if required
            virtual void _updateFromShadow()
            {
                if (mShadowBuffer && mShadowUpdated && !mSuppressHardwareUpdate)
                {
                    // Do this manually to avoid locking problems
                    const void* srcData = mShadowBuffer->lockImpl(mLockStart, mLockSize, LockOptions::READ_ONLY);
                    // Lock with discard if the whole buffer was locked, otherwise w/o
                    bool discardWholeBuffer = mLockStart == 0 && mLockSize == mSizeInBytes;
                    LockOptions lockOpt = discardWholeBuffer ? LockOptions::DISCARD : LockOptions::WRITE_ONLY;
                    void* destData = this->lockImpl(mLockStart, mLockSize, lockOpt);
                    // Copy shadow to real
                    memcpy(destData, srcData, mLockSize);
                    this->unlockImpl();
                    mShadowBuffer->unlockImpl();
                    mShadowUpdated = false;
                }
            }

            /// Returns the size of this buffer in bytes
            [[nodiscard]] auto getSizeInBytes() const noexcept -> size_t { return mSizeInBytes; }
            /// Returns the Usage flags with which this buffer was created
            [[nodiscard]] auto getUsage() const noexcept -> Usage { return mUsage; }
            /// Returns whether this buffer is held in system memory
            [[nodiscard]] auto isSystemMemory() const noexcept -> bool { return mSystemMemory; }
            /// Returns whether this buffer has a system memory shadow for quicker reading
            [[nodiscard]] auto hasShadowBuffer() const -> bool { return mShadowBuffer || (mDelegate && mDelegate->hasShadowBuffer()); }
            /// Returns whether or not this buffer is currently locked.
            [[nodiscard]] auto isLocked() const noexcept -> bool { 
                return mIsLocked || (mShadowBuffer && mShadowBuffer->isLocked());
            }
            /// Pass true to suppress hardware upload of shadow buffer changes
            void suppressHardwareUpdate(bool suppress) {
                mSuppressHardwareUpdate = suppress;
                if (!suppress)
                    _updateFromShadow();

                if(mDelegate)
                    mDelegate->suppressHardwareUpdate(suppress);
            }

            template <typename T> auto _getImpl() -> T*
            {
                return static_cast<T*>(mDelegate.get());
            }
    };

    using HardwareCounterBuffer = HardwareBuffer;
    using HardwareUniformBuffer = HardwareBuffer;

    /** Locking helper. Guaranteed unlocking even in case of exception. */
    struct HardwareBufferLockGuard
    {
        HardwareBufferLockGuard() : pBuf(nullptr), pData(nullptr) {}
        
        HardwareBufferLockGuard(HardwareBuffer* p, HardwareBuffer::LockOptions options)
            : pBuf(nullptr), pData(nullptr) { lock(p, options); }
        
        HardwareBufferLockGuard(HardwareBuffer* p, size_t offset, size_t length, HardwareBuffer::LockOptions options)
            : pBuf(nullptr), pData(nullptr) { lock(p, offset, length, options); }
        
        template <typename T>
        HardwareBufferLockGuard(const SharedPtr<T>& p, HardwareBuffer::LockOptions options)
            : pBuf(nullptr), pData(nullptr) { lock(p.get(), options); }
        
        template <typename T>
        HardwareBufferLockGuard(const SharedPtr<T>& p, size_t offset, size_t length, HardwareBuffer::LockOptions options)
            : pBuf(nullptr), pData(nullptr) { lock(p.get(), offset, length, options); }
        
        ~HardwareBufferLockGuard() { unlock(); }
        
        void unlock()
        {
            if(pBuf)
            {
                pBuf->unlock();
                pBuf = nullptr;
                pData = nullptr;
            }   
        }

        void lock(HardwareBuffer* p, HardwareBuffer::LockOptions options)
        {
            assert(p);
            unlock();
            pBuf = p;
            pData = pBuf->lock(options);
        }
        
        void lock(HardwareBuffer* p, size_t offset, size_t length, HardwareBuffer::LockOptions options)
        {
            assert(p);
            unlock();
            pBuf = p;
            pData = pBuf->lock(offset, length, options);
        }
        
        template <typename T>
        void lock(const SharedPtr<T>& p, HardwareBuffer::LockOptions options)
            { lock(p.get(), options); }
        
        template <typename T>
        void lock(const SharedPtr<T>& p, size_t offset, size_t length, HardwareBuffer::LockOptions options)
            { lock(p.get(), offset, length, options); }
        
        HardwareBuffer* pBuf;
        void* pData;
    };

    /** @} */
    /** @} */
}
