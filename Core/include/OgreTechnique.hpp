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

export module Ogre.Core:Technique;

export import :BlendMode;
export import :ColourValue;
export import :Common;
export import :IteratorWrapper;
export import :MemoryAllocatorConfig;
export import :Pass;
export import :Platform;
export import :Prerequisites;
export import :RenderSystemCapabilities;
export import :SharedPtr;
export import :UserObjectBindings;

export import <algorithm>;
export import <string>;
export import <utility>;
export import <vector>;

export
namespace Ogre {
class Material;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Materials
    *  @{
    */
    /** Class representing an approach to rendering this particular Material. 
    @remarks
        Ogre will attempt to use the best technique supported by the active hardware, 
        unless you specifically request a lower detail technique (say for distant
        rendering).
    */
    class Technique : public TechniqueAlloc
    {
    public:
        using Passes = std::vector<Pass *>;

    private:
        /// Illumination pass state type
        enum class IlluminationPassesState : int8
        {
            COMPILE_DISABLED = -1,
            NOT_COMPILED = 0,
            COMPILED = 1
        };

        /// List of primary passes
        Passes mPasses;
        /// List of derived passes, categorised into IlluminationStage (ordered)
        IlluminationPassList mIlluminationPasses;
        // Raw pointer since we don't want child to stop parent's destruction
        Material* mParent;
        IlluminationPassesState mIlluminationPassesCompilationPhase;

        bool mIsSupported;
        /// LOD level
        unsigned short mLodIndex;
        /** Scheme index, derived from scheme name but the names are held on
            MaterialManager, for speed an index is used here.
        */
        unsigned short mSchemeIndex;
        /// Optional name for the technique
        String mName;
        /// Internal method for clearing illumination pass list
        void clearIlluminationPasses();
        /// Internal method - check for manually assigned illumination passes
        auto checkManuallyOrganisedIlluminationPasses() -> bool;


        /** When casting shadow, if not using default Ogre shadow casting material, or 
        * nor using fixed function casting, mShadowCasterMaterial let you customize per material
        * shadow caster behavior
        */
        MaterialPtr mShadowCasterMaterial;
        /** When casting shadow, if not using default Ogre shadow casting material, or 
        * nor using fixed function casting, mShadowCasterMaterial let you customize per material
        * shadow caster behavior.There only material name is stored so that it can be loaded once all file parsed in a resource group.
        */
        String mShadowCasterMaterialName;
        /** When receiving shadow, if not using default Ogre shadow receiving material, or 
        * nor using fixed function texture projection receiving, mShadowReceiverMaterial let you customize per material
        * shadow caster behavior
        */
        MaterialPtr mShadowReceiverMaterial;
        /** When receiving shadow, if not using default Ogre shadow receiving material, or 
        * nor using fixed function texture projection receiving, mShadowReceiverMaterial let you customize per material
        * shadow caster behavior. There only material name is stored so that it can be loaded once all file parsed in a resource group.
        */
        String mShadowReceiverMaterialName; 

        // User objects binding.
        UserObjectBindings  mUserObjectBindings;
    public:
        /** Directive used to manually control technique support based on the
            inclusion or exclusion of some factor.
        */
        enum class IncludeOrExclude
        {
            /// Inclusive - only support if present
            INCLUDE = 0,
            /// Exclusive - do not support if present
            EXCLUDE = 1
        };
        using enum IncludeOrExclude;

        /// Rule controlling whether technique is deemed supported based on GPU vendor
        struct GPUVendorRule
        {
            GPUVendor vendor{GPUVendor::UNKNOWN};
            IncludeOrExclude includeOrExclude{EXCLUDE};
        };
        /// Rule controlling whether technique is deemed supported based on GPU device name
        struct GPUDeviceNameRule
        {
            std::string devicePattern;
            IncludeOrExclude includeOrExclude{EXCLUDE};
            bool caseSensitive{false};
        };
        using GPUVendorRuleList = std::vector<GPUVendorRule>;
        using GPUDeviceNameRuleList = std::vector<GPUDeviceNameRule>;
    private:
        GPUVendorRuleList mGPUVendorRules;
        GPUDeviceNameRuleList mGPUDeviceNameRules;
    public:
        /// Constructor
        Technique(Material* parent);
        /// Copy constructor
        Technique(Material* parent, const Technique& oth);
        ~Technique();
        /** Indicates if this technique is supported by the current graphics card.
        @remarks
            This will only be correct after the Technique has been compiled, which is
            usually done from Material::compile.
        */
        [[nodiscard]] auto isSupported() const noexcept -> bool;
        /** Internal compilation method; see Material::compile. 
        @return Any information explaining problems with the compile.
        */
        auto _compile(bool autoManageTextureUnits) -> String;
        /// Internal method for checking hardware support
        auto checkHardwareSupport(bool autoManageTextureUnits, StringStream& compileErrors) -> bool;
        [[nodiscard]] auto calculateSize() const -> size_t;

        using PassIterator = VectorIterator<Passes>;
        using IlluminationPassIterator = VectorIterator<IlluminationPassList>;
        /// @name Passes
        /// @{
        /** Creates a new Pass for this Technique.
        @remarks
            A Pass is a single rendering pass, i.e. a single draw of the given material.
            Note that if you create a pass without a fragment program, during compilation of the
            material the pass may be split into multiple passes if the graphics card cannot
            handle the number of texture units requested. For passes with fragment programs, however, 
            the number of passes you create will never be altered, so you have to make sure 
            that you create an alternative fallback Technique for if a card does not have 
            enough facilities for what you're asking for.
        */
        auto createPass() -> Pass*;
        /** Retrieves the Pass with the given index.*/
        [[nodiscard]] auto getPass(size_t index) const -> Pass* { return mPasses.at(index); }
        /** Retrieves the Pass matching name.
            Returns 0 if name match is not found.
        */
        [[nodiscard]] auto getPass(std::string_view name) const -> Pass*;
        /** Retrieves the number of passes. */
        [[nodiscard]] auto getNumPasses() const -> size_t { return mPasses.size(); }
        /** Removes the Pass with the given index. */
        void removePass(unsigned short index);
        /** Removes all Passes from this Technique. */
        void removeAllPasses();
        /** Move a pass from source index to destination index.
            If successful then returns true.
        */
        auto movePass(const unsigned short sourceIndex, const unsigned short destinationIndex) -> bool;

        /** Gets the passes in this Technique. */
        [[nodiscard]] auto getPasses() const noexcept -> const Passes& {
            return mPasses;
        }

        /** Gets the illumination-stage categorised passes
         * @note triggers compilation if needed */
        auto getIlluminationPasses() noexcept -> const IlluminationPassList&;

        /** Internal method for splitting the passes into illumination passes. */
        void _compileIlluminationPasses();
        /// @}

        /// Gets the parent Material
        [[nodiscard]] auto getParent() const noexcept -> Material* { return mParent; }

        /** Overloaded operator to copy on Technique to another. */
        auto operator=(const Technique& rhs) -> Technique&;

        /// Gets the resource group of the ultimate parent Material
        [[nodiscard]] auto getResourceGroup() const noexcept -> std::string_view ;

        /** Returns true if this Technique involves transparency. 
        @remarks
            This basically boils down to whether the first pass
            has a scene blending factor. Even if the other passes 
            do not, the base colour, including parts of the original 
            scene, may be used for blending, therefore we have to treat
            the whole Technique as transparent.
        */
        [[nodiscard]] auto isTransparent() const noexcept -> bool;

        /** Returns true if this Technique has transparent sorting enabled. 
        @remarks
            This basically boils down to whether the first pass
            has transparent sorting enabled or not
        */
        [[nodiscard]] auto isTransparentSortingEnabled() const noexcept -> bool;

        /** Returns true if this Technique has transparent sorting forced. 
        @remarks
            This basically boils down to whether the first pass
            has transparent sorting forced or not
        */
        [[nodiscard]] auto isTransparentSortingForced() const noexcept -> bool;

        /** Internal prepare method, derived from call to Material::prepare. */
        void _prepare();
        /** Internal unprepare method, derived from call to Material::unprepare. */
        void _unprepare();
        /** Internal load method, derived from call to Material::load. */
        void _load();
        /** Internal unload method, derived from call to Material::unload. */
        void _unload();

        /// Is this loaded?
        [[nodiscard]] auto isLoaded() const noexcept -> bool;

        /** Tells the technique that it needs recompilation. */
        void _notifyNeedsRecompile();

        /// @name Shadow Materials
        /// @{
        /** return this material specific  shadow casting specific material
        */
        [[nodiscard]] auto getShadowCasterMaterial() const -> MaterialPtr;
        /** Sets the details of the material to use when rendering as a
            shadow caster.
            @remarks
            Texture-based shadows require that the caster is rendered to a texture
            in a solid colour (the shadow colour in the case of modulative texture
            shadows). Whilst Ogre can arrange this for the fixed function
            pipeline, passes which use vertex programs might need the vertex
            programs still to run in order to preserve any deformation etc
            that it does. However, lighting calculations must be a lot simpler,
            with only the ambient colour being used (which the engine will ensure
            is bound to the shadow colour).
            @par
            Therefore, it is up to implementors of vertex programs to provide an
            alternative material which can be used to render the object
            to a shadow texture. Do all the same vertex transforms, but set the
            colour of the vertex to the ambient colour, as bound using the
            standard auto parameter binding mechanism.
        */
        void setShadowCasterMaterial(MaterialPtr val);
        /** set this material specific  shadow casting specific material
        */
        void setShadowCasterMaterial(std::string_view name);
        /** return this material specific shadow receiving specific material
        */
        [[nodiscard]] auto getShadowReceiverMaterial() const -> MaterialPtr;
        /** set this material specific  shadow receiving specific material
        */
        void setShadowReceiverMaterial(MaterialPtr val);
        /** set this material specific  shadow receiving specific material
        */
        void setShadowReceiverMaterial(std::string_view name);
        /// @}

        /** @name Forwarded Pass Properties

            The following methods are to make migration from previous versions simpler
            and to make code easier to write when dealing with simple materials
            They set the properties which have been moved to Pass for all Techniques and all Passes
        */

        /// @{
        /** Sets the point size properties for every Pass in this Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setPointSize
        */
        void setPointSize(Real ps);

        /** Sets the ambient colour reflectance properties for every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setAmbient
        */
        void setAmbient(float red, float green, float blue);

        /// @overload
        void setAmbient(const ColourValue& ambient);

        /** Sets the diffuse colour reflectance properties of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDiffuse
        */
        void setDiffuse(float red, float green, float blue, float alpha);

        /// @overload
        void setDiffuse(const ColourValue& diffuse);

        /** Sets the specular colour reflectance properties of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSpecular
        */
        void setSpecular(float red, float green, float blue, float alpha);

        /// @overload
        void setSpecular(const ColourValue& specular);

        /** Sets the shininess properties of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setShininess
        */
        void setShininess(Real val);

        /** Sets the amount of self-illumination of every Pass in every Technique.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSelfIllumination
        */
        void setSelfIllumination(float red, float green, float blue);

        /// @overload
        void setSelfIllumination(const ColourValue& selfIllum);

        /** Sets whether or not each Pass renders with depth-buffer checking on or not.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDepthCheckEnabled
        */
        void setDepthCheckEnabled(bool enabled);

        /** Sets whether or not each Pass renders with depth-buffer writing on or not.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDepthWriteEnabled
        */
        void setDepthWriteEnabled(bool enabled);

        /** Sets the function used to compare depth values when depth checking is on.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDepthFunction
        */
        void setDepthFunction( CompareFunction func );

        /** Sets whether or not colour buffer writing is enabled for each Pass.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setColourWriteEnabled
        */
        void setColourWriteEnabled(bool enabled);

        /** Sets which colour buffer channels are enabled for writing for each Pass.
         @see Pass::setColourWriteEnabled
         */
        void setColourWriteEnabled(bool red, bool green, bool blue, bool alpha);

        /** Sets the culling mode for each pass  based on the 'vertex winding'.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setCullingMode
        */
        void setCullingMode( CullingMode mode );

        /** Sets the manual culling mode, performed by CPU rather than hardware.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setManualCullingMode
        */
        void setManualCullingMode( ManualCullingMode mode );

        /** Sets whether or not dynamic lighting is enabled for every Pass.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setLightingEnabled
        */
        void setLightingEnabled(bool enabled);

        /** Sets the type of light shading required
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setShadingMode
        */
        void setShadingMode( ShadeOptions mode );

        /** Sets the fogging mode applied to each pass.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setFog
        */
        void setFog(
            bool overrideScene,
            FogMode mode = FogMode::NONE,
            const ColourValue& colour = ColourValue::White,
            Real expDensity = 0.001f, Real linearStart = 0.0f, Real linearEnd = 1.0f );

        /** Sets the depth bias to be used for each Pass.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setDepthBias
        */
        void setDepthBias(float constantBias, float slopeScaleBias);

        /** Set texture filtering for every texture unit in every Pass
        @note
            This property actually exists on the TextureUnitState class
            For simplicity, this method allows you to set these properties for 
            every current TeextureUnitState, If you need more precision, retrieve the  
            Pass and TextureUnitState instances and set the property there.
        @see TextureUnitState::setTextureFiltering
        */
        void setTextureFiltering(TextureFilterOptions filterType);
        /** Sets the anisotropy level to be used for all textures.
        @note
            This property has been moved to the TextureUnitState class, which is accessible via the 
            Technique and Pass. For simplicity, this method allows you to set these properties for 
            every current TeextureUnitState, If you need more precision, retrieve the Technique, 
            Pass and TextureUnitState instances and set the property there.
        @see TextureUnitState::setTextureAnisotropy
        */
        void setTextureAnisotropy(unsigned int maxAniso);

        /** Sets the kind of blending every pass has with the existing contents of the scene.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSceneBlending
        */
        void setSceneBlending( const SceneBlendType sbt );

        /** Sets the kind of blending every pass has with the existing contents of the scene, using individual factors both color and alpha channels
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSeparateSceneBlending
        */
        void setSeparateSceneBlending( const SceneBlendType sbt, const SceneBlendType sbta );

        /** Allows very fine control of blending every Pass with the existing contents of the scene.
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSceneBlending
        */
        void setSceneBlending( const SceneBlendFactor sourceFactor, const SceneBlendFactor destFactor);

        /** Allows very fine control of blending every Pass with the existing contents of the scene, using individual factors both color and alpha channels
        @note
            This property actually exists on the Pass class. For simplicity, this method allows 
            you to set these properties for every current Pass within this Technique. If 
            you need more precision, retrieve the Pass instance and set the
            property there.
        @see Pass::setSeparateSceneBlending
        */
        void setSeparateSceneBlending( const SceneBlendFactor sourceFactor, const SceneBlendFactor destFactor, const SceneBlendFactor sourceFactorAlpha, const SceneBlendFactor destFactorAlpha);
        /// @}

        /** Assigns a level-of-detail (LOD) index to this Technique.
        @remarks
            As noted previously, as well as providing fallback support for various
            graphics cards, multiple Technique objects can also be used to implement
            material LOD, where the detail of the material diminishes with distance to 
            save rendering power.
        @par
            By default, all Techniques have a LOD index of 0, which means they are the highest
            level of detail. Increasing LOD indexes are lower levels of detail. You can 
            assign more than one Technique to the same LOD index, meaning that the best 
            Technique that is supported at that LOD index is used. 
        @par
            You should not leave gaps in the LOD sequence; Ogre will allow you to do this
            and will continue to function as if the LODs were sequential, but it will 
            confuse matters.
        */
        void setLodIndex(unsigned short index);
        /** Gets the level-of-detail index assigned to this Technique. */
        [[nodiscard]] auto getLodIndex() const noexcept -> unsigned short { return mLodIndex; }

        /** Set the 'scheme name' for this technique. 
        @remarks
            Material schemes are used to control top-level switching from one
            set of techniques to another. For example, you might use this to 
            define 'high', 'medium' and 'low' complexity levels on materials
            to allow a user to pick a performance / quality ratio. Another
            possibility is that you have a fully HDR-enabled pipeline for top
            machines, rendering all objects using unclamped shaders, and a 
            simpler pipeline for others; this can be implemented using 
            schemes.
        @par
            Every technique belongs to a scheme - if you don't specify one, the
            Technique belongs to the scheme called 'Default', which is also the
            scheme used to render by default. The active scheme is set one of
            two ways - either by calling Viewport::setMaterialScheme, or
            by manually calling MaterialManager::setActiveScheme.
        */
        void setSchemeName(std::string_view schemeName);
        /** Returns the scheme to which this technique is assigned.
            @see Technique::setSchemeName
        */
        [[nodiscard]] auto getSchemeName() const noexcept -> std::string_view ;
        
        /// Internal method for getting the scheme index
        [[nodiscard]] auto _getSchemeIndex() const -> unsigned short;
            
        /** Is depth writing going to occur on this technique? */
        [[nodiscard]] auto isDepthWriteEnabled() const noexcept -> bool;

        /** Is depth checking going to occur on this technique? */
        [[nodiscard]] auto isDepthCheckEnabled() const noexcept -> bool;

        /** Exists colour writing disabled pass on this technique? */
        [[nodiscard]] auto hasColourWriteDisabled() const -> bool;

        /** Set the name of the technique.
        @remarks
        The use of technique name is optional.  Its useful in material scripts where a material could inherit
        from another material and only want to modify a particular technique.
        */
        void setName(std::string_view name);
        /// Gets the name of the technique
        [[nodiscard]] auto getName() const noexcept -> std::string_view { return mName; }

        using GPUVendorRuleIterator = ConstVectorIterator<GPUVendorRuleList>;
        using GPUDeviceNameRuleIterator = ConstVectorIterator<GPUDeviceNameRuleList>;
        /// @name GPU Vendor Rules
        /// @{

        /// Internal method for checking GPU vendor / device rules
        auto checkGPURules(StringStream& errors) -> bool;
        /** Add a rule which manually influences the support for this technique based
            on a GPU vendor.
        @remarks
            You can use this facility to manually control whether a technique is
            considered supported, based on a GPU vendor. You can add inclusive
            or exclusive rules, and you can add as many of each as you like. If
            at least one inclusive rule is added, a technique is considered 
            unsupported if it does not match any of those inclusive rules. If exclusive rules are
            added, the technique is considered unsupported if it matches any of
            those inclusive rules. 
        @note
            Any rule for the same vendor will be removed before adding this one.
        @param vendor The GPU vendor
        @param includeOrExclude Whether this is an inclusive or exclusive rule
        */
        void addGPUVendorRule(GPUVendor vendor, IncludeOrExclude includeOrExclude);
        /** Add a rule which manually influences the support for this technique based
            on a GPU vendor.
        @remarks
            You can use this facility to manually control whether a technique is
            considered supported, based on a GPU vendor. You can add inclusive
            or exclusive rules, and you can add as many of each as you like. If
            at least one inclusive rule is added, a technique is considered 
            unsupported if it does not match any of those inclusive rules. If exclusive rules are
            added, the technique is considered unsupported if it matches any of
            those inclusive rules. 
        @note
            Any rule for the same vendor will be removed before adding this one.
        */
        void addGPUVendorRule(const GPUVendorRule& rule);
        /** Removes a matching vendor rule.
        @see addGPUVendorRule
        */
        void removeGPUVendorRule(GPUVendor vendor);

        /// Get the currently registered vendor rules.
        [[nodiscard]] auto getGPUVendorRules() const noexcept -> const GPUVendorRuleList& {
            return mGPUVendorRules;
        }

        /** Add a rule which manually influences the support for this technique based
            on a pattern that matches a GPU device name (e.g. '*8800*').
        @remarks
            You can use this facility to manually control whether a technique is
            considered supported, based on a GPU device name pattern. You can add inclusive
            or exclusive rules, and you can add as many of each as you like. If
            at least one inclusive rule is added, a technique is considered 
            unsupported if it does not match any of those inclusive rules. If exclusive rules are
            added, the technique is considered unsupported if it matches any of
            those inclusive rules. The pattern you supply can include wildcard
            characters ('*') if you only want to match part of the device name.
        @note
            Any rule for the same device pattern will be removed before adding this one.
        @param devicePattern The GPU vendor
        @param includeOrExclude Whether this is an inclusive or exclusive rule
        @param caseSensitive Whether the match is case sensitive or not
        */
        void addGPUDeviceNameRule(std::string_view devicePattern, IncludeOrExclude includeOrExclude, bool caseSensitive = false);
        /// @overload
        void addGPUDeviceNameRule(const GPUDeviceNameRule& rule);
        /** Removes a matching device name rule.
        @see addGPUDeviceNameRule
        */
        void removeGPUDeviceNameRule(std::string_view devicePattern);

        /// Get the currently registered device name rules.
        [[nodiscard]] auto getGPUDeviceNameRules() const noexcept -> const GPUDeviceNameRuleList& { return mGPUDeviceNameRules; }
        /// @}

        /** Return an instance of user objects binding associated with this class.
        You can use it to associate one or more custom objects with this class instance.
        @see UserObjectBindings::setUserAny.
        */
        auto getUserObjectBindings() noexcept -> UserObjectBindings& { return mUserObjectBindings; }

        /** Return an instance of user objects binding associated with this class.
        You can use it to associate one or more custom objects with this class instance.
        @see UserObjectBindings::setUserAny.        
        */
        [[nodiscard]] auto getUserObjectBindings() const noexcept -> const UserObjectBindings& { return mUserObjectBindings; }

    };

    /** @} */
    /** @} */

}
