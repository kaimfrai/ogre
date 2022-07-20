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

#include <cstddef>

export module Ogre.RenderSystems.GL:PBRenderTexture;

export import Ogre.Core;
export import Ogre.RenderSystems.GLSupport;

export
namespace Ogre {
    
    /** RenderTexture that uses a PBuffer (offscreen rendering context) for rendering.
    */
    class GLPBRTTManager;
class GLContext;
class GLNativeSupport;
class GLPBuffer;
class RenderTarget;
class RenderTexture;

    class GLPBRenderTexture: public GLRenderTexture
    {
    public:
        GLPBRenderTexture(GLPBRTTManager *manager, std::string_view name, const GLSurfaceDesc &target, bool writeGamma, uint fsaa);
        ~GLPBRenderTexture() override;
        
        void getCustomAttribute(std::string_view name, void* pData) override;

        [[nodiscard]] auto getContext() const noexcept -> GLContext* override;
    protected:
        GLPBRTTManager *mManager;
        PixelComponentType mPBFormat;
    };
        
    /** Manager for rendertextures and PBuffers (offscreen rendering contexts)
    */
    class GLPBRTTManager: public GLRTTManager
    {
    public:
        GLPBRTTManager(GLNativeSupport *support, RenderTarget *mainwindow);
        ~GLPBRTTManager() override;
        
        /** @copydoc GLRTTManager::createRenderTexture
        */
        auto createRenderTexture(std::string_view name, 
            const GLSurfaceDesc &target, bool writeGamma, uint fsaa) -> RenderTexture * override;
        
         /** @copydoc GLRTTManager::checkFormat
        */
        virtual auto checkFormat(PixelFormat format) -> bool;
        
        /** @copydoc GLRTTManager::bind
        */
        void bind(RenderTarget *target) override;
        
        /** @copydoc GLRTTManager::unbind
        */
        void unbind(RenderTarget *target) override;
        
        /** Create PBuffer for a certain pixel format and size
        */
        void requestPBuffer(PixelComponentType ctype, uint32 width, uint32 height);
        
        /** Release PBuffer for a certain pixel format
        */
        void releasePBuffer(PixelComponentType ctype);
        
        /** Get GL rendering context for a certain component type and size.
        */
        auto getContextFor(PixelComponentType ctype, uint32 width, uint32 height) -> GLContext *;
    protected:
        /** GLSupport reference, used to create PBuffers */
        GLNativeSupport *mSupport;
        /** Primary window reference */
        RenderTarget *mMainWindow;
        /** Primary window context */
        GLContext *mMainContext{nullptr};
        /** Reference to a PBuffer, with refcount */
        struct PBRef
        {
            GLPBuffer* pb{nullptr};
            size_t refcount{0};
        };
        /** Type to map each component type to a PBuffer */
        PBRef mPBuffers[std::to_underlying(PixelComponentType::COUNT)];
    };
}
