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

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/X.h>

module Ogre.RenderSystems.GLSupport;

import :GLRenderSystemCommon;
import :GLX.Context;
import :GLX.GLSupport;

import Ogre.Core;

namespace Ogre
{
    GLXContext::GLXContext(GLXGLSupport* glsupport, ::GLXFBConfig fbconfig, ::GLXDrawable drawable, ::GLXContext context) :
        mDrawable(drawable),  mFBConfig(fbconfig), mGLSupport(glsupport) 
    {
        auto *renderSystem = static_cast<GLRenderSystemCommon*>(Root::getSingleton().getRenderSystem());
        auto* mainContext = static_cast<GLXContext*>(renderSystem->_getMainContext());
        ::GLXContext shareContext = nullptr;

        if (mainContext)
        {
            shareContext = mainContext->mContext;
        }

        if (context)
        {
            mContext = context;
            mExternalContext = true;
        }
        else
        {
            mContext = mGLSupport->createNewContext(mFBConfig, GLX_RGBA_TYPE, shareContext, GL_TRUE);
        }

        if (! mContext)
        {
            OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, "Unable to create a suitable GLXContext", "GLXContext::GLXContext");
        }
    }

    GLXContext::~GLXContext()
    {
        auto *rs = static_cast<GLRenderSystemCommon*>(Root::getSingleton().getRenderSystem());

        if (!mExternalContext)
            glXDestroyContext(mGLSupport->getGLDisplay(), mContext);

        rs->_unregisterContext(this);
    }

    void GLXContext::setCurrent()
    {
        glXMakeCurrent(mGLSupport->getGLDisplay(), mDrawable, mContext);
    }

    void GLXContext::endCurrent()
    {
        glXMakeCurrent(mGLSupport->getGLDisplay(), None, None);
    }

    auto GLXContext::clone() const -> GLContext*
    {
        return new GLXContext(mGLSupport, mFBConfig, mDrawable);
    }
}
