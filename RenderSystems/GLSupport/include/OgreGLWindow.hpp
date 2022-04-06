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

#ifndef OGRE_RENDERSYSTEMS_GLSUPPORT_WINDOW_H
#define OGRE_RENDERSYSTEMS_GLSUPPORT_WINDOW_H

#include "OgreGLRenderTarget.hpp"
#include "OgreRenderTarget.hpp"
#include "OgreRenderWindow.hpp"

namespace Ogre
{
class GLContext;
class PixelBox;
struct Box;

    class GLWindow : public RenderWindow, public GLRenderTarget
    {
    public:
        GLWindow();

        [[nodiscard]]
        auto isVisible() const -> bool override { return mVisible; }
        void setVisible(bool visible) override { mVisible = visible; }
        [[nodiscard]]
        auto isHidden() const -> bool override { return mHidden; }

        [[nodiscard]]
        auto isVSyncEnabled() const -> bool override { return mVSync; }
        void setVSyncInterval(unsigned int interval) override;

        void copyContentsToMemory(const Box& src, const PixelBox &dst, FrameBuffer buffer) override;
        [[nodiscard]]
        auto requiresTextureFlipping() const -> bool override { return false; }
        [[nodiscard]]
        auto getContext() const -> GLContext* override { return mContext; }

    protected:
        bool mVisible;
        bool mHidden;
        bool mIsTopLevel;
        bool mIsExternal;
        bool mIsExternalGLControl;
        bool mVSync;

        GLContext*   mContext;
    };
}

#endif
