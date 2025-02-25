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

module Ogre.Core;

import :Controller;
import :Exception;
import :Math;
import :RibbonTrail;
import :SceneNode;
import :StringConverter;
import :Vector;

import <algorithm>;
import <format>;
import <iterator>;
import <memory>;
import <string>;
import <utility>;

namespace Ogre
{
    namespace
    {
        /** Controller value for pass frame time to RibbonTrail
        */
        class TimeControllerValue : public ControllerValue<Real>
        {
        protected:
            RibbonTrail* mTrail;
        public:
            TimeControllerValue(RibbonTrail* r) { mTrail = r; }

            [[nodiscard]] auto getValue() const noexcept -> Real override { return 0; }// not a source 
            void setValue(Real value) override { mTrail->_timeUpdate(value); }
        };
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    RibbonTrail::RibbonTrail(std::string_view name, size_t maxElements, 
        size_t numberOfChains, bool useTextureCoords, bool useColours)
        :BillboardChain(name, maxElements, 0, useTextureCoords, useColours, true)
        
    {
        setTrailLength(100);
        setNumberOfChains(numberOfChains);
        mTimeControllerValue = ControllerValueRealPtr(new TimeControllerValue(this));

        // use V as varying texture coord, so we can use 1D textures to 'smear'
        setTextureCoordDirection(TexCoordDirection::V);


    }
    //-----------------------------------------------------------------------
    RibbonTrail::~RibbonTrail()
    {
        // Detach listeners
        for (auto & i : mNodeList)
        {
            i->setListener(nullptr);
        }

        if (mFadeController)
        {
            // destroy controller
            ControllerManager::getSingleton().destroyController(mFadeController);
        }

    }
    //-----------------------------------------------------------------------
    void RibbonTrail::addNode(Node* n)
    {
        if (mNodeList.size() == mChainCount)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, 
                ::std::format("{} cannot monitor any more nodes, chain count exceeded",
                    mName),
                "RibbonTrail::addNode");
        }
        if (n->getListener())
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, 
                ::std::format("{} cannot monitor node {} since it already has a listener.", mName, n->getName()),
                "RibbonTrail::addNode");
        }

        // get chain index
        size_t chainIndex = mFreeChains.back();
        mFreeChains.pop_back();
        mNodeToChainSegment.push_back(chainIndex);
        mNodeToSegMap[n] = chainIndex;

        // initialise the chain
        resetTrail(chainIndex, n);

        mNodeList.push_back(n);
        n->setListener(this);

    }
    //-----------------------------------------------------------------------
    auto RibbonTrail::getChainIndexForNode(const Node* n) -> size_t
    {
        auto i = mNodeToSegMap.find(n);
        if (i == mNodeToSegMap.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, 
                "This node is not being tracked", "RibbonTrail::getChainIndexForNode");
        }
        return i->second;
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::removeNode(const Node* n)
    {
        auto i = std::ranges::find(mNodeList, n);
        if (i != mNodeList.end())
        {
            // also get matching chain segment
            size_t index = std::distance(mNodeList.begin(), i);
            auto mi = mNodeToChainSegment.begin();
            std::advance(mi, index);
            size_t chainIndex = *mi;
            BillboardChain::clearChain(chainIndex);
            // mark as free now
            mFreeChains.push_back(chainIndex);
            (*i)->setListener(nullptr);
            mNodeList.erase(i);
            mNodeToChainSegment.erase(mi);
            mNodeToSegMap.erase(mNodeToSegMap.find(n));

        }
    }
    //-----------------------------------------------------------------------
    auto 
    RibbonTrail::getNodeIterator() const -> RibbonTrail::NodeIterator
    {
        return {mNodeList.begin(), mNodeList.end()};
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::setTrailLength(Real len)
    {
        mTrailLength = len;
        mElemLength = mTrailLength / mMaxElementsPerChain;
        mSquaredElemLength = mElemLength * mElemLength;
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::setMaxChainElements(size_t maxElements)
    {
        BillboardChain::setMaxChainElements(maxElements);
        mElemLength = mTrailLength / mMaxElementsPerChain;
        mSquaredElemLength = mElemLength * mElemLength;

        resetAllTrails();
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::setNumberOfChains(size_t numChains)
    {
        OgreAssert(numChains >= mNodeList.size(),
                   "Can't shrink the number of chains less than number of tracking nodes");
        size_t oldChains = getNumberOfChains();

        BillboardChain::setNumberOfChains(numChains);

        mInitialColour.resize(numChains, ColourValue::White);
        mDeltaColour.resize(numChains, ColourValue::ZERO);
        mInitialWidth.resize(numChains, 10);
        mDeltaWidth.resize(numChains, 0);

        if (oldChains > numChains)
        {
            // remove free chains
            for (auto i = mFreeChains.begin(); i != mFreeChains.end();)
            {
                if (*i >= numChains)
                    i = mFreeChains.erase(i);
                else
                    ++i;
            }
        }
        else if (oldChains < numChains)
        {
            // add new chains, at front to preserve previous ordering (pop_back)
            for (size_t i = oldChains; i < numChains; ++i)
                mFreeChains.insert(mFreeChains.begin(), i);
        }
        resetAllTrails();
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::clearChain(size_t chainIndex)
    {
        BillboardChain::clearChain(chainIndex);

        // Reset if we are tracking for this chain
        auto i = std::ranges::find(mNodeToChainSegment, chainIndex);
        if (i != mNodeToChainSegment.end())
        {
            size_t nodeIndex = std::distance(mNodeToChainSegment.begin(), i);
            resetTrail(*i, mNodeList[nodeIndex]);
        }
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::setInitialColour(size_t chainIndex, const ColourValue& col)
    {
        setInitialColour(chainIndex, col.r, col.g, col.b, col.a);
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::setInitialColour(size_t chainIndex, float r, float g, float b, float a)
    {
        mInitialColour.at(chainIndex) = ColourValue{r, g, b, a};
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::setInitialWidth(size_t chainIndex, Real width)
    {
        mInitialWidth.at(chainIndex) = width;
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::setColourChange(size_t chainIndex, const ColourValue& valuePerSecond)
    {
        setColourChange(chainIndex, 
            valuePerSecond.r, valuePerSecond.g, valuePerSecond.b, valuePerSecond.a);
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::setColourChange(size_t chainIndex, float r, float g, float b, float a)
    {
        mDeltaColour.at(chainIndex) = ColourValue{r, g, b, a};
        manageController();
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::setWidthChange(size_t chainIndex, Real widthDeltaPerSecond)
    {
        mDeltaWidth.at(chainIndex) = widthDeltaPerSecond;
        manageController();
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::manageController()
    {
        bool needController = false;
        for (size_t i = 0; i < mChainCount; ++i)
        {
            if (mDeltaWidth[i] != 0 || mDeltaColour[i] != ColourValue::ZERO)
            {
                needController = true;
                break;
            }
        }
        if (!mFadeController && needController)
        {
            // Set up fading via frame time controller
            ControllerManager& mgr = ControllerManager::getSingleton();
            mFadeController = mgr.createFrameTimePassthroughController(mTimeControllerValue);
        }
        else if (mFadeController && !needController)
        {
            // destroy controller
            ControllerManager::getSingleton().destroyController(mFadeController);
            mFadeController = nullptr;
        }

    }
    //-----------------------------------------------------------------------
    void RibbonTrail::nodeUpdated(const Node* node)
    {
        size_t chainIndex = getChainIndexForNode(node);
        updateTrail(chainIndex, node);
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::nodeDestroyed(const Node* node)
    {
        removeNode(node);

    }
    //-----------------------------------------------------------------------
    void RibbonTrail::updateTrail(size_t index, const Node* node)
    {
        // Repeat this entire process if chain is stretched beyond its natural length
        bool done = false;
        while (!done)
        {
            // Node has changed somehow, we're only interested in the derived position
            ChainSegment& seg = mChainSegmentList[index];
            Element& headElem = mChainElementList[seg.start + seg.head];
            size_t nextElemIdx = seg.head + 1;
            // wrap
            if (nextElemIdx == mMaxElementsPerChain)
                nextElemIdx = 0;
            Element& nextElem = mChainElementList[seg.start + nextElemIdx];

            // Vary the head elem, but bake new version if that exceeds element len
            Vector3 newPos = node->_getDerivedPosition();
            if (mParentNode)
            {
                // Transform position to ourself space
                newPos = mParentNode->convertWorldToLocalPosition(newPos);
            }
            Vector3 diff = newPos - nextElem.position;
            Real sqlen = diff.squaredLength();
            if (sqlen >= mSquaredElemLength)
            {
                // Move existing head to mElemLength
                Vector3 scaledDiff = diff * (mElemLength / Math::Sqrt(sqlen));
                headElem.position = nextElem.position + scaledDiff;
                // Add a new element to be the new head
                Element newElem( newPos, mInitialWidth[index], 0.0f,
                                 mInitialColour[index], node->_getDerivedOrientation() );
                addChainElement(index, newElem);
                // alter diff to represent new head size
                diff = newPos - headElem.position;
                // check whether another step is needed or not
                if (diff.squaredLength() <= mSquaredElemLength)   
                    done = true;

            }
            else
            {
                // Extend existing head
                headElem.position = newPos;
                done = true;
            }

            // Is this segment full?
            if ((seg.tail + 1) % mMaxElementsPerChain == seg.head)
            {
                // If so, shrink tail gradually to match head extension
                Element& tailElem = mChainElementList[seg.start + seg.tail];
                size_t preTailIdx;
                if (seg.tail == 0)
                    preTailIdx = mMaxElementsPerChain - 1;
                else
                    preTailIdx = seg.tail - 1;
                Element& preTailElem = mChainElementList[seg.start + preTailIdx];

                // Measure tail diff from pretail to tail
                Vector3 taildiff = tailElem.position - preTailElem.position;
                Real taillen = taildiff.length();
                if (taillen > 1e-06)
                {
                    Real tailsize = mElemLength - diff.length();
                    taildiff *= tailsize / taillen;
                    tailElem.position = preTailElem.position + taildiff;
                }

            }
        } // end while


        mBoundsDirty = true;
        // Need to dirty the parent node, but can't do it using needUpdate() here 
        // since we're in the middle of the scene graph update (node listener), 
        // so re-entrant calls don't work. Queue.
        if (mParentNode)
        {
            Node::queueNeedUpdate(getParentSceneNode());
        }

    }
    //-----------------------------------------------------------------------
    void RibbonTrail::_timeUpdate(Real time)
    {
        // Apply all segment effects
        for (size_t s = 0; s < mChainSegmentList.size(); ++s)
        {
            ChainSegment& seg = mChainSegmentList[s];
            if (seg.head != SEGMENT_EMPTY && seg.head != seg.tail)
            {
                
                for(size_t e = seg.head + 1;; ++e) // until break
                {
                    e = e % mMaxElementsPerChain;

                    Element& elem = mChainElementList[seg.start + e];
                    elem.width = elem.width - (time * mDeltaWidth[s]);
                    elem.width = std::max(Real(0.0f), elem.width);
                    elem.colour = elem.colour - (mDeltaColour[s] * time);
                    elem.colour.saturate();

                    if (e == seg.tail)
                        break;
                }
            }
        }
        mVertexContentDirty = true;
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::resetTrail(size_t index, const Node* node)
    {
        assert(index < mChainCount);

        ChainSegment& seg = mChainSegmentList[index];
        // set up this segment
        seg.head = seg.tail = SEGMENT_EMPTY;
        // Create new element, v coord is always 0.0f
        // need to convert to take parent node's position into account
        Vector3 position = node->_getDerivedPosition();
        if (mParentNode)
        {
            position = mParentNode->convertWorldToLocalPosition(position);
        }
        Element e(position,
            mInitialWidth[index], 0.0f, mInitialColour[index], node->_getDerivedOrientation());
        // Add the start position
        addChainElement(index, e);
        // Add another on the same spot, this will extend
        addChainElement(index, e);
    }
    //-----------------------------------------------------------------------
    void RibbonTrail::resetAllTrails()
    {
        for (size_t i = 0; i < mNodeList.size(); ++i)
        {
            resetTrail(i, mNodeList[i]);
        }
    }
    //-----------------------------------------------------------------------
    auto RibbonTrail::getMovableType() const noexcept -> std::string_view
    {
        return RibbonTrailFactory::FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string_view const constinit RibbonTrailFactory::FACTORY_TYPE_NAME = "RibbonTrail";
    //-----------------------------------------------------------------------
    auto RibbonTrailFactory::getType() const noexcept -> std::string_view
    {
        return FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
    auto RibbonTrailFactory::createInstanceImpl( std::string_view name,
        const NameValuePairList* params) -> MovableObject*
    {
        size_t maxElements = 20;
        size_t numberOfChains = 1;
        bool useTex = true;
        bool useCol = true;
        // optional params
        if (params != nullptr)
        {
            auto ni = params->find("maxElements");
            if (ni != params->end())
            {
                maxElements = StringConverter::parseSizeT(ni->second);
            }
            ni = params->find("numberOfChains");
            if (ni != params->end())
            {
                numberOfChains = StringConverter::parseSizeT(ni->second);
            }
            ni = params->find("useTextureCoords");
            if (ni != params->end())
            {
                useTex = StringConverter::parseBool(ni->second);
            }
            ni = params->find("useVertexColours");
            if (ni != params->end())
            {
                useCol = StringConverter::parseBool(ni->second);
            }

        }

        return new RibbonTrail(name, maxElements, numberOfChains, useTex, useCol);

    }

}
