// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.

#include "OgreGLDepthBufferCommon.hpp"

#include "OgreGLHardwarePixelBufferCommon.hpp"
#include "OgreGLRenderSystemCommon.hpp"
#include "OgreGLRenderTarget.hpp"
#include "OgreGLRenderTexture.hpp"
#include "OgrePixelFormat.hpp"
#include "OgreRenderSystemCapabilities.hpp"
#include "OgreRenderTarget.hpp"

namespace Ogre
{
class GLContext;

GLDepthBufferCommon::GLDepthBufferCommon(uint16 poolId, GLRenderSystemCommon* renderSystem,
                                         GLContext* creatorContext, GLHardwarePixelBufferCommon* depth,
                                         GLHardwarePixelBufferCommon* stencil, const RenderTarget* target,
                                         bool manual)
    : DepthBuffer(poolId, target->getWidth(), target->getHeight(), target->getFSAA(), manual),
      mCreatorContext(creatorContext), mDepthBuffer(depth), mStencilBuffer(stencil),
      mRenderSystem(renderSystem)
{
}

GLDepthBufferCommon::~GLDepthBufferCommon()
{
    if (mStencilBuffer.get() == mDepthBuffer.get())
    {
        mStencilBuffer.release();
    }
}

auto GLDepthBufferCommon::isCompatible(RenderTarget* renderTarget) const -> bool
{
    bool retVal = false;

    // Check standard stuff first.
    if (mRenderSystem->getCapabilities()->hasCapability(RSC_RTT_DEPTHBUFFER_RESOLUTION_LESSEQUAL))
    {
        if (!DepthBuffer::isCompatible(renderTarget))
            return false;
    }
    else
    {
        if (this->getWidth() != renderTarget->getWidth() ||
            this->getHeight() != renderTarget->getHeight() || this->getFSAA() != renderTarget->getFSAA())
            return false;
    }

    // Now check this is the appropriate format
    auto fbo = dynamic_cast<GLRenderTarget*>(renderTarget)->getFBO();

    if (!fbo)
    {
        GLContext* windowContext = dynamic_cast<GLRenderTarget*>(renderTarget)->getContext();

        // Non-FBO targets and FBO depth surfaces don't play along, only dummies which match the same
        // context
        if (!mDepthBuffer && !mStencilBuffer && (!windowContext || mCreatorContext == windowContext))
            retVal = true;
    }
    else
    {
        // Check this isn't a dummy non-FBO depth buffer with an FBO target, don't mix them.
        // If you don't want depth buffer, use a Null Depth Buffer, not a dummy one.
        if (mDepthBuffer || mStencilBuffer)
        {
            PixelFormat internalFormat = fbo->getFormat();
            uint32 depthFormat, stencilFormat;
            mRenderSystem->_getDepthStencilFormatFor(internalFormat, &depthFormat, &stencilFormat);

            bool bSameDepth = false;

            if (mDepthBuffer)
                bSameDepth |= mDepthBuffer->getGLFormat() == depthFormat;

            bool bSameStencil = false;

            if (!mStencilBuffer || mStencilBuffer == mDepthBuffer)
                bSameStencil = stencilFormat == 0; // GL_NONE
            else
            {
                if (mStencilBuffer)
                    bSameStencil = stencilFormat == mStencilBuffer->getGLFormat();
            }

            retVal = PixelUtil::isDepth(internalFormat) ? bSameDepth : (bSameDepth && bSameStencil);
        }
    }

    return retVal;
}
} // namespace Ogre
