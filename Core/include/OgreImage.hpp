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
#ifndef OGRE_CORE_IMAGE_H
#define OGRE_CORE_IMAGE_H

#include <cassert>
#include <cstddef>
#include <vector>

#include "OgreColourValue.hpp"
#include "OgreCommon.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePixelFormat.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"

namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Image
    *  @{
    */

    enum ImageFlags
    {
        IF_COMPRESSED = 0x00000001,
        IF_CUBEMAP    = 0x00000002,
        IF_3D_TEXTURE = 0x00000004
    };
    /** Class representing an image file.
        @remarks
            The Image class usually holds uncompressed image data and is the
            only object that can be loaded in a texture. Image  objects handle 
            image data decoding themselves by the means of locating the correct 
            Codec object for each data type.
        @par
            Typically, you would want to use an Image object to load a texture
            when extra processing needs to be done on an image before it is
            loaded or when you want to blit to an existing texture.
    */
    class Image : public ImageAlloc
    {
    friend class ImageCodec;
    public:
        /** Standard constructor.
         *
         * allocates a buffer of given size if buffer pointer is NULL.
         */
        Image(PixelFormat format = PF_UNKNOWN, uint32 width = 0, uint32 height = 0, uint32 depth = 1,
              uchar* buffer = nullptr, bool autoDelete = true);
        /** Copy-constructor - copies all the data from the target image.
         */
        Image( const Image &img );

        /**
         * allocates a buffer of given size if needed
         *
         * - If the current allocation is equal to the requested size, this does nothing
         * - Otherwise any current allocation is freed, and memory of specified size is allocated
         *
         * @see loadDynamicImage
         */
        void create(PixelFormat format, uint32 width, uint32 height, uint32 depth = 1, uint32 numFaces = 1,
                    uint32 numMipMaps = 0);

        /** Standard destructor.
        */
        ~Image();

        /** Assignment operator - copies all the data from the target image.
        */
        auto operator = ( const Image & img ) -> Image &;

        /**
         * sets all pixels to the specified colour
         *
         * format conversion is performed as needed
         */
        void setTo(const ColourValue& col);

        /** Flips (mirrors) the image around the Y-axis. 
            @remarks
                An example of an original and flipped image:
                <pre>                
                originalimg
                00000000000
                00000000000
                00000000000
                00000000000
                00000000000
                ------------> flip axis
                00000000000
                00000000000
                00000000000
                00000000000
                00000000000
                originalimg
                </pre>
        */
        auto flipAroundY() -> Image &;

        /** Flips (mirrors) the image around the X-axis.
            @remarks
                An example of an original and flipped image:
                <pre>
                        flip axis
                            |
                originalimg|gmilanigiro
                00000000000|00000000000
                00000000000|00000000000
                00000000000|00000000000
                00000000000|00000000000
                00000000000|00000000000
                </pre>
        */                 
        auto flipAroundX() -> Image &;

        /** Stores a pointer to raw data in memory. The pixel format has to be specified.
            @remarks
                This method loads an image into memory held in the object. The 
                pixel format will be either greyscale or RGB with an optional
                Alpha component.
                The type can be determined by calling getFormat().             
            @note
                Whilst typically your image is likely to be a simple 2D image,
                you can define complex images including cube maps, volume maps,
                and images including custom mip levels. The layout of the 
                internal memory should be:
                <ul><li>face 0, mip 0 (top), width x height (x depth)</li>
                <li>face 0, mip 1, width/2 x height/2 (x depth/2)</li>
                <li>face 0, mip 2, width/4 x height/4 (x depth/4)</li>
                <li>.. remaining mips for face 0 .. </li>
                <li>face 1, mip 0 (top), width x height (x depth)</li
                <li>.. and so on. </li>
                </ul>
                Of course, you will never have multiple faces (cube map) and
                depth too.
            @param data
                The data pointer
            @param width
                Width of image
            @param height
                Height of image
            @param depth
                Image Depth (in 3d images, numbers of layers, otherwise 1)
            @param format
                Pixel Format
            @param autoDelete
                If memory associated with this buffer is to be destroyed
                with the Image object.
            @param numFaces
                The number of faces the image data has inside (6 for cubemaps, 1 otherwise)
            @param numMipMaps
                The number of mipmaps the image data has inside
            @note
                 The memory associated with this buffer is NOT destroyed with the
                 Image object, unless autoDelete is set to true.
            @remarks 
                The size of the buffer must be numFaces * PixelUtil::getMemorySize(width, height, depth, format)
         */
        auto loadDynamicImage(uchar* data, uint32 width, uint32 height, uint32 depth, PixelFormat format,
                                bool autoDelete = false, uint32 numFaces = 1, uint32 numMipMaps = 0) -> Image&;

        /// @overload
        auto loadDynamicImage(uchar* data, uint32 width, uint32 height, PixelFormat format) -> Image&
        {
            return loadDynamicImage(data, width, height, 1, format);
        }
        /** Loads raw data from a stream. See the function
            loadDynamicImage for a description of the parameters.
            @remarks 
                The size of the buffer must be numFaces * PixelUtil::getMemorySize(width, height, depth, format)
            @note
                Whilst typically your image is likely to be a simple 2D image,
                you can define complex images including cube maps
                and images including custom mip levels. The layout of the 
                internal memory should be:
                <ul><li>face 0, mip 0 (top), width x height (x depth)</li>
                <li>face 0, mip 1, width/2 x height/2 (x depth/2)</li>
                <li>face 0, mip 2, width/4 x height/4 (x depth/4)</li>
                <li>.. remaining mips for face 0 .. </li>
                <li>face 1, mip 0 (top), width x height (x depth)</li
                <li>.. and so on. </li>
                </ul>
                Of course, you will never have multiple faces (cube map) and
                depth too.
        */
        auto loadRawData(const DataStreamPtr& stream, uint32 width, uint32 height, uint32 depth,
                           PixelFormat format, uint32 numFaces = 1, uint32 numMipMaps = 0) -> Image&;
        /// @overload
        auto loadRawData(const DataStreamPtr& stream, uint32 width, uint32 height, PixelFormat format) -> Image&
        {
            return loadRawData(stream, width, height, 1, format);
        }

        /** Loads an image file.
            @remarks
                This method loads an image into memory. Any format for which 
                an associated ImageCodec is registered can be loaded. 
                This can include complex formats like DDS with embedded custom 
                mipmaps, cube faces and volume textures.
                The type can be determined by calling getFormat().             
            @param
                filename Name of an image file to load.
            @param
                groupName Name of the resource group to search for the image
            @note
                The memory associated with this buffer is destroyed with the
                Image object.
        */
        auto load( StringView filename, StringView groupName ) -> Image &;

        /** Loads an image file from a stream.
            @remarks
                This method works in the same way as the filename-based load 
                method except it loads the image from a DataStream object. 
                This DataStream is expected to contain the 
                encoded data as it would be held in a file. 
                Any format for which an associated ImageCodec is registered 
                can be loaded. 
                This can include complex formats like DDS with embedded custom 
                mipmaps, cube faces and volume textures.
                The type can be determined by calling getFormat().             
            @param
                stream The source data.
            @param
                type The type of the image. Used to decide what decompression
                codec to use. Can be left blank if the stream data includes
                a header to identify the data.
            @see
                Image::load( StringView filename )
        */
        auto load(const DataStreamPtr& stream, StringView type = BLANKSTRING ) -> Image &;

        /** Utility method to combine 2 separate images into this one, with the first
        image source supplying the RGB channels, and the second image supplying the 
        alpha channel (as luminance or separate alpha). 
        @param rgbFilename Filename of image supplying the RGB channels (any alpha is ignored)
        @param alphaFilename Filename of image supplying the alpha channel. If a luminance image the
            single channel is used directly, if an RGB image then the values are
            converted to greyscale.
        @param groupName The resource group from which to load the images
        @param format The destination format
        */
        auto loadTwoImagesAsRGBA(StringView rgbFilename, StringView alphaFilename,
            StringView groupName, PixelFormat format = PF_BYTE_RGBA) -> Image &;

        /** Utility method to combine 2 separate images into this one, with the first
        image source supplying the RGB channels, and the second image supplying the 
        alpha channel (as luminance or separate alpha). 
        @param rgbStream Stream of image supplying the RGB channels (any alpha is ignored)
        @param alphaStream Stream of image supplying the alpha channel. If a luminance image the
            single channel is used directly, if an RGB image then the values are
            converted to greyscale.
        @param format The destination format
        @param rgbType The type of the RGB image. Used to decide what decompression
            codec to use. Can be left blank if the stream data includes
            a header to identify the data.
        @param alphaType The type of the alpha image. Used to decide what decompression
            codec to use. Can be left blank if the stream data includes
            a header to identify the data.
        */
        auto loadTwoImagesAsRGBA(const DataStreamPtr& rgbStream, const DataStreamPtr& alphaStream,
                                   PixelFormat format = PF_BYTE_RGBA,
                                   StringView rgbType = BLANKSTRING,
                                   StringView alphaType = BLANKSTRING) -> Image&;

        /** Utility method to combine 2 separate images into this one, with the first
            image source supplying the RGB channels, and the second image supplying the 
            alpha channel (as luminance or separate alpha). 
        @param rgb Image supplying the RGB channels (any alpha is ignored)
        @param alpha Image supplying the alpha channel. If a luminance image the
            single channel is used directly, if an RGB image then the values are
            converted to greyscale.
        @param format The destination format
        */
        auto combineTwoImagesAsRGBA(const Image& rgb, const Image& alpha, PixelFormat format = PF_BYTE_RGBA) -> Image &;

        
        /** Save the image as a file. 
        @remarks
            Saving and loading are implemented by back end (sometimes third 
            party) codecs.  Implemented saving functionality is more limited
            than loading in some cases. Particularly DDS file format support 
            is currently limited to true colour or single channel float32, 
            square, power of two textures with no mipmaps.  Volumetric support
            is currently limited to DDS files.
        */
        void save(StringView filename);

        /** Encode the image and return a stream to the data. 
            @param formatextension An extension to identify the image format
                to encode into, e.g. "jpg" or "png"
        */
        auto encode(StringView formatextension) -> DataStreamPtr;

        /** Returns a pointer to the internal image buffer at the specified pixel location.

            Be careful with this method. You will almost certainly
            prefer to use getPixelBox, especially with complex images
            which include many faces or custom mipmaps.
        */
        auto getData(uint32 x = 0, uint32 y = 0, uint32 z = 0) -> uchar*
        {
            assert((!mBuffer && (x + y + z) == 0) || (x < mWidth && y < mHeight && z < mDepth));
            return mBuffer + mPixelSize * (z * mWidth * mHeight + mWidth * y + x);
        }

        /// @overload
        [[nodiscard]] auto getData(uint32 x = 0, uint32 y = 0, uint32 z = 0) const -> const uchar*
        {
            assert(mBuffer);
            assert(x < mWidth && y < mHeight && z < mDepth);
            return mBuffer + mPixelSize * (z * mWidth * mHeight + mWidth * y + x);
        }

        /// @overload
        template <typename T> auto getData(uint32 x = 0, uint32 y = 0, uint32 z = 0) -> T*
        {
            return reinterpret_cast<T*>(getData(x, y, z));
        }

        /// @overload
        template <typename T> auto getData(uint32 x = 0, uint32 y = 0, uint32 z = 0) const -> const T*
        {
            return reinterpret_cast<const T*>(getData(x, y, z));
        }

        /** Returns the size of the data buffer in bytes
        */
        [[nodiscard]] auto getSize() const -> size_t;

        /** Returns the number of mipmaps contained in the image.
        */
        [[nodiscard]] auto getNumMipmaps() const noexcept -> uint32;

        /** Returns true if the image has the appropriate flag set.
        */
        [[nodiscard]] auto hasFlag(const ImageFlags imgFlag) const -> bool;

        /** Gets the width of the image in pixels.
        */
        [[nodiscard]] auto getWidth() const noexcept -> uint32;

        /** Gets the height of the image in pixels.
        */
        [[nodiscard]] auto getHeight() const noexcept -> uint32;

        /** Gets the depth of the image.
        */
        [[nodiscard]] auto getDepth() const noexcept -> uint32;
        
        /** Get the number of faces of the image. This is usually 6 for a cubemap, and
            1 for a normal image.
        */
        [[nodiscard]] auto getNumFaces() const noexcept -> uint32;

        /** Gets the physical width in bytes of each row of pixels.
        */
        [[nodiscard]] auto getRowSpan() const -> size_t;

        /** Returns the image format.
        */
        [[nodiscard]] auto getFormat() const -> PixelFormat;

        /** Returns the number of bits per pixel.
        */
        [[nodiscard]] auto getBPP() const -> uchar;

        /** Returns true if the image has an alpha component.
        */
        [[nodiscard]] auto getHasAlpha() const noexcept -> bool;
        
        /** Does gamma adjustment.
            @note
                Basic algo taken from Titan Engine, copyright (c) 2000 Ignacio 
                Castano Iguado
        */
        static void applyGamma( uchar *buffer, Real gamma, size_t size, uchar bpp );

        /**
         * Get colour value from a certain location in the image. The z coordinate
         * is only valid for cubemaps and volume textures. This uses the first (largest)
         * mipmap.
         */
        [[nodiscard]] auto getColourAt(uint32 x, uint32 y, uint32 z) const -> ColourValue;
        
        /**
         * Set colour value at a certain location in the image. The z coordinate
         * is only valid for cubemaps and volume textures. This uses the first (largest)
         * mipmap.
         */
        void setColourAt(ColourValue const &cv, uint32 x, uint32 y, uint32 z);

        /**
         * Get a PixelBox encapsulating the image data of a mipmap
         */
        [[nodiscard]] auto getPixelBox(uint32 face = 0, uint32 mipmap = 0) const -> PixelBox;

        /// Delete all the memory held by this image, if owned by this image (not dynamic)
        void freeMemory();

        enum Filter
        {
            FILTER_NEAREST,
            FILTER_LINEAR,
            FILTER_BILINEAR = FILTER_LINEAR
        };
        /** Scale a 1D, 2D or 3D image volume. 
            @param  src         PixelBox containing the source pointer, dimensions and format
            @param  dst         PixelBox containing the destination pointer, dimensions and format
            @param  filter      Which filter to use
            @remarks    This function can do pixel format conversion in the process.
            @note   dst and src can point to the same PixelBox object without any problem
        */
        static void scale(const PixelBox &src, const PixelBox &dst, Filter filter = FILTER_BILINEAR);
        
        /** Resize a 2D image, applying the appropriate filter. */
        void resize(ushort width, ushort height, Filter filter = FILTER_BILINEAR);
        
        /// Static function to calculate size in bytes from the number of mipmaps, faces and the dimensions
        static auto calculateSize(uint32 mipmaps, uint32 faces, uint32 width, uint32 height, uint32 depth, PixelFormat format) -> size_t;

        /// Static function to get an image type string from a stream via magic numbers
        static auto getFileExtFromMagic(DataStreamPtr stream) -> StringView;

    private:
        /// The width of the image in pixels
        uint32 mWidth{0};
        /// The height of the image in pixels
        uint32 mHeight{0};
        /// The depth of the image
        uint32 mDepth{0};
        /// The number of mipmaps the image contains
        uint32 mNumMipmaps{0};
        /// The size of the image buffer
        size_t mBufSize{0};
        /// Image specific flags.
        int mFlags{0};

        /// The pixel format of the image
        PixelFormat mFormat;

        uchar* mBuffer{ nullptr };
        /// The number of bytes per pixel
        uchar mPixelSize;
        /// A bool to determine if we delete the buffer or the calling app does
        bool mAutoDelete{ true };
    };

    using ImagePtrList = std::vector<Image *>;
    using ConstImagePtrList = std::vector<const Image *>;

    /** @} */
    /** @} */

} // namespace

#endif
