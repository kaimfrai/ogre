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
module;

#define generic _generic    // keyword for C++/CX
#include <freetype/freetype.h>
#undef generic

module Ogre.Components.Overlay;

import :Font;
import :Manager;
import :utf8;

import Ogre.Core;

import <algorithm>;
import <ostream>;
import <string>;

namespace Ogre
{
    //---------------------------------------------------------------------
    namespace {
    class CmdType : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    class CmdSource : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    class CmdCharSpacer : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    class CmdSize : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    class CmdResolution : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    class CmdCodePoints : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };

    // Command object for setting / getting parameters
    static CmdType msTypeCmd;
    static CmdSource msSourceCmd;
    static CmdCharSpacer msCharacterSpacerCmd;
    static CmdSize msSizeCmd;
    static CmdResolution msResolutionCmd;
    static CmdCodePoints msCodePointsCmd;
    }

    auto utftoc32(String str) -> std::vector<uint32>
    {
        std::vector<uint32> decoded;
        decoded.reserve(str.size());

        str.resize(str.size() + 3); // add padding for decoder
        auto it = str.c_str();
        auto end = str.c_str() + str.size() - 3;
        while(it < end)
        {
            uint32 cpId;
            int err = 0;
            it = utf8_decode(it, &cpId, &err);
            if(err)
                continue;
            decoded.push_back(cpId);
        }
        return decoded;
    }

    //---------------------------------------------------------------------
    Font::Font(ResourceManager* creator, std::string_view name, ResourceHandle handle,
        std::string_view group, bool isManual, ManualResourceLoader* loader)
        :Resource (creator, name, handle, group, isManual, loader) 
    {

        if (createParamDictionary("Font"))
        {
            ParamDictionary* dict = getParamDictionary();
            dict->addParameter(
                ParameterDef("type", "'truetype' or 'image' based font", ParameterType::STRING),
                &msTypeCmd);
            dict->addParameter(
                ParameterDef("source", "Filename of the source of the font.", ParameterType::STRING),
                &msSourceCmd);
            dict->addParameter(
                ParameterDef("character_spacer", "Spacing between characters to prevent overlap artifacts.", ParameterType::STRING),
                &msCharacterSpacerCmd);
            dict->addParameter(
                ParameterDef("size", "True type size", ParameterType::REAL),
                &msSizeCmd);
            dict->addParameter(
                ParameterDef("resolution", "True type resolution", ParameterType::UNSIGNED_INT),
                &msResolutionCmd);
            dict->addParameter(
                ParameterDef("code_points", "Add a range of code points", ParameterType::STRING),
                &msCodePointsCmd);
        }

    }
    //---------------------------------------------------------------------
    Font::~Font()
    {
        // have to call this here reather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        unload();
    }
    //---------------------------------------------------------------------
    void Font::setType(FontType ftype)
    {
        mType = ftype;
    }
    //---------------------------------------------------------------------
    auto Font::getType() const -> FontType
    {
        return mType;
    }
    //---------------------------------------------------------------------
    void Font::setSource(std::string_view source)
    {
        mSource = source;
    }
    //---------------------------------------------------------------------
    void Font::setTrueTypeSize(Real ttfSize)
    {
        mTtfSize = ttfSize;
    }
    //---------------------------------------------------------------------
    void Font::setTrueTypeResolution(uint ttfResolution)
    {
        mTtfResolution = ttfResolution;
    }
    //---------------------------------------------------------------------
    auto Font::getSource() const noexcept -> std::string_view
    {
        return mSource;
    }
    //---------------------------------------------------------------------
    auto Font::getTrueTypeSize() const -> Real
    {
        return mTtfSize;
    }
    //---------------------------------------------------------------------
    auto Font::getTrueTypeResolution() const noexcept -> uint
    {
        return mTtfResolution;
    }
    //---------------------------------------------------------------------
    auto Font::getTrueTypeMaxBearingY() const noexcept -> int
    {
        return mTtfMaxBearingY;
    }
    //---------------------------------------------------------------------
    void Font::_setMaterial(const MaterialPtr &mat)
    {
        mMaterial = mat;
    }

    void Font::putText(BillboardSet* bbs, String text, float height, const ColourValue& colour)
    {
        // ensure loaded
        load();
        // configure Billboard for display
        bbs->setMaterial(mMaterial);
        bbs->setBillboardType(BillboardType::PERPENDICULAR_COMMON);
        bbs->setBillboardOrigin(BillboardOrigin::CENTER_LEFT);
        bbs->setDefaultDimensions(0, 0);

        float spaceWidth = mCodePointMap.find('0')->second.advance * height;

        text.resize(text.size() + 3); // add padding for decoder
        auto it = text.c_str();
        auto end = text.c_str() + text.size() - 3;

        const auto& bbox = bbs->getBoundingBox();

        float left = 0;
        float top = bbox == AxisAlignedBox::BOX_NULL ? 0 : bbox.getMinimum().y - height;
        while (it < end)
        {
            uint32 cpId;
            int err = 0;
            it = utf8_decode(it, &cpId, &err);
            if(err)
                continue;

            if (cpId == ' ')
            {
                left += spaceWidth;
                continue;
            }

            if(cpId == '\n')
            {
                top -= height;
                left = 0;
                continue;
            }

            auto cp = mCodePointMap.find(cpId);
            if (cp == mCodePointMap.end())
                continue;

            left += cp->second.bearing * height;

            auto bb = bbs->createBillboard(Vector3{left, top, 0}, colour);
            bb->setDimensions(cp->second.aspectRatio * height, height);
            bb->setTexcoordRect(cp->second.uvRect);

            left += (cp->second.advance - cp->second.bearing) * height;
        }
    }

    //---------------------------------------------------------------------
    void Font::loadImpl()
    {
        // Create a new material
        mMaterial =  MaterialManager::getSingleton().create(
            ::std::format("Fonts/{}", mName),  mGroup);

        if (!mMaterial)
        {
            OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR,
                "Error creating new material!", "Font::load" );
        }

        if (mType == FontType::TRUETYPE)
        {
            createTextureFromFont();
        }
        else
        {
            // Manually load since we need to load to get alpha
            mTexture = TextureManager::getSingleton().load(mSource, mGroup, TextureType::_2D, TextureMipmap{});
        }

        // Make sure material is aware of colour per vertex.
        auto pass = mMaterial->getTechnique(0)->getPass(0);
        pass->setVertexColourTracking(TrackVertexColourEnum::DIFFUSE);

        // lighting and culling also do not make much sense
        pass->setCullingMode(CullingMode::NONE);
        pass->setLightingEnabled(false);
        mMaterial->setReceiveShadows(false);
        // font quads should not occlude things
        pass->setDepthWriteEnabled(false);

        TextureUnitState *texLayer = mMaterial->getTechnique(0)->getPass(0)->createTextureUnitState();
        texLayer->setTexture(mTexture);
        // Clamp to avoid fuzzy edges
        texLayer->setTextureAddressingMode( TextureAddressingMode::CLAMP );
        // Allow min/mag filter, but no mip
        texLayer->setTextureFiltering(FilterOptions::LINEAR, FilterOptions::LINEAR, FilterOptions::NONE);


        // Set up blending
        if (mTexture->hasAlpha())
        {
            mMaterial->setSceneBlending( SceneBlendType::TRANSPARENT_ALPHA );
            mMaterial->getTechnique(0)->getPass(0)->setTransparentSortingEnabled(false);
        }
        else
        {
            // Use add if no alpha (assume black background)
            mMaterial->setSceneBlending(SceneBlendType::ADD);
        }
    }
    //---------------------------------------------------------------------
    void Font::unloadImpl()
    {
        if (mMaterial)
        {
            MaterialManager::getSingleton().remove(mMaterial);
            mMaterial.reset();
        }

        if (mTexture)
        {
            TextureManager::getSingleton().remove(mTexture);
            mTexture.reset();
        }
    }
    //---------------------------------------------------------------------
    void Font::createTextureFromFont()
    {
        // Just create the texture here, and point it at ourselves for when
        // it wants to (re)load for real
        mTexture = TextureManager::getSingleton().create(::std::format("{}Texture", mName), mGroup, true, this);
        mTexture->setTextureType(TextureType::_2D);
        mTexture->setNumMipmaps(TextureMipmap{});
        mTexture->load();
    }
    //---------------------------------------------------------------------
    void Font::loadResource(Resource* res)
    {
        // Locate ttf file, load it pre-buffered into memory by wrapping the
        // original DataStream in a MemoryDataStream
        DataStreamPtr dataStreamPtr =
            ResourceGroupManager::getSingleton().openResource(
                mSource, mGroup, this);
        MemoryDataStream ttfchunk(dataStreamPtr);

        // If codepoints not supplied, assume ASCII
        if (mCodePointRangeList.empty())
        {
            mCodePointRangeList.push_back(CodePointRange(33, 126));
        }
        float vpScale = OverlayManager::getSingleton().getPixelRatio();

        // ManualResourceLoader implementation - load the texture
        FT_Library ftLibrary;
        // Init freetype
        if( FT_Init_FreeType( &ftLibrary ) )
            OGRE_EXCEPT( ExceptionCodes::INTERNAL_ERROR, "Could not init FreeType library!",
            "Font::Font");

        FT_Face face;

        // Load font
        if( FT_New_Memory_Face( ftLibrary, ttfchunk.getPtr(), (FT_Long)ttfchunk.size() , 0, &face ) )
            OGRE_EXCEPT( ExceptionCodes::INTERNAL_ERROR,
            "Could not open font face!", "Font::createTextureFromFont" );


        // Convert our point size to freetype 26.6 fixed point format
        auto ftSize = (FT_F26Dot6)(mTtfSize * (1 << 6));
        if (FT_Set_Char_Size(face, ftSize, 0, mTtfResolution * vpScale, mTtfResolution * vpScale))
            OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, "Could not set char size!");

        //FILE *fo_def = stdout;

        FT_Pos max_height = 0, max_width = 0;

        // Calculate maximum width, height and bearing
        size_t glyphCount = 0;
        for (auto const& [begin, end] : mCodePointRangeList)
        {
            for(CodePoint cp = begin; cp <= end; ++cp, ++glyphCount)
            {
                FT_Load_Char( face, cp, FT_LOAD_RENDER );

                max_height = std::max<FT_Pos>(2 * face->glyph->bitmap.rows - (face->glyph->metrics.horiBearingY >> 6), max_height);
                mTtfMaxBearingY = std::max(int(face->glyph->metrics.horiBearingY >> 6), mTtfMaxBearingY);
                max_width = std::max<FT_Pos>(face->glyph->bitmap.width, max_width);
            }

        }

        uint char_spacer = 1;

        // Now work out how big our texture needs to be
        size_t rawSize = (max_width + char_spacer) * (max_height + char_spacer) * glyphCount;

        auto tex_side = static_cast<uint32>(Math::Sqrt((Real)rawSize));
        // Now round up to nearest power of two
        uint32 roundUpSize = Bitwise::firstPO2From(tex_side);

        // Would we benefit from using a non-square texture (2X width)
        uint32 finalWidth, finalHeight;
        if (roundUpSize * roundUpSize * 0.5 >= rawSize)
        {
            finalHeight = static_cast<uint32>(roundUpSize * 0.5);
        }
        else
        {
            finalHeight = roundUpSize;
        }
        finalWidth = roundUpSize;

        Real textureAspect = (Real)finalWidth / (Real)finalHeight;

        Image img(PixelFormat::BYTE_LA, finalWidth, finalHeight);
        // Reset content (transparent)
        img.setTo(ColourValue::ZERO);

        uint32 l = 0, m = 0;
        for (auto const& [begin, end] : mCodePointRangeList)
        {
            for(CodePoint cp = begin; cp <= end; ++cp )
            {
                uchar* buffer;
                int buffer_h = 0, buffer_pitch = 0;

                // Load & render glyph
                FT_Error ftResult = FT_Load_Char( face, cp, FT_LOAD_RENDER );
                if (ftResult)
                {
                    // problem loading this glyph, continue
                    LogManager::getSingleton().logError(std::format(
                        "Freetype could not load charcode {} in font {}", cp, mSource.c_str()));
                    continue;
                }

                buffer = face->glyph->bitmap.buffer;
                if (!buffer)
                {
                    // Yuck, FT didn't detect this but generated a null pointer!
                    LogManager::getSingleton().logWarning(std::format(
                        "Freetype did not find charcode {} in font {}", cp, mSource.c_str()));
                    continue;
                }

                uint advance = face->glyph->advance.x >> 6;
                uint width = face->glyph->bitmap.width;
                buffer_pitch = face->glyph->bitmap.pitch;
                buffer_h = face->glyph->bitmap.rows;

                FT_Pos y_bearing = mTtfMaxBearingY - (face->glyph->metrics.horiBearingY >> 6);
                FT_Pos x_bearing = face->glyph->metrics.horiBearingX >> 6;

                // If at end of row
                if( finalWidth - 1 < l + width )
                {
                    m += max_height + char_spacer;
                    l = 0;
                }

                for(int j = 0; j < buffer_h; j++ )
                {
                    uchar* pSrc = buffer + j * buffer_pitch;
                    uint32 row = j + m + y_bearing;
                    uchar* pDest = img.getData(l, row);
                    for(unsigned int k = 0; k < width; k++ )
                    {
                        if (mAntialiasColour)
                        {
                            // Use the same greyscale pixel for all components RGBA
                            *pDest++= *pSrc;
                        }
                        else
                        {
                            // Always white whether 'on' or 'off' pixel, since alpha
                            // will turn off
                            *pDest++= 0xFF;
                        }
                        // Always use the greyscale value for alpha
                        *pDest++= *pSrc++;
                    }
                }

                UVRect uvs{(Real)l / (Real)finalWidth,                   // u1
                           (Real)m / (Real)finalHeight,                  // v1
                           (Real)(l + width) / (Real)finalWidth,         // u2
                           (m + max_height) / (Real)finalHeight}; // v2
                this->setGlyphInfo({cp, uvs, float(textureAspect * uvs.width() / uvs.height()),
                                    float(x_bearing) / max_height, float(advance) / max_height});

                // Advance a column
                l += (width + char_spacer);
            }
        }

        FT_Done_FreeType(ftLibrary);

        auto* tex = static_cast<Texture*>(res);
        // Call internal _loadImages, not loadImage since that's external and 
        // will determine load status etc again, and this is a manual loader inside load()
        tex->_loadImages({&img});
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    auto CmdType::doGet(const void* target) const -> String
    {
        const Font* f = static_cast<const Font*>(target);
        if (f->getType() == FontType::TRUETYPE)
        {
            return "truetype";
        }
        else
        {
            return "image";
        }
    }
    void CmdType::doSet(void* target, std::string_view val)
    {
        Font* f = static_cast<Font*>(target);
        if (val == "truetype")
        {
            f->setType(FontType::TRUETYPE);
        }
        else
        {
            f->setType(FontType::IMAGE);
        }
    }
    //-----------------------------------------------------------------------
    auto CmdSource::doGet(const void* target) const -> String
    {
        const Font* f = static_cast<const Font*>(target);
        return std::string{ f->getSource() };
    }
    void CmdSource::doSet(void* target, std::string_view val)
    {
        Font* f = static_cast<Font*>(target);
        f->setSource(val);
    }
    //-----------------------------------------------------------------------
    auto CmdCharSpacer::doGet(const void* target) const -> String
    {
        return "1";
    }
    void CmdCharSpacer::doSet(void* target, std::string_view val) {}
    //-----------------------------------------------------------------------
    auto CmdSize::doGet(const void* target) const -> String
    {
        const Font* f = static_cast<const Font*>(target);
        return StringConverter::toString(f->getTrueTypeSize());
    }
    void CmdSize::doSet(void* target, std::string_view val)
    {
        Font* f = static_cast<Font*>(target);
        f->setTrueTypeSize(StringConverter::parseReal(val));
    }
    //-----------------------------------------------------------------------
    auto CmdResolution::doGet(const void* target) const -> String
    {
        const Font* f = static_cast<const Font*>(target);
        return StringConverter::toString(f->getTrueTypeResolution());
    }
    void CmdResolution::doSet(void* target, std::string_view val)
    {
        Font* f = static_cast<Font*>(target);
        f->setTrueTypeResolution(StringConverter::parseUnsignedInt(val));
    }
    //-----------------------------------------------------------------------
    auto CmdCodePoints::doGet(const void* target) const -> String
    {
        const Font* f = static_cast<const Font*>(target);
        StringStream str;
        for (const auto& i : f->getCodePointRangeList())
        {
            str << i.first << "-" << i.second << " ";
        }
        return str.str();
    }
    void CmdCodePoints::doSet(void* target, std::string_view val)
    {
        // Format is "code_points start1-end1 start2-end2"
        Font* f = static_cast<Font*>(target);

        auto const vec = StringUtil::split(val, " \t");
        for (auto & item : vec)
        {
            auto const itemVec = StringUtil::split(item, "-");
            if (itemVec.size() == 2)
            {
                f->addCodePointRange({StringConverter::parseUnsignedInt(itemVec[0]),
                                      StringConverter::parseUnsignedInt(itemVec[1])});
            }
        }
    }


}
