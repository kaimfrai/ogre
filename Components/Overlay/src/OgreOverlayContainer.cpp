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

#include "OgreOverlayContainer.hpp"

#include <cstddef>
#include <utility>

#include "OgreException.hpp"
#include "OgreIteratorWrapper.hpp"
#include "OgreMath.hpp"
#include "OgreOverlay.hpp"
#include "OgreOverlayManager.hpp"

namespace Ogre {
struct Matrix4;
class RenderQueue;

    //---------------------------------------------------------------------
    OverlayContainer::OverlayContainer(std::string_view name)
        : OverlayElement(name)
        
    {
    }
    //---------------------------------------------------------------------
    OverlayContainer::~OverlayContainer()
    {
        // remove from parent overlay if root
        if (mOverlay && !mParent)
        {
            mOverlay->remove2D(this);
        }

        for (const auto& p : mChildren)
        {
            p.second->_notifyParent(nullptr, nullptr);
        }
    }
    //---------------------------------------------------------------------
    void OverlayContainer::addChild(OverlayElement* elem)
    {
        if (elem->isContainer())
        {
            addChildImpl(static_cast<OverlayContainer*>(elem));
        }
        else
        {
            addChildImpl(elem);
        }

    }
    //---------------------------------------------------------------------
    void OverlayContainer::addChildImpl(OverlayElement* elem)
    {
        auto const name = elem->getName();
        auto i = mChildren.find(name);
        if (i != mChildren.end())
        {
            OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM, ::std::format("Child with name {} already defined.", name), "OverlayContainer::addChild");
        }

        mChildren.emplace(name, elem);
        // tell child about parent & Z-order
        elem->_notifyParent(this, mOverlay);
        elem->_notifyZOrder(Math::uint16Cast(mZOrder + 1));
        elem->_notifyWorldTransforms(mXForm);
    }
    //---------------------------------------------------------------------
    void OverlayContainer::addChildImpl(OverlayContainer* cont)
    {
        // Add to main map first 
        // This will pick up duplicates
        OverlayElement* pElem = cont;
        addChildImpl(pElem);

        /*
        cont->_notifyParent(this, mOverlay);
        cont->_notifyZOrder(mZOrder + 1);
        cont->_notifyWorldTransforms(mXForm);

        // tell children of new container the current overlay
        ChildIterator it = cont->getChildIterator();
        while (it.hasMoreElements())
        {
            // Give children Z-order 1 higher than this
            GuiElement* pElemChild = it.getNext();
            pElemChild->_notifyParent(cont, mOverlay);
            pElemChild->_notifyZOrder(cont->getZOrder() + 1);
            pElemChild->_notifyWorldTransforms(mXForm);
        }
        */

        // Now add to specific map too
        mChildContainers.emplace(cont->getName(), cont);

    }
    //---------------------------------------------------------------------
    auto OverlayContainer::removeChild(std::string_view name) -> OverlayContainer::ChildMap::iterator
    {
        auto i = mChildren.find(name);
        if (i == mChildren.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("Child with name {} not found.", name), "OverlayContainer::removeChild");
        }

        OverlayElement* element = i->second;
        auto eraseIt = mChildren.erase(i);

        // Remove from container list (if found)
        auto j = mChildContainers.find(name);
        if (j != mChildContainers.end())
            mChildContainers.erase(j);

        element->_setParent(nullptr);

        return eraseIt;
    }
    //---------------------------------------------------------------------
    void OverlayContainer::_addChild(OverlayElement* elem)
    {
        if (elem->isContainer())
        {
            addChildImpl(static_cast<OverlayContainer*>(elem));
        }
        else
        {
            addChildImpl(elem);
        }
    }
    //---------------------------------------------------------------------
    auto OverlayContainer::_removeChild(std::string_view name) -> OverlayContainer::ChildMap::iterator
    {
        auto i = mChildren.find(name);
        if (i == mChildren.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("Child with name {} not found.", name), "OverlayContainer::removeChild");
        }

        OverlayElement* element = i->second;
        auto eraseIt = mChildren.erase(i);

        // Remove from container list (if found)
        auto j = mChildContainers.find(name);
        if (j != mChildContainers.end())
            mChildContainers.erase(j);

        element->_setParent(nullptr);

        return eraseIt;
    }
    //---------------------------------------------------------------------
    auto OverlayContainer::getChild(std::string_view name) -> OverlayElement*
    {
        auto i = mChildren.find(name);
        if (i == mChildren.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("Child with name {} not found.", name), "OverlayContainer::getChild");
        }

        return i->second;
    }

    //---------------------------------------------------------------------
    auto OverlayContainer::getChildContainerIterator() -> OverlayContainer::ChildContainerIterator
    {
        return {mChildContainers.begin(), mChildContainers.end()};
    }
    //---------------------------------------------------------------------
    void OverlayContainer::initialise()
    {
        for (auto const& coni : mChildContainers)
        {
            coni.second->initialise();
        }
        for (auto const& ci : mChildren)
        {
            ci.second->initialise();
        }
    }
    //---------------------------------------------------------------------
    void OverlayContainer::_positionsOutOfDate()
    {
        OverlayElement::_positionsOutOfDate();

        for (const auto& p : mChildren)
        {
            p.second->_positionsOutOfDate();
        }
    }

    //---------------------------------------------------------------------
    void OverlayContainer::_update()
    {
        // call superclass
        OverlayElement::_update();

        // Update children
        for (const auto& p : mChildren)
        {
            p.second->_update();
        }
    }
    //---------------------------------------------------------------------
    auto OverlayContainer::_notifyZOrder(ushort newZOrder) -> ushort
    {
        OverlayElement::_notifyZOrder(newZOrder);
        // One for us
        newZOrder++; 

        // Update children
        for (const auto& [key, value] : mChildren)
        {
            // Children "consume" Z-order values, so keep track of them
            newZOrder = value->_notifyZOrder(newZOrder);
        }

        return newZOrder;
    }
    //---------------------------------------------------------------------
    void OverlayContainer::_notifyWorldTransforms(const Matrix4& xform)
    {
        OverlayElement::_notifyWorldTransforms(xform);

        // Update children
        for (const auto& p : mChildren)
        {
            p.second->_notifyWorldTransforms(xform);
        }
    }
    //---------------------------------------------------------------------
    void OverlayContainer::_notifyViewport()
    {
        OverlayElement::_notifyViewport();

        // Update children
        for (const auto& p : mChildren)
        {
            p.second->_notifyViewport();
        }
    }
    //---------------------------------------------------------------------
    void OverlayContainer::_notifyParent(OverlayContainer* parent, Overlay* overlay)
    {
        OverlayElement::_notifyParent(parent, overlay);

        // Update children
        for (const auto& p : mChildren)
        {
            // Notify the children of the overlay 
            p.second->_notifyParent(this, overlay);
        }
    }

    //---------------------------------------------------------------------
    void OverlayContainer::_updateRenderQueue(RenderQueue* queue)
    {
        if (mVisible)
        {

            OverlayElement::_updateRenderQueue(queue);

            // Also add children
            for (const auto& p : mChildren)
            {
                // Give children Z-order 1 higher than this
                p.second->_updateRenderQueue(queue);
            }
        }

    }


    auto OverlayContainer::findElementAt(Real x, Real y) -> OverlayElement*         // relative to parent
    {

        OverlayElement* ret = nullptr;

        int currZ = -1;

        if (mVisible)
        {
            ret = OverlayElement::findElementAt(x,y);   //default to the current container if no others are found
            if (ret && mChildrenProcessEvents)
            {
                for (auto const& [key, currentOverlayElement] : mChildren)
                {
                    if (currentOverlayElement->isVisible() && currentOverlayElement->isEnabled())
                    {
                        int z = currentOverlayElement->getZOrder();
                        if (z > currZ)
                        {
                            OverlayElement* elementFound = currentOverlayElement->findElementAt(x ,y );
                            if (elementFound)
                            {
                                currZ = z;
                                ret = elementFound;
                            }
                        }
                    }
                }
            }
        }
        return ret;
    }

    void OverlayContainer::copyFromTemplate(OverlayElement* templateOverlay)
    {
        OverlayElement::copyFromTemplate(templateOverlay);

            if (templateOverlay->isContainer() && isContainer())
            {
             for (auto const& [key, oldChildElement] : static_cast<OverlayContainer*>(templateOverlay)->getChildren())
             {
                 if (oldChildElement->isCloneable())
                 {
                     OverlayElement* newChildElement = 
                         OverlayManager::getSingleton().createOverlayElement(
                            oldChildElement->getTypeName(), 
                            mName+::std::format("/{}", oldChildElement->getName()));
                     newChildElement->copyFromTemplate(oldChildElement);
                     addChild(newChildElement);
                 }
             }
        }
    }

    auto OverlayContainer::clone(std::string_view instanceName) -> OverlayElement*
    {
        OverlayContainer *newContainer;

        newContainer = static_cast<OverlayContainer*>(OverlayElement::clone(instanceName));

        for (auto const& [key, oldChildElement] : mChildren)
        {
            if (oldChildElement->isCloneable())
            {
                OverlayElement* newChildElement = oldChildElement->clone(instanceName);
                newContainer->_addChild(newChildElement);
            }
        }

        return newContainer;
    }

}

