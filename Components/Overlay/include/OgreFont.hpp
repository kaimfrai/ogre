/*-------------------------------------------------------------------------
This source file is a part of OGRE
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
THE SOFTWARE
-------------------------------------------------------------------------*/

#ifndef OGRE_COMPONENTS_OVERLAY_FONT_H
#define OGRE_COMPONENTS_OVERLAY_FONT_H

#include <algorithm>
#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include "OgreColourValue.hpp"
#include "OgreCommon.hpp"
#include "OgreException.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResource.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreString.hpp"

namespace Ogre
{
    class BillboardSet;
class ResourceManager;

    /** \addtogroup Optional
    *  @{
    */
    /** \addtogroup Overlays
    *  @{
    */

    /// decode UTF8 encoded bytestream to uint32 codepoints
    std::vector<uint32> utftoc32(String str);

    /** Enumerates the types of Font usable in the engine. */
    enum FontType
    {
        /// Generated from a truetype (.ttf) font
        FT_TRUETYPE = 1,
        /// Loaded from an image created by an artist
        FT_IMAGE = 2
    };


    /// Information about the position and size of a glyph in a texture
    struct GlyphInfo
    {
        using CodePoint = uint32;
        using UVRect = FloatRect;

        CodePoint codePoint;
        UVRect uvRect;
        float aspectRatio; // width/ height
        float bearing; // bearingX/ height
        float advance; // advanceX/ height
    };

    /** Class representing a font in the system.
    @remarks
    This class is simply a way of getting a font texture into the OGRE system and
    to easily retrieve the texture coordinates required to accurately render them.
    Fonts can either be loaded from precreated textures, or the texture can be generated
    using a truetype font. You can either create the texture manually in code, or you
    can use a .fontdef script to define it (probably more practical since you can reuse
    the definition more easily)
    @note
    This class extends both Resource and ManualResourceLoader since it is
    both a resource in it's own right, but it also provides the manual load
    implementation for the Texture it creates.
    */
    class Font : public Resource, public ManualResourceLoader
    {
    protected:
        /// The type of font
        FontType mType{FT_TRUETYPE};

        /// Source of the font (either an image name or a truetype font)
        String mSource;

        /// Size of the truetype font, in points
        Real mTtfSize{0};
        /// Resolution (dpi) of truetype font
        uint mTtfResolution{0};
        /// Max distance to baseline of this (truetype) font
        int mTtfMaxBearingY{0};


    public:
        using CodePoint = GlyphInfo::CodePoint;
        using UVRect = GlyphInfo::UVRect;
        /// A range of code points, inclusive on both ends
        using CodePointRange = std::pair<CodePoint, CodePoint>;
        using CodePointRangeList = std::vector<CodePointRange>;
    protected:
        /// Map from unicode code point to texture coordinates
        using CodePointMap = std::map<CodePoint, GlyphInfo>;
        CodePointMap mCodePointMap;

        /// The material which is generated for this font
        MaterialPtr mMaterial;

        /// Texture pointer
        TexturePtr mTexture;

        /// For TRUE_TYPE font only
        bool mAntialiasColour{false};

        /// Range of code points to generate glyphs for (truetype only)
        CodePointRangeList mCodePointRangeList;

        /// Internal method for loading from ttf
        void createTextureFromFont();

        /// @copydoc Resource::loadImpl
        void loadImpl() override;
        /// @copydoc Resource::unloadImpl
        void unloadImpl() override;
        /// @copydoc Resource::calculateSize
        size_t calculateSize() const noexcept override { return 0; } // permanent resource is in the texture 
    public:

        /** Constructor.
        @see Resource
        */
        Font(ResourceManager* creator, const String& name, ResourceHandle handle,
            const String& group, bool isManual = false, ManualResourceLoader* loader = nullptr);
        ~Font() override;

        /** Sets the type of font. Must be set before loading. */
        void setType(FontType ftype);

        /** Gets the type of font. */
        FontType getType() const;

        /** Sets the source of the font.
        @remarks
            If you have created a font of type FT_IMAGE, this method tells the
            Font which image to use as the source for the characters. So the parameter 
            should be the name of an appropriate image file. Note that when using an image
            as a font source, you will also need to tell the font where each character is
            located using setGlyphTexCoords (for each character).
        @par
            If you have created a font of type FT_TRUETYPE, this method tells the
            Font which .ttf file to use to generate the text. You will also need to call 
            setTrueTypeSize and setTrueTypeResolution, and call addCodePointRange
            as many times as required to define the range of glyphs you want to be
            available.
        @param source An image file or a truetype font, depending on the type of this font
        */
        void setSource(const String& source);

        /** Gets the source this font (either an image or a truetype font).
        */
        const String& getSource() const noexcept;

        /** Sets the size of a truetype font (only required for FT_TRUETYPE). 
        @param ttfSize The size of the font in points. Note that the
            size of the font does not affect how big it is on the screen, just how large it is in
            the texture and thus how detailed it is.
        */
        void setTrueTypeSize(Real ttfSize);
        /** Gets the resolution (dpi) of the font used to generate the texture
        (only required for FT_TRUETYPE).
        @param ttfResolution The resolution in dpi
        */
        void setTrueTypeResolution(uint ttfResolution);

        /** Gets the point size of the font used to generate the texture.
        @remarks
            Only applicable for FT_TRUETYPE Font objects.
            Note that the size of the font does not affect how big it is on the screen, 
            just how large it is in the texture and thus how detailed it is.            
        */
        Real getTrueTypeSize() const;
        /** Gets the resolution (dpi) of the font used to generate the texture.
        @remarks
            Only applicable for FT_TRUETYPE Font objects.
        */
        uint getTrueTypeResolution() const noexcept;
        /** Gets the maximum baseline distance of all glyphs used in the texture.
        @remarks
            Only applicable for FT_TRUETYPE Font objects.
            The baseline is the vertical origin of horizontal based glyphs.  The bearingY
            attribute is the distance from the baseline (origin) to the top of the glyph's 
            bounding box.
        @note
            This value is only available after the font has been loaded.
        */
        int getTrueTypeMaxBearingY() const noexcept;


        /** Returns the texture coordinates of the associated glyph. 
            @remarks Parameter is a short to allow both ASCII and wide chars.
            @param id The code point (unicode)
            @return A rectangle with the UV coordinates, or null UVs if the
                code point was not present
        */
        const UVRect& getGlyphTexCoords(CodePoint id) const { return getGlyphInfo(id).uvRect; }

        /** Sets the texture coordinates of a glyph.
        @remarks
            You only need to call this if you're setting up a font loaded from a texture manually.
        @note
            Also sets the aspect ratio (width / height) of this character. textureAspect
            is the width/height of the texture (may be non-square)
        */
        void setGlyphInfoFromTexCoords(CodePoint id, const UVRect& rect, float textureAspect = 1.0)
        {
            auto glyphAspect = textureAspect * rect.width()  / rect.height();
            setGlyphInfo({id, rect, glyphAspect, 0, glyphAspect});
        }

        void setGlyphInfo(const GlyphInfo& info) { mCodePointMap[info.codePoint] = info; }

        /** Gets the aspect ratio (width / height) of this character. */
        float getGlyphAspectRatio(CodePoint id) const { return getGlyphInfo(id).aspectRatio; }
        /** Sets the aspect ratio (width / height) of this character.
        @remarks
            You only need to call this if you're setting up a font loaded from a 
            texture manually.
        */
        void setGlyphAspectRatio(CodePoint id, Real ratio)
        {
            CodePointMap::iterator i = mCodePointMap.find(id);
            if (i != mCodePointMap.end())
            {
                i->second.aspectRatio = ratio;
            }
        }

        /** Gets the information available for a glyph corresponding to a
            given code point, or throws an exception if it doesn't exist;
        */
        const GlyphInfo& getGlyphInfo(CodePoint id) const
        {
            CodePointMap::const_iterator i = mCodePointMap.find(id);
            if (i == mCodePointMap.end())
            {
                OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                            StringUtil::format("Code point %d not found in font %s", id, mName.c_str()));
            }
            return i->second;
        }

        /** Adds a range of code points to the list of code point ranges to generate
            glyphs for, if this is a truetype based font.
        @remarks
            In order to save texture space, only the glyphs which are actually
            needed by the application are generated into the texture. Before this
            object is loaded you must call this method as many times as necessary
            to define the code point range that you need.
        */
        void addCodePointRange(const CodePointRange& range)
        {
            mCodePointRangeList.push_back(range);
        }

        /** Clear the list of code point ranges.
        */
        void clearCodePointRanges()
        {
            mCodePointRangeList.clear();
        }
        /** Get a const reference to the list of code point ranges to be used to
            generate glyphs from a truetype font.
        */
        const CodePointRangeList& getCodePointRangeList() const noexcept
        {
            return mCodePointRangeList;
        }
        /** Gets the material generated for this font, as a weak reference. 
        @remarks
            This will only be valid after the Font has been loaded. 
        */
        inline const MaterialPtr& getMaterial() const noexcept
        {
            return mMaterial;
        }

        /**
         * Write a text into a BillboardSet for positioning in Space
         *
         * Text is laid out in the x-y plane, running into x+ and using y+ as up
         * @param bbs the target BillboardSet
         * @param text text to write
         * @param height character height in world units
         * @param colour text colour
         */
        void putText(BillboardSet* bbs, String text, float height, const ColourValue& colour = ColourValue::White);

        /** Sets whether or not the colour of this font is antialiased as it is generated
            from a true type font.
        @remarks
            This is valid only for a FT_TRUETYPE font. If you are planning on using 
            alpha blending to draw your font, then it is a good idea to set this to
            false (which is the default), otherwise the darkening of the font will combine
            with the fading out of the alpha around the edges and make your font look thinner
            than it should. However, if you intend to blend your font using a colour blending
            mode (add or modulate for example) then it's a good idea to set this to true, in
            order to soften your font edges.
        */
        inline void setAntialiasColour(bool enabled)
        {
            mAntialiasColour = enabled;
        }

        /** Gets whether or not the colour of this font is antialiased as it is generated
        from a true type font.
        */
        inline bool getAntialiasColour() const noexcept
        {
            return mAntialiasColour;
        }

        /** Implementation of ManualResourceLoader::loadResource, called
            when the Texture that this font creates needs to (re)load.
        */
        void loadResource(Resource* resource) override;

        /** Manually set the material used for this font.
        @remarks
            This should only be used when the font is being loaded from a
            ManualResourceLoader.
        */
        void _setMaterial(const MaterialPtr& mat);
    };

    using FontPtr = SharedPtr<Font>;
    /** @} */
    /** @} */
}

#endif
