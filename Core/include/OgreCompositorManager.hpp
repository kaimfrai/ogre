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

export module Ogre.Core:CompositorManager;

export import :Common;
export import :CompositionTechnique;
export import :IteratorWrapper;
export import :PixelFormat;
export import :Platform;
export import :Prerequisites;
export import :Rectangle2D;
export import :Resource;
export import :ResourceGroupManager;
export import :ResourceManager;
export import :SharedPtr;
export import :Singleton;
export import :Texture;

export import <map>;
export import <memory>;
export import <set>;
export import <string>;
export import <tuple>;
export import <utility>;
export import <vector>;

export
namespace Ogre {

class CompositorChain;
class CompositorInstance;
class CompositorLogic;
class CustomCompositionPass;
class Renderable;
class Viewport;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
    /** Class for managing Compositor settings for Ogre. Compositors provide the means
        to flexibly "composite" the final rendering result from multiple scene renders
        and intermediate operations like rendering fullscreen quads. This makes
        it possible to apply postfilter effects, HDRI postprocessing, and shadow
        effects to a Viewport.
        @par
            When loaded from a script, a Compositor is in an 'unloaded' state and only stores the settings
            required. It does not at that stage load any textures. This is because the material settings may be
            loaded 'en masse' from bulk material script files, but only a subset will actually be required.
        @par
            Because this is a subclass of ResourceManager, any files loaded will be searched for in any path or
            archive added to the resource paths/archives. See ResourceManager for details.
    */
    class CompositorManager : public ResourceManager, public Singleton<CompositorManager>
    {
    public:
        CompositorManager();
        ~CompositorManager() override;

        /** Initialises the Compositor manager, which also triggers it to
            parse all available .compositor scripts. */
        void initialise();

        /**
         * Create a new compositor
         * @see ResourceManager::createResource
         */
        auto create (std::string_view name, std::string_view group,
                            bool isManual = false, ManualResourceLoader* loader = nullptr,
                            const NameValuePairList* createParams = nullptr) -> CompositorPtr;

        /// Get a resource by name
        /// @see ResourceManager::getResourceByName
        auto getByName(std::string_view name, std::string_view groupName = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME) const -> CompositorPtr;

        /** Get the compositor chain for a Viewport. If there is none yet, a new
            compositor chain is registered.
            XXX We need a _notifyViewportRemoved to find out when this viewport disappears,
            so we can destroy its chain as well.
        */
        auto getCompositorChain(Viewport *vp) -> CompositorChain *;

        /** Returns whether exists compositor chain for a viewport.
        */
        auto hasCompositorChain(const Viewport *vp) const -> bool;

        /** Remove the compositor chain from a viewport if exists.
        */
        void removeCompositorChain(const Viewport *vp);

        /** Add a compositor to a viewport. By default, it is added to end of the chain,
            after the other compositors.
            @param vp           Viewport to modify
            @param compositor   The name of the compositor to apply
            @param addPosition  At which position to add, defaults to the end (-1).
            @return pointer to instance, or 0 if it failed.
        */
        auto addCompositor(Viewport *vp, std::string_view compositor, int addPosition=-1) -> CompositorInstance *;

        /** Remove a compositor from a viewport
        */
        void removeCompositor(Viewport *vp, std::string_view compositor);

        /** Set the state of a compositor on a viewport to enabled or disabled.
            Disabling a compositor stops it from rendering but does not free any resources.
            This can be more efficient than using removeCompositor and addCompositor in cases
            the filter is switched on and off a lot.
        */
        void setCompositorEnabled(Viewport *vp, std::string_view compositor, bool value);

        /** Get a textured fullscreen 2D rectangle, for internal use.
        */
        auto _getTexturedRectangle2D() -> Renderable *;

        /** Overridden from ResourceManager since we have to clean up chains too. */
        void removeAll() override;

        /** Internal method for forcing all active compositors to recreate their resources. */
        void _reconstructAllCompositorResources();

        using UniqueTextureSet = std::set<Texture *>;

        /** Utility function to get an existing pooled texture matching a given
            definition, or creating one if one doesn't exist. It also takes into
            account whether a pooled texture has already been supplied to this
            same requester already, in which case it won't give the same texture
            twice (this is important for example if you request 2 ping-pong textures, 
            you don't want to get the same texture for both requests!
        */
        auto getPooledTexture(std::string_view name, std::string_view localName, 
            uint32 w, uint32 h,
            PixelFormat f, uint aa, std::string_view aaHint, bool srgb, UniqueTextureSet& texturesAlreadyAssigned, 
            CompositorInstance* inst, CompositionTechnique::TextureScope scope, TextureType type = TextureType::_2D) -> TexturePtr;

        /** Free pooled textures from the shared pool (compositor instances still 
            using them will keep them in memory though). 
        */
        void freePooledTextures(bool onlyIfUnreferenced = true);

        /** Register a compositor logic for listening in to expecting composition
            techniques.
        */
        void registerCompositorLogic(std::string_view name, CompositorLogic* logic);

        /** Removes a listener for compositor logic registered with registerCompositorLogic
        */
        void unregisterCompositorLogic(std::string_view name);
        
        /** Get a compositor logic by its name
        */
        auto getCompositorLogic(std::string_view name) -> CompositorLogic*;

		/** Check if a compositor logic exists
		*/
		auto hasCompositorLogic(std::string_view name) -> bool;
		
        /** Register a custom composition pass.
        */
        void registerCustomCompositionPass(std::string_view name, CustomCompositionPass* customPass);

        void unregisterCustomCompositionPass(std::string_view name);

        /** Get a custom composition pass by its name 
        */
        auto getCustomCompositionPass(std::string_view name) -> CustomCompositionPass*;

		/** Check if a compositor pass exists
		*/
        auto hasCustomCompositionPass(std::string_view name) -> bool;

        /**
        Relocates a compositor chain from one viewport to another
        @param sourceVP The viewport to take the chain from
        @param destVP The viewport to connect the chain to
        */
        void _relocateChain(Viewport* sourceVP, Viewport* destVP);

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> CompositorManager&;

        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> CompositorManager*;
    
    private:
        auto createImpl(std::string_view name, ResourceHandle handle,
            std::string_view group, bool isManual, ManualResourceLoader* loader,
            const NameValuePairList* params) -> Resource* override;

        using Chains = std::map<const Viewport *, CompositorChain *>;
        Chains mChains;

        /** Clear composition chains for all viewports
         */
        void freeChains();

        ::std::unique_ptr<Rectangle2D> mRectangle{nullptr};

        /// List of instances
        using Instances = std::vector<CompositorInstance *>;
        Instances mInstances;

        /// Map of registered compositor logics
        using CompositorLogicMap = std::map<std::string_view, CompositorLogic *>;
        CompositorLogicMap mCompositorLogics;

        /// Map of registered custom composition passes
        using CustomCompositionPassMap = std::map<std::string_view, CustomCompositionPass *>;
        CustomCompositionPassMap mCustomCompositionPasses;

        using TextureList = std::vector<TexturePtr>;
        using TextureIterator = VectorIterator<TextureList>;

        struct TextureDef
        {
            size_t width, height;
            TextureType type;
            PixelFormat format;
            uint fsaa;
            std::string_view fsaaHint;
            bool sRGBwrite;

            [[nodiscard]] auto operator<=> (const TextureDef& y) const noexcept = default;
        };
        using TexturesByDef = std::map<TextureDef, TextureList>;
        TexturesByDef mTexturesByDef;

        using StringPair = std::pair<String, String>;
        using TextureDefMap = std::map<TextureDef, TexturePtr>;
        using ChainTexturesByDef = std::map<StringPair, TextureDefMap>;
        
        ChainTexturesByDef mChainTexturesByDef;

        auto isInputPreviousTarget(CompositorInstance* inst, std::string_view localName) -> bool;
        auto isInputPreviousTarget(CompositorInstance* inst, TexturePtr tex) -> bool;
        auto isInputToOutputTarget(CompositorInstance* inst, std::string_view localName) -> bool;
        auto isInputToOutputTarget(CompositorInstance* inst, TexturePtr tex) -> bool;

    };
    /** @} */
    /** @} */

}
