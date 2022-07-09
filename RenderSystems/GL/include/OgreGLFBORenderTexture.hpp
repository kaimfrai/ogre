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
#ifndef OGRE_RENDERSYSTEMS_GL_FBORENDERTEXTURE_H
#define OGRE_RENDERSYSTEMS_GL_FBORENDERTEXTURE_H

#include "OgreGLFrameBufferObject.hpp"
#include "OgreGLRenderTexture.hpp"
#include "OgrePixelFormat.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "glad/glad.h"

namespace Ogre {
class DepthBuffer;
class GLContext;
class RenderTarget;
}  // namespace Ogre

/// Extra GL constants
#define GL_DEPTH24_STENCIL8_EXT                           0x88F0


namespace Ogre {
    class GLFBOManager;

    /** RenderTexture for GL FBO
    */
    class GLFBORenderTexture: public GLRenderTexture
    {
    public:
        GLFBORenderTexture(GLFBOManager *manager, std::string_view name, const GLSurfaceDesc &target, bool writeGamma, uint fsaa);

        void getCustomAttribute(std::string_view name, void* pData) override;

        /// Override needed to deal with multisample buffers
        void swapBuffers() override;

        /// Override so we can attach the depth buffer to the FBO
        auto attachDepthBuffer( DepthBuffer *depthBuffer ) -> bool override;
        void detachDepthBuffer() override;
        void _detachDepthBuffer() override;

        [[nodiscard]] auto getContext() const noexcept -> GLContext* override { return mFB.getContext(); }
        auto getFBO() noexcept -> GLFrameBufferObjectCommon* override { return &mFB; }
    protected:
        GLFrameBufferObject mFB;
    };
    
    /** Factory for GL Frame Buffer Objects, and related things.
    */
    class GLFBOManager: public GLRTTManager
    {
    public:
        GLFBOManager(bool atimode);
        ~GLFBOManager() override;
        
        /** Bind a certain render target if it is a FBO. If it is not a FBO, bind the
            main frame buffer.
        */
        void bind(RenderTarget *target) override;
        
        /** Unbind a certain render target. No-op for FBOs.
        */
        void unbind(RenderTarget *target) override {};
        
        /** Get best depth and stencil supported for given internalFormat
        */
        void getBestDepthStencil(PixelFormat internalFormat, GLenum *depthFormat, GLenum *stencilFormat) override;
        
        /** Create a texture rendertarget object
        */
        auto createRenderTexture(std::string_view name, 
            const GLSurfaceDesc &target, bool writeGamma, uint fsaa) -> GLFBORenderTexture * override;
        
        /** Request a render buffer. If format is GL_NONE, return a zero buffer.
        */
        auto requestRenderBuffer(GLenum format, uint32 width, uint32 height, uint fsaa) -> GLSurfaceDesc;
        /** Get a FBO without depth/stencil for temporary use, like blitting between textures.
        */
        auto getTemporaryFBO() noexcept -> GLuint { return mTempFBO; }
    private:
        /** Temporary FBO identifier
         */
        GLuint mTempFBO;
        
        /// Buggy ATI driver?
        bool mATIMode;
        
        /** Detect allowed FBO formats */
        void detectFBOFormats();
        auto _tryFormat(GLenum depthFormat, GLenum stencilFormat) -> GLuint;
        auto _tryPackedFormat(GLenum packedFormat) -> bool;
        void _createTempFramebuffer(GLuint internalfmt, GLuint fmt, GLenum type, GLuint &fb, GLuint &tid);
    };
    

}

#endif
