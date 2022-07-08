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

#ifndef OGRE_COMPONENTS_OVERLAY_TEXTAREAOVERLAYELEMENT_H
#define OGRE_COMPONENTS_OVERLAY_TEXTAREAOVERLAYELEMENT_H

#include <cstddef>

#include "OgreColourValue.hpp"
#include "OgreFont.hpp"
#include "OgreOverlayElement.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderOperation.hpp"

namespace Ogre
{
    /** \addtogroup Optional
    *  @{
    */
    /** \addtogroup Overlays
    *  @{
    */
    /** This class implements an overlay element which contains simple unformatted text.
    */
    class TextAreaOverlayElement : public OverlayElement
    {
    public:
        enum Alignment
        {
            Left,
            Right,
            Center
        };

    public:
        /** Constructor. */
        TextAreaOverlayElement(std::string_view name);
        ~TextAreaOverlayElement() override;

        void initialise() override;

        /** @copydoc OverlayElement::_releaseManualHardwareResources */
        void _releaseManualHardwareResources() override;
        /** @copydoc OverlayElement::_restoreManualHardwareResources */
        void _restoreManualHardwareResources() override;

        void setCaption(const DisplayString& text) override;

        void setCharHeight( Real height );
        [[nodiscard]] auto getCharHeight() const -> Real;

        void setSpaceWidth( Real width );
        [[nodiscard]] auto getSpaceWidth() const -> Real;

        void setFontName( std::string_view font, std::string_view group = DEFAULT_RESOURCE_GROUP );

        [[nodiscard]] auto getFont() const noexcept -> const FontPtr& {
            return mFont;
        }

        [[nodiscard]] auto getTypeName() const noexcept -> std::string_view override;
        [[nodiscard]] auto getMaterial() const noexcept -> const MaterialPtr& override;
        void getRenderOperation(RenderOperation& op) override;

        /** Sets the colour of the text. 
        @remarks
            This method establishes a constant colour for 
            the entire text. Also see setColourBottom and 
            setColourTop which allow you to set a colour gradient.
        */
        void setColour(const ColourValue& col) override;

        /** Gets the colour of the text. */
        [[nodiscard]] auto getColour() const noexcept -> const ColourValue& override;
        /** Sets the colour of the bottom of the letters.
        @remarks
            By setting a separate top and bottom colour, you
            can create a text area which has a graduated colour
            effect to it.
        */
        void setColourBottom(const ColourValue& col);
        /** Gets the colour of the bottom of the letters. */
        [[nodiscard]] auto getColourBottom() const noexcept -> const ColourValue&;
        /** Sets the colour of the top of the letters.
        @remarks
            By setting a separate top and bottom colour, you
            can create a text area which has a graduated colour
            effect to it.
        */
        void setColourTop(const ColourValue& col);
        /** Gets the colour of the top of the letters. */
        [[nodiscard]] auto getColourTop() const noexcept -> const ColourValue&;

        inline void setAlignment( Alignment a )
        {
            mAlignment = a;
            mGeomPositionsOutOfDate = true;
        }
        [[nodiscard]] inline auto getAlignment() const -> Alignment
        {
            return mAlignment;
        }

        /** Overridden from OverlayElement */
        void setMetricsMode(GuiMetricsMode gmm) override;

        /** Overridden from OverlayElement */
        void _update() override;
    private:
        /// The text alignment
        Alignment mAlignment;

        /// Flag indicating if this panel should be visual or just group things
        bool mTransparent;

        /// Render operation
        RenderOperation mRenderOp;

        /// Method for setting up base parameters for this class
        void addBaseParameters() override;

        static String msTypeName;

        FontPtr mFont;
        Real mCharHeight;
        ushort mPixelCharHeight;
        Real mSpaceWidth;
        ushort mPixelSpaceWidth;
        size_t mAllocSize;
        Real mViewportAspectCoef;

        /// Colours to use for the vertices
        ColourValue mColourBottom;
        ColourValue mColourTop;
        bool mColoursChanged;


        /// Internal method to allocate memory, only reallocates when necessary
        void checkMemoryAllocation( size_t numChars );
        /// Inherited function
        void updatePositionGeometry() override;
        /// Inherited function
        void updateTextureGeometry() override;
        /// Updates vertex colours
        virtual void updateColours();
    };
    /** @} */
    /** @} */
}

#endif

