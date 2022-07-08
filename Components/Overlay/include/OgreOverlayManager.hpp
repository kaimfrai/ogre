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
#ifndef OGRE_COMPONENTS_OVERLAY_MANAGER_H
#define OGRE_COMPONENTS_OVERLAY_MANAGER_H

#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "OgreFrustum.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreScriptLoader.hpp"
#include "OgreSingleton.hpp"
#include "OgreStringVector.hpp"

namespace Ogre {
    class Overlay;
    class OverlayContainer;
    class OverlayElement;
    class OverlayElementFactory;
    template <typename T> class MapIterator;
    class Camera;
    class RenderQueue;
    class ScriptTranslatorManager;
    class Viewport;

    /** \addtogroup Optional
    *  @{
    */
    /** \addtogroup Overlays
    *  @{
    */
    /** Manages Overlay objects, parsing them from .overlay files and
        storing a lookup library of them. Also manages the creation of 
        OverlayContainers and OverlayElements, used for non-interactive 2D 
        elements such as HUDs.
    */
    class OverlayManager : public Singleton<OverlayManager>, public ScriptLoader, public OverlayAlloc
    {
    public:
        using OverlayMap = std::map<std::string, Overlay*, std::less<>>;
        using ElementMap = std::map<std::string, OverlayElement*, std::less<>>;
        using FactoryMap = std::map<std::string_view, ::std::unique_ptr<OverlayElementFactory>>;
    private:
        OverlayMap mOverlayMap;
        StringVector mScriptPatterns;

        int mLastViewportWidth{0}, mLastViewportHeight{0};
        OrientationMode mLastViewportOrientationMode{OR_DEGREE_0};
        float mPixelRatio{1};

        auto parseChildren( DataStreamPtr& chunk, StringView line, int& l,
            Overlay* pOverlay, bool isTemplate, OverlayContainer* parent = nullptr) -> bool;

        FactoryMap mFactories;

        ElementMap mElements;

        using LoadedScripts = std::set<String>;
        LoadedScripts mLoadedScripts;

        std::unique_ptr<ScriptTranslatorManager> mTranslatorManager;

    public:
        OverlayManager();
        ~OverlayManager() override;

        /** Notifies that hardware resources were lost */
        void _releaseManualHardwareResources();
        /** Notifies that hardware resources should be restored */
        void _restoreManualHardwareResources();

        /// @copydoc ScriptLoader::getScriptPatterns
        [[nodiscard]] auto getScriptPatterns() const noexcept -> const StringVector& override;
        /// @copydoc ScriptLoader::parseScript
        void parseScript(DataStreamPtr& stream, StringView groupName) override;
        /// @copydoc ScriptLoader::getLoadingOrder
        [[nodiscard]] auto getLoadingOrder() const -> Real override;

        void addOverlay(Overlay* overlay);

        /** Create a new Overlay. */
        auto create(StringView name) -> Overlay*;
        /** Retrieve an Overlay by name 
        @return A pointer to the Overlay, or 0 if not found
        */
        auto getByName(StringView name) -> Overlay*;
        /** Destroys an existing overlay by name */
        void destroy(StringView name);
        /** Destroys an existing overlay */
        void destroy(Overlay* overlay);
        /** Destroys all existing overlays */
        void destroyAll();
        using OverlayMapIterator = MapIterator<OverlayMap>;
        auto getOverlayIterator() -> OverlayMapIterator;

        /** Internal method for queueing the visible overlays for rendering. */
        void _queueOverlaysForRendering(Camera* cam, RenderQueue* pQueue, Viewport *vp);

        /** Gets the height of the destination viewport in pixels. */
        [[nodiscard]] auto getViewportHeight() const noexcept -> int;
        
        /** Gets the width of the destination viewport in pixels. */
        [[nodiscard]] auto getViewportWidth() const noexcept -> int;
        [[nodiscard]] auto getViewportAspectRatio() const -> Real;

        /** Gets the orientation mode of the destination viewport. */
        [[nodiscard]] auto getViewportOrientationMode() const -> OrientationMode;

       /** Sets the pixel ratio: how many viewport pixels represent a single overlay pixel (in one dimension).

       By default this is an 1:1 mapping. However on HiDPI screens you want to increase that to scale up your Overlay.
       @see RenderWindow::getViewPointToPixelScale */
       void setPixelRatio(float ratio);
       [[nodiscard]] auto getPixelRatio() const noexcept -> float;

        /** Creates a new OverlayElement of the type requested.
        @remarks
        The type of element to create is passed in as a string because this
        allows plugins to register new types of component.
        @param typeName The type of element to create.
        @param instanceName The name to give the new instance.
        */
        auto createOverlayElement(StringView typeName, StringView instanceName, bool = false) -> OverlayElement*;

        /** Gets a reference to an existing element. */
        auto getOverlayElement(StringView name, bool = false) -> OverlayElement*;

        /** Tests if an element exists. */
        auto hasOverlayElement(StringView name, bool = false) -> bool;
        
        /** Destroys a OverlayElement. 
        @remarks
        Make sure you're not still using this in an Overlay. If in
        doubt, let OGRE destroy elements on shutdown.
        */
        void destroyOverlayElement(StringView instanceName, bool = false);

        /** Destroys a OverlayElement. 
        @remarks
        Make sure you're not still using this in an Overlay. If in
        doubt, let OGRE destroy elements on shutdown.
        */
        void destroyOverlayElement(OverlayElement* pInstance, bool = false);

        /** Destroys all the OverlayElement  created so far.
        @remarks
        Best to leave this to the engine to call internally, there
        should rarely be a need to call it yourself.
        */
        void destroyAllOverlayElements(bool = false);

        /** Registers a new OverlayElementFactory with this manager.
        @remarks
        Should be used by plugins or other apps wishing to provide
        a new OverlayElement subclass.
        */
        void addOverlayElementFactory(OverlayElementFactory* elemFactory);
        
        /** Get const access to the list of registered OverlayElement factories. */
        [[nodiscard]] auto getOverlayElementFactoryMap() const noexcept -> const FactoryMap& {
            return mFactories;
        }

        auto createOverlayElementFromTemplate(StringView templateName, StringView typeName, StringView instanceName, bool = false) -> OverlayElement*;
        /**
        *  @remarks
        *  Creates a new OverlayElement object from the specified template name.  The new
        *  object's name, and all of it's children, will be instanceName/orignalName.
        */
        auto cloneOverlayElementFromTemplate(StringView templateName, StringView instanceName) -> OverlayElement*;

        auto createOverlayElementFromFactory(StringView typeName, StringView instanceName) -> OverlayElement*;

        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static auto getSingleton() noexcept -> OverlayManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> OverlayManager*;
    };


    /** @} */
    /** @} */

}


#endif 
