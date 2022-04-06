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

#include "OgreOverlayManager.hpp"

#include <cassert>
#include <utility>

#include "OgreDataStream.hpp"
#include "OgreException.hpp"
#include "OgreIteratorWrapper.hpp"
#include "OgreLogManager.hpp"
#include "OgreOverlay.hpp"
#include "OgreOverlayContainer.hpp"
#include "OgreOverlayElement.hpp"
#include "OgreOverlayElementFactory.hpp"
#include "OgreOverlayTranslator.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreScriptTranslator.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreString.hpp"
#include "OgreViewport.hpp"

namespace Ogre {
class Camera;
class RenderQueue;

    //---------------------------------------------------------------------
    template<> OverlayManager *Singleton<OverlayManager>::msSingleton = nullptr;
    auto OverlayManager::getSingletonPtr() -> OverlayManager*
    {
        return msSingleton;
    }
    auto OverlayManager::getSingleton() -> OverlayManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //---------------------------------------------------------------------
    OverlayManager::OverlayManager() 
      
        
    {

        // Scripting is supported by this manager
        mScriptPatterns.push_back("*.overlay");
        ResourceGroupManager::getSingleton()._registerScriptLoader(this);
        mTranslatorManager.reset(new OverlayTranslatorManager());
    }
    //---------------------------------------------------------------------
    OverlayManager::~OverlayManager()
    {
        destroyAll();
        // Overlays notify OverlayElements of their destruction, so we destroy Overlays first
        destroyAllOverlayElements(false);
        destroyAllOverlayElements(true);

        for(auto i = mFactories.begin(); i != mFactories.end(); ++i)
        {
            delete i->second;
        }

        // Unregister with resource group manager
        ResourceGroupManager::getSingleton()._unregisterScriptLoader(this);
    }
    //---------------------------------------------------------------------
    void OverlayManager::_releaseManualHardwareResources()
    {
        for(auto i = mElements.begin(), i_end = mElements.end(); i != i_end; ++i)
            i->second->_releaseManualHardwareResources();
    }
    //---------------------------------------------------------------------
    void OverlayManager::_restoreManualHardwareResources()
    {
        for(auto i = mElements.begin(), i_end = mElements.end(); i != i_end; ++i)
            i->second->_restoreManualHardwareResources();
    }
    //---------------------------------------------------------------------
    auto OverlayManager::getScriptPatterns() const -> const StringVector&
    {
        return mScriptPatterns;
    }
    //---------------------------------------------------------------------
    auto OverlayManager::getLoadingOrder() const -> Real
    {
        // Load late
        return 1100.0f;
    }
    //---------------------------------------------------------------------
    auto OverlayManager::create(const String& name) -> Overlay*
    {
        Overlay* ret = nullptr;
        auto i = mOverlayMap.find(name);

        if (i == mOverlayMap.end())
        {
            ret = new Overlay(name);
            assert(ret && "Overlay creation failed");
            mOverlayMap[name] = ret;
        }
        else
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
                "Overlay with name '" + name + "' already exists!",
                "OverlayManager::create");
        }

        return ret;

    }
    //---------------------------------------------------------------------
    auto OverlayManager::getByName(const String& name) -> Overlay*
    {
        auto i = mOverlayMap.find(name);
        if (i == mOverlayMap.end())
        {
            return nullptr;
        }
        else
        {
            return i->second;
        }

    }

    void OverlayManager::addOverlay(Overlay* overlay)
    {
        bool succ = mOverlayMap.emplace(overlay->getName(), overlay).second;
        if(succ) return;
        OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM,
                    "Overlay with name '" + overlay->getName() + "' already exists!");
    }
    //---------------------------------------------------------------------
    void OverlayManager::destroy(const String& name)
    {
        auto i = mOverlayMap.find(name);
        if (i == mOverlayMap.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
                "Overlay with name '" + name + "' not found.",
                "OverlayManager::destroy");
        }
        else
        {
            delete i->second;
            mOverlayMap.erase(i);
        }
    }
    //---------------------------------------------------------------------
    void OverlayManager::destroy(Overlay* overlay)
    {
        for (auto i = mOverlayMap.begin();
            i != mOverlayMap.end(); ++i)
        {
            if (i->second == overlay)
            {
                delete i->second;
                mOverlayMap.erase(i);
                return;
            }
        }

        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
            "Overlay not found.",
            "OverlayManager::destroy");
    }
    //---------------------------------------------------------------------
    void OverlayManager::destroyAll()
    {
        for (auto i = mOverlayMap.begin();
            i != mOverlayMap.end(); ++i)
        {
            delete i->second;
        }
        mOverlayMap.clear();
    }
    //---------------------------------------------------------------------
    auto OverlayManager::getOverlayIterator() -> OverlayManager::OverlayMapIterator
    {
        return {mOverlayMap.begin(), mOverlayMap.end()};
    }
    void OverlayManager::parseScript(DataStreamPtr& stream, const String& groupName)
    {
        // skip scripts that were already loaded as we lack proper re-loading support
        if(!stream->getName().empty() && !mLoadedScripts.emplace(stream->getName()).second)
        {
            LogManager::getSingleton().logWarning(
                StringUtil::format("Skipping loading '%s' as it is already loaded", stream->getName().c_str()));
            return;
        }

        ScriptCompilerManager::getSingleton().parseScript(stream, groupName);
    }
    //---------------------------------------------------------------------
    void OverlayManager::_queueOverlaysForRendering(Camera* cam, 
        RenderQueue* pQueue, Viewport* vp)
    {
        bool orientationModeChanged = false;

        // Flag for update pixel-based GUIElements if viewport has changed dimensions
        if (mLastViewportWidth != int(vp->getActualWidth() / mPixelRatio) ||
            mLastViewportHeight != int(vp->getActualHeight() / mPixelRatio) || orientationModeChanged)
        {
            mLastViewportWidth = int(vp->getActualWidth() / mPixelRatio);
            mLastViewportHeight = int(vp->getActualHeight() / mPixelRatio);
        }

        OverlayMap::iterator i, iend;
        iend = mOverlayMap.end();
        for (i = mOverlayMap.begin(); i != iend; ++i)
        {
            Overlay* o = i->second;
            o->_findVisibleObjects(cam, pQueue, vp);
        }
    }
    //---------------------------------------------------------------------
    auto OverlayManager::getViewportHeight() const -> int
    {
        return mLastViewportHeight;
    }
    //---------------------------------------------------------------------
    auto OverlayManager::getViewportWidth() const -> int
    {
        return mLastViewportWidth;
    }
    //---------------------------------------------------------------------
    auto OverlayManager::getViewportAspectRatio() const -> Real
    {
        return (Real)mLastViewportWidth / (Real)mLastViewportHeight;
    }
    //---------------------------------------------------------------------
    auto OverlayManager::getViewportOrientationMode() const -> OrientationMode
    {
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
                    "Getting ViewPort orientation mode is not supported");
    }
    //---------------------------------------------------------------------
    auto OverlayManager::getPixelRatio() const -> float
    {
        return mPixelRatio;
    }
    //---------------------------------------------------------------------
    void OverlayManager::setPixelRatio(float ratio)
    {
        mPixelRatio = ratio;
    }
    //---------------------------------------------------------------------
    auto OverlayManager::createOverlayElementFromTemplate(const String& templateName, const String& typeName, const String& instanceName, bool) -> OverlayElement*
    {

        OverlayElement* newObj  = nullptr;

        if (templateName.empty())
        {
            newObj = createOverlayElement(typeName, instanceName);
        }
        else
        {
            // no template 
            OverlayElement* templateGui = getOverlayElement(templateName, true);

            String typeNameToCreate;
            if (typeName.empty())
            {
                typeNameToCreate = templateGui->getTypeName();
            }
            else
            {
                typeNameToCreate = typeName;
            }

            newObj = createOverlayElement(typeNameToCreate, instanceName);

            ((OverlayContainer*)newObj)->copyFromTemplate(templateGui);
        }

        return newObj;
    }


    //---------------------------------------------------------------------
    auto OverlayManager::cloneOverlayElementFromTemplate(const String& templateName, const String& instanceName) -> OverlayElement*
    {
        OverlayElement* templateGui = getOverlayElement(templateName, true);
        return templateGui->clone(instanceName);
    }

    //---------------------------------------------------------------------
    auto OverlayManager::createOverlayElement(const String& typeName, const String& instanceName, bool) -> OverlayElement*
    {
        // Check not duplicated
        auto ii = mElements.find(instanceName);
        if (ii != mElements.end())
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, "OverlayElement with name " + instanceName +
                " already exists.", "OverlayManager::createOverlayElement" );
        }
        OverlayElement* newElem = createOverlayElementFromFactory(typeName, instanceName);

        // Register
        mElements.emplace(instanceName, newElem);

        return newElem;


    }

    //---------------------------------------------------------------------
    auto OverlayManager::createOverlayElementFromFactory(const String& typeName, const String& instanceName) -> OverlayElement*
    {
        // Look up factory
        auto fi = mFactories.find(typeName);
        if (fi == mFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Cannot locate factory for element type " + typeName,
                "OverlayManager::createOverlayElement");
        }

        // create
        return fi->second->createOverlayElement(instanceName);
    }
    //---------------------------------------------------------------------
    auto OverlayManager::getOverlayElement(const String& name,bool) -> OverlayElement*
    {
        // Locate instance
        auto ii = mElements.find(name);
        if (ii == mElements.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "OverlayElement with name " + name + " not found.");
        }

        return ii->second;
    }
    //---------------------------------------------------------------------
    auto OverlayManager::hasOverlayElement(const String& name,bool) -> bool
    {
        auto ii = mElements.find(name);
        return ii != mElements.end();
    }

    //---------------------------------------------------------------------
    void OverlayManager::destroyOverlayElement(OverlayElement* pInstance,bool)
    {
        destroyOverlayElement(pInstance->getName());
    }
    //---------------------------------------------------------------------
    void OverlayManager::destroyOverlayElement(const String& instanceName,bool)
    {
        // Locate instance
        auto ii = mElements.find(instanceName);
        if (ii == mElements.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "OverlayElement with name " + instanceName +
                " not found.", "OverlayManager::destroyOverlayElement" );
        }
        // Look up factory
        const String& typeName = ii->second->getTypeName();
        auto fi = mFactories.find(typeName);
        if (fi == mFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Cannot locate factory for element type " + typeName,
                "OverlayManager::destroyOverlayElement");
        }

        fi->second->destroyOverlayElement(ii->second);
        mElements.erase(ii);
    }
    //---------------------------------------------------------------------
    void OverlayManager::destroyAllOverlayElements(bool)
    {
        ElementMap::iterator i;

        while ((i = mElements.begin()) != mElements.end())
        {
            OverlayElement* element = i->second;

            // Get factory to delete
            auto fi = mFactories.find(element->getTypeName());
            if (fi == mFactories.end())
            {
                OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Cannot locate factory for element " 
                    + element->getName(),
                    "OverlayManager::destroyAllOverlayElements");
            }

            // remove from parent, if any
            OverlayContainer* parent;
            if ((parent = element->getParent()) != nullptr)
            {
                parent->_removeChild(element->getName());
            }

            // children of containers will be auto-removed when container is destroyed.
            // destroy the element and remove it from the list
            fi->second->destroyOverlayElement(element);
            mElements.erase(i);
        }
    }
    //---------------------------------------------------------------------
    void OverlayManager::addOverlayElementFactory(OverlayElementFactory* elemFactory)
    {
        // Add / replace
        mFactories[elemFactory->getTypeName()] = elemFactory;

        LogManager::getSingleton().logMessage("OverlayElementFactory for type " + elemFactory->getTypeName()
            + " registered.");
    }
}

