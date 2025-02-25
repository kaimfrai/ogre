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

#include <cassert>
#include <cstddef>

module Ogre.Components.Overlay;

import :Container;
import :Element;

import Ogre.Core;

import <memory>;
import <string>;
import <utility>;

namespace Ogre {

    //---------------------------------------------------------------------
    Overlay::Overlay(std::string_view name) :
        mName(name),
        mRotate{0.0f}

    {
        mRootNode = ::std::make_unique<SceneNode>(nullptr);

    }
    //---------------------------------------------------------------------
    Overlay::~Overlay()
    {
        // remove children

        for (auto & m2DElement : m2DElements)
        {
            m2DElement->_notifyParent(nullptr, nullptr);
        }
    }
    //---------------------------------------------------------------------
    auto Overlay::getName() const noexcept -> std::string_view
    {
        return mName;
    }
    //---------------------------------------------------------------------
    void Overlay::assignZOrders()
    {
        auto zorder = static_cast<ushort>(mZOrder * 100.0f);

        // Notify attached 2D elements
        for (auto const& i : m2DElements)
        {
            zorder = i->_notifyZOrder(zorder);
        }
    }
    //---------------------------------------------------------------------
    void Overlay::setZOrder(ushort zorder)
    {
        // Limit to 650 since this is multiplied by 100 to pad out for containers
        assert (zorder <= 650 && "Overlay Z-order cannot be greater than 650!");

        mZOrder = zorder;

        assignZOrders();
    }
    //---------------------------------------------------------------------
    auto Overlay::getZOrder() const noexcept -> ushort
    {
        return (ushort)mZOrder;
    }
    //---------------------------------------------------------------------
    auto Overlay::isVisible() const noexcept -> bool
    {
        return mVisible;
    }
    //---------------------------------------------------------------------
    void Overlay::show()
    {
        mVisible = true;
        if (!mInitialised)
        {
            initialise();
        }
    }
    //---------------------------------------------------------------------
    void Overlay::hide()
    {
        mVisible = false;
    }
    //---------------------------------------------------------------------
    void Overlay::setVisible(bool visible)
    {
        mVisible = visible;
    }
    //---------------------------------------------------------------------
    void Overlay::initialise()
    {
        for (auto const& i : m2DElements)
        {
            i->initialise();
        }
        mInitialised = true;
    }
    //---------------------------------------------------------------------
    void Overlay::add2D(OverlayContainer* cont)
    {
        m2DElements.push_back(cont);
        // Notify parent
        cont->_notifyParent(nullptr, this);

        assignZOrders();

        Matrix4 xform;
        _getWorldTransforms(&xform);
        cont->_notifyWorldTransforms(xform);
    }
    //---------------------------------------------------------------------
    void Overlay::remove2D(OverlayContainer* cont)
    {
        m2DElements.remove(cont);
        cont->_notifyParent(nullptr, nullptr);
        assignZOrders();
    }
    //---------------------------------------------------------------------
    void Overlay::add3D(SceneNode* node)
    {
        mRootNode->addChild(node);
    }
    //---------------------------------------------------------------------
    void Overlay::remove3D(SceneNode* node)
    {
        mRootNode->removeChild(node);
    }
    //---------------------------------------------------------------------
    void Overlay::clear()
    {
        mRootNode->removeAllChildren();
        m2DElements.clear();
        // Note no deallocation, memory handled by OverlayManager & SceneManager
    }
    //---------------------------------------------------------------------
    void Overlay::setScroll(Real x, Real y)
    {
        mScrollX = x;
        mScrollY = y;
        mTransformOutOfDate = true;
        mTransformUpdated = true;
    }
    //---------------------------------------------------------------------
    auto Overlay::getScrollX() const -> Real
    {
        return mScrollX;
    }
    //---------------------------------------------------------------------
    auto Overlay::getScrollY() const -> Real
    {
        return mScrollY;
    }
      //---------------------------------------------------------------------
    auto Overlay::getChild(std::string_view name) -> OverlayContainer*
    {
        for (auto const& i : m2DElements)
        {
            if (i->getName() == name)
            {
                return i;

            }
        }
        return nullptr;
    }
  //---------------------------------------------------------------------
    void Overlay::scroll(Real xoff, Real yoff)
    {
        mScrollX += xoff;
        mScrollY += yoff;
        mTransformOutOfDate = true;
        mTransformUpdated = true;
    }
    //---------------------------------------------------------------------
    void Overlay::setRotate(const Radian& angle)
    {
        mRotate = angle;
        mTransformOutOfDate = true;
        mTransformUpdated = true;
    }
    //---------------------------------------------------------------------
    void Overlay::rotate(const Radian& angle)
    {
        setRotate(mRotate + angle);
    }
    //---------------------------------------------------------------------
    void Overlay::setScale(Real x, Real y)
    {
        mScaleX = x;
        mScaleY = y;
        mTransformOutOfDate = true;
        mTransformUpdated = true;
    }
    //---------------------------------------------------------------------
    auto Overlay::getScaleX() const -> Real
    {
        return mScaleX;
    }
    //---------------------------------------------------------------------
    auto Overlay::getScaleY() const -> Real
    {
        return mScaleY;
    }
    //---------------------------------------------------------------------
    void Overlay::_getWorldTransforms(Matrix4* xform) const
    {
        if (mTransformOutOfDate)
        {
            updateTransform();
        }
        *xform = mTransform;

    }
    //---------------------------------------------------------------------
    void Overlay::_findVisibleObjects(Camera* cam, RenderQueue* queue, Viewport* vp)
    {
        if (mVisible)
        {
            // Flag for update pixel-based GUIElements if viewport has changed dimensions
            bool tmpViewportDimensionsChanged = false;
            if (mLastViewportWidth != vp->getActualWidth() ||
                mLastViewportHeight != vp->getActualHeight())
            {
                tmpViewportDimensionsChanged = true;
                mLastViewportWidth = vp->getActualWidth();
                mLastViewportHeight = vp->getActualHeight();
            }

            if(tmpViewportDimensionsChanged)
            {
                for (auto const& i : m2DElements)
                {
                    i->_notifyViewport();
                }
            }

            // update elements
            if (mTransformUpdated)
            {
                Matrix4 xform;
                _getWorldTransforms(&xform);

                for (auto const& i : m2DElements)
                {
                    i->_notifyWorldTransforms(xform);
                }

                mTransformUpdated = false;
            }

            // Add 3D elements
            mRootNode->setPosition(cam->getDerivedPosition());
            mRootNode->setOrientation(cam->getDerivedOrientation());
            mRootNode->_update(true, false);
            // Set up the default queue group for the objects about to be added
            RenderQueueGroupID oldgrp = queue->getDefaultQueueGroup();
            ushort oldPriority = queue-> getDefaultRenderablePriority();
            queue->setDefaultQueueGroup(RenderQueueGroupID::OVERLAY);
            queue->setDefaultRenderablePriority(static_cast<ushort>((mZOrder*100)-1));
            mRootNode->_findVisibleObjects(cam, queue, nullptr, true, false);
            // Reset the group
            queue->setDefaultQueueGroup(oldgrp);
            queue->setDefaultRenderablePriority(oldPriority);
            // Add 2D elements
            for (auto const& i : m2DElements)
            {
                i->_update();

                i->_updateRenderQueue(queue);
            }
        }
    }
    //---------------------------------------------------------------------
    void Overlay::updateTransform() const
    {
        // Ordering:
        //    1. Scale
        //    2. Rotate
        //    3. Translate

        auto orientationRotation = Radian{0};

        Matrix3 rot3x3, scale3x3;
        rot3x3.FromEulerAnglesXYZ(Radian{0}, Radian{0}, mRotate + orientationRotation);
        scale3x3 = Matrix3::ZERO;
        scale3x3[0][0] = mScaleX;
        scale3x3[1][1] = mScaleY;
        scale3x3[2][2] = 1.0f;

        mTransform = Matrix4::IDENTITY;
        mTransform = rot3x3 * scale3x3;
        mTransform.setTrans(Vector3{mScrollX, mScrollY, 0});

        mTransformOutOfDate = false;
    }
    //---------------------------------------------------------------------
    auto Overlay::findElementAt(Real x, Real y) -> OverlayElement*
    {
        OverlayElement* ret = nullptr;
        int currZ = -1;
        for (auto const& i : m2DElements)
        {
            int z = i->getZOrder();
            if (z > currZ)
            {
                OverlayElement* elementFound = i->findElementAt(x,y);
                if(elementFound)
                {
                    currZ = elementFound->getZOrder();
                    ret = elementFound;
                }
            }
        }
        return ret;
    }

}
