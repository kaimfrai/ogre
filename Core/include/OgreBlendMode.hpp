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
#ifndef OGRE_CORE_BLENDMODE_H
#define OGRE_CORE_BLENDMODE_H

#include "OgreColourValue.hpp"
#include "OgrePrerequisites.hpp"

namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Materials
    *  @{
    */

    /** Type of texture blend mode.
    */
    enum class LayerBlendType
    {
        COLOUR,
        ALPHA
    };

    /** List of valid texture blending operations, for use with TextureUnitState::setColourOperation.
        @remarks
            This list is a more limited list than LayerBlendOperationEx because it only
            includes operations that are supportable in both multipass and multitexture
            rendering and thus provides automatic fallback if multitexture hardware
            is lacking or insufficient.
    */
    enum class LayerBlendOperation {
        /// Replace all colour with texture with no adjustment
        REPLACE,
        /// Add colour components together.
        ADD,
        /// Multiply colour components together.
        MODULATE,
        /// Blend based on texture alpha
        ALPHA_BLEND

    };

    /** Expert list of valid texture blending operations, for use with TextureUnitState::setColourOperationEx
        and TextureUnitState::setAlphaOperation, and internally in the LayerBlendModeEx class. It's worth
        noting that these operations are for blending <i>between texture layers</i> and not between rendered objects
        and the existing scene. Because all of these modes are only supported in multitexture hardware it may be
        required to set up a fallback operation where this hardware is not available.
    */
    enum class LayerBlendOperationEx {
        /// use source1 without modification
        SOURCE1,
        /// use source2 without modification
        SOURCE2,
        /// multiply source1 and source2 together
        MODULATE,
        /// as MODULATE but brighten afterwards (x2)
        MODULATE_X2,
        /// as MODULATE but brighten more afterwards (x4)
        MODULATE_X4,
        /// add source1 and source2 together
        ADD,
        /// as ADD, but subtract 0.5 from the result
        ADD_SIGNED,
        /// as ADD, but subtract product from the sum
        ADD_SMOOTH,
        /// subtract source2 from source1
        SUBTRACT,
        /// use interpolated alpha value from vertices to scale source1, then add source2 scaled by (1-alpha)
        BLEND_DIFFUSE_ALPHA,
        /// as BLEND_DIFFUSE_ALPHA, but use alpha from texture
        BLEND_TEXTURE_ALPHA,
        /// as BLEND_DIFFUSE_ALPHA, but use current alpha from previous stages
        BLEND_CURRENT_ALPHA,
        /// as BLEND_DIFFUSE_ALPHA but use a constant manual blend value (0.0-1.0)
        BLEND_MANUAL,
        /// dot product of color1 and color2 
        DOTPRODUCT,
        /// use interpolated color values from vertices to scale source1, then add source2 scaled by (1-color)
        BLEND_DIFFUSE_COLOUR
    };

    /** List of valid sources of values for blending operations used
        in TextureUnitState::setColourOperation and TextureUnitState::setAlphaOperation,
        and internally in the LayerBlendModeEx class.
    */
    enum class LayerBlendSource
    {
        /// the colour as built up from previous stages
        CURRENT,
        /// the colour derived from the texture assigned to this layer
        TEXTURE,
        /// the interpolated diffuse colour from the vertices
        DIFFUSE,
        /// the interpolated specular colour from the vertices
        SPECULAR,
        /// a colour supplied manually as a separate argument
        MANUAL
    };
    /** Class which manages blending of both colour and alpha components.
        @remarks
            This class is a utility class used by both TextureUnitState and
            RenderSystem to wrap up the details of a blending operation. This blending
            operation could be used for blending colour or alpha in a texture layer.
            This class is really only for use by OGRE, since apps can deal with
            blending modes through the TextureUnitState class methods
            setColourOperation and setAlphaOperation.
        @par
            It's worth noting that these operations are for blending <i>between texture
            layers</i> and not between rendered objects and the existing scene. If
            you wish to make an object blend with others in the scene, e.g. to make
            transparent objects etc, use the Material::setSceneBlending method.
    */
    class LayerBlendModeEx
    {
    public:
        /// The type of blending (colour or alpha)
        LayerBlendType blendType;
        /// The operation to be applied
        LayerBlendOperationEx operation;
        /// The first source of colour/alpha
        LayerBlendSource source1;
        /// The second source of colour/alpha
        LayerBlendSource source2;

        /// Manual colour value for manual source1
        ColourValue colourArg1;
        /// Manual colour value for manual source2
        ColourValue colourArg2;
        /// Manual alpha value for manual source1
        Real alphaArg1;
        /// Manual alpha value for manual source2
        Real alphaArg2;
        /// Manual blending factor
        Real factor;

        [[nodiscard]] auto operator==(const LayerBlendModeEx& rhs) const noexcept -> bool
        {
            if (blendType != rhs.blendType) return false;

            if (blendType == LayerBlendType::COLOUR)
            {

                if (operation == rhs.operation &&
                    source1 == rhs.source1 &&
                    source2 == rhs.source2 &&
                    colourArg1 == rhs.colourArg1 &&
                    colourArg2 == rhs.colourArg2 &&
                    factor == rhs.factor)
                {
                    return true;
                }
            }
            else // if (blendType == LayerBlendType::ALPHA)
            {
                if (operation == rhs.operation &&
                    source1 == rhs.source1 &&
                    source2 == rhs.source2 &&
                    alphaArg1 == rhs.alphaArg1 &&
                    alphaArg2 == rhs.alphaArg2 &&
                    factor == rhs.factor)
                {
                    return true;
                }
            }
            return false;
        }
    };

    /** Types of blending that you can specify between an object and the existing contents of the scene.

        As opposed to the LayerBlendType, which classifies blends between texture layers, these blending
        types blend between the output of the texture units and the pixels already in the viewport,
        allowing for object transparency, glows, etc.

        These types are provided to give quick and easy access to common effects. You can also use
        the more manual method of supplying source and destination blending factors.
        See Material::setSceneBlending for more details.
        @see
            Pass::setSceneBlending
    */
    enum class SceneBlendType
    {
        TRANSPARENT_ALPHA, //!< The alpha value of the rendering output is used as a mask.
        TRANSPARENT_COLOUR, //!< Colour the scene based on the brightness of the input colours, but don’t darken.
        ADD, //!< The colour of the rendering output is added to the scene. Good for explosions, flares, lights, ghosts etc.
        MODULATE, //!< The colour of the rendering output is multiplied with the scene contents. Generally colours and darkens the scene, good for smoked glass, semi-transparent objects etc.
        REPLACE //!< The default blend mode where source replaces destination
        // TODO : more
    };

    /** Blending factors for manually blending objects with the scene. If there isn't a predefined
        SceneBlendType that you like, then you can specify the blending factors directly to affect the
        combination of object and the existing scene. See Material::setSceneBlending for more details.
    */
    enum class SceneBlendFactor
    {
        ONE, //!< Constant value of 1.0
        ZERO, //!< Constant value of 0.0
        DEST_COLOUR, //!< The existing pixel colour
        SOURCE_COLOUR, //!< The texture pixel (texel) colour
        ONE_MINUS_DEST_COLOUR, //!< 1 - SceneBlendFactor::DEST_COLOUR
        ONE_MINUS_SOURCE_COLOUR, //!< 1 - SceneBlendFactor::SOURCE_COLOUR
        DEST_ALPHA, //!< The existing pixel alpha value
        SOURCE_ALPHA, //!< The texel alpha value
        ONE_MINUS_DEST_ALPHA, //!< 1 - SceneBlendFactor::DEST_ALPHA
        ONE_MINUS_SOURCE_ALPHA //!< 1 - SceneBlendFactor::SOURCE_ALPHA
    };

    /** Blending operations controls how objects are blended into the scene. The default operation
        is add (+) but by changing this you can change how drawn objects are blended into the
        existing scene.
    */
    enum class SceneBlendOperation
    {
        ADD,
        SUBTRACT,
        REVERSE_SUBTRACT,
        MIN,
        MAX
    };

    /** Describes the global blending factors for combining subsequent renders with the existing frame contents.

    By default the operation is Ogre::SceneBlendOperation::ADD, which creates this equation

    $$final = (passOutput * sourceFactor) + (frameBuffer * destFactor)$$

    Each of the factors is specified as one of Ogre::SceneBlendFactor.

    By setting a different Ogre::SceneBlendOperation you can achieve a different effect.
    */
    struct ColourBlendState
    {
        /** @name Write Mask
         * Whether writing is enabled for each of the 4 colour channels */
        /// @{
        bool writeR : 1{true};
        bool writeG : 1{true};
        bool writeB : 1{true};
        bool writeA : 1{true};
        /// @}

        /** @name Blending factors
         * used to weight the render colour components and the frame colour components */
        /// @{
        SceneBlendFactor sourceFactor{SceneBlendFactor::ONE};
        SceneBlendFactor destFactor{SceneBlendFactor::ZERO};
        SceneBlendFactor sourceFactorAlpha{SceneBlendFactor::ONE};
        SceneBlendFactor destFactorAlpha{SceneBlendFactor::ZERO};
        /// @}

        /** @name Blending operations
         * The blend operation mode for combining colour values */
        /// @{
        SceneBlendOperation operation{SceneBlendOperation::ADD};
        SceneBlendOperation alphaOperation{SceneBlendOperation::ADD};
        /// @}

        ColourBlendState()
             
        = default;

        /// can we simply overwrite the existing pixels or do we have to blend
        [[nodiscard]] auto blendingEnabled() const -> bool
        {
            return !(sourceFactor == SceneBlendFactor::ONE && destFactor == SceneBlendFactor::ZERO &&
                     sourceFactorAlpha == SceneBlendFactor::ONE && destFactorAlpha == SceneBlendFactor::ZERO);
        }
    };
    /** @} */
    /** @} */

}

#endif
