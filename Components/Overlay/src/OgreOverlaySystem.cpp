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

import :BorderPanelOverlayElement;
import :ElementFactory;
import :FontManager;
import :Manager;
import :PanelOverlayElement;
import :System;
import :TextAreaOverlayElement;

import Ogre.Core;

import <string>;

namespace Ogre {
class OverlayElement;

    //---------------------------------------------------------------------
    /** Factory for creating PanelOverlayElement instances. */
    class PanelOverlayElementFactory: public OverlayElementFactory
    {
    public:
        auto createOverlayElement(std::string_view instanceName) -> OverlayElement* override
        {
            return new PanelOverlayElement(instanceName);
        }
        [[nodiscard]] auto getTypeName() const noexcept -> std::string_view override
        {
            static std::string_view const constexpr name = "Panel";
            return name;
        }
    };

    /** Factory for creating BorderPanelOverlayElement instances. */
    class BorderPanelOverlayElementFactory: public OverlayElementFactory
    {
    public:
        auto createOverlayElement(std::string_view instanceName) -> OverlayElement* override
        {
            return new BorderPanelOverlayElement(instanceName);
        }
        [[nodiscard]] auto getTypeName() const noexcept -> std::string_view override
        {
            static std::string_view const constexpr name = "BorderPanel";
            return name;
        }
    };

    /** Factory for creating TextAreaOverlayElement instances. */
    class TextAreaOverlayElementFactory: public OverlayElementFactory
    {
    public:
        auto createOverlayElement(std::string_view instanceName) -> OverlayElement* override
        {
            return new TextAreaOverlayElement(instanceName);
        }
        [[nodiscard]] auto getTypeName() const noexcept -> std::string_view override
        {
            static std::string_view const constexpr name = "TextArea";
            return name;
        }
    };

    auto OverlaySystem::getSingletonPtr() noexcept -> OverlaySystem*
    {
        return msSingleton;
    }
    auto OverlaySystem::getSingleton() noexcept -> OverlaySystem&
    {
        assert( msSingleton );  return ( *msSingleton );
    }
    //---------------------------------------------------------------------
    OverlaySystem::OverlaySystem()
    {
        RenderSystem::setSharedListener(this);

        mOverlayManager = ::std::make_unique<Ogre::OverlayManager>();
        mOverlayManager->addOverlayElementFactory(new Ogre::PanelOverlayElementFactory());

        mOverlayManager->addOverlayElementFactory(new Ogre::BorderPanelOverlayElementFactory());

        mOverlayManager->addOverlayElementFactory(new Ogre::TextAreaOverlayElementFactory());

        mFontManager = ::std::make_unique<FontManager>();
    }
    //---------------------------------------------------------------------
    OverlaySystem::~OverlaySystem()
    {
        if(RenderSystem::getSharedListener() == this)
            RenderSystem::setSharedListener(nullptr);
    }
    //---------------------------------------------------------------------
    void OverlaySystem::renderQueueStarted(Ogre::RenderQueueGroupID queueGroupId, std::string_view invocation,
            bool& skipThisInvocation)
    {
        if(queueGroupId == Ogre::RenderQueueGroupID::OVERLAY)
        {
            Ogre::Viewport* vp = Ogre::Root::getSingletonPtr()->getRenderSystem()->_getViewport();
            if(vp != nullptr)
            {
                Ogre::SceneManager* sceneMgr = vp->getCamera()->getSceneManager();
                if (vp->getOverlaysEnabled() && sceneMgr->_getCurrentRenderStage() != Ogre::SceneManager::IlluminationRenderStage::RENDER_TO_TEXTURE)
                {
                    OverlayManager::getSingleton()._queueOverlaysForRendering(vp->getCamera(), sceneMgr->getRenderQueue(), vp);
                }
            }
        }
    }
    //---------------------------------------------------------------------
	void OverlaySystem::eventOccurred(std::string_view eventName, const NameValuePairList* parameters)
	{
		if(eventName == "DeviceLost")
		{
			mOverlayManager->_releaseManualHardwareResources();
		}
		else if(eventName == "DeviceRestored")
		{
			mOverlayManager->_restoreManualHardwareResources();
		}
	}
	//---------------------------------------------------------------------
}
