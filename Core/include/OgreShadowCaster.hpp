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

export module Ogre.Core:ShadowCaster;

export import :Common;
export import :IteratorWrapper;
export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :RenderOperation;
export import :Renderable;
export import :SharedPtr;

export import <vector>;

export
namespace Ogre {
struct AxisAlignedBox;
class Camera;
class EdgeData;
class Light;
struct Matrix4;
class MovableObject;
class VertexData;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */
    /** Class which represents the renderable aspects of a set of shadow volume faces. 
    @remarks
        Note that for casters comprised of more than one set of vertex buffers (e.g. SubMeshes each
        using their own geometry), it will take more than one ShadowRenderable to render the 
        shadow volume. Therefore for shadow caster geometry, it is best to stick to one set of
        vertex buffers (not necessarily one buffer, but the positions for the entire geometry 
        should come from one buffer if possible)
    */
    class ShadowRenderable : public Renderable, public ShadowDataAlloc
    {
    protected:
        RenderOperation mRenderOp;
        MaterialPtr mMaterial;
        ::std::unique_ptr<ShadowRenderable> mLightCap; /// Used only if isLightCapSeparate == true
        MovableObject* mParent;
        /// Shared link to position buffer.
        HardwareVertexBufferSharedPtr mPositionBuffer;
        /// Shared link to w-coord buffer (optional).
        HardwareVertexBufferSharedPtr mWBuffer;
    public:
        ShadowRenderable() : mLightCap(nullptr) {}
        ShadowRenderable(MovableObject* parent, const HardwareIndexBufferSharedPtr& indexBuffer,
                         const VertexData* vertexData, bool createSeparateLightCap,
                         bool isLightCap = false);
        ~ShadowRenderable() override;
        /** Set the material to be used by the shadow, should be set by the caller 
            before adding to a render queue
        */
        void setMaterial(const MaterialPtr& mat) { mMaterial = mat; }
        [[nodiscard]] auto getMaterial() const noexcept -> const MaterialPtr& override { return mMaterial; }
        void getRenderOperation(RenderOperation& op) override { op = mRenderOp; }
        /// Get the internal render operation for set up.
        auto getRenderOperationForUpdate() noexcept -> RenderOperation* {return &mRenderOp;}
        void getWorldTransforms(Matrix4* xform) const override;
        auto getSquaredViewDepth(const Camera*) const noexcept -> Real override { return 0; /* not used */}
        [[nodiscard]] auto getLights() const noexcept -> const LightList& override;
        /** Does this renderable require a separate light cap?
        @remarks
            If possible, the light cap (when required) should be contained in the
            usual geometry of the shadow renderable. However, if for some reason
            the normal depth function (less than) could cause artefacts, then a
            separate light cap with a depth function of 'always fail' can be used 
            instead. The primary example of this is when there are floating point
            inaccuracies caused by calculating the shadow geometry separately from
            the real geometry. 
        */
        [[nodiscard]] auto isLightCapSeparate() const noexcept -> bool { return mLightCap != nullptr; }

        /// Get the light cap version of this renderable.
        auto getLightCapRenderable() noexcept -> ShadowRenderable* { return mLightCap.get(); }
        /// Should this ShadowRenderable be treated as visible?
        [[nodiscard]] virtual auto isVisible() const noexcept -> bool { return true; }

        /** This function informs the shadow renderable that the global index buffer
            from the SceneManager has been updated. As all shadow use this buffer their pointer 
            must be updated as well.
        @param indexBuffer
            Pointer to the new index buffer.
        */
        void rebindIndexBuffer(const HardwareIndexBufferSharedPtr& indexBuffer);

        [[nodiscard]] auto getPositionBuffer() const noexcept -> const HardwareVertexBufferSharedPtr& { return mPositionBuffer; }
    };

    /** A set of flags that can be used to influence ShadowRenderable creation. */
    enum class ShadowRenderableFlags : unsigned long
    {
        /// For shadow volume techniques only, generate a light cap on the volume.
        INCLUDE_LIGHT_CAP = 0x00000001,
        /// For shadow volume techniques only, generate a dark cap on the volume.
        INCLUDE_DARK_CAP  = 0x00000002,
        /// For shadow volume techniques only, indicates volume is extruded to infinity.
        EXTRUDE_TO_INFINITY  = 0x00000004,
        /// For shadow volume techniques only, indicates hardware extrusion is not supported.
        EXTRUDE_IN_SOFTWARE  = 0x00000008,
    };

    auto constexpr operator bitor(ShadowRenderableFlags left, ShadowRenderableFlags right) -> ShadowRenderableFlags
    {
        return static_cast<ShadowRenderableFlags>
        (   std::to_underlying(left)
        bitor
            std::to_underlying(right)
        );
    }

    auto constexpr operator |=(ShadowRenderableFlags& left, ShadowRenderableFlags right) -> ShadowRenderableFlags&
    {
        return left = left bitor right;
    }

    auto constexpr operator not(ShadowRenderableFlags value) -> bool
    {
        return not std::to_underlying(value);
    }

    auto constexpr operator bitand(ShadowRenderableFlags left, ShadowRenderableFlags right) -> ShadowRenderableFlags
    {
        return static_cast<ShadowRenderableFlags>
        (   std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }

    using ShadowRenderableList = std::vector<ShadowRenderable *>;

    /** This class defines the interface that must be implemented by shadow casters.
    */
    class ShadowCaster
    {
    public:
        virtual ~ShadowCaster() = default;
        /** Returns whether or not this object currently casts a shadow. */
        [[nodiscard]] virtual auto getCastShadows() const noexcept -> bool = 0;

        /** Returns details of the edges which might be used to determine a silhouette. */
        virtual auto getEdgeList() noexcept -> EdgeData* = 0;
        /** Returns whether the object has a valid edge list. */
        auto hasEdgeList() -> bool { return getEdgeList() != nullptr; }

        /** Get the world bounding box of the caster. */
        [[nodiscard]] virtual auto getWorldBoundingBox(bool derive = false) const -> const AxisAlignedBox& = 0;
        /** Gets the world space bounding box of the light cap. */
        [[nodiscard]] virtual auto getLightCapBounds() const noexcept -> const AxisAlignedBox& = 0;
        /** Gets the world space bounding box of the dark cap, as extruded using the light provided. */
        [[nodiscard]] virtual auto getDarkCapBounds(const Light& light, Real dirLightExtrusionDist) const -> const AxisAlignedBox& = 0;

        using ShadowRenderableList = Ogre::ShadowRenderableList;
        using ShadowRenderableListIterator = VectorIterator<ShadowRenderableList>;

        /** Gets an list of the renderables required to render the shadow volume.
        @remarks
            Shadowable geometry should ideally be designed such that there is only one
            ShadowRenderable required to render the the shadow; however this is not a necessary
            limitation and it can be exceeded if required.
        @param light
            The light to generate the shadow from.
        @param indexBuffer
            The index buffer to build the renderables into, 
            the current contents are assumed to be disposable.
        @param indexBufferUsedSize
            If the rest of buffer is enough than it would be locked with
            LockOptions::NO_OVERWRITE semantic and indexBufferUsedSize would be increased,
            otherwise LockOptions::DISCARD would be used and indexBufferUsedSize would be reset.
        @param extrusionDistance
            The distance to extrude the shadow volume.
        @param flags
            Technique-specific flags, see ShadowRenderableFlags.
        */
        virtual auto
        getShadowVolumeRenderableList(const Light* light, const HardwareIndexBufferPtr& indexBuffer,
                                      size_t& indexBufferUsedSize, float extrusionDistance,
                                      ShadowRenderableFlags flags = {}) -> const ShadowRenderableList& = 0;

        /** Common implementation of releasing shadow renderables.*/
        static void clearShadowRenderableList(ShadowRenderableList& shadowRenderables);

        /** Utility method for extruding vertices based on a light. 
        @remarks
            Unfortunately, because D3D cannot handle homogeneous (4D) position
            coordinates in the fixed-function pipeline (GL can, but we have to
            be cross-API), when we extrude in software we cannot extrude to 
            infinity the way we do in the vertex program (by setting w to
            0.0f). Therefore we extrude by a fixed distance, which may cause 
            some problems with larger scenes. Luckily better hardware (ie
            vertex programs) can fix this.
        @param vertexBuffer
            The vertex buffer containing ONLY xyz position
            values, which must be originalVertexCount * 2 * 3 floats long.
        @param originalVertexCount
            The count of the original number of
            vertices, i.e. the number in the mesh, not counting the doubling
            which has already been done (by VertexData::prepareForShadowVolume)
            to provide the extruded area of the buffer.
        @param lightPos
            4D light position in object space, when w=0.0f this
            represents a directional light.
        @param extrudeDist
            The distance to extrude.
        */
        static void extrudeVertices(const HardwareVertexBufferSharedPtr& vertexBuffer, 
            size_t originalVertexCount, const Vector4& lightPos, Real extrudeDist);
        /** Get the distance to extrude for a point/spot light. */
        virtual auto getPointExtrusionDistance(const Light* l) const -> Real = 0;
    protected:
        /** Tells the caster to perform the tasks necessary to update the 
            edge data's light listing. Can be overridden if the subclass needs 
            to do additional things. 
        @param edgeData
            The edge information to update.
        @param lightPos
            4D vector representing the light, a directional light has w=0.0.
       */
        virtual void updateEdgeListLightFacing(EdgeData* edgeData, 
            const Vector4& lightPos);

        /** Generates the indexes required to render a shadow volume into the 
            index buffer which is passed in, and updates shadow renderables
            to use it.
        @param edgeData
            The edge information to use.
        @param indexBuffer
            The buffer into which to write data into; current 
            contents are assumed to be discardable.
        @param indexBufferUsedSize
            If the rest of buffer is enough than it would be locked with
            LockOptions::NO_OVERWRITE semantic and indexBufferUsedSize would be increased,
            otherwise LockOptions::DISCARD would be used and indexBufferUsedSize would be reset.
        @param light
            The light, mainly for type info as silhouette calculations
            should already have been done in updateEdgeListLightFacing
        @param shadowRenderables
            A list of shadow renderables which has 
            already been constructed but will need populating with details of
            the index ranges to be used.
        @param flags
            Additional controller flags, see ShadowRenderableFlags.
        */
        virtual void generateShadowVolume(EdgeData* edgeData, 
            const HardwareIndexBufferSharedPtr& indexBuffer, size_t& indexBufferUsedSize,
            const Light* light, ShadowRenderableList& shadowRenderables, ShadowRenderableFlags flags);
        /** Utility method for extruding a bounding box. 
        @param box
            Original bounding box, will be updated in-place.
        @param lightPos
            4D light position in object space, when w=0.0f this
            represents a directional light.
        @param extrudeDist
            The distance to extrude.
        */
        virtual void extrudeBounds(AxisAlignedBox& box, const Vector4& lightPos, 
            Real extrudeDist) const;


    };
    /** @} */
    /** @} */
} // namespace Ogre
