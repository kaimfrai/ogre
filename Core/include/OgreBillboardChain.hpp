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
// Thanks to Vincent Cantin (karmaGfa) for the original implementation of this
// class, although it has now been mostly rewritten
module;

#include <cstddef>

export module Ogre.Core:BillboardChain;

export import :AxisAlignedBox;
export import :ColourValue;
export import :Common;
export import :Material;
export import :MovableObject;
export import :Prerequisites;
export import :Quaternion;
export import :Renderable;
export import :ResourceGroupManager;
export import :SharedPtr;
export import :Vector;

export import <algorithm>;
export import <memory>;
export import <vector>;

export
namespace Ogre {
class Camera;
class IndexData;
struct Matrix4;
class RenderOperation;
class RenderQueue;
class RenderSystem;
class SceneManager;
class VertexData;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */

    /** Allows the rendering of a chain of connected billboards.
    @remarks
        A billboard chain operates much like a traditional billboard, i.e. its
        segments always face the camera; the difference being that instead of
        a set of disconnected quads, the elements in this class are connected
        together in a chain which must always stay in a continuous strip. This
        kind of effect is useful for creating effects such as trails, beams,
        lightning effects, etc.
    @par
        A single instance of this class can actually render multiple separate
        chain segments in a single render operation, provided they all use the
        same material. To clarify the terminology: a 'segment' is a separate 
        sub-part of the chain with its own start and end (called the 'head'
        and the 'tail'). An 'element' is a single position / colour / texcoord
        entry in a segment. You can add items to the head of a chain, and 
        remove them from the tail, very efficiently. Each segment has a max
        size, and if adding an element to the segment would exceed this size, 
        the tail element is automatically removed and re-used as the new item
        on the head.
    @par
        This class has no auto-updating features to do things like alter the
        colour of the elements or to automatically add / remove elements over
        time - you have to do all this yourself as a user of the class. 
        Subclasses can however be used to provide this kind of behaviour 
        automatically. @see RibbonTrail
    */
    class BillboardChain : public MovableObject, public Renderable
    {
        auto getCastsShadows() const noexcept -> bool override { return getCastShadows(); }
    public:

        /** Contains the data of an element of the BillboardChain.
        */
        class Element
        {

        public:

            Element();

            Element(const Vector3 &position,
                Real width,
                Real texCoord,
                const ColourValue &colour,
                const Quaternion &orientation);

            Vector3 position;
            Real width;
            /// U or V texture coord depending on options
            Real texCoord;
            ColourValue colour;

            /// Only used when mFaceCamera == false
            Quaternion orientation;
        };
        using ElementList = std::vector<Element>;

        /** Constructor (don't use directly, use factory) 
        @param name The name to give this object
        @param maxElements The maximum number of elements per chain
        @param numberOfChains The number of separate chain segments contained in this object
        @param useTextureCoords If true, use texture coordinates from the chain elements
        @param useColours If true, use vertex colours from the chain elements
        @param dynamic If true, buffers are created with the intention of being updated
        */
        BillboardChain(std::string_view name, size_t maxElements = 20, size_t numberOfChains = 1, 
            bool useTextureCoords = true, bool useColours = true, bool dynamic = true);

        ~BillboardChain() override;

        /** Set the maximum number of chain elements per chain 
        */
        virtual void setMaxChainElements(size_t maxElements);
        /** Get the maximum number of chain elements per chain 
        */
        virtual auto getMaxChainElements() const noexcept -> size_t { return mMaxElementsPerChain; }
        /** Set the number of chain segments (this class can render multiple chains
            at once using the same material). 
        */
        virtual void setNumberOfChains(size_t numChains);
        /** Get the number of chain segments (this class can render multiple chains
        at once using the same material). 
        */
        virtual auto getNumberOfChains() const noexcept -> size_t { return mChainCount; }

        /** Sets whether texture coordinate information should be included in the
            final buffers generated.
        @note You must use either texture coordinates or vertex colour since the
            vertices have no normals and without one of these there is no source of
            colour for the vertices.
        */
        virtual void setUseTextureCoords(bool use);
        /** Gets whether texture coordinate information should be included in the
            final buffers generated.
        */
        virtual auto getUseTextureCoords() const noexcept -> bool { return mUseTexCoords; }

        /** The direction in which texture coordinates from elements of the
            chain are used.
        */
        enum class TexCoordDirection
        {
            /// Tex coord in elements is treated as the 'u' texture coordinate
            U,
            /// Tex coord in elements is treated as the 'v' texture coordinate
            V
        };
        /** Sets the direction in which texture coords specified on each element
            are deemed to run along the length of the chain.
        @param dir The direction, default is TexCoordDirection::U.
        */
        virtual void setTextureCoordDirection(TexCoordDirection dir);
        /** Gets the direction in which texture coords specified on each element
            are deemed to run.
        */
        virtual auto getTextureCoordDirection() noexcept -> TexCoordDirection { return mTexCoordDir; }

        /** Set the range of the texture coordinates generated across the width of
            the chain elements.
        @param start Start coordinate, default 0.0
        @param end End coordinate, default 1.0
        */
        virtual void setOtherTextureCoordRange(Real start, Real end);
        /** Get the range of the texture coordinates generated across the width of
            the chain elements.
        */
        virtual auto getOtherTextureCoordRange() const noexcept -> const Real* { return mOtherTexCoordRange; }

        /** Sets whether vertex colour information should be included in the
            final buffers generated.
        @note You must use either texture coordinates or vertex colour since the
            vertices have no normals and without one of these there is no source of
            colour for the vertices.
        */
        virtual void setUseVertexColours(bool use);
        /** Gets whether vertex colour information should be included in the
            final buffers generated.
        */
        virtual auto getUseVertexColours() const noexcept -> bool { return mUseVertexColour; }

        /** Sets whether or not the buffers created for this object are suitable
            for dynamic alteration.
        */
        virtual void setDynamic(bool dyn);

        /** Gets whether or not the buffers created for this object are suitable
            for dynamic alteration.
        */
        virtual auto getDynamic() const noexcept -> bool { return mDynamic; }
        
        /** Add an element to the 'head' of a chain.
        @remarks
            If this causes the number of elements to exceed the maximum elements
            per chain, the last element in the chain (the 'tail') will be removed
            to allow the additional element to be added.
        @param chainIndex The index of the chain
        @param billboardChainElement The details to add
        */
        virtual void addChainElement(size_t chainIndex, 
            const Element& billboardChainElement);
        /** Remove an element from the 'tail' of a chain.
        @param chainIndex The index of the chain
        */
        virtual void removeChainElement(size_t chainIndex);
        /** Update the details of an existing chain element.
        @param chainIndex The index of the chain
        @param elementIndex The element index within the chain, measured from 
            the 'head' of the chain
        @param billboardChainElement The details to set
        */
        virtual void updateChainElement(size_t chainIndex, size_t elementIndex, 
            const Element& billboardChainElement);
        /** Get the detail of a chain element.
        @param chainIndex The index of the chain
        @param elementIndex The element index within the chain, measured from
            the 'head' of the chain
        */
        virtual auto getChainElement(size_t chainIndex, size_t elementIndex) const -> const Element&;

        /** Returns the number of chain elements. */
        virtual auto getNumChainElements(size_t chainIndex) const -> size_t;

        /** Remove all elements of a given chain (but leave the chain intact). */
        virtual void clearChain(size_t chainIndex);
        /** Remove all elements from all chains (but leave the chains themselves intact). */
        virtual void clearAllChains();

        /** Sets whether the billboard should always be facing the camera or a custom direction
            set by each point element.
        @remarks
            Billboards facing the camera are useful for smoke trails, light beams, etc by
            simulating a cylinder. However, because of this property, wide trails can cause
            several artefacts unless the head is properly covered.
            Therefore, non-camera-facing billboards are much more convenient for leaving big
            trails of movement from thin objects, for example a sword swing as seen in many
            fighting games.
        @param faceCamera True to be always facing the camera (Default value: True)
        @param normalVector Only used when faceCamera == false. Must be a non-zero vector.
        This vector is the "point of reference" for each point orientation. For example,
        if normalVector is Vector3::UNIT_Z, and the point's orientation is an identity
        matrix, the segment corresponding to that point will be facing towards UNIT_Z
        This vector is internally normalized.
        */
        void setFaceCamera( bool faceCamera, const Vector3 &normalVector=Vector3::UNIT_X );

        /// Get the material name in use
        virtual auto getMaterialName() const noexcept -> std::string_view { return mMaterial->getName(); }
        /// Set the material name to use for rendering
        virtual void setMaterialName( std::string_view name, std::string_view groupName = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );


        // Overridden members follow
        auto getSquaredViewDepth(const Camera* cam) const -> Real override;
        auto getBoundingRadius() const -> Real override;
        auto getBoundingBox() const noexcept -> const AxisAlignedBox& override;
        auto getMaterial() const noexcept -> const MaterialPtr& override;
        auto getMovableType() const noexcept -> std::string_view override;
        void _updateRenderQueue(RenderQueue *) override;
        void getRenderOperation(RenderOperation &) override;
        auto preRender(SceneManager* sm, RenderSystem* rsys) -> bool override;
        void getWorldTransforms(Matrix4 *) const override;
        auto getLights() const noexcept -> const LightList& override;
        /// @copydoc MovableObject::visitRenderables
        void visitRenderables(Renderable::Visitor* visitor, 
            bool debugRenderables = false) override;



    protected:

        /// Maximum length of each chain
        size_t mMaxElementsPerChain;
        /// Number of chains
        size_t mChainCount;
        /// Use texture coords?
        bool mUseTexCoords;
        /// Use vertex colour?
        bool mUseVertexColour;
        /// Dynamic use?
        bool mDynamic;
        /// Vertex data
        std::unique_ptr<VertexData> mVertexData;
        /// Index data (to allow multiple unconnected chains)
        std::unique_ptr<IndexData> mIndexData;
        /// Is the vertex declaration dirty?
        bool mVertexDeclDirty{true};
        /// Do the buffers need recreating?
        bool mBuffersNeedRecreating{true};
        /// Do the bounds need redefining?
        mutable bool mBoundsDirty{true};
        /// Is the index buffer dirty?
        bool mIndexContentDirty{true};
        /// Is the vertex buffer dirty?
        bool mVertexContentDirty{true};
        /// AABB
        mutable AxisAlignedBox mAABB;
        /// Bounding radius
        mutable Real mRadius{0.0f};
        /// Material 
        MaterialPtr mMaterial;
        /// Texture coord direction
        TexCoordDirection mTexCoordDir{TexCoordDirection::U};
        /// Other texture coord range
        Real mOtherTexCoordRange[2];
        /// Camera last used to build the vertex buffer
        Camera *mVertexCameraUsed{nullptr};
        /// When true, the billboards always face the camera
        bool mFaceCamera{true};
        /// Used when mFaceCamera == false; determines the billboard's "normal". i.e.
        /// when the orientation is identity, the billboard is perpendicular to this
        /// vector
        Vector3 mNormalBase;


        /// The list holding the chain elements
        ElementList mChainElementList;

        /** Simple struct defining a chain segment by referencing a subset of
            the preallocated buffer (which will be mMaxElementsPerChain * mChainCount
            long), by it's chain index, and a head and tail value which describe
            the current chain. The buffer subset wraps at mMaxElementsPerChain
            so that head and tail can move freely. head and tail are inclusive,
            when the chain is empty head and tail are filled with high-values.
        */
        struct ChainSegment
        {
            /// The start of this chains subset of the buffer
            size_t start;
            /// The 'head' of the chain, relative to start
            size_t head;
            /// The 'tail' of the chain, relative to start
            size_t tail;
        };
        using ChainSegmentList = std::vector<ChainSegment>;
        ChainSegmentList mChainSegmentList;

        /// Setup the STL collections
        virtual void setupChainContainers();
        /// Setup vertex declaration
        virtual void setupVertexDeclaration();
        /// Setup buffers
        virtual void setupBuffers();
        /// Update the contents of the vertex buffer
        virtual void updateVertexBuffer(Camera* cam);
        /// Update the contents of the index buffer
        virtual void updateIndexBuffer();
        virtual void updateBoundingBox() const;

        /// Chain segment has no elements
        static const size_t SEGMENT_EMPTY;
    };


    /** Factory object for creating BillboardChain instances */
    class BillboardChainFactory : public MovableObjectFactory
    {
    private:
        auto createInstanceImpl( std::string_view name, const NameValuePairList* params) -> MovableObject* override;
    public:
        BillboardChainFactory() = default;
        ~BillboardChainFactory() override = default;

        static std::string_view const FACTORY_TYPE_NAME;

        [[nodiscard]] auto getType() const noexcept -> std::string_view override;
    };

    /** @} */
    /** @} */

} // namespace
