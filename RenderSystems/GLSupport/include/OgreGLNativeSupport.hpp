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

#ifndef OGRE_RENDERSYSTEMS_GLSUPPORT_NATIVESUPPORT_H
#define OGRE_RENDERSYSTEMS_GLSUPPORT_NATIVESUPPORT_H

#include "OgreConfigOptionMap.hpp"
#include "OgreException.hpp"
#include "OgrePixelFormat.hpp"

namespace Ogre
{
    class GLPBuffer;

    struct VideoMode {
        uint32 width;
        uint32 height;
        int16 refreshRate;
        uint8  bpp;

        [[nodiscard]] auto getDescription() const -> String;
    };
    using VideoModes = std::vector<VideoMode>;

    /**
    * provides OpenGL Context creation using GLX, WGL, EGL, Cocoa
    */
    class GLNativeSupport
    {
        public:
            using ExtensionList = std::set<StringView>;

            enum ContextProfile {
                CONTEXT_CORE = 1,
                CONTEXT_COMPATIBILITY = 2,
                CONTEXT_ES = 4
            };

            GLNativeSupport(int profile) : mContextProfile(ContextProfile(profile)) {}
            virtual ~GLNativeSupport() = default;

            /// @copydoc RenderSystem::_createRenderWindow
            virtual auto newWindow(StringView name,
                                            unsigned int width, unsigned int height,
                                            bool fullScreen,
                                            const NameValuePairList *miscParams = nullptr) -> RenderWindow* = 0;

            virtual auto createPBuffer(PixelComponentType format, size_t width, size_t height) -> GLPBuffer* {
                return nullptr;
            }

            /**
            * Get the address of a function
            */
            virtual auto getProcAddress(const char* procname) const -> void * = 0;

            [[nodiscard]] auto checkExtension(StringView ext) const -> bool {
                return extensionList.find(ext) != extensionList.end();
            }

            /// @copydoc RenderSystem::getDisplayMonitorCount
            [[nodiscard]] virtual auto getDisplayMonitorCount() const noexcept -> unsigned int
            {
                return 1;
            }

            /**
            * Start anything special
            */
            virtual void start() = 0;
            /**
            * Stop anything special
            */
            virtual void stop() = 0;

            /**
            * Add any special config values to the system.
            */
            virtual auto getConfigOptions() -> ConfigOptionMap { return {}; }

            [[nodiscard]] auto getFSAALevels() const noexcept -> const std::vector<int>& { return mFSAALevels; }
            [[nodiscard]] auto getVideoModes() const noexcept -> const VideoModes& { return mVideoModes; }

            [[nodiscard]] auto getContextProfile() const noexcept -> ContextProfile { return mContextProfile; }
        protected:
            // Allowed video modes
            VideoModes mVideoModes;
            std::vector<int> mFSAALevels;

            // Supported platform extensions (e.g EGL_*, GLX_*)
            ExtensionList extensionList;

            ContextProfile mContextProfile;
    };
}

#endif
