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

export module Ogre.Core:TextureManager;

export import :Common;
export import :PixelFormat;
export import :Platform;
export import :Prerequisites;
export import :Resource;
export import :ResourceGroupManager;
export import :ResourceManager;
export import :SharedPtr;
export import :Singleton;
export import :Texture;
export import :TextureUnitState;

export import <map>;
export import <memory>;
export import <string>;

export
namespace Ogre {
class Image;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    /** Class for loading & managing textures.
        @remarks
            Note that this class is abstract - the particular
            RenderSystem that is in use at the time will create
            a concrete subclass of this. Note that the concrete
            class will be available via the abstract singleton
            obtained from TextureManager::getSingleton(), but
            you should not assume that it is available until you
            have a) initialised Ogre (after selecting a RenderSystem
            and calling initialise from the Root object), and b)
            created at least one window - this may be done at the
            same time as part a if you allow Ogre to autocreate one.
     */
    class TextureManager : public ResourceManager, public Singleton<TextureManager>
    {
    public:

        TextureManager();
        ~TextureManager() override = default;

        /// create a new sampler
        auto createSampler(std::string_view name = BLANKSTRING) -> SamplerPtr;

        /// retrieve an named sampler
        auto getSampler(std::string_view name) const -> const SamplerPtr&;

        /// clear the list of named samplers
        /// @copydetails removeAll()
        void removeAllNamedSamplers() { mNamedSamplers.clear(); }

        /// Create a new texture
        /// @copydetails ResourceManager::createResource
        auto create (std::string_view name, std::string_view group,
                            bool isManual = false, ManualResourceLoader* loader = nullptr,
                            const NameValuePairList* createParams = nullptr) -> TexturePtr;
        /// @copydoc ResourceManager::getResourceByName
        auto getByName(std::string_view name, std::string_view groupName  = RGN_DEFAULT) const -> TexturePtr;

        using ResourceManager::createOrRetrieve;

        /** @overload createOrRetrieve
            
            @copydetails ResourceManager::createResource

            @param
                texType The type of texture to load/create, defaults to normal 2D textures
            @param
                numMipmaps The number of pre-filtered mipmaps to generate. If left to TextureMipmap::DEFAULT then
                the TextureManager's default number of mipmaps will be used (see setDefaultNumMipmaps())
                If set to TextureMipmap::UNLIMITED mipmaps will be generated until the lowest possible
                level, 1x1x1.
            @param
                gamma The gamma adjustment factor to apply to this texture (brightening/darkening)
            @param 
                isAlpha deprecated: same as specifying #PixelFormat::A8 for @c desiredFormat
            @param 
                desiredFormat The format you would like to have used instead of
                the format being based on the contents of the texture
            @param hwGammaCorrection Pass 'true' to enable hardware gamma correction
                (sRGB) on this texture. The hardware will convert from gamma space
                to linear space when reading from this texture. Only applicable for 
                8-bits per channel textures, will be ignored for other types. Has the advantage
                over pre-applied gamma that the texture precision is maintained.
        */
        auto createOrRetrieve(
            std::string_view name, std::string_view group, bool isManual,
            ManualResourceLoader* loader, const NameValuePairList* createParams,
            TextureType texType, TextureMipmap numMipmaps = TextureMipmap::DEFAULT,
            Real gamma = 1.0f, bool isAlpha = false,
            PixelFormat desiredFormat = PixelFormat::UNKNOWN, bool hwGammaCorrection = false) -> ResourceCreateOrRetrieveResult;

        /** Prepares to loads a texture from a file.
            @copydetails TextureManager::load
            @param isAlpha deprecated: same as specifying #PixelFormat::A8 for @c desiredFormat
        */
        auto prepare(
            std::string_view name, std::string_view group,
            TextureType texType = TextureType::_2D, TextureMipmap numMipmaps = TextureMipmap::DEFAULT,
            Real gamma = 1.0f, bool isAlpha = false,
            PixelFormat desiredFormat = PixelFormat::UNKNOWN, bool hwGammaCorrection = false) -> TexturePtr;

        /** Loads a texture from a file.
            @param
                name The file to load, or a String identifier in some cases
            @param
                group The name of the resource group to assign the texture to
            @param
                texType The type of texture to load/create, defaults to normal 2D textures
            @param
                numMipmaps The number of pre-filtered mipmaps to generate. If left to TextureMipmap::DEFAULT then
                the TextureManager's default number of mipmaps will be used (see setDefaultNumMipmaps())
                If set to TextureMipmap::UNLIMITED mipmaps will be generated until the lowest possible
                level, 1x1x1.
            @param
                gamma The gamma adjustment factor to apply to this texture (brightening/darkening)

            @param 
                desiredFormat The format you would like to have used instead of
                the format being based on the contents of the texture; the manager reserves
                the right to create a different format for the texture if the 
                original format is not available in this context.
            @param hwGammaCorrection pass 'true' to enable hardware gamma correction
                (sRGB) on this texture. The hardware will convert from gamma space
                to linear space when reading from this texture. Only applicable for 
                8-bits per channel textures, will be ignored for other types. Has the advantage
                over pre-applied gamma that the texture precision is maintained.
        */
        auto load(std::string_view name, std::string_view group, TextureType texType = TextureType::_2D,
                        TextureMipmap numMipmaps = TextureMipmap::DEFAULT, Real gamma = 1.0f,
                        PixelFormat desiredFormat = PixelFormat::UNKNOWN, bool hwGammaCorrection = false) -> TexturePtr;

        /** Loads a texture from an Image object.
            @note
                The texture will create as manual texture without loader.
            @copydetails TextureManager::prepare
            @param
                img The Image object which contains the data to load
        */
        virtual auto loadImage( 
            std::string_view name, std::string_view group, const Image &img, 
            TextureType texType = TextureType::_2D,
            TextureMipmap numMipmaps = TextureMipmap::DEFAULT, Real gamma = 1.0f, bool isAlpha = false,
            PixelFormat desiredFormat = PixelFormat::UNKNOWN, bool hwGammaCorrection = false) -> TexturePtr;
            
        /** Loads a texture from a raw data stream.
            @note
                The texture will create as manual texture without loader.
            @param name
                The name to give the resulting texture
            @param group
                The name of the resource group to assign the texture to
            @param stream
                Incoming data stream
            @param width
                The width of the texture
            @param height
                The height of the texture
            @param format
                The format of the data being passed in; the manager reserves
                the right to create a different format for the texture if the 
                original format is not available in this context.
            @param texType
                The type of texture to load/create, defaults to normal 2D textures
            @param numMipmaps
                The number of pre-filtered mipmaps to generate. If left to TextureMipmap::DEFAULT then
                the TextureManager's default number of mipmaps will be used (see setDefaultNumMipmaps())
                If set to TextureMipmap::UNLIMITED mipmaps will be generated until the lowest possible
                level, 1x1x1.
            @param gamma
                The gamma adjustment factor to apply to this texture (brightening/darkening)
                while loading
            @param hwGammaCorrection Pass 'true' to enable hardware gamma correction
                 (sRGB) on this texture. The hardware will convert from gamma space
                 to linear space when reading from this texture. Only applicable for 
                 8-bits per channel textures, will be ignored for other types. Has the advantage
                 over pre-applied gamma that the texture precision is maintained.
        */
        virtual auto loadRawData(std::string_view name, std::string_view group,
            DataStreamPtr& stream, ushort width, ushort height, 
            PixelFormat format, TextureType texType = TextureType::_2D, 
            TextureMipmap numMipmaps = TextureMipmap::DEFAULT, Real gamma = 1.0f, bool hwGammaCorrection = false) -> TexturePtr;

        /** Create a manual texture with specified width, height and depth (not loaded from a file).
            @param
                name The name to give the resulting texture
            @param
                group The name of the resource group to assign the texture to
            @param
                texType The type of texture to load/create, defaults to normal 2D textures
            @param width
                The width of the texture
            @param height
                The height of the texture
            @param depth
                The depth of the texture
            @param
                numMipmaps The number of pre-filtered mipmaps to generate. If left to TextureMipmap::DEFAULT then
                the TextureManager's default number of mipmaps will be used (see setDefaultNumMipmaps())
                If set to TextureMipmap::UNLIMITED mipmaps will be generated until the lowest possible
                level, 1x1x1.
            @param
                format The internal format you wish to request; the manager reserves
                the right to create a different format if the one you select is
                not available in this context.
            @param 
                usage The kind of usage this texture is intended for. It 
                is a combination of TextureUsage::STATIC, TextureUsage::DYNAMIC, TextureUsage::WRITE_ONLY, 
                TextureUsage::AUTOMIPMAP and TextureUsage::RENDERTARGET (see TextureUsage enum). You are
                strongly advised to use HardwareBufferUsage::GPU_ONLY wherever possible, if you need to
                update regularly, consider HardwareBufferUsage::CPU_TO_GPU.
            @param
                loader If you intend the contents of the manual texture to be 
                regularly updated, to the extent that you don't need to recover 
                the contents if the texture content is lost somehow, you can leave
                this parameter as 0. However, if you intend to populate the
                texture only once, then you should implement ManualResourceLoader
                and pass a pointer to it in this parameter; this means that if the
                manual texture ever needs to be reloaded, the ManualResourceLoader
                will be called to do it.
            @param hwGammaCorrection pass 'true' to enable hardware gamma correction
                (sRGB) on this texture. The hardware will convert from gamma space
                to linear space when reading from this texture. Only applicable for 
                8-bits per channel textures, will be ignored for other types. Has the advantage
                over pre-applied gamma that the texture precision is maintained.
            @param fsaa The level of multisampling to use if this is a render target. Ignored
                if usage does not include TextureUsage::RENDERTARGET or if the device does
                not support it.
            @param fsaaHint @copybrief RenderTarget::getFSAAHint
        */
        virtual auto createManual(std::string_view name, std::string_view group,
            TextureType texType, uint width, uint height, uint depth, 
            TextureMipmap numMipmaps, PixelFormat format, HardwareBufferUsage usage = TextureUsage::DEFAULT, ManualResourceLoader* loader = nullptr,
            bool hwGammaCorrection = false, uint fsaa = 0, std::string_view fsaaHint = BLANKSTRING) -> TexturePtr;
            
        /** @overload
        */
        auto createManual(std::string_view name, std::string_view group,
            TextureType texType, uint width, uint height, TextureMipmap numMipmaps,
            PixelFormat format, HardwareBufferUsage usage = TextureUsage::DEFAULT, ManualResourceLoader* loader = nullptr,
            bool hwGammaCorrection = false, uint fsaa = 0, std::string_view fsaaHint = BLANKSTRING) -> TexturePtr
        {
            return createManual(name, group, texType, width, height, 1, 
                numMipmaps, format, usage, loader, hwGammaCorrection, fsaa, fsaaHint);
        }

        /** Sets preferred bit depth for integer pixel format textures.
        @param
            bits Number of bits. Available values: 0, 16 and 32, where 0 (the default) means keep
            original format as it is. This value is number of bits for the pixel.
        @param
            reloadTextures If true (the default), will reloading all reloadable textures.
        */
        virtual void setPreferredIntegerBitDepth(ushort bits, bool reloadTextures = true);

        /** Gets preferred bit depth for integer pixel format textures.
        */
        virtual auto getPreferredIntegerBitDepth() const noexcept -> ushort;

        /** Sets preferred bit depth for float pixel format textures.
        @param
            bits Number of bits. Available values: 0, 16 and 32, where 0 (the default) means keep
            original format as it is. This value is number of bits for a channel of the pixel.
        @param
            reloadTextures If true (the default), will reloading all reloadable textures.
        */
        virtual void setPreferredFloatBitDepth(ushort bits, bool reloadTextures = true);

        /** Gets preferred bit depth for float pixel format textures.
        */
        virtual auto getPreferredFloatBitDepth() const noexcept -> ushort;

        /** Sets preferred bit depth for integer and float pixel format.
        @param
            integerBits Number of bits. Available values: 0, 16 and 32, where 0 (the default) means keep
            original format as it is. This value is number of bits for the pixel.
        @param
            floatBits Number of bits. Available values: 0, 16 and 32, where 0 (the default) means keep
            original format as it is. This value is number of bits for a channel of the pixel.
        @param
            reloadTextures If true (the default), will reloading all reloadable textures.
        */
        virtual void setPreferredBitDepths(ushort integerBits, ushort floatBits, bool reloadTextures = true);

        /** Returns whether this render system can natively support the precise texture 
            format requested with the given usage options.
        @remarks
            You can still create textures with this format even if this method returns
            false; the texture format will just be altered to one which the device does
            support.
        @note
            Sometimes the device may just slightly change the format, such as reordering the 
            channels or packing the channels differently, without it making and qualitative 
            differences to the texture. If you want to just detect whether the quality of a
            given texture will be reduced, use isEquivalentFormatSupport instead.
        @param ttype The type of texture
        @param format The pixel format requested
        @param usage The kind of usage this texture is intended for, a combination of 
            the TextureUsage flags.
        @return true if the format is natively supported, false if a fallback would be used.
        */
        virtual auto isFormatSupported(TextureType ttype, PixelFormat format, HardwareBufferUsage usage) -> bool;

        /** Returns whether this render system can support the texture format requested
            with the given usage options, or another format with no quality reduction.
        */
        virtual auto isEquivalentFormatSupported(TextureType ttype, PixelFormat format, HardwareBufferUsage usage) -> bool;

        /** Gets the format which will be natively used for a requested format given the
            constraints of the current device.
        */
        virtual auto getNativeFormat(TextureType ttype, PixelFormat format, HardwareBufferUsage usage) -> PixelFormat = 0;

        /** Returns whether this render system has hardware filtering supported for the
            texture format requested with the given usage options.
        @remarks
            Not all texture format are supports filtering by the hardware, i.e. some
            cards support floating point format, but it doesn't supports filtering on
            the floating point texture at all, or only a subset floating point formats
            have flitering supported.
        @par
            In the case you want to write shader to work with floating point texture, and
            you want to produce better visual quality, it's necessary to flitering the
            texture manually in shader (potential requires four or more texture fetch
            instructions, plus several arithmetic instructions) if filtering doesn't
            supported by hardware. But in case on the hardware that supports floating
            point filtering natively, it had better to adopt this capability for
            performance (because only one texture fetch instruction are required) and
            doesn't loss visual quality.
        @par
            This method allow you queries hardware texture filtering capability to deciding
            which verion of the shader to be used. Note it's up to you to write multi-version
            shaders for support various hardware, internal engine can't do that for you
            automatically.
        @note
            Under GL, texture filtering are always supported by driver, but if it's not
            supported by hardware natively, software simulation will be used, and you
            will end up with very slow speed (less than 0.1 fps for example). To slove
            this performance problem, you must disable filtering manually (by use
            <b>filtering none</b> in the material script's texture_unit section, or
            call TextureUnitState::setTextureFiltering with TextureFilterOptions::NONE if populate
            material in code).
        @param ttype The texture type requested
        @param format The pixel format requested
        @param usage The kind of usage this texture is intended for, a combination of 
            the TextureUsage flags.
        @param preciseFormatOnly Whether precise or fallback format mode is used to detecting.
            In case the pixel format doesn't supported by device, false will be returned
            if in precise mode, and natively used pixel format will be actually use to
            check if in fallback mode.
        @return true if the texture filtering is supported.
        */
        virtual auto isHardwareFilteringSupported(TextureType ttype, PixelFormat format, HardwareBufferUsage usage,
            bool preciseFormatOnly = false) -> bool;

        /** Sets the default number of mipmaps to be used for loaded textures, for when textures are
            loaded automatically (e.g. by Material class) or when 'load' is called with the default
            parameters by the application.
            If set to TextureMipmap::UNLIMITED mipmaps will be generated until the lowest possible
                level, 1x1x1.
            @note
                The default value is 0.
        */
        virtual void setDefaultNumMipmaps(TextureMipmap num);

        /** Gets the default number of mipmaps to be used for loaded textures.
        */
        virtual auto getDefaultNumMipmaps() noexcept -> TextureMipmap
        {
            return mDefaultNumMipmaps;
        }

        /// Internal method to create a warning texture (bound when a texture unit is blank)
        auto _getWarningTexture() -> const TexturePtr&;

        /// get the default sampler
        auto getDefaultSampler() noexcept -> const SamplerPtr&;

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> TextureManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> TextureManager*;

    protected:

        virtual auto _createSamplerImpl() -> SamplerPtr { return std::make_shared<Sampler>(); }

        ushort mPreferredIntegerBitDepth{0};
        ushort mPreferredFloatBitDepth{0};
        TextureMipmap mDefaultNumMipmaps{TextureMipmap::UNLIMITED};
        TexturePtr mWarningTexture;
        SamplerPtr mDefaultSampler;
        std::map<std::string, SamplerPtr, std::less<>> mNamedSamplers;
    };

    /// Specialisation of TextureManager for offline processing. Cannot be used with an active RenderSystem.
    class DefaultTextureManager : public TextureManager
    {
        /// noop implementation
        class NullTexture : public Texture
        {
        public:
            NullTexture(ResourceManager* creator, std::string_view name, ResourceHandle handle,
                        std::string_view group)
                : Texture(creator, name, handle, group)
            {
            }
            auto getBuffer(size_t, TextureMipmap) -> const HardwarePixelBufferSharedPtr& override
            {
                static HardwarePixelBufferSharedPtr nullBuffer;
                return nullBuffer;
            }

        protected:
            void createInternalResourcesImpl() override {}
            void freeInternalResourcesImpl() override {}
            void loadImpl() override {}
        };

        auto createImpl(std::string_view name, ResourceHandle handle, std::string_view group, bool,
                             ManualResourceLoader*, const NameValuePairList*) -> Resource* override
        {
            return new NullTexture(this, name, handle, group);
        }

    public:
        auto isHardwareFilteringSupported(TextureType, PixelFormat, HardwareBufferUsage, bool) noexcept -> bool override { return false; }
        auto getNativeFormat(TextureType, PixelFormat, HardwareBufferUsage) noexcept -> PixelFormat override { return PixelFormat::UNKNOWN; }
    };
    /** @} */
    /** @} */
}// Namespace
