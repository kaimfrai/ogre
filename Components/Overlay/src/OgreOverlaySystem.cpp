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

module Ogre.Components.Overlay:System.Obj;

import :BorderPanelOverlayElement;
import :ElementFactory;
import :FontManager;
import :Manager;
import :PanelOverlayElement;
import :ProfileSessionListener;
import :System;
import :TextAreaOverlayElement;

import Ogre.Core;

import <cstddef>;
import <string>;

namespace Ogre {
    class OverlayElement;

    //---------------------------------------------------------------------
    /** Factory for creating PanelOverlayElement instances. */
    class PanelOverlayElementFactory: public OverlayElementFactory
    {
    public:
        auto createOverlayElement(const String& instanceName) -> OverlayElement* override
        {
            return new PanelOverlayElement(instanceName);
        }
        [[nodiscard]]
        auto getTypeName() const -> const String& override
        {
            static String name = "Panel";
            return name;
        }
    };

    /** Factory for creating BorderPanelOverlayElement instances. */
    class BorderPanelOverlayElementFactory: public OverlayElementFactory
    {
    public:
        auto createOverlayElement(const String& instanceName) -> OverlayElement* override
        {
            return new BorderPanelOverlayElement(instanceName);
        }
        [[nodiscard]]
        auto getTypeName() const -> const String& override
        {
            static String name = "BorderPanel";
            return name;
        }
    };

    /** Factory for creating TextAreaOverlayElement instances. */
    class TextAreaOverlayElementFactory: public OverlayElementFactory
    {
    public:
        auto createOverlayElement(const String& instanceName) -> OverlayElement* override
        {
            return new TextAreaOverlayElement(instanceName);
        }
        [[nodiscard]]
        auto getTypeName() const -> const String& override
        {
            static String name = "TextArea";
            return name;
        }
    };

    template<> OverlaySystem *Singleton<OverlaySystem>::msSingleton = nullptr;
    auto OverlaySystem::getSingletonPtr() -> OverlaySystem*
    {
        return msSingleton;
    }
    auto OverlaySystem::getSingleton() -> OverlaySystem&
    {
        assert( msSingleton );  return ( *msSingleton );
    }
    //---------------------------------------------------------------------
    OverlaySystem::OverlaySystem()
    {
        RenderSystem::setSharedListener(this);

        mOverlayManager = new Ogre::OverlayManager();
        mOverlayManager->addOverlayElementFactory(new Ogre::PanelOverlayElementFactory());

        mOverlayManager->addOverlayElementFactory(new Ogre::BorderPanelOverlayElementFactory());

        mOverlayManager->addOverlayElementFactory(new Ogre::TextAreaOverlayElementFactory());

        mFontManager = new FontManager();
        if (auto prof = Profiler::getSingletonPtr())
        {
            mProfileListener = new Ogre::OverlayProfileSessionListener();
            prof->addListener(mProfileListener);
        }
    }
    //---------------------------------------------------------------------
    OverlaySystem::~OverlaySystem()
    {
        if(RenderSystem::getSharedListener() == this)
            RenderSystem::setSharedListener(nullptr);

        if (auto prof = Profiler::getSingletonPtr())
        {
            prof->removeListener(mProfileListener);
            delete mProfileListener;
        }

        delete mOverlayManager;
        delete mFontManager;
    }
    //---------------------------------------------------------------------
    void OverlaySystem::renderQueueStarted(uint8 queueGroupId, const String& invocation, 
            bool& skipThisInvocation)
    {
        if(queueGroupId == Ogre::RENDER_QUEUE_OVERLAY)
        {
            Ogre::Viewport* vp = Ogre::Root::getSingletonPtr()->getRenderSystem()->_getViewport();
            if(vp != nullptr)
            {
                Ogre::SceneManager* sceneMgr = vp->getCamera()->getSceneManager();
                if (vp->getOverlaysEnabled() && sceneMgr->_getCurrentRenderStage() != Ogre::SceneManager::IRS_RENDER_TO_TEXTURE)
                {
                    OverlayManager::getSingleton()._queueOverlaysForRendering(vp->getCamera(), sceneMgr->getRenderQueue(), vp);
                }
            }
        }
    }
    //---------------------------------------------------------------------
	void OverlaySystem::eventOccurred(const String& eventName, const NameValuePairList* parameters)
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
