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

#include "glad/glad.h"

export module Ogre.RenderSystems.GL:Texture;

export import Ogre.Core;
export import Ogre.RenderSystems.GLSupport;

export
namespace Ogre {

    class GLRenderSystem;
    class GLTexture : public GLTextureCommon
    {
    public:
        // Constructor
        GLTexture(ResourceManager* creator, std::string_view name, ResourceHandle handle,
            std::string_view group, bool isManual, ManualResourceLoader* loader, 
            GLRenderSystem* renderSystem);

        ~GLTexture() override;      

        /// Takes the OGRE texture type (1d/2d/3d/cube) and returns the appropriate GL one
        auto getGLTextureTarget() const -> GLenum;

    protected:
        /// @copydoc Texture::createInternalResourcesImpl
        void createInternalResourcesImpl() override;
        /// @copydoc Texture::freeInternalResourcesImpl
        void freeInternalResourcesImpl() override;

        /** internal method, create GLHardwarePixelBuffers for every face and
             mipmap level. This method must be called after the GL texture object was created,
            the number of mipmaps was set (GL_TEXTURE_MAX_LEVEL) and glTexImageXD was called to
            actually allocate the buffer
        */
        void _createSurfaceList();

    private:
        GLRenderSystem* mRenderSystem;
    };
}
