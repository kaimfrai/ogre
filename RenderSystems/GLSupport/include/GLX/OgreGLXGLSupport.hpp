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

#define GLX_GLXEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstddef>

export module Ogre.RenderSystems.GLSupport:GLX.GLSupport;

export import :GLNativeSupport;

export import Ogre.Core;

export
namespace Ogre {

    class GLXGLSupport : public GLNativeSupport
    {
    public:
        GLXGLSupport(GLNativeSupport::ContextProfile profile);
        ~GLXGLSupport() override;

        Atom mAtomDeleteWindow;
        Atom mAtomFullScreen;
        Atom mAtomState;

        /// @copydoc RenderSystem::createRenderWindow
        auto newWindow(std::string_view name, unsigned int width, unsigned int height,
                                bool fullScreen, const NameValuePairList *miscParams = nullptr) -> RenderWindow* override;

        /// @copydoc GLNativeSupport::createPBuffer
        auto createPBuffer(PixelComponentType format, size_t width, size_t height) -> GLPBuffer* override;

        /** @copydoc see GLNativeSupport::start */
        void start() override;

        /** @copydoc see GLNativeSupport::stop */
        void stop() override;

        /** @copydoc see GLNativeSupport::initialiseExtensions */
        void initialiseExtensions();

        /** @copydoc see GLNativeSupport::getProcAddress */
        auto getProcAddress(const char* procname) const -> void* override;

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
        auto getGLDisplay() noexcept -> Display*;

        /**
         * Get the Display connection used for window management & events
         *
         * @returns              Display connection
         */
        auto getXDisplay() noexcept -> Display*;

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
        Display* mGLDisplay{nullptr}; // used for GL/GLX commands
        Display* mXDisplay{nullptr};  // used for other X commands and events
        bool mIsExternalDisplay;

        VideoMode  mOriginalMode;
        VideoMode  mCurrentMode;

        int mGLXVerMajor, mGLXVerMinor;
    };
}
