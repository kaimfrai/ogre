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

export module Ogre.Core:RenderTexture;

export import :PixelFormat;
export import :Platform;
export import :Prerequisites;
export import :RenderTarget;

export import <vector>;

export
namespace Ogre
{    
    class HardwarePixelBuffer;
    struct Box;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */
    /** This class represents a RenderTarget that renders to a Texture. There is no 1 on 1
        relation between Textures and RenderTextures, as there can be multiple 
        RenderTargets rendering to different mipmaps, faces (for cubemaps) or slices (for 3D textures)
        of the same Texture.
    */
    class RenderTexture: public RenderTarget
    {
    public:
        RenderTexture(HardwarePixelBuffer *buffer, uint32 zoffset);
        ~RenderTexture() override;

        using RenderTarget::copyContentsToMemory;
        void copyContentsToMemory(const Box& src, const PixelBox &dst, FrameBuffer buffer = FrameBuffer::AUTO) override;
        [[nodiscard]] auto suggestPixelFormat() const noexcept -> PixelFormat override;

    protected:
        HardwarePixelBuffer *mBuffer;
        uint32 mZOffset;
    };

    /** This class represents a render target that renders to multiple RenderTextures
        at once. Surfaces can be bound and unbound at will, as long as the following constraints
        are met:
        - All bound surfaces have the same size
        - All bound surfaces have the same bit depth
        - Target 0 is bound
    */
    class MultiRenderTarget: public RenderTarget
    {
    public:
        MultiRenderTarget(std::string_view name);

        /** Bind a surface to a certain attachment point.
            @param attachment   0 .. mCapabilities->getNumMultiRenderTargets()-1
            @param target       RenderTexture to bind.

            It does not bind the surface and fails with an exception (ERR_INVALIDPARAMS) if:
            - Not all bound surfaces have the same size
            - Not all bound surfaces have the same internal format 
        */

        virtual void bindSurface(size_t attachment, RenderTexture *target);

        /** Unbind attachment.
        */
        virtual void unbindSurface(size_t attachment)
        {
            if (attachment < mBoundSurfaces.size())
                mBoundSurfaces[attachment] = nullptr;
            unbindSurfaceImpl(attachment);
        }

        using RenderTarget::copyContentsToMemory;

        /** Error throwing implementation, it's not possible to write a MultiRenderTarget
            to disk. 
        */
        void copyContentsToMemory(const Box& src, const PixelBox &dst, FrameBuffer buffer = FrameBuffer::AUTO) override;

        /// Irrelevant implementation since cannot copy
        [[nodiscard]] auto suggestPixelFormat() const noexcept -> PixelFormat override { return PixelFormat::UNKNOWN; }

        using BoundSufaceList = std::vector<RenderTexture *>;
        /// Get a list of the surfaces which have been bound
        [[nodiscard]] auto getBoundSurfaceList() const noexcept -> const BoundSufaceList& { return mBoundSurfaces; }

        /** Get a pointer to a bound surface */
        auto getBoundSurface(size_t index) -> RenderTexture* { return mBoundSurfaces.at(index); }

    protected:
        BoundSufaceList mBoundSurfaces;

        /// Implementation of bindSurface, must be provided
        virtual void bindSurfaceImpl(size_t attachment, RenderTexture *target) = 0;
        /// Implementation of unbindSurface, must be provided
        virtual void unbindSurfaceImpl(size_t attachment) = 0;


    };
    /** @} */
    /** @} */
}
