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
export module Ogre.Core:Codec;

export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :StringVector;

export import <cstddef>;
export import <map>;
export import <string>;

export
namespace Ogre {
class Any;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */

    /** Abstract class that defines a 'codec'.
        @remarks
            A codec class works like a two-way filter for data - data entered on
            one end (the decode end) gets processed and transformed into easily
            usable data while data passed the other way around codes it back.
        @par
            The codec concept is a pretty generic one - you can easily understand
            how it can be used for images, sounds, archives, even compressed data.
    */
    class Codec : public CodecAlloc
    {
    private:
        using CodecList = std::map<String, Codec *>; 
        /** A map that contains all the registered codecs.
        */
        static CodecList msMapCodecs;

    public:
        virtual ~Codec();
        
        /** Registers a new codec in the database.
        */
        static void registerCodec( Codec *pCodec );

        /** Return whether a codec is registered already. 
        */
        static auto isCodecRegistered( const String& codecType ) -> bool
        {
            return msMapCodecs.find(codecType) != msMapCodecs.end();
        }

        /** Unregisters a codec from the database.
        */
        static void unregisterCodec( Codec *pCodec )
        {
            msMapCodecs.erase(pCodec->getType());
        }

        /** Gets the file extension list for the registered codecs. */
        static auto getExtensions() -> StringVector;

        /** Gets the codec registered for the passed in file extension. */
        static auto getCodec(const String& extension) -> Codec*;

        /** Gets the codec that can handle the given 'magic' identifier. 
        @param magicNumberPtr Pointer to a stream of bytes which should identify the file.
            Note that this may be more than needed - each codec may be looking for 
            a different size magic number.
        @param maxbytes The number of bytes passed
        */
        static auto getCodec(char *magicNumberPtr, size_t maxbytes) -> Codec*;

        /** Codes the input and saves the result in the output
            stream.
        */
        [[nodiscard]]
        virtual auto encode(const Any& input) const -> DataStreamPtr;

        /** Codes the data in the input chunk and saves the result in the output
            filename provided. Provided for efficiency since coding to memory is
            progressive therefore memory required is unknown leading to reallocations.
        @param input The input data (codec type specific)
        @param outFileName The filename to write to
        */
        virtual void encodeToFile(const Any& input, const String& outFileName) const;

        /** Codes the data from the input chunk into the output chunk.
            @param input Stream containing the encoded data
            @param output codec type specific result
        */
        virtual void decode(const DataStreamPtr& input, const Any& output) const = 0;

        /** Returns the type of the codec as a String
        */
        [[nodiscard]]
        virtual auto getType() const -> String = 0;

        /** Returns whether a magic number header matches this codec.
        @param magicNumberPtr Pointer to a stream of bytes which should identify the file.
            Note that this may be more than needed - each codec may be looking for 
            a different size magic number.
        @param maxbytes The number of bytes passed
        */
        auto magicNumberMatch(const char *magicNumberPtr, size_t maxbytes) const -> bool
        { return !magicNumberToFileExt(magicNumberPtr, maxbytes).empty(); }
        /** Maps a magic number header to a file extension, if this codec recognises it.
        @param magicNumberPtr Pointer to a stream of bytes which should identify the file.
            Note that this may be more than needed - each codec may be looking for 
            a different size magic number.
        @param maxbytes The number of bytes passed
        @return A blank string if the magic number was unknown, or a file extension.
        */
        virtual auto magicNumberToFileExt(const char *magicNumberPtr, size_t maxbytes) const -> String = 0;
    };
    /** @} */
    /** @} */

} // namespace
