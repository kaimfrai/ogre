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

export module Ogre.Core:Texture;

export import :HardwareBuffer;
export import :Image;
export import :PixelFormat;
export import :Platform;
export import :Prerequisites;
export import :Resource;
export import :SharedPtr;

export import <algorithm>;
export import <string>;
export import <vector>;

export
namespace Ogre {
class ResourceManager;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */
    /** Enum identifying the texture usage
     */
    struct TextureUsage
    {
        /// same as #HardwareBufferUsage::GPU_TO_CPU
        static auto constexpr STATIC = HardwareBufferUsage::GPU_TO_CPU;
        /// same as #HardwareBufferUsage::CPU_ONLY
        static auto constexpr DYNAMIC = HardwareBufferUsage::CPU_ONLY;
        /// same as #HardwareBufferUsage::DETAIL_WRITE_ONLY
        static auto constexpr WRITE_ONLY = HardwareBufferUsage::DETAIL_WRITE_ONLY;
        /// same as #HardwareBufferUsage::GPU_ONLY
        static auto constexpr STATIC_WRITE_ONLY = HardwareBufferUsage::GPU_ONLY;
        /// same as #HardwareBufferUsage::CPU_TO_GPU
        static auto constexpr DYNAMIC_WRITE_ONLY = HardwareBufferUsage::CPU_TO_GPU;
        /// @deprecated do not use
        static auto constexpr DYNAMIC_WRITE_ONLY_DISCARDABLE = HardwareBufferUsage::CPU_TO_GPU;
        /// Mipmaps will be automatically generated for this texture
        static auto constexpr AUTOMIPMAP = static_cast<HardwareBufferUsage>(0x10);
        /** This texture will be a render target, i.e. used as a target for render to texture
            setting this flag will ignore all other texture usages except AUTOMIPMAP, UAV, NOT_SRV */
        static auto constexpr RENDERTARGET = static_cast<HardwareBufferUsage>(0x20);
        /// Texture would not be used as Shader Resource View, i.e. as regular texture.
        /// That flag could be combined with RENDERTARGET or UAV to remove possible limitations on some hardware
        static auto constexpr NOT_SRV = static_cast<HardwareBufferUsage>(0x40);
        /// Texture can be bound as an Unordered Access View
        /// (imageStore/imageRead/glBindImageTexture in GL jargon)
        static auto constexpr UAV = static_cast<HardwareBufferUsage>(0x80);
        /// Texture can be used as an UAV, but not as a regular texture.
        static auto constexpr UAV_NOT_SRV = UAV | NOT_SRV;
        /// Default to automatic mipmap generation static textures
        static auto constexpr DEFAULT = AUTOMIPMAP | HardwareBufferUsage::GPU_ONLY;

        // deprecated
        static auto constexpr NOTSHADERRESOURCE = NOT_SRV;
    };

    /** Enum identifying the texture access privilege
     */
    enum class TextureAccess
    {
        READ = 0x01,
        WRITE = 0x10,
        READ_WRITE = READ | WRITE
    };

    /** Enum identifying the texture type
    */
    enum class TextureType : uint8
    {
        /// 1D texture, used in combination with 1D texture coordinates
        _1D = 1,
        /// 2D texture, used in combination with 2D texture coordinates (default)
        _2D = 2,
        /// 3D volume texture, used in combination with 3D texture coordinates
        _3D = 3,
        /// cube map (six two dimensional textures, one for each cube face), used in combination with 3D
        /// texture coordinates
        CUBE_MAP = 4,
        /// 2D texture array
        _2D_ARRAY = 5,
        /// GLES2 only OES texture type
        EXTERNAL_OES = 6
    };

    /** Abstract class representing a Texture resource.
        @remarks
            The actual concrete subclass which will exist for a texture
            is dependent on the rendering system in use (Direct3D, OpenGL etc).
            This class represents the commonalities, and is the one 'used'
            by programmers even though the real implementation could be
            different in reality. Texture objects are created through
            the 'create' method of the TextureManager concrete subclass.
     */
    class Texture : public Resource
    {
    public:
        Texture(ResourceManager* creator, std::string_view name, ResourceHandle handle,
            std::string_view group, bool isManual = false, ManualResourceLoader* loader = nullptr);

        ~Texture() override = default;
        
        /** Sets the type of texture; can only be changed before load() 
        */
        void setTextureType(TextureType ttype ) { mTextureType = ttype; }

        /** Gets the type of texture 
        */
        auto getTextureType() const noexcept -> TextureType { return mTextureType; }

        /** Gets the number of mipmaps to be used for this texture.
        */
        auto getNumMipmaps() const noexcept -> TextureMipmap {return mNumMipmaps;}

        /** Sets the number of mipmaps to be used for this texture.
            @note
                Must be set before calling any 'load' method.
        */
        void setNumMipmaps(TextureMipmap num)
        {
            mNumRequestedMipmaps = mNumMipmaps = num;
            if (!num)
                mUsage &= ~TextureUsage::AUTOMIPMAP;
        }

        /** Are mipmaps hardware generated?
        @remarks
            Will only be accurate after texture load, or createInternalResources
        */
        auto getMipmapsHardwareGenerated() const noexcept -> bool { return mMipmapsHardwareGenerated; }

        /** Returns the gamma adjustment factor applied to this texture on loading.
        */
        auto getGamma() const noexcept -> float { return mGamma; }

        /** Sets the gamma adjustment factor applied to this texture on loading the
            data.
            @note
                Must be called before any 'load' method. This gamma factor will
                be premultiplied in and may reduce the precision of your textures.
                You can use setHardwareGamma if supported to apply gamma on 
                sampling the texture instead.
        */
        void setGamma(float g) { mGamma = g; }

        /** Sets whether this texture will be set up so that on sampling it, 
            hardware gamma correction is applied.
        @remarks
            24-bit textures are often saved in gamma colour space; this preserves
            precision in the 'darks'. However, if you're performing blending on 
            the sampled colours, you really want to be doing it in linear space. 
            One way is to apply a gamma correction value on loading (see setGamma),
            but this means you lose precision in those dark colours. An alternative
            is to get the hardware to do the gamma correction when reading the 
            texture and converting it to a floating point value for the rest of
            the pipeline. This option allows you to do that; it's only supported
            in relatively recent hardware (others will ignore it) but can improve
            the quality of colour reproduction.
        @note
            Must be called before any 'load' method since it may affect the
            construction of the underlying hardware resources.
            Also note this only useful on textures using 8-bit colour channels.
        */
        void setHardwareGammaEnabled(bool enabled) { mHwGamma = enabled; }

        /** Gets whether this texture will be set up so that on sampling it, 
        hardware gamma correction is applied.
        */
        auto isHardwareGammaEnabled() const noexcept -> bool { return mHwGamma; }

        /** Set the level of multisample AA to be used if this texture is a 
            rendertarget.
        @note This option will be ignored if TextureUsage::RENDERTARGET is not part of the
            usage options on this texture, or if the hardware does not support it. 
        @param fsaa The number of samples
        @param fsaaHint Any hinting text (see Root::createRenderWindow)
        */
        void setFSAA(uint fsaa, std::string_view fsaaHint) { mFSAA = fsaa; mFSAAHint = fsaaHint; }

        /** Get the level of multisample AA to be used if this texture is a 
        rendertarget.
        */
        auto getFSAA() const noexcept -> uint { return mFSAA; }

        /** Get the multisample AA hint if this texture is a rendertarget.
        */
        auto getFSAAHint() const noexcept -> std::string_view { return mFSAAHint; }

        /** Returns the height of the texture.
        */
        auto getHeight() const noexcept -> uint32 { return mHeight; }

        /** Returns the width of the texture.
        */
        auto getWidth() const noexcept -> uint32 { return mWidth; }

        /** Returns the depth of the texture (only applicable for 3D textures).
        */
        auto getDepth() const noexcept -> uint32 { return mDepth; }

        /** Returns the height of the original input texture (may differ due to hardware requirements).
        */
        auto getSrcHeight() const noexcept -> uint32 { return mSrcHeight; }

        /** Returns the width of the original input texture (may differ due to hardware requirements).
        */
        auto getSrcWidth() const noexcept -> uint32 { return mSrcWidth; }

        /** Returns the original depth of the input texture (only applicable for 3D textures).
        */
        auto getSrcDepth() const noexcept -> uint32 { return mSrcDepth; }

        /** Set the height of the texture; can only do this before load();
        */
        void setHeight(uint32 h) { mHeight = mSrcHeight = h; }

        /** Set the width of the texture; can only do this before load();
        */
        void setWidth(uint32 w) { mWidth = mSrcWidth = w; }

        /** Set the depth of the texture (only applicable for 3D textures);
            can only do this before load();
        */
        void setDepth(uint32 d)  { mDepth = mSrcDepth = d; }

        /** Returns the TextureUsage identifier for this Texture
        */
        auto getUsage() const noexcept -> HardwareBufferUsage
        {
            return mUsage;
        }

        /** Sets the TextureUsage identifier for this Texture; only useful before load()
            
            @param u is a combination of TextureUsage::STATIC, TextureUsage::DYNAMIC, TextureUsage::WRITE_ONLY
                TextureUsage::AUTOMIPMAP and TextureUsage::RENDERTARGET (see TextureUsage enum). You are
                strongly advised to use HardwareBufferUsage::STATIC_WRITE_ONLY wherever possible, if you need to 
                update regularly, consider HardwareBufferUsage::DYNAMIC_WRITE_ONLY.
        */
        void setUsage(HardwareBufferUsage u) { mUsage = u; }

        /** Creates the internal texture resources for this texture. 
        @remarks
            This method creates the internal texture resources (pixel buffers, 
            texture surfaces etc) required to begin using this texture. You do
            not need to call this method directly unless you are manually creating
            a texture, in which case something must call it, after having set the
            size and format of the texture (e.g. the ManualResourceLoader might
            be the best one to call it). If you are not defining a manual texture,
            or if you use one of the self-contained load...() methods, then it will be
            called for you.
        */
        void createInternalResources();

        /// @deprecated use unload() instead
        void freeInternalResources();
        
        /** Copies (and maybe scales to fit) the contents of this texture to
            another texture. */
        virtual void copyToTexture( TexturePtr& target );

        /** Loads the data from an image.
        @attention only call this from outside the load() routine of a 
            Resource. Don't call it within (including ManualResourceLoader) - use
            _loadImages() instead. This method is designed to be external, 
            performs locking and checks the load status before loading.
        */
        void loadImage( const Image &img );
            
        /** Loads the data from a raw stream.
        @attention only call this from outside the load() routine of a 
            Resource. Don't call it within (including ManualResourceLoader) - use
            _loadImages() instead. This method is designed to be external, 
            performs locking and checks the load status before loading.
        @param stream Data stream containing the raw pixel data
        @param uWidth Width of the image
        @param uHeight Height of the image
        @param eFormat The format of the pixel data
        */
        void loadRawData( DataStreamPtr& stream,
            ushort uWidth, ushort uHeight, PixelFormat eFormat);

        /** Internal method to load the texture from a set of images. 
        @attention Do NOT call this method unless you are inside the load() routine
            already, e.g. a ManualResourceLoader. It is not threadsafe and does
            not check or update resource loading status.
        */
        void _loadImages( const ConstImagePtrList& images );

        /** Returns the pixel format for the texture surface. */
        auto getFormat() const -> PixelFormat
        {
            return mFormat;
        }

        /** Returns the desired pixel format for the texture surface. */
        auto getDesiredFormat() const -> PixelFormat
        {
            return mDesiredFormat;
        }

        /** Returns the pixel format of the original input texture (may differ due to
            hardware requirements and pixel format conversion).
        */
        auto getSrcFormat() const -> PixelFormat
        {
            return mSrcFormat;
        }

        /** Sets the desired pixel format for the texture surface; can only be set before load(). */
        void setFormat(PixelFormat pf);

        /** Returns true if the texture has an alpha layer. */
        auto hasAlpha() const -> bool;

        /** Sets desired bit depth for integer pixel format textures.

            Available values: 0, 16 and 32, where 0 (the default) means keep original format
            as it is. This value is number of bits for the pixel.
        */
        void setDesiredIntegerBitDepth(ushort bits);

        /** gets desired bit depth for integer pixel format textures.
        */
        auto getDesiredIntegerBitDepth() const noexcept -> ushort;

        /** Sets desired bit depth for float pixel format textures.

            Available values: 0, 16 and 32, where 0 (the default) means keep original format
            as it is. This value is number of bits for a channel of the pixel.
        */
        void setDesiredFloatBitDepth(ushort bits);

        /** gets desired bit depth for float pixel format textures.
        */
        auto getDesiredFloatBitDepth() const noexcept -> ushort;

        /** Sets desired bit depth for integer and float pixel format.
        */
        void setDesiredBitDepths(ushort integerBits, ushort floatBits);

        /// @deprecated use setFormat(PixelFormat::A8)
        void setTreatLuminanceAsAlpha(bool asAlpha);

        /** Return the number of faces this texture has. This will be 6 for a cubemap
            texture and 1 for a 1D, 2D or 3D one.
        */
        auto getNumFaces() const noexcept -> uint32;

        /** Return hardware pixel buffer for a surface. This buffer can then
            be used to copy data from and to a particular level of the texture.
            @param face Face number, in case of a cubemap texture. Must be 0
            for other types of textures.
            For cubemaps, this is one of 
            +X (0), -X (1), +Y (2), -Y (3), +Z (4), -Z (5)
            @param mipmap Mipmap level. This goes from 0 for the first, largest
            mipmap level to getNumMipmaps()-1 for the smallest.
            @return A shared pointer to a hardware pixel buffer.
            @remarks The buffer is invalidated when the resource is unloaded or destroyed.
            Do not use it after the lifetime of the containing texture.
        */
        virtual auto getBuffer(size_t face=0, TextureMipmap mipmap={}) -> const HardwarePixelBufferSharedPtr&;


        /** Populate an Image with the contents of this texture. 
            @param destImage The target image (contents will be overwritten)
            @param includeMipMaps Whether to embed mipmaps in the image
        */
        void convertToImage(Image& destImage, bool includeMipMaps = false);
        
        /** Retrieve a platform or API-specific piece of information from this texture.
            This method of retrieving information should only be used if you know what you're doing.

            | Name        | Description                  |
            |-------------|------------------------------|
            | GLID        | The OpenGL texture object id |

            @param name The name of the attribute to retrieve.
            @param pData Pointer to memory matching the type of data you want to retrieve.
        */
        virtual void getCustomAttribute(std::string_view name, void* pData);
        
        /** simplified API for bindings
         * 
         * @overload
         */
        auto getCustomAttribute(std::string_view name) -> uint
        {
            uint ret = 0;
            getCustomAttribute(name, &ret);
            return ret;
        }

        /** Enable read and/or write privileges to the texture from shaders.
            @param bindPoint The buffer binding location for shader access. For OpenGL this must be unique and is not related to the texture binding point.
            @param access The texture access privileges given to the shader.
            @param mipmapLevel The texture mipmap level to use.
            @param textureArrayIndex The index of the texture array to use. If texture is not a texture array, set to 0.
            @param format Texture format to be read in by shader. For OpenGL this may be different than the bound texture format.
        */
        virtual void createShaderAccessPoint(uint bindPoint, TextureAccess access = TextureAccess::READ_WRITE,
                                        int mipmapLevel = 0, int textureArrayIndex = 0,
                                        PixelFormat format = PixelFormat::UNKNOWN) {}
        /** Set image names to be loaded as layers (3d & texture array) or cubemap faces
         */
        void setLayerNames(const std::vector<String>& names)
        {
            mLayerNames = names;
        }

    protected:
        uint32 mHeight{512};
        uint32 mWidth{512};
        uint32 mDepth{1};

        TextureMipmap mNumRequestedMipmaps{0};
        TextureMipmap mNumMipmaps{0};

        float mGamma{1.0f};
        uint mFSAA{0};

        PixelFormat mFormat{PixelFormat::UNKNOWN};
        HardwareBufferUsage mUsage{TextureUsage::DEFAULT}; /// Bit field, so this can't be TextureUsage

        PixelFormat mSrcFormat{PixelFormat::UNKNOWN};
        uint32 mSrcWidth{0}, mSrcHeight{0}, mSrcDepth{0};

        PixelFormat mDesiredFormat{PixelFormat::UNKNOWN};
        unsigned short mDesiredIntegerBitDepth{0};
        unsigned short mDesiredFloatBitDepth{0};

        bool mTreatLuminanceAsAlpha{false};
        bool mInternalResourcesCreated{false};
        bool mMipmapsHardwareGenerated{false};
        bool mHwGamma{false};

        /// vector of images that should be loaded (cubemap/ texture array)
        std::vector<String> mLayerNames;
        String mFSAAHint;

        /** Vector of images that were pulled from disk by
            prepareLoad but have yet to be pushed into texture memory
            by loadImpl.  Images should be deleted by loadImpl and unprepareImpl.
        */
        using LoadedImages = std::vector<Image>;
        LoadedImages mLoadedImages;

        /// Vector of pointers to subsurfaces
        using SurfaceList = std::vector<HardwarePixelBufferSharedPtr>;
        SurfaceList mSurfaceList;

        TextureType mTextureType{TextureType::_2D};

        void readImage(LoadedImages& imgs, std::string_view name, std::string_view ext, bool haveNPOT);

        void prepareImpl() override;
        void unprepareImpl() override;
        void loadImpl() override;

        /// @copydoc Resource::calculateSize
        auto calculateSize() const -> size_t override;
        

        /** Implementation of creating internal texture resources 
        */
        virtual void createInternalResourcesImpl() = 0;

        /** Implementation of freeing internal texture resources 
        */
        virtual void freeInternalResourcesImpl() = 0;

        /** Default implementation of unload which calls freeInternalResources */
        void unloadImpl() override;

        /** Identify the source file type as a string, either from the extension
            or from a magic number.
        */
        auto getSourceFileType() const -> String;

        static const char constexpr*  CUBEMAP_SUFFIXES[6]
        {"_rt", "_lf", "_up", "_dn", "_fr", "_bk"};
    };
    /** @} */
    /** @} */

}
