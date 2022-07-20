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
export module Ogre.RenderSystems.GL:FrameBufferObject;

export import Ogre.Core;
export import Ogre.RenderSystems.GLSupport;

export
namespace Ogre {
    class DepthBuffer;
    class GLFBOManager;
    
    /** Frame Buffer Object abstraction.
    */
    class GLFrameBufferObject : public GLFrameBufferObjectCommon
    {
    public:
        GLFrameBufferObject(GLFBOManager *manager, uint fsaa);
        ~GLFrameBufferObject() override;

        auto bind(bool recreateIfNeeded) -> bool override;

        /** Swap buffers - only useful when using multisample buffers.
        */
        void swapBuffers();

        /** This function acts very similar to @see GLFBORenderTexture::attachDepthBuffer
            The difference between D3D & OGL is that D3D setups the DepthBuffer before rendering,
            while OGL setups the DepthBuffer per FBO. So the DepthBuffer (RenderBuffer) needs to
            be attached for OGL.
        */
        void attachDepthBuffer( DepthBuffer *depthBuffer );
        void detachDepthBuffer();
        
        auto getManager() noexcept -> GLFBOManager * { return mManager; }
    private:
        GLFBOManager *mManager;
        GLSurfaceDesc mMultisampleColourBuffer;

        void initialise() override;
    };

}
