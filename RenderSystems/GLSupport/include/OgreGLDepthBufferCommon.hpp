// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.

#ifndef OGRE_RENDERSYSTEMS_GLSUPPORT_DEPTHBUFFERCOMMON_H
#define OGRE_RENDERSYSTEMS_GLSUPPORT_DEPTHBUFFERCOMMON_H

#include "OgreDepthBuffer.hpp"
#include "OgrePlatform.hpp"

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
        GLDepthBufferCommon(uint16 poolId, GLRenderSystemCommon* renderSystem, GLContext* creatorContext,
                            GLHardwarePixelBufferCommon* depth, GLHardwarePixelBufferCommon* stencil,
                            const RenderTarget* target, bool isManual);

        virtual ~GLDepthBufferCommon();

        auto isCompatible( RenderTarget *renderTarget ) const -> bool override;

        auto getGLContext() const -> GLContext* { return mCreatorContext; }
        auto getDepthBuffer() const -> GLHardwarePixelBufferCommon*  { return mDepthBuffer; }
        auto getStencilBuffer() const -> GLHardwarePixelBufferCommon* { return mStencilBuffer; }

    protected:
        GLContext                   *mCreatorContext;
        GLHardwarePixelBufferCommon *mDepthBuffer;
        GLHardwarePixelBufferCommon *mStencilBuffer;
        GLRenderSystemCommon        *mRenderSystem;
    };
} // namespace Ogre

#endif
