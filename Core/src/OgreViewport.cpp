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
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iosfwd>

#include "OgreCamera.hpp"
#include "OgreException.hpp"
#include "OgreLog.hpp"
#include "OgreLogManager.hpp"
#include "OgreMaterialManager.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderTarget.hpp"
#include "OgreRoot.hpp"
#include "OgreVector.hpp"
#include "OgreViewport.hpp"

namespace Ogre {
    OrientationMode constinit Viewport::mDefaultOrientationMode = OrientationMode::DEGREE_0;
    //---------------------------------------------------------------------
    Viewport::Viewport(Camera* cam, RenderTarget* target, Real left, Real top, Real width, Real height, int ZOrder)
        : mCamera(cam)
        , mTarget(target)
        , mRelLeft(left)
        , mRelTop(top)
        , mRelWidth(width)
        , mRelHeight(height)
        // Actual dimensions will update later
        , mZOrder(ZOrder)
        , mBackColour(ColourValue::Black)
        , 
         mClearBuffers(FrameBufferType::COLOUR | FrameBufferType::DEPTH)
        , 
         mMaterialSchemeName(MaterialManager::DEFAULT_SCHEME_NAME)
         
    {           
        LogManager::getSingleton().stream(LogMessageLevel::Trivial)
            << "Creating viewport on target '" << target->getName() << "'"
            << ", rendering from camera '" << (cam != nullptr ? cam->getName() : "NULL") << "'"
            << ", relative dimensions " << std::ios::fixed << std::setprecision(2) 
            << "L: " << left << " T: " << top << " W: " << width << " H: " << height
            << " Z-order: " << ZOrder;

        // Set the default orientation mode
        mOrientationMode = mDefaultOrientationMode;
            
        // Set the default material scheme
        RenderSystem* rs = Root::getSingleton().getRenderSystem();
        mMaterialSchemeName = rs->_getDefaultViewportMaterialScheme();
        
        // Calculate actual dimensions
        _updateDimensions();

        // notify camera
        if(cam) cam->_notifyViewport(this);
    }
    //---------------------------------------------------------------------
    Viewport::~Viewport()
    {
        ListenerList listenersCopy;
        std::swap(mListeners, listenersCopy);
        for (auto & i : listenersCopy)
        {
            i->viewportDestroyed(this);
        }

        RenderSystem* rs = Root::getSingleton().getRenderSystem();
        if ((rs) && (rs->_getViewport() == this))
        {
            rs->_setViewport(nullptr);
        }
    }
    //---------------------------------------------------------------------
    auto Viewport::_isUpdated() const -> bool
    {
        return mUpdated;
    }
    //---------------------------------------------------------------------
    void Viewport::_clearUpdatedFlag()
    {
        mUpdated = false;
    }
    //---------------------------------------------------------------------
    void Viewport::_updateDimensions()
    {
        Real height = (Real) mTarget->getHeight();
        Real width = (Real) mTarget->getWidth();

        mActLeft = (int) (mRelLeft * width);
        mActTop = (int) (mRelTop * height);
        mActWidth = (int) (mRelWidth * width);
        mActHeight = (int) (mRelHeight * height);

        // This will check if the cameras getAutoAspectRatio() property is set.
        // If it's true its aspect ratio is fit to the current viewport
        // If it's false the camera remains unchanged.
        // This allows cameras to be used to render to many viewports,
        // which can have their own dimensions and aspect ratios.

        if (mCamera) 
        {
            if (mCamera->getAutoAspectRatio())
                mCamera->setAspectRatio((Real) mActWidth / (Real) mActHeight);
        }

        LogManager::getSingleton().stream(LogMessageLevel::Trivial)
            << "Viewport for camera '" << (mCamera != nullptr ? mCamera->getName() : "NULL") << "'"
            << ", actual dimensions "   << std::ios::fixed << std::setprecision(2) 
            << "L: " << mActLeft << " T: " << mActTop << " W: " << mActWidth << " H: " << mActHeight;

         mUpdated = true;

        for (auto & mListener : mListeners)
        {
            mListener->viewportDimensionsChanged(this);
        }
    }
    //---------------------------------------------------------------------
    auto Viewport::getZOrder() const noexcept -> int
    {
        return mZOrder;
    }
    //---------------------------------------------------------------------
    auto Viewport::getTarget() const noexcept -> RenderTarget*
    {
        return mTarget;
    }
    //---------------------------------------------------------------------
    auto Viewport::getCamera() const noexcept -> Camera*
    {
        return mCamera;
    }
    //---------------------------------------------------------------------
    auto Viewport::getLeft() const -> Real
    {
        return mRelLeft;
    }
    //---------------------------------------------------------------------
    auto Viewport::getTop() const -> Real
    {
        return mRelTop;
    }
    //---------------------------------------------------------------------
    auto Viewport::getWidth() const -> Real
    {
        return mRelWidth;
    }
    //---------------------------------------------------------------------
    auto Viewport::getHeight() const -> Real
    {
        return mRelHeight;
    }
    //---------------------------------------------------------------------
    auto Viewport::getActualLeft() const noexcept -> int
    {
        return mActLeft;
    }
    //---------------------------------------------------------------------
    auto Viewport::getActualTop() const noexcept -> int
    {
        return mActTop;
    }
    //---------------------------------------------------------------------
    auto Viewport::getActualWidth() const noexcept -> int
    {
        return mActWidth;
    }
    //---------------------------------------------------------------------
    auto Viewport::getActualHeight() const noexcept -> int
    {
        return mActHeight;
    }
    //---------------------------------------------------------------------
    void Viewport::setDimensions(Real left, Real top, Real width, Real height)
    {
        mRelLeft = left;
        mRelTop = top;
        mRelWidth = width;
        mRelHeight = height;
        _updateDimensions();
    }
    //---------------------------------------------------------------------
    void Viewport::update()
    {
        if (mCamera)
        {
            if (mCamera->getViewport() != this)
                mCamera->_notifyViewport(this);

            // Tell Camera to render into me
            mCamera->_renderScene(this);
        }
    }
    //---------------------------------------------------------------------
    void Viewport::setOrientationMode(OrientationMode orientationMode, bool setDefault)
    {
        OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED,
                    "Setting Viewport orientation mode is not supported");
    }
    //---------------------------------------------------------------------
    auto Viewport::getOrientationMode() const -> OrientationMode
    {
        OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED,
                    "Getting Viewport orientation mode is not supported");
    }
    //---------------------------------------------------------------------
    void Viewport::setDefaultOrientationMode(OrientationMode orientationMode)
    {
        OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED,
                    "Setting default Viewport orientation mode is not supported");
    }
    //---------------------------------------------------------------------
    auto Viewport::getDefaultOrientationMode() -> OrientationMode
    {
        OGRE_EXCEPT(ExceptionCodes::NOT_IMPLEMENTED,
                    "Getting default Viewport orientation mode is not supported");
    }
    //---------------------------------------------------------------------
    void Viewport::setBackgroundColour(const ColourValue& colour)
    {
        mBackColour = colour;
    }
    //---------------------------------------------------------------------
    auto Viewport::getBackgroundColour() const noexcept -> const ColourValue&
    {
        return mBackColour;
    }
    //---------------------------------------------------------------------
    void Viewport::setDepthClear( float depth )
    {
        mDepthClearValue = depth;
    }
    //---------------------------------------------------------------------
    auto Viewport::getDepthClear() const noexcept -> float
    {
        return mDepthClearValue;
    }
    //---------------------------------------------------------------------
    void Viewport::setClearEveryFrame(bool inClear, FrameBufferType inBuffers)
    {
        mClearEveryFrame = inClear;
        mClearBuffers = inClear ? inBuffers : FrameBufferType{};
    }
    //---------------------------------------------------------------------
    auto Viewport::getClearEveryFrame() const noexcept -> bool
    {
        return mClearEveryFrame;
    }
    //---------------------------------------------------------------------
    auto Viewport::getClearBuffers() const noexcept -> FrameBufferType
    {
        return mClearBuffers;
    }
    //---------------------------------------------------------------------
    void Viewport::clear(FrameBufferType buffers, const ColourValue& col, float depth, uint16 stencil)
    {
        RenderSystem* rs = Root::getSingleton().getRenderSystem();
        if (rs)
        {
            Viewport* currentvp = rs->_getViewport();
            if (currentvp && currentvp == this)
                rs->clearFrameBuffer(buffers, col, depth, stencil);
            else
            {
                rs->_setViewport(this);
                rs->clearFrameBuffer(buffers, col, depth, stencil);
                rs->_setViewport(currentvp);
            }
        }
    }
    //---------------------------------------------------------------------
    auto Viewport::getActualDimensions() const -> Rect
    {
        return { mActLeft, mActTop, mActLeft + mActWidth, mActTop + mActHeight };
    }

    //---------------------------------------------------------------------
    auto Viewport::_getNumRenderedFaces() const -> unsigned int
    {
        return mCamera ? mCamera->_getNumRenderedFaces() : 0;
    }
    //---------------------------------------------------------------------
    auto Viewport::_getNumRenderedBatches() const -> unsigned int
    {
        return mCamera ? mCamera->_getNumRenderedBatches() : 0;
    }
    //---------------------------------------------------------------------
    void Viewport::setCamera(Camera* cam)
    {
        if (cam != nullptr && mCamera != nullptr && mCamera->getViewport() == this)
        {
                mCamera->_notifyViewport(nullptr);
        }

        mCamera = cam;
        if (cam)
        {
            // update aspect ratio of new camera if needed.
            if (cam->getAutoAspectRatio())
            {
                cam->setAspectRatio((Real) mActWidth / (Real) mActHeight);
            }

            cam->_notifyViewport(this);
        }

        for (auto & mListener : mListeners)
        {
            mListener->viewportCameraChanged(this);
        }
    }
    //---------------------------------------------------------------------
    void Viewport::setAutoUpdated(bool inAutoUpdated)
    {
        mIsAutoUpdated = inAutoUpdated;
    }
    //---------------------------------------------------------------------
    auto Viewport::isAutoUpdated() const noexcept -> bool
    {
        return mIsAutoUpdated;
    }
    //---------------------------------------------------------------------
    void Viewport::setOverlaysEnabled(bool enabled)
    {
        mShowOverlays = enabled;
    }
    //---------------------------------------------------------------------
    auto Viewport::getOverlaysEnabled() const noexcept -> bool
    {
        return mShowOverlays;
    }
    //---------------------------------------------------------------------
    void Viewport::setSkiesEnabled(bool enabled)
    {
        mShowSkies = enabled;
    }
    //---------------------------------------------------------------------
    auto Viewport::getSkiesEnabled() const noexcept -> bool
    {
        return mShowSkies;
    }
    //---------------------------------------------------------------------
    void Viewport::setShadowsEnabled(bool enabled)
    {
        mShowShadows = enabled;
    }
    //---------------------------------------------------------------------
    auto Viewport::getShadowsEnabled() const noexcept -> bool
    {
        return mShowShadows;
    }
    //-----------------------------------------------------------------------
    void Viewport::pointOrientedToScreen(const Vector2 &v, int orientationMode, Vector2 &outv)
    {
        pointOrientedToScreen(v.x, v.y, orientationMode, outv.x, outv.y);
    }
    //-----------------------------------------------------------------------
    void Viewport::pointOrientedToScreen(Real orientedX, Real orientedY, int orientationMode,
                                         Real &screenX, Real &screenY)
    {
        Real orX = orientedX;
        Real orY = orientedY;
        switch (orientationMode)
        {
        case 1:
            screenX = orY;
            screenY = Real(1.0) - orX;
            break;
        case 2:
            screenX = Real(1.0) - orX;
            screenY = Real(1.0) - orY;
            break;
        case 3:
            screenX = Real(1.0) - orY;
            screenY = orX;
            break;
        default:
            screenX = orX;
            screenY = orY;
            break;
        }
    }
    //-----------------------------------------------------------------------
    void Viewport::addListener(Listener* l)
    {
        if (std::ranges::find(mListeners, l) == mListeners.end())
            mListeners.push_back(l);
    }
    //-----------------------------------------------------------------------
    void Viewport::removeListener(Listener* l)
    {
        auto i = std::ranges::find(mListeners, l);
        if (i != mListeners.end())
            mListeners.erase(i);
    }
	//-----------------------------------------------------------------------
	void Viewport::setDrawBuffer(ColourBufferType colourBuffer) 
	{
		mColourBuffer = colourBuffer;
	}
	//-----------------------------------------------------------------------
	auto Viewport::getDrawBuffer() const -> ColourBufferType
	{
		return mColourBuffer;
	}
	//-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void Viewport::Listener::viewportCameraChanged(Viewport*)
    {
    }

    //-----------------------------------------------------------------------
    void Viewport::Listener::viewportDimensionsChanged(Viewport*)
    {
    }

    //-----------------------------------------------------------------------
    void Viewport::Listener::viewportDestroyed(Viewport*)
    {
    }

}
