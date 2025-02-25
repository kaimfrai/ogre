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
export module Ogre.Core:MaterialManager;

export import :Common;
export import :Prerequisites;
export import :Resource;
export import :ResourceGroupManager;
export import :ResourceManager;
export import :SharedPtr;
export import :Singleton;

export import <list>;
export import <map>;
export import <string>;

export
namespace Ogre {

class Material;
class Renderable;
class Technique;


    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Materials
    *  @{
    */
    /** Class for managing Material settings for %Ogre.

        Materials control the eventual surface rendering properties of geometry. This class
        manages the library of materials, dealing with programmatic registrations and lookups,
        as well as loading predefined Material settings from scripts.

        When loaded from a script, a Material is in an 'unloaded' state and only stores the settings
        required. It does not at that stage load any textures. This is because the material settings may be
        loaded 'en masse' from bulk material script files, but only a subset will actually be required.

        Because this is a subclass of ResourceManager, any files loaded will be searched for in any path or
        archive added to the resource paths/archives. See ResourceManager for details.

        For a definition of the material script format, see @ref Material-Scripts.

        Ogre comes configured with a set of defaults for newly created
        materials. If you wish to have a different set of defaults,
        simply call getDefaultSettings() and change the returned Material's
        settings. All materials created from then on will be configured
        with the new defaults you have specified.
    */
    class MaterialManager : public ResourceManager, public Singleton<MaterialManager>
    {
    public:
        /** Listener on any general material events.
        @see MaterialManager::addListener
        */
        class Listener
        {
        public:
            /** Virtual destructor needed as class has virtual methods. */
            virtual ~Listener() = default;
            /** Called if a technique for a given scheme is not found within a material,
                allows the application to specify a Technique instance manually.
            @remarks
                Material schemes allow you to switch wholesale between families of 
                techniques on a material. However they require you to define those
                schemes on the materials up-front, which might not be possible or
                desirable for all materials, particular if, for example, you wanted
                a simple way to replace all materials with another using a scheme.
            @par
                This callback allows you to handle the case where a scheme is requested
                but the material doesn't have an entry for it. You can return a
                Technique pointer from this method to specify the material technique
                you'd like to be applied instead, which can be from another material
                entirely (and probably will be). Note that it is critical that you
                only return a Technique that is supported on this hardware; there are
                utility methods like Material::getBestTechnique to help you with this.
            @param schemeIndex The index of the scheme that was requested - all 
                schemes have a unique index when created that does not alter. 
            @param schemeName The friendly name of the scheme being requested
            @param originalMaterial The material that is being processed, that 
                didn't have a specific technique for this scheme
            @param lodIndex The material level-of-detail that was being asked for, 
                in case you need to use it to determine a technique.
            @param rend Pointer to the Renderable that is requesting this technique
                to be used, so this may influence your choice of Technique. May be
                null if the technique isn't being requested in that context.
            @return A pointer to the technique to be used, or NULL if you wish to
                use the default technique for this material
            */
            virtual auto handleSchemeNotFound(unsigned short schemeIndex, 
                std::string_view schemeName, Material* originalMaterial, unsigned short lodIndex, 
                const Renderable* rend) -> Technique* = 0;

			/** Called right after illuminated passes were created,
				so that owner of runtime generated technique can handle this.
			@return True if notification is handled and should not be propagated further.
			*/
			virtual auto afterIlluminationPassesCreated(Technique* technique) noexcept -> bool { return false; }

			/** Called right before illuminated passes would be removed,
				so that owner of runtime generated technique can handle this.
			@return True if notification is handled and should not be propagated further.
			*/
			virtual auto beforeIlluminationPassesCleared(Technique* technique) noexcept -> bool { return false; }
        };

    private:
        /// Default settings
        MaterialPtr mDefaultSettings;

        auto createImpl(std::string_view name, ResourceHandle handle, 
            std::string_view group, bool isManual, ManualResourceLoader* loader,
            const NameValuePairList* params) -> Resource* override;

        /// Scheme name -> index. Never shrinks! Should be pretty static anyway
        using SchemeMap = std::map<std::string, unsigned short, std::less<>>;
        /// List of material schemes
        SchemeMap mSchemes;
        /// Current material scheme
        String mActiveSchemeName;
        /// Current material scheme
        unsigned short mActiveSchemeIndex;

        /// The list of per-scheme (and general) material listeners
        using ListenerList = std::list<Listener *>;
        using ListenerMap = std::map<std::string_view, ListenerList>;
        ListenerMap mListenerMap;

    public:
        /// Default material scheme
        static std::string_view const DEFAULT_SCHEME_NAME;

        /// Create a new material
        /// @see ResourceManager::createResource
        auto create (std::string_view name, std::string_view group,
                            bool isManual = false, ManualResourceLoader* loader = nullptr,
                            const NameValuePairList* createParams = nullptr) -> MaterialPtr;
        
        /// Get a resource by name
        /// @see ResourceManager::getResourceByName
        auto getByName(std::string_view name, std::string_view groupName = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME) const -> MaterialPtr;

        /// Get a default material that is always available even when no resources were loaded
        /// @param useLighting whether the material should be lit
        auto getDefaultMaterial(bool useLighting = true) -> MaterialPtr;

        /** Default constructor.
        */
        MaterialManager();

        /** Default destructor.
        */
        ~MaterialManager() override;

        /** Initialises the material manager, which also triggers it to 
         * parse all available .program and .material scripts. */
        void initialise();

        /** Sets the default texture filtering to be used for loaded textures, for when textures are
            loaded automatically (e.g. by Material class) or when 'load' is called with the default
            parameters by the application.
            @note
                The default value is TextureFilterOptions::BILINEAR.
        */
        virtual void setDefaultTextureFiltering(TextureFilterOptions fo);
        /** Sets the default texture filtering to be used for loaded textures, for when textures are
            loaded automatically (e.g. by Material class) or when 'load' is called with the default
            parameters by the application.
        */
        virtual void setDefaultTextureFiltering(FilterType ftype, FilterOptions opts);
        /** Sets the default texture filtering to be used for loaded textures, for when textures are
            loaded automatically (e.g. by Material class) or when 'load' is called with the default
            parameters by the application.
        */
        virtual void setDefaultTextureFiltering(FilterOptions minFilter, FilterOptions magFilter, FilterOptions mipFilter);

        /// Get the default texture filtering
        virtual auto getDefaultTextureFiltering(FilterType ftype) const -> FilterOptions;

        /** Sets the default anisotropy level to be used for loaded textures, for when textures are
            loaded automatically (e.g. by Material class) or when 'load' is called with the default
            parameters by the application.
            @note
                The default value is 1 (no anisotropy).
        */
        void setDefaultAnisotropy(unsigned int maxAniso);
        /// Get the default maxAnisotropy
        auto getDefaultAnisotropy() const noexcept -> unsigned int;

        /** Returns a pointer to the default Material settings.

            The default settings begin as a single Technique with a single, non-programmable Pass:

            - ambient = ColourValue::White
            - diffuse = ColourValue::White
            - specular = ColourValue::Black
            - emissive = ColourValue::Black
            - shininess = 0 (not shiny)
            - No texture unit settings (& hence no textures)
            - SourceBlendFactor = Ogre::SceneBlendFactor::ONE
            - DestBlendFactor = Ogre::SceneBlendFactor::ZERO (no blend, replace with new colour)
            - Depth buffer checking on
            - Depth buffer writing on
            - Depth buffer comparison function = Ogre::CompareFunction::LESS_EQUAL
            - Colour buffer writing on for all channels
            - Culling mode = Ogre::CullingMode::CLOCKWISE
            - Ambient lighting = ColourValue{0.5, 0.5, 0.5} (mid-grey)
            - Dynamic lighting enabled
            - Gourad shading mode
            - Bilinear texture filtering
        */
        virtual auto getDefaultSettings() const noexcept -> MaterialPtr { return mDefaultSettings; }

        /** Internal method - returns index for a given material scheme name.
        @see Technique::setSchemeName
        */
        virtual auto _getSchemeIndex(std::string_view name) -> unsigned short;
        /** Internal method - returns name for a given material scheme index.
        @see Technique::setSchemeName
        */
        virtual auto _getSchemeName(unsigned short index) -> std::string_view ;
        /** Internal method - returns the active scheme index.
        @see Technique::setSchemeName
        */
        auto _getActiveSchemeIndex() const noexcept -> unsigned short { return mActiveSchemeIndex; }

        /** Returns the name of the active material scheme. 
        @see Technique::setSchemeName
        */
        auto getActiveScheme() const noexcept -> std::string_view { return mActiveSchemeName; }
        
        /** Sets the name of the active material scheme. 
        @see Technique::setSchemeName
        */
        virtual void setActiveScheme(std::string_view schemeName);

        /** 
        Add a listener to handle material events. 
        If schemeName is supplied, the listener will only receive events for that certain scheme.
        */
        virtual void addListener(Listener* l, std::string_view schemeName = BLANKSTRING);

        /** 
        Remove a listener handling material events. 
        If the listener was added with a custom scheme name, it needs to be supplied here as well.
        */
        virtual void removeListener(Listener* l, std::string_view schemeName = BLANKSTRING);

        /// Internal method for sorting out missing technique for a scheme
        virtual auto _arbitrateMissingTechniqueForActiveScheme(
            Material* mat, unsigned short lodIndex, const Renderable* rend) -> Technique*;

		/// Internal method for sorting out illumination passes for a scheme
		virtual void _notifyAfterIlluminationPassesCreated(Technique* mat);

		/// Internal method for sorting out illumination passes for a scheme
		virtual void _notifyBeforeIlluminationPassesCleared(Technique* mat);


		/// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> MaterialManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> MaterialManager*;

    };
    /** @} */
    /** @} */

}
