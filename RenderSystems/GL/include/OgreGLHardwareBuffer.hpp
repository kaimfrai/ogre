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
#include "glad/glad.h"

export module Ogre.RenderSystems.GL:HardwareBuffer;

export import Ogre.Core;

export
namespace Ogre {

    class GLRenderSystem;
    /// Specialisation of HardwareVertexBuffer for OpenGL
    class GLHardwareVertexBuffer : public HardwareBuffer
    {
    private:
        GLenum mTarget;
        GLuint mBufferId;
        // Scratch buffer handling
        bool mLockedToScratch{false};
        size_t mScratchOffset{0};
        size_t mScratchSize{0};
        void* mScratchPtr{nullptr};
        bool mScratchUploadOnUnlock{false};
        GLRenderSystem* mRenderSystem;

    protected:
        /** See HardwareBuffer. */
        auto lockImpl(size_t offset, size_t length, LockOptions options) -> void* override;
        /** See HardwareBuffer. */
        void unlockImpl() override;
    public:
        GLHardwareVertexBuffer(GLenum target, size_t sizeInBytes, Usage usage, bool useShadowBuffer);
        ~GLHardwareVertexBuffer() override;
        /** See HardwareBuffer. */
        void readData(size_t offset, size_t length, void* pDest) override;
        /** See HardwareBuffer. */
        void writeData(size_t offset, size_t length, 
            const void* pSource, bool discardWholeBuffer = false) override;
        /** See HardwareBuffer. */
        void _updateFromShadow() override;

        [[nodiscard]] auto getGLBufferId() const noexcept -> GLuint { return mBufferId; }
    };
    using GLHardwareBuffer = GLHardwareVertexBuffer;

}
