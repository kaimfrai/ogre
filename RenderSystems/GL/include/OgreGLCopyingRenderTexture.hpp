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
export module Ogre.RenderSystems.GL:CopyingRenderTexture;

export import Ogre.Core;
export import Ogre.RenderSystems.GLSupport;

export import <cstddef>;

export
namespace Ogre {

    class GLCopyingRTTManager;

    /** RenderTexture for simple copying from frame buffer
    */
    class GLCopyingRenderTexture: public GLRenderTexture
    {
    public:
        GLCopyingRenderTexture(GLCopyingRTTManager *manager, const String &name, const GLSurfaceDesc &target, 
            bool writeGamma, uint fsaa);
        
        void getCustomAttribute(const String& name, void* pData) override;

        [[nodiscard]]
        auto getContext() const -> GLContext* override { return nullptr; }
    };
    
    /** Simple, copying manager/factory for RenderTextures. This is only used as the last fallback if
        both PBuffers and FBOs aren't supported.
    */
    class GLCopyingRTTManager: public GLRTTManager
    {
    public:
        auto createRenderTexture(const String &name, const GLSurfaceDesc &target, bool writeGamma, uint fsaa) -> RenderTexture * override {
            return new GLCopyingRenderTexture(this, name, target, writeGamma, fsaa);
        }

        auto checkFormat(PixelFormat format) -> bool {
            return true;
        }
        
        void bind(RenderTarget *target) override {
            // Nothing to do here
        }

        void unbind(RenderTarget *target) override;
    };
}
