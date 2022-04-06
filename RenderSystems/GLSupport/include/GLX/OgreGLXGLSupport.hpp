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
#ifndef OGRE_RENDERSYSTEMS_GLSUPPORT_GLX_GLSUPPORT_H
#define OGRE_RENDERSYSTEMS_GLSUPPORT_GLX_GLSUPPORT_H

#include "OgreCommon.hpp"
#include "OgreGLNativeSupport.hpp"
#include "OgrePixelFormat.hpp"
#include "OgrePrerequisites.hpp"

namespace Ogre {
class GLPBuffer;
class RenderWindow;
}  // namespace Ogre

#define GLX_GLXEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstddef>

namespace Ogre {

    class GLXGLSupport : public GLNativeSupport
    {
    public:
        GLXGLSupport(int profile);
        ~GLXGLSupport();

        Atom mAtomDeleteWindow;
        Atom mAtomFullScreen;
        Atom mAtomState;

        /// @copydoc RenderSystem::createRenderWindow
        auto newWindow(const String &name, unsigned int width, unsigned int height,
                                bool fullScreen, const NameValuePairList *miscParams = 0) -> RenderWindow*;

        /// @copydoc GLNativeSupport::createPBuffer
        auto createPBuffer(PixelComponentType format, size_t width, size_t height) -> GLPBuffer*;

        /** @copydoc see GLNativeSupport::start */
        void start();

        /** @copydoc see GLNativeSupport::stop */
        void stop();

        /** @copydoc see GLNativeSupport::initialiseExtensions */
        void initialiseExtensions();

        /** @copydoc see GLNativeSupport::getProcAddress */
        auto getProcAddress(const char* procname) const -> void*;

        // The remaining functions are internal to the GLX Rendersystem:

        /**
         * Get the name of the display and screen used for rendering
         *
         * Ogre normally opens its own connection to the X server
         * and renders onto the screen where the user logged in
         *
         * However, if Ogre is passed a current GL context when the first
         * RenderTarget is created, then it will connect to the X server
         * using the same connection as that GL context and direct all
         * subsequent rendering to the screen targeted by that GL context.
         *
         * @returns              Display name.
         */
        auto getDisplayName () -> String;

        /**
         * Get the Display connection used for rendering
         *
         * This function establishes the initial connection when necessary.
         *
         * @returns              Display connection
         */
        auto getGLDisplay() -> Display*;

        /**
         * Get the Display connection used for window management & events
         *
         * @returns              Display connection
         */
        auto getXDisplay() -> Display*;

        /**
         * Switch video modes
         *
         * @param width   Receiver for requested and final width
         * @param height         Receiver for requested and final drawable height
         * @param height         Receiver for requested and final drawable frequency
         */
        void switchMode (uint& width, uint& height, short& frequency);

        /**
         * Switch back to original video mode
         */
        void switchMode ();

        /**
         * Get the GLXFBConfig used to create a ::GLXContext
         *
         * @param drawable   GLXContext
         * @returns               GLXFBConfig used to create the context
         */
        auto getFBConfigFromContext (::GLXContext context) -> GLXFBConfig;

        /**
         * Get the GLXFBConfig used to create a GLXDrawable.
         * Caveat: GLX version 1.3 is needed when the drawable is a GLXPixmap
         *
         * @param drawable   GLXDrawable
         * @param width   Receiver for the drawable width
         * @param height         Receiver for the drawable height
         * @returns               GLXFBConfig used to create the drawable
         */
        auto getFBConfigFromDrawable (GLXDrawable drawable, unsigned int *width, unsigned int *height) -> GLXFBConfig;

        /**
         * Select an FBConfig given a list of required and a list of desired properties
         *
         * @param minAttribs FBConfig attributes that must be provided with minimum values
         * @param maxAttribs FBConfig attributes that are desirable with maximum values
         * @returns               GLXFBConfig with attributes or 0 when unsupported.
         */
        auto selectFBConfig(const int *minAttribs, const int *maxAttribs) -> GLXFBConfig;

        /**
         * Gets a GLXFBConfig compatible with a VisualID
         *
         * Some platforms fail to implement glXGetFBconfigFromVisualSGIX as
         * part of the GLX_SGIX_fbconfig extension, but this portable
         * alternative suffices for the creation of compatible contexts.
         *
         * @param visualid   VisualID
         * @returns               FBConfig for VisualID
         */
        auto getFBConfigFromVisualID(VisualID visualid) -> GLXFBConfig;

        /**
         * Portable replacement for glXChooseFBConfig
         */
        auto chooseFBConfig(const GLint *attribList, GLint *nElements) -> GLXFBConfig*;

        /**
         * Portable replacement for glXCreateNewContext
         */
        auto createNewContext(GLXFBConfig fbConfig, GLint renderType, ::GLXContext shareList, GLboolean direct) const -> ::GLXContext;

        /**
         * Portable replacement for glXGetFBConfigAttrib
         */
        auto getFBConfigAttrib(GLXFBConfig fbConfig, GLint attribute, GLint *value) -> GLint;

        /**
         * Portable replacement for glXGetVisualFromFBConfig
         */
        auto getVisualFromFBConfig(GLXFBConfig fbConfig) -> XVisualInfo*;

    private:
        Display* mGLDisplay; // used for GL/GLX commands
        Display* mXDisplay;  // used for other X commands and events
        bool mIsExternalDisplay;

        VideoMode  mOriginalMode;
        VideoMode  mCurrentMode;

        int mGLXVerMajor, mGLXVerMinor;
    };
}

#endif // OGRE_RENDERSYSTEMS_GLSUPPORT_GLX_GLSUPPORT_H
