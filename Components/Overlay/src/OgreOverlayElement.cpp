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

module Ogre.Components.Overlay;

import :Container;
import :Element;
import :ElementCommands;
import :Manager;

import Ogre.Core;

import <string>;
import <utility>;

namespace Ogre {


    //---------------------------------------------------------------------
    // Define static members
    // Command object for setting / getting parameters
    static OverlayElementCommands::CmdLeft msLeftCmd;
    static OverlayElementCommands::CmdTop msTopCmd;
    static OverlayElementCommands::CmdWidth msWidthCmd;
    static OverlayElementCommands::CmdHeight msHeightCmd;
    static OverlayElementCommands::CmdMaterial msMaterialCmd;
    static OverlayElementCommands::CmdCaption msCaptionCmd;
    static OverlayElementCommands::CmdMetricsMode msMetricsModeCmd;
    static OverlayElementCommands::CmdHorizontalAlign msHorizontalAlignCmd;
    static OverlayElementCommands::CmdVerticalAlign msVerticalAlignCmd;
    static OverlayElementCommands::CmdVisible msVisibleCmd;

    std::string_view const constinit OverlayElement::DEFAULT_RESOURCE_GROUP = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
    //---------------------------------------------------------------------
    OverlayElement::OverlayElement(std::string_view name)
      : mName(name)
       
    {
        // default overlays to preserve their own detail level
        mPolygonModeOverrideable = false;

        // use identity projection and view matrices
        mUseIdentityProjection = true;
        mUseIdentityView = true;
    }
    //---------------------------------------------------------------------
    OverlayElement::~OverlayElement()
    {
        if (mParent)
        {
            mParent->removeChild(mName);
            mParent = nullptr;
        }
    }
    //---------------------------------------------------------------------
    void OverlayElement::setDimensions(Real width, Real height)
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            mPixelWidth = width;
            mPixelHeight = height;
        }
        else
        {
            mWidth = width;
            mHeight = height;
        }
        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    void OverlayElement::setPosition(Real left, Real top)
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            mPixelLeft = left;
            mPixelTop = top;
        }
        else
        {
            mLeft = left;
            mTop = top;
        }
        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    void OverlayElement::setWidth(Real width)
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            mPixelWidth = width;
        }
        else
        {
            mWidth = width;
        }
        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    auto OverlayElement::getWidth() const -> Real
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            return mPixelWidth;
        }
        else
        {
            return mWidth;
        }
    }
    //---------------------------------------------------------------------
    void OverlayElement::setHeight(Real height)
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            mPixelHeight = height;
        }
        else
        {
            mHeight = height;
        }
        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    auto OverlayElement::getHeight() const -> Real
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            return mPixelHeight;
        }
        else
        {
            return mHeight;
        }
    }
    //---------------------------------------------------------------------
    void OverlayElement::setLeft(Real left)
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            mPixelLeft = left;
        }
        else
        {
            mLeft = left;
        }
        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    auto OverlayElement::getLeft() const -> Real
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            return mPixelLeft;
        }
        else
        {
            return mLeft;
        }
    }
    //---------------------------------------------------------------------
    void OverlayElement::setTop(Real top)
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            mPixelTop = top;
        }
        else
        {
            mTop = top;
        }

        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    auto OverlayElement::getTop() const -> Real
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            return mPixelTop;
        }
        else
        {
            return mTop;
        }
    }
    //---------------------------------------------------------------------
    void OverlayElement::_setLeft(Real left)
    {
        mLeft = left;
        mPixelLeft = left / mPixelScaleX;

        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    void OverlayElement::_setTop(Real top)
    {
        mTop = top;
        mPixelTop = top / mPixelScaleY;

        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    void OverlayElement::_setWidth(Real width)
    {
        mWidth = width;
        mPixelWidth = width / mPixelScaleX;

        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    void OverlayElement::_setHeight(Real height)
    {
        mHeight = height;
        mPixelHeight = height / mPixelScaleY;

        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    void OverlayElement::_setPosition(Real left, Real top)
    {
        mLeft = left;
        mTop  = top;
        mPixelLeft = left / mPixelScaleX;
        mPixelTop  = top / mPixelScaleY;

        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    void OverlayElement::_setDimensions(Real width, Real height)
    {
        mWidth  = width;
        mHeight = height;
        mPixelWidth  = width / mPixelScaleX;
        mPixelHeight = height / mPixelScaleY;

        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //---------------------------------------------------------------------
    auto OverlayElement::getMaterialName() const noexcept -> std::string_view
    {
        return mMaterial ? mMaterial->getName() : BLANKSTRING;
    }
    //---------------------------------------------------------------------
    void OverlayElement::setMaterial(const MaterialPtr& mat)
    {
        mMaterial = mat;

        if(!mMaterial)
            return;

        mMaterial->load();

        auto dstPass = mMaterial->getTechnique(0)->getPass(0); // assume this is representative
        if (dstPass->getLightingEnabled() || dstPass->getDepthCheckEnabled())
        {
            LogManager::getSingleton().logWarning(
                ::std::format("force-disabling 'lighting' and 'depth_check' of Material {}"
                " for use with OverlayElement {}", mat->getName(), getName()));
        }

        // Set some prerequisites to be sure
        mMaterial->setLightingEnabled(false);
        mMaterial->setReceiveShadows(false);
        mMaterial->setDepthCheckEnabled(false);
    }

    void OverlayElement::setMaterialName(std::string_view matName, std::string_view group)
    {
        if (!matName.empty())
        {
            mMaterial = MaterialManager::getSingleton().getByName(matName, group);
            if (!mMaterial)
                OGRE_EXCEPT( ExceptionCodes::ITEM_NOT_FOUND, ::std::format("Could not find material {}", matName),
                    "OverlayElement::setMaterialName" );

            setMaterial(mMaterial);
        }
        else
        {
            mMaterial.reset();
        }
    }
    //---------------------------------------------------------------------
    auto OverlayElement::getMaterial() const noexcept -> const MaterialPtr&
    {
        return mMaterial;
    }
    //---------------------------------------------------------------------
    void OverlayElement::getWorldTransforms(Matrix4* xform) const
    {
        mOverlay->_getWorldTransforms(xform);
    }
    //---------------------------------------------------------------------
    void OverlayElement::_positionsOutOfDate()
    {
        mGeomPositionsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    void OverlayElement::_update()
    {
        Real vpWidth, vpHeight;
        OverlayManager& oMgr = OverlayManager::getSingleton();
        vpWidth = (Real) (oMgr.getViewportWidth());
        vpHeight = (Real) (oMgr.getViewportHeight());

        // Check size if pixel-based or relative-aspect-adjusted
        switch (mMetricsMode)
        {
        case GuiMetricsMode::PIXELS :
            if (mGeomPositionsOutOfDate)
            {               
                mPixelScaleX = 1.0f / vpWidth;
                mPixelScaleY = 1.0f / vpHeight; 
            }
            break;

        case GuiMetricsMode::RELATIVE_ASPECT_ADJUSTED :
            if (mGeomPositionsOutOfDate)
            {
                mPixelScaleX = 1.0f / (10000.0f * (vpWidth / vpHeight));
                mPixelScaleY = 1.0f /  10000.0f;
            }
            break;

        default:
        case GuiMetricsMode::RELATIVE :
            mPixelScaleX = 1.0;
            mPixelScaleY = 1.0;
            mPixelLeft = mLeft;
            mPixelTop = mTop;
            mPixelWidth = mWidth;
            mPixelHeight = mHeight;
            break;
        }

        mLeft   = mPixelLeft   * mPixelScaleX;
        mTop    = mPixelTop    * mPixelScaleY;
        mWidth  = mPixelWidth  * mPixelScaleX;
        mHeight = mPixelHeight * mPixelScaleY;

        Real tmpPixelWidth = mPixelWidth;

        _updateFromParent();
        // NB container subclasses will update children too

        // Tell self to update own position geometry
        if (mGeomPositionsOutOfDate && mInitialised)
        {
            updatePositionGeometry();

            // Within updatePositionGeometry() of TextOverlayElements,
            // the needed pixel width is calculated and as a result a new 
            // second update call is needed, so leave the dirty flag on 'true'
            // so that that in a second call in the next frame, the element
            // width can be correctly set and the text gets displayed.
            if(mMetricsMode == GuiMetricsMode::PIXELS && tmpPixelWidth != mPixelWidth)
                mGeomPositionsOutOfDate = true;
            else
                mGeomPositionsOutOfDate = false;
        }

        // Tell self to update own texture geometry
        if (mGeomUVsOutOfDate && mInitialised)
        {
            updateTextureGeometry();
            mGeomUVsOutOfDate = false;
        } 
    }
    //---------------------------------------------------------------------
    void OverlayElement::_updateFromParent()
    {
        Real parentLeft = 0, parentTop = 0, parentBottom = 0, parentRight = 0;

        if (mParent)
        {
            parentLeft = mParent->_getDerivedLeft();
            parentTop = mParent->_getDerivedTop();
            if (mHorzAlign == GuiHorizontalAlignment::CENTER || mHorzAlign == GuiHorizontalAlignment::RIGHT)
            {
                parentRight = parentLeft + mParent->_getRelativeWidth();
            }
            if (mVertAlign == GuiVerticalAlignment::CENTER || mVertAlign == GuiVerticalAlignment::BOTTOM)
            {
                parentBottom = parentTop + mParent->_getRelativeHeight();
            }
        }
        else
        {
            RenderSystem* rSys = Root::getSingleton().getRenderSystem();
            OverlayManager& oMgr = OverlayManager::getSingleton();

            // Calculate offsets required for mapping texel origins to pixel origins in the
            // current rendersystem
            Real hOffset = rSys->getHorizontalTexelOffset() / oMgr.getViewportWidth();
            Real vOffset = rSys->getVerticalTexelOffset() / oMgr.getViewportHeight();

            parentLeft = 0.0f + hOffset;
            parentTop = 0.0f + vOffset;
            parentRight = 1.0f + hOffset;
            parentBottom = 1.0f + vOffset;
        }

        // Sort out position based on alignment
        // NB all we do is derived the origin, we don't automatically sort out the position
        // This is more flexible than forcing absolute right & middle 
        switch(mHorzAlign)
        {
        using enum GuiHorizontalAlignment;
        case CENTER:
            mDerivedLeft = ((parentLeft + parentRight) * 0.5f) + mLeft;
            break;
        case LEFT:
            mDerivedLeft = parentLeft + mLeft;
            break;
        case RIGHT:
            mDerivedLeft = parentRight + mLeft;
            break;
        };

        switch(mVertAlign)
        {
        using enum GuiVerticalAlignment;
        case CENTER:
            mDerivedTop = ((parentTop + parentBottom) * 0.5f) + mTop;
            break;
        case TOP:
            mDerivedTop = parentTop + mTop;
            break;
        case BOTTOM:
            mDerivedTop = parentBottom + mTop;
            break;
        };

        mDerivedOutOfDate = false;

        if (mParent != nullptr)
        {
            RealRect parent;
            RealRect child;

            mParent->_getClippingRegion(parent);

            child.left   = mDerivedLeft;
            child.top    = mDerivedTop;
            child.right  = mDerivedLeft + mWidth;
            child.bottom = mDerivedTop + mHeight;

            mClippingRegion = parent.intersect(child);
        }
        else
        {
            mClippingRegion.left   = mDerivedLeft;
            mClippingRegion.top    = mDerivedTop;
            mClippingRegion.right  = mDerivedLeft + mWidth;
            mClippingRegion.bottom = mDerivedTop + mHeight;
        }
    }
    //---------------------------------------------------------------------
    void OverlayElement::_notifyParent(OverlayContainer* parent, Overlay* overlay)
    {
        mParent = parent;
        mOverlay = overlay;

        if (mOverlay && mOverlay->isInitialised() && !mInitialised)
        {
            initialise();
        }

        mDerivedOutOfDate = true;
    }
    //---------------------------------------------------------------------
    auto OverlayElement::_getDerivedLeft() -> Real
    {
        if (mDerivedOutOfDate)
        {
            _updateFromParent();
        }
        return mDerivedLeft;
    }
    //---------------------------------------------------------------------
    auto OverlayElement::_getDerivedTop() -> Real
    {
        if (mDerivedOutOfDate)
        {
            _updateFromParent();
        }
        return mDerivedTop;
    }
    //---------------------------------------------------------------------
    auto OverlayElement::_getRelativeWidth() -> Real
    {
        return mWidth;
    }
    //---------------------------------------------------------------------
    auto OverlayElement::_getRelativeHeight() -> Real
    {
        return mHeight;
    }
    //---------------------------------------------------------------------    
    void OverlayElement::_getClippingRegion(RealRect &clippingRegion)
    {
        if (mDerivedOutOfDate)
        {
            _updateFromParent();
        }
        clippingRegion = mClippingRegion;
    }
    //---------------------------------------------------------------------
    auto OverlayElement::_notifyZOrder(ushort newZOrder) -> ushort
    {
        mZOrder = newZOrder;
        return mZOrder + 1;
    }
    //---------------------------------------------------------------------
    void OverlayElement::_notifyWorldTransforms(const Matrix4& xform)
    {
        mXForm = xform;
    }
    //---------------------------------------------------------------------
    void OverlayElement::_notifyViewport()
    {
        switch (mMetricsMode)
        {
        case GuiMetricsMode::PIXELS :
            {
                Real vpWidth, vpHeight;
                OverlayManager& oMgr = OverlayManager::getSingleton();
                vpWidth = (Real) (oMgr.getViewportWidth());
                vpHeight = (Real) (oMgr.getViewportHeight());

                mPixelScaleX = 1.0f / vpWidth;
                mPixelScaleY = 1.0f / vpHeight;
            }
            break;

        case GuiMetricsMode::RELATIVE_ASPECT_ADJUSTED :
            {
                Real vpWidth, vpHeight;
                OverlayManager& oMgr = OverlayManager::getSingleton();
                vpWidth = (Real) (oMgr.getViewportWidth());
                vpHeight = (Real) (oMgr.getViewportHeight());

                mPixelScaleX = 1.0f / (10000.0f * (vpWidth / vpHeight));
                mPixelScaleY = 1.0f /  10000.0f;
            }
            break;

        default:
        case GuiMetricsMode::RELATIVE :
            mPixelScaleX = 1.0;
            mPixelScaleY = 1.0;
            mPixelLeft = mLeft;
            mPixelTop = mTop;
            mPixelWidth = mWidth;
            mPixelHeight = mHeight;
            break;
        }

        mLeft = mPixelLeft * mPixelScaleX;
        mTop = mPixelTop * mPixelScaleY;
        mWidth = mPixelWidth * mPixelScaleX;
        mHeight = mPixelHeight * mPixelScaleY;

        mGeomPositionsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    void OverlayElement::_updateRenderQueue(RenderQueue* queue)
    {
        if (mVisible)
        {
            queue->addRenderable(this, RenderQueueGroupID::OVERLAY, mZOrder);
        }      
    }
    //---------------------------------------------------------------------
    void OverlayElement::visitRenderables(Renderable::Visitor* visitor, 
        bool debugRenderables)
    {
        visitor->visit(this, 0, false);
    }
    //-----------------------------------------------------------------------
    void OverlayElement::addBaseParameters()    
    {
        ParamDictionary* dict = getParamDictionary();

        dict->addParameter(ParameterDef("left", 
            "The position of the left border of the gui element."
            , ParameterType::REAL),
            &msLeftCmd);
        dict->addParameter(ParameterDef("top", 
            "The position of the top border of the gui element."
            , ParameterType::REAL),
            &msTopCmd);
        dict->addParameter(ParameterDef("width", 
            "The width of the element."
            , ParameterType::REAL),
            &msWidthCmd);
        dict->addParameter(ParameterDef("height", 
            "The height of the element."
            , ParameterType::REAL),
            &msHeightCmd);
        dict->addParameter(ParameterDef("material", 
            "The name of the material to use."
            , ParameterType::STRING),
            &msMaterialCmd);
        dict->addParameter(ParameterDef("caption", 
            "The element caption, if supported."
            , ParameterType::STRING),
            &msCaptionCmd);
        dict->addParameter(ParameterDef("metrics_mode", 
            "The type of metrics to use, either 'relative' to the screen, 'pixels' or 'relative_aspect_adjusted'."
            , ParameterType::STRING),
            &msMetricsModeCmd);
        dict->addParameter(ParameterDef("horz_align", 
            "The horizontal alignment, 'left', 'right' or 'center'."
            , ParameterType::STRING),
            &msHorizontalAlignCmd);
        dict->addParameter(ParameterDef("vert_align", 
            "The vertical alignment, 'top', 'bottom' or 'center'."
            , ParameterType::STRING),
            &msVerticalAlignCmd);
        dict->addParameter(ParameterDef("visible", 
            "Initial visibility of element, either 'true' or 'false' (default true)."
            , ParameterType::STRING),
            &msVisibleCmd);
    }
    //-----------------------------------------------------------------------
    void OverlayElement::setCaption( std::string_view caption )
    {
        mCaption = caption;
        _positionsOutOfDate();
    }
    //-----------------------------------------------------------------------
    void OverlayElement::setColour(const ColourValue& col)
    {
        mColour = col;
    }
    //-----------------------------------------------------------------------
    auto OverlayElement::getColour() const noexcept -> const ColourValue&
    {
        return mColour;
    }
    //-----------------------------------------------------------------------
    void OverlayElement::setMetricsMode(GuiMetricsMode gmm)
    {
        switch (gmm)
        {
        case GuiMetricsMode::PIXELS :
            {
                Real vpWidth, vpHeight;
                OverlayManager& oMgr = OverlayManager::getSingleton();
                vpWidth = (Real) (oMgr.getViewportWidth());
                vpHeight = (Real) (oMgr.getViewportHeight());

                // cope with temporarily zero dimensions, avoid divide by zero
                vpWidth = vpWidth == 0.0f? 1.0f : vpWidth;
                vpHeight = vpHeight == 0.0f? 1.0f : vpHeight;

                mPixelScaleX = 1.0f / vpWidth;
                mPixelScaleY = 1.0f / vpHeight;

                if (mMetricsMode == GuiMetricsMode::RELATIVE)
                {
                    mPixelLeft = mLeft;
                    mPixelTop = mTop;
                    mPixelWidth = mWidth;
                    mPixelHeight = mHeight;
                }
            }
            break;

        case GuiMetricsMode::RELATIVE_ASPECT_ADJUSTED :
            {
                Real vpWidth, vpHeight;
                OverlayManager& oMgr = OverlayManager::getSingleton();
                vpWidth = (Real) (oMgr.getViewportWidth());
                vpHeight = (Real) (oMgr.getViewportHeight());

                mPixelScaleX = 1.0f / (10000.0f * (vpWidth / vpHeight));
                mPixelScaleY = 1.0f /  10000.0f;

                if (mMetricsMode == GuiMetricsMode::RELATIVE)
                {
                    mPixelLeft = mLeft;
                    mPixelTop = mTop;
                    mPixelWidth = mWidth;
                    mPixelHeight = mHeight;
                }
            }
            break;

        default:
        case GuiMetricsMode::RELATIVE :
            mPixelScaleX = 1.0;
            mPixelScaleY = 1.0;
            mPixelLeft = mLeft;
            mPixelTop = mTop;
            mPixelWidth = mWidth;
            mPixelHeight = mHeight;
            break;
        }

        mLeft = mPixelLeft * mPixelScaleX;
        mTop = mPixelTop * mPixelScaleY;
        mWidth = mPixelWidth * mPixelScaleX;
        mHeight = mPixelHeight * mPixelScaleY;

        mMetricsMode = gmm;
        mDerivedOutOfDate = true;
        _positionsOutOfDate();
    }
    //-----------------------------------------------------------------------
    void OverlayElement::setHorizontalAlignment(GuiHorizontalAlignment gha)
    {
        mHorzAlign = gha;
        _positionsOutOfDate();
    }
    //-----------------------------------------------------------------------
    void OverlayElement::setVerticalAlignment(GuiVerticalAlignment gva)
    {
        mVertAlign = gva;
        _positionsOutOfDate();
    }
    //-----------------------------------------------------------------------    
    auto OverlayElement::contains(Real x, Real y) const -> bool
    {
        return x >= mClippingRegion.left && x <= mClippingRegion.right && y >= mClippingRegion.top && y <= mClippingRegion.bottom;
    }
    //-----------------------------------------------------------------------
    auto OverlayElement::findElementAt(Real x, Real y) -> OverlayElement*       // relative to parent
    {
        OverlayElement* ret = nullptr;
        if (contains(x , y ))
        {
            ret = this;
        }
        return ret;
    }
    //-----------------------------------------------------------------------
    auto OverlayElement::clone(std::string_view instanceName) -> OverlayElement*
    {
        OverlayElement* newElement;

        newElement = OverlayManager::getSingleton().createOverlayElement(
            getTypeName(),
            ::std::format("{}/{}", instanceName , mName));
        copyParametersTo(newElement);

        return newElement;
    }
    //-----------------------------------------------------------------------

}
