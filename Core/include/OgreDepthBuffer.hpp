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
export module Ogre.Core:DepthBuffer;

export import :MemoryAllocatorConfig;
export import :Platform;

export import <set>;

export
namespace Ogre
{
class RenderTarget;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */

    /** An abstract class that contains a depth/stencil buffer.
        Depth Buffers can be attached to render targets. Note we handle Depth & Stencil together.
        DepthBuffer sharing is handled automatically for you. However, there are times where you want
        to specifically control depth buffers to achieve certain effects or increase performance.
        You can control this by hinting Ogre with POOL IDs. Created depth buffers can live in different
        pools, or all together in the same one.
        Usually, a depth buffer can only be attached to a RenderTarget if it's dimensions are bigger
        and have the same bit depth and same multisample settings. Depth Buffers are created automatically
        for new RTs when needed, and stored in the pool where the RenderTarget should have drawn from.
        By default, all RTs have the Id PoolId::DEFAULT, which means all depth buffers are stored by default
        in that pool. By choosing a different Pool Id for a specific RenderTarget, that RT will only
        retrieve depth buffers from _that_ pool, therefore not conflicting with sharing depth buffers
        with other RTs (such as shadows maps).
        Setting an RT to PoolId::MANUAL_USAGE means Ogre won't manage the DepthBuffer for you (not recommended)
        RTs with PoolId::NO_DEPTH are very useful when you don't want to create a DepthBuffer for it. You can
        still manually attach a depth buffer though as internally PoolId::NO_DEPTH & PoolId::MANUAL_USAGE are
        handled in the same way.

        Behavior is consistent across all render systems, if, and only if, the same RSC flags are set
        RSC flags that affect this class are:
            * Capabilities::RTT_MAIN_DEPTHBUFFER_ATTACHABLE:
                some APIs (ie. OpenGL w/ FBO) don't allow using
                the main depth buffer for offscreen RTTs. When this flag is set, the depth buffer can be
                shared between the main window and an RTT.
            * Capabilities::RTT_DEPTHBUFFER_RESOLUTION_LESSEQUAL:
                When this flag isn't set, the depth buffer can only be shared across RTTs who have the EXACT
                same resolution. When it's set, it can be shared with RTTs as long as they have a
                resolution less or equal than the depth buffer's.

        @remarks
            Design discussion http://www.ogre3d.org/forums/viewtopic.php?f=4&t=53534&p=365582
     */
    class DepthBuffer : public RenderSysAlloc
    {
    public:
        enum class PoolId : uint16
        {
            NO_DEPTH       = 0,
            MANUAL_USAGE   = 0,
            DEFAULT        = 1
        };

        DepthBuffer(PoolId poolId, uint32 width, uint32 height, uint32 fsaa, bool manual);
        virtual ~DepthBuffer();

        /** Sets the pool id in which this DepthBuffer lives.
            Note this will detach any render target from this depth buffer */
        void _setPoolId( PoolId poolId );

        /// Gets the pool id in which this DepthBuffer lives
        [[nodiscard]] virtual auto getPoolId() const noexcept -> PoolId;
        [[nodiscard]] virtual auto getWidth() const noexcept -> uint32;
        [[nodiscard]] virtual auto getHeight() const noexcept -> uint32;
        [[nodiscard]] auto getFSAA() const noexcept -> uint32 { return mFsaa; }

        /** Manual DepthBuffers are cleared in RenderSystem's destructor. Non-manual ones are released
            with it's render target (aka, a backbuffer or similar) */
        [[nodiscard]] auto isManual() const noexcept -> bool;

        /** Returns whether the specified RenderTarget is compatible with this DepthBuffer
            That is, this DepthBuffer can be attached to that RenderTarget
            @remarks
                Most APIs impose the following restrictions:
                Width & height must be equal or higher than the render target's
                They must be of the same bit depth.
                They need to have the same FSAA setting
            @param renderTarget The render target to test against
        */
        virtual auto isCompatible( RenderTarget *renderTarget ) const -> bool;

        /** Called when a RenderTarget is attaches this DepthBuffer
            @remarks
                This function doesn't actually attach. It merely informs the DepthBuffer
                which RenderTarget did attach. The real attachment happens in
                RenderTarget::attachDepthBuffer()
            @param renderTarget The RenderTarget that has just been attached
        */
        virtual void _notifyRenderTargetAttached( RenderTarget *renderTarget );

        /** Called when a RenderTarget is detaches from this DepthBuffer
            @remarks
                Same as DepthBuffer::_notifyRenderTargetAttached()
            @param renderTarget The RenderTarget that has just been detached
        */
        virtual void _notifyRenderTargetDetached( RenderTarget *renderTarget );

    protected:
        using RenderTargetSet = std::set<RenderTarget *>;

        DepthBuffer::PoolId         mPoolId;
        uint32                      mWidth;
        uint32                      mHeight;
        uint32                      mFsaa;

        bool                        mManual; //We don't Release manual surfaces on destruction
        RenderTargetSet             mAttachedRenderTargets;

        void detachFromAllRenderTargets();
    };
}
