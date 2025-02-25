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

export module Ogre.Core:RibbonTrail;

export import :BillboardChain;
export import :ColourValue;
export import :Common;
export import :ControllerManager;
export import :IteratorWrapper;
export import :MovableObject;
export import :Node;
export import :Prerequisites;

export import <algorithm>;
export import <map>;
export import <vector>;

export
namespace Ogre {
template <typename T> class Controller;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
    /** Subclass of BillboardChain which automatically leaves a trail behind
        one or more Node instances.
    @remarks
        An instance of this class will watch one or more Node instances, and
        automatically generate a trail behind them as they move. Because this
        class can monitor multiple modes, it generates its own geometry in 
        world space and thus, even though it has to be attached to a SceneNode
        to be visible, changing the position of the scene node it is attached to
        makes no difference to the geometry rendered.
    @par
        The 'head' element grows smoothly in size until it reaches the required size,
        then a new element is added. If the segment is full, the tail element
        shrinks by the same proportion as the head grows before disappearing.
    @par
        Elements can be faded out on a time basis, either by altering their colour
        or altering their alpha. The width can also alter over time.
    @par
        'v' texture coordinates are fixed at 0.0 if used, meaning that you can
        use a 1D texture to 'smear' a colour pattern along the ribbon if you wish.
        The 'u' coordinates are by default (0.0, 1.0), but you can alter this 
        using setOtherTexCoordRange if you wish.
    */
    class RibbonTrail : public BillboardChain, public Node::Listener
    {
    public:
        /** Constructor (don't use directly, use factory) 
        @param name The name to give this object
        @param maxElements The maximum number of elements per chain
        @param numberOfChains The number of separate chain segments contained in this object,
            ie the maximum number of nodes that can have trails attached
        @param useTextureCoords If true, use texture coordinates from the chain elements
        @param useVertexColours If true, use vertex colours from the chain elements (must
            be true if you intend to use fading)
        */
        RibbonTrail(std::string_view name, size_t maxElements = 20, size_t numberOfChains = 1, 
            bool useTextureCoords = true, bool useVertexColours = true);
        /// destructor
        ~RibbonTrail() override;

        using NodeList = std::vector<Node *>;
        using NodeIterator = ConstVectorIterator<NodeList>;

        /** Add a node to be tracked.
        @param n The node that will be tracked.
        */
        virtual void addNode(Node* n);
        /** Remove tracking on a given node. */
        virtual void removeNode(const Node* n);
        /** Get an iterator over the nodes which are being tracked. */
        virtual auto getNodeIterator() const -> NodeIterator;
        /** Get the chain index for a given Node being tracked. */
        virtual auto getChainIndexForNode(const Node* n) -> size_t;

        /** Set the length of the trail. 
        @remarks
            This sets the length of the trail, in world units. It also sets how
            far apart each segment will be, ie length / max_elements. 
        @param len The length of the trail in world units
        */
        virtual void setTrailLength(Real len);
        /** Get the length of the trail. */
        virtual auto getTrailLength() const noexcept -> Real { return mTrailLength; }

        /** @copydoc BillboardChain::setMaxChainElements */
        void setMaxChainElements(size_t maxElements) override;
        /** @copydoc BillboardChain::setNumberOfChains */
        void setNumberOfChains(size_t numChains) override;
        /** @copydoc BillboardChain::clearChain */
        void clearChain(size_t chainIndex) override;

        /** Set the starting ribbon colour for a given segment. 
        @param chainIndex The index of the chain
        @param col The initial colour
        @note
            Only used if this instance is using vertex colours.
        */
        virtual void setInitialColour(size_t chainIndex, const ColourValue& col);
        /** Set the starting ribbon colour. 
        @param chainIndex The index of the chain
        @param r,b,g,a The initial colour
        @note
            Only used if this instance is using vertex colours.
        */
        virtual void setInitialColour(size_t chainIndex, float r, float g, float b, float a = 1.0);
        /** Get the starting ribbon colour. */
        auto getInitialColour(size_t chainIndex) const -> const ColourValue& { return mInitialColour.at(chainIndex); }

        /** Enables / disables fading the trail using colour. 
        @param chainIndex The index of the chain
        @param valuePerSecond The amount to subtract from colour each second
        */
        virtual void setColourChange(size_t chainIndex, const ColourValue& valuePerSecond);

        /** Set the starting ribbon width in world units. 
        @param chainIndex The index of the chain
        @param width The initial width of the ribbon
        */
        virtual void setInitialWidth(size_t chainIndex, Real width);
        /** Get the starting ribbon width in world units. */
        auto getInitialWidth(size_t chainIndex) const -> Real { return mInitialWidth.at(chainIndex); }
        
        /** Set the change in ribbon width per second. 
        @param chainIndex The index of the chain
        @param widthDeltaPerSecond The amount the width will reduce by per second
        */
        virtual void setWidthChange(size_t chainIndex, Real widthDeltaPerSecond);
        /** Get the change in ribbon width per second. */
        auto getWidthChange(size_t chainIndex) const -> Real { return mDeltaWidth.at(chainIndex); }

        /** Enables / disables fading the trail using colour. 
        @param chainIndex The index of the chain
        @param r,g,b,a The amount to subtract from each colour channel per second
        */
        virtual void setColourChange(size_t chainIndex, float r, float g, float b, float a);

        /** Get the per-second fading amount */
        auto getColourChange(size_t chainIndex) const -> const ColourValue& { return mDeltaColour.at(chainIndex); }

        /// @see Node::Listener::nodeUpdated
        void nodeUpdated(const Node* node) override;
        /// @see Node::Listener::nodeDestroyed
        void nodeDestroyed(const Node* node) override;

        /// Perform any fading / width delta required; internal method
        virtual void _timeUpdate(Real time);

        auto getMovableType() const noexcept -> std::string_view override;

    private:
        /// List of nodes being trailed
        NodeList mNodeList;
        /// Mapping of nodes to chain segments
        using IndexVector = std::vector<size_t>;
        /// Ordered like mNodeList, contains chain index
        IndexVector mNodeToChainSegment;
        // chains not in use
        IndexVector mFreeChains;

        // fast lookup node->chain index
        // we use positional map too because that can be useful
        using NodeToChainSegmentMap = std::map<const Node *, size_t>;
        NodeToChainSegmentMap mNodeToSegMap;

        /// Total length of trail in world units
        Real mTrailLength;
        /// length of each element
        Real mElemLength;
        /// Squared length of each element
        Real mSquaredElemLength;
        using ColourValueList = std::vector<ColourValue>;
        using RealList = std::vector<Real>;
        /// Initial colour of the ribbon
        ColourValueList mInitialColour;
        /// fade amount per second
        ColourValueList mDeltaColour;
        /// Initial width of the ribbon
        RealList mInitialWidth;
        /// Delta width of the ribbon
        RealList mDeltaWidth;
        /// controller used to hook up frame time to fader
        Controller<Real>* mFadeController{nullptr};
        /// controller value for hooking up frame time to fader
        ControllerValueRealPtr mTimeControllerValue;

        /// Manage updates to the time controller
        virtual void manageController();
        /// Node has changed position, update
        virtual void updateTrail(size_t index, const Node* node);
        /// Reset the tracked chain to initial state
        virtual void resetTrail(size_t index, const Node* node);
        /// Reset all tracked chains to initial state
        virtual void resetAllTrails();

    };


    /** Factory object for creating RibbonTrail instances */
    class RibbonTrailFactory : public MovableObjectFactory
    {
    protected:
        auto createInstanceImpl( std::string_view name, const NameValuePairList* params) -> MovableObject* override;
    public:
        RibbonTrailFactory() = default;
        ~RibbonTrailFactory() override = default;

        static std::string_view const FACTORY_TYPE_NAME;

        [[nodiscard]] auto getType() const noexcept -> std::string_view override;
    };
    /** @} */
    /** @} */

}
