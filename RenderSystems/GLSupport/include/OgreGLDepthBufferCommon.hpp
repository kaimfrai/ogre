// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
export module Ogre.RenderSystems.GLSupport:GLDepthBufferCommon;

export import Ogre.Core;

export
namespace Ogre
{
    class GLContext;
    class GLHardwarePixelBufferCommon;
    class GLRenderSystemCommon;
    class RenderTarget;

    /**
        OpenGL supports 3 different methods: FBO, pbuffer & Copy.
        Each one has it's own limitations. Non-FBO methods are solved using "dummy" DepthBuffers.
        That is, a DepthBuffer pointer is attached to the RenderTarget (for the sake of consistency)
        but it doesn't actually contain a Depth surface/renderbuffer (mDepthBuffer & mStencilBuffer are
        null pointers all the time) Those dummy DepthBuffers are identified thanks to their GL context.
        Note that FBOs don't allow sharing with the main window's depth buffer. Therefore even
        when FBO is enabled, a dummy DepthBuffer is still used to manage the windows.
    */
    class GLDepthBufferCommon : public DepthBuffer
    {
    public:
        GLDepthBufferCommon(DepthBuffer::PoolId poolId, GLRenderSystemCommon* renderSystem, GLContext* creatorContext,
                            GLHardwarePixelBufferCommon* depth, GLHardwarePixelBufferCommon* stencil,
                            const RenderTarget* target, bool isManual);

        ~GLDepthBufferCommon() override;

        auto isCompatible( RenderTarget *renderTarget ) const -> bool override;

        [[nodiscard]] auto getGLContext() const noexcept -> GLContext* { return mCreatorContext; }
        [[nodiscard]] auto getDepthBuffer() const noexcept -> GLHardwarePixelBufferCommon* { return mDepthBuffer.get(); }
        [[nodiscard]] auto getStencilBuffer() const noexcept -> GLHardwarePixelBufferCommon* { return mStencilBuffer.get(); }

    protected:
        GLContext                   *mCreatorContext;
        ::std::unique_ptr<GLHardwarePixelBufferCommon> mDepthBuffer;
        ::std::unique_ptr<GLHardwarePixelBufferCommon> mStencilBuffer;
        GLRenderSystemCommon        *mRenderSystem;
    };
} // namespace Ogre
