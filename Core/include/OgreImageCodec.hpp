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
#ifndef OGRE_CORE_IMAGECODEC_H
#define OGRE_CORE_IMAGECODEC_H

#include "OgreBitwise.hpp"
#include "OgreCodec.hpp"
#include "OgrePixelFormat.hpp"

#include <any>

namespace Ogre {

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Image
    *  @{
    */
    /** Codec specialized in images.
        @remarks
            The users implementing subclasses of ImageCodec are required to return
            a valid pointer to a ImageData class from the decode(...) function.
    */
    class ImageCodec : public Codec
    {
    protected:
        static void flipEndian(void* pData, size_t size, size_t count)
        {
        }
        static void flipEndian(void* pData, size_t size)
        {
        }

    public:
        using Codec::decode;
        using Codec::encode;
        using Codec::encodeToFile;

        void decode(const DataStreamPtr& input, ::std::any const& output) const override;
        [[nodiscard]] DataStreamPtr encode(::std::any const& input) const override;
        void encodeToFile(::std::any const& input, const String& outFileName) const override;

        ~ImageCodec() override;
        /** Codec return class for images. Has information about the size and the
            pixel format of the image. */
        class ImageData
        {
        public:
            ImageData() 
            = default;
            uint32 height{0};
            uint32 width{0};
            uint32 depth{1};
            size_t size{0};
            
            uint32 num_mipmaps{0};
            uint flags{0};

            PixelFormat format{PF_UNKNOWN};
        };
        using CodecDataPtr = SharedPtr<ImageData>;

        /// @deprecated
        [[nodiscard]] virtual DataStreamPtr encode(const MemoryDataStreamPtr& input, const CodecDataPtr& pData) const { return encode(::std::any{}); }
        /// @deprecated
        virtual void encodeToFile(const MemoryDataStreamPtr& input, const String& outFileName, const CodecDataPtr& pData) const
        { encodeToFile(::std::any{}, ""); }
        /// Result of a decoding; both a decoded data stream and CodecData metadata
        using DecodeResult = std::pair<MemoryDataStreamPtr, CodecDataPtr>;
        /// @deprecated
        [[nodiscard]] virtual DecodeResult decode(const DataStreamPtr& input) const
        {
            return {};
        }
    };

    /** @} */
    /** @} */
} // namespace

#endif
