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
#ifndef OGRE_COMPONENTS_BITES_APPLICATIONCONTEXTBASE_H
#define OGRE_COMPONENTS_BITES_APPLICATIONCONTEXTBASE_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreException.hpp"
#include "OgreFrameListener.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreStaticPluginLoader.hpp"

// forward declarations
extern "C" struct SDL_Window;

namespace Ogre {
    class OverlaySystem;
    class FileSystemLayer;
    class RenderWindow;
    class Root;

    namespace RTShader {
        class ShaderGenerator;
    }  // namespace RTShader
}

namespace OgreBites
{
    class SGTechniqueResolverListener;
    struct InputListener;
    union Event;

    using NativeWindowType = SDL_Window;

    /** \addtogroup Optional Optional Components
    *  @{
    */
    /** \defgroup Bites Bites
    * reusable utilities for rapid prototyping
    *  @{
    */

    /**
     * link between a renderwindow and a platform specific window
     */
    struct NativeWindowPair
    {
        Ogre::RenderWindow* render;
        NativeWindowType* native;
    };

    /**
    Base class responsible for setting up a common context for applications.
    Subclass to implement specific event callbacks.
    */
    class ApplicationContextBase : public Ogre::FrameListener
    {
    public:
        explicit ApplicationContextBase(const Ogre::String& appName = "Ogre3D");

        ~ApplicationContextBase() override;

        /**
         * get the main RenderWindow
         * owns the context on OpenGL
         */
        [[nodiscard]]
        auto getRenderWindow() const -> Ogre::RenderWindow*
        {
            return mWindows.empty() ? nullptr : mWindows[0].render;
        }

        [[nodiscard]]
        auto getRoot() const -> Ogre::Root* {
            return mRoot;
        }

        [[nodiscard]]
        auto getOverlaySystem() const -> Ogre::OverlaySystem* {
            return mOverlaySystem;
        }

        /**
        This function initializes the render system and resources.
        */
        void initApp(ulong frameCount = -1);

        /**
        This function closes down the application - saves the configuration then
        shutdowns.
        */
        void closeApp();

        // callback interface copied from various listeners to be used by ApplicationContext
        auto frameStarted(const Ogre::FrameEvent& evt) -> bool override {
            pollEvents();
            return true;
        }
        auto frameRenderingQueued(const Ogre::FrameEvent& evt) -> bool override;
        auto frameEnded(const Ogre::FrameEvent& evt) -> bool override { return true; }
        virtual void windowMoved(Ogre::RenderWindow* rw) {}
        virtual void windowResized(Ogre::RenderWindow* rw) {}
        virtual auto windowClosing(Ogre::RenderWindow* rw) -> bool { return true; }
        virtual void windowClosed(Ogre::RenderWindow* rw) {}
        virtual void windowFocusChange(Ogre::RenderWindow* rw) {}

        /**
         * inspect the event and call one of the corresponding functions on the registered InputListener
         * @param event Input Event
         * @param windowID only call listeners of this window
         */
        void _fireInputEvent(const Event& event, uint32_t windowID) const;

        /**
          Initialize the RT Shader system.
          */
        auto initialiseRTShaderSystem() -> bool;

        /**
         * make the RTSS write out the generated shaders for caching and debugging
         *
         * by default all shaders are generated to system memory.
         * Must be called before loadResources
         * @param write Whether to write out the generated shaders
         */
        void setRTSSWriteShadersToDisk(bool write);

        /**
        Destroy the RT Shader system.
          */
        void destroyRTShaderSystem();

        /**
         Sets up the context after configuration.
         */
        virtual void setup();

        /**
        Creates the OGRE root.
        */
        virtual void createRoot(ulong frameCount = -1);

        /**
        Configures the startup settings for OGRE. I use the config dialog here,
        but you can also restore from a config file. Note that this only happens
        when you start the context, and not when you reset it.
        */
        virtual auto oneTimeConfig() -> bool;

        /**
        When input is grabbed the mouse is confined to the window.
        */
        virtual void setWindowGrab(NativeWindowType* win, bool grab = true) {}

        /// get the vertical DPI of the display
        [[nodiscard]]
        virtual auto getDisplayDPI() const -> float { return 96.0f; }

        /// @overload
        void setWindowGrab(bool grab = true) {
            Ogre::OgreAssert(!mWindows.empty(), "create a window first");
            setWindowGrab(mWindows[0].native, grab);
        }

        /**
        Finds context-wide resource groups. I load paths from a config file here,
        but you can choose your resource locations however you want.
        */
        virtual void locateResources();

        /**
        Loads context-wide resource groups. I chose here to simply initialise all
        groups, but you can fully load specific ones if you wish.
        */
        virtual void loadResources();

        /**
        Reconfigures the context. Attempts to preserve the current sample state.
        */
        virtual void reconfigure(const Ogre::String& renderer, Ogre::NameValuePairList& options);


        /**
        Cleans up and shuts down the context.
        */
        virtual void shutdown();

        /**
        process all window events since last call
        */
        virtual void pollEvents();

        /**
        Creates dummy scene to allow rendering GUI in viewport.
          */
        void createDummyScene();

        /**
        Destroys dummy scene.
          */
        void destroyDummyScene();

        /** attach input listener
         *
         * @param lis the listener
         * @param win the window to receive the events for.
         */
        virtual void addInputListener(NativeWindowType* win, InputListener* lis);

        /// @overload
        void addInputListener(InputListener* lis) {
            Ogre::OgreAssert(!mWindows.empty(), "create a window first");
            addInputListener(mWindows[0].native, lis);
        }

        /** detatch input listener
         *
         * @param lis the listener
         * @param win the window to receive the events for.
         */
        virtual void removeInputListener(NativeWindowType* win, InputListener* lis);

        /// @overload
        void removeInputListener(InputListener* lis) {
            Ogre::OgreAssert(!mWindows.empty(), "called after all windows were deleted");
            removeInputListener(mWindows[0].native, lis);
        }

        /**
         * Create a new render window
         *
         * You must use SDL and not an auto-created window as SDL does not get the events
         * otherwise.
         *
         * By default the values from ogre.cfg are used for w, h and miscParams.
         */
        virtual auto
        createWindow(const Ogre::String& name, uint32_t w = 0, uint32_t h = 0,
                     Ogre::NameValuePairList miscParams = Ogre::NameValuePairList()) -> NativeWindowPair;

        /// destroy and erase an NativeWindowPair by name
        void destroyWindow(const Ogre::String& name);

        /**
         * get the FileSystemLayer instace pointing to an application specific directory
         */
        auto getFSLayer() -> Ogre::FileSystemLayer& { return *mFSLayer; }

        /**
         * the directory where the media files were installed
         *
         * same as OGRE_MEDIA_DIR in CMake
         */
        static auto getDefaultMediaDir() -> Ogre::String;
    protected:
        /// internal method to destroy both the render and the native window
        virtual void _destroyWindow(const NativeWindowPair& win);

        Ogre::OverlaySystem* mOverlaySystem;  // Overlay system

        Ogre::FileSystemLayer* mFSLayer; // File system abstraction layer
        Ogre::Root* mRoot;              // OGRE root
        StaticPluginLoader mStaticPluginLoader;
        bool mFirstRun;
        Ogre::String mNextRenderer;     // name of renderer used for next run
        Ogre::String mAppName;

        using WindowList = std::vector<NativeWindowPair>;
        WindowList mWindows; // all windows

        using InputListenerList = std::set<std::pair<uint32_t, InputListener *>>;
        InputListenerList mInputListeners;

        Ogre::RTShader::ShaderGenerator*       mShaderGenerator; // The Shader generator instance.
        SGTechniqueResolverListener*       mMaterialMgrListener; // Shader generator material manager listener.
    };

    /** @} */
    /** @} */
}
#endif
