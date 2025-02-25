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

export module Ogre.Core:SubEntity;

export import :Common;
export import :GpuProgramParams;
export import :HardwareBufferManager;
export import :MemoryAllocatorConfig;
export import :Platform;
export import :Prerequisites;
export import :Renderable;
export import :RenderQueue;
export import :ResourceGroupManager;
export import :SharedPtr;

export import <memory>;

export
namespace Ogre {
class Camera;
class Entity;
struct Matrix4;
class RenderOperation;
class SubMesh;
class Technique;
class VertexData;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */
    /** Utility class which defines the sub-parts of an Entity.
        @remarks
            Just as meshes are split into submeshes, an Entity is made up of
            potentially multiple SubMeshes. These are mainly here to provide the
            link between the Material which the SubEntity uses (which may be the
            default Material for the SubMesh or may have been changed for this
            object) and the SubMesh data.
        @par
            The SubEntity also allows the application some flexibility in the
            material properties for this section of a particular instance of this
            Mesh, e.g. tinting the windows on a car model.
        @par
            SubEntity instances are never created manually. They are created at
            the same time as their parent Entity by the SceneManager method
            createEntity.
    */
    class SubEntity: public Renderable, public SubEntityAlloc
    {
        // Note no virtual functions for efficiency
        friend class Entity;
        friend class SceneManager;
    private:
        /** Private constructor - don't allow creation by anybody else.
        */
        SubEntity(Entity* parent, SubMesh* subMeshBasis);
        ~SubEntity() override;

        /// Pointer to parent.
        Entity* mParentEntity;

        /// Cached pointer to material.
        MaterialPtr mMaterialPtr;

        /// Pointer to the SubMesh defining geometry.
        SubMesh* mSubMesh;

        /// override the start index for the RenderOperation
        size_t mIndexStart;

        /// override the end index for the RenderOperation
        size_t mIndexEnd;

        /// Is this SubEntity visible?
        bool mVisible;

        /// The render queue to use when rendering this renderable
        RenderQueueGroupID mRenderQueueID;
        /// Flags whether the RenderQueue's default should be used.
        bool mRenderQueueIDSet;
        /// Flags whether the RenderQueue's default should be used.
        bool mRenderQueuePrioritySet;
        /// The render queue priority to use when rendering this renderable
        ushort mRenderQueuePriority;

        /// The LOD number of the material to use, calculated by Entity::_notifyCurrentCamera
        unsigned short mMaterialLodIndex{0};

        /// Blend buffer details for dedicated geometry
        std::unique_ptr<VertexData> mSkelAnimVertexData;
        /// Quick lookup of buffers
        TempBlendedBufferInfo mTempSkelAnimInfo;
        /// Temp buffer details for software Vertex anim geometry
        TempBlendedBufferInfo mTempVertexAnimInfo;
        /// Vertex data details for software Vertex anim of shared geometry
        std::unique_ptr<VertexData> mSoftwareVertexAnimVertexData;
        /// Vertex data details for hardware Vertex anim of shared geometry
        /// - separate since we need to s/w anim for shadows whilst still altering
        ///   the vertex data for hardware morphing (pos2 binding)
        std::unique_ptr<VertexData> mHardwareVertexAnimVertexData;
        /// Cached distance to last camera for getSquaredViewDepth
        mutable Real mCachedCameraDist;
        /// Number of hardware blended poses supported by material
        ushort mHardwarePoseCount;
        /// Have we applied any vertex animation to geometry?
        bool mVertexAnimationAppliedThisFrame;
        /// The camera for which the cached distance is valid
        mutable const Camera *mCachedCamera{nullptr};

        /** Internal method for preparing this Entity for use in animation. */
        void prepareTempBlendBuffers();

    public:
        /** Gets the name of the Material in use by this instance.
        */
        auto getMaterialName() const noexcept -> std::string_view ;

        /** Sets the name of the Material to be used.
            @remarks
                By default a SubEntity uses the default Material that the SubMesh
                uses. This call can alter that so that the Material is different
                for this instance.
        */
        void setMaterialName( std::string_view name, std::string_view groupName = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

        /// @copydoc setMaterialName
        void setMaterial( const MaterialPtr& material );

        /** Tells this SubEntity whether to be visible or not. */
        void setVisible(bool visible);

        /** Returns whether or not this SubEntity is supposed to be visible. */
        auto isVisible() const noexcept -> bool { return mVisible; }

        /** Sets the render queue group this SubEntity will be rendered through.
        @remarks
            Render queues are grouped to allow you to more tightly control the ordering
            of rendered objects. If you do not call this method, the SubEntity will use
            either the Entity's queue or it will use the default
            (RenderQueue::getDefaultQueueGroup).
        @par
            See Entity::setRenderQueueGroup for more details.
        @param queueID Enumerated value of the queue group to use. See the
            enum class RenderQueueGroupID for what kind of values can be used here.
        */
        void setRenderQueueGroup(RenderQueueGroupID queueID);

        /** Sets the render queue group and group priority this SubEntity will be rendered through.
        @remarks
            Render queues are grouped to allow you to more tightly control the ordering
            of rendered objects. Within a single render group there another type of grouping
            called priority which allows further control.  If you do not call this method, 
            all Entity objects default to the default queue and priority 
            (RenderQueue::getDefaultQueueGroup, RenderQueue::getDefaultRenderablePriority).
        @par
            See Entity::setRenderQueueGroupAndPriority for more details.
        @param queueID Enumerated value of the queue group to use. See the
            enum class RenderQueueGroupID for what kind of values can be used here.
        @param priority The priority within a group to use.
        */
        void setRenderQueueGroupAndPriority(RenderQueueGroupID queueID, ushort priority);

        /** Gets the queue group for this entity, see setRenderQueueGroup for full details. */
        auto getRenderQueueGroup() const noexcept -> RenderQueueGroupID { return mRenderQueueID; }

        /** Gets the queue group for this entity, see setRenderQueueGroup for full details. */
        auto getRenderQueuePriority() const noexcept -> ushort { return mRenderQueuePriority; }

        /** Gets the queue group for this entity, see setRenderQueueGroup for full details. */
        auto isRenderQueueGroupSet() const noexcept -> bool { return mRenderQueueIDSet; }

        /** Gets the queue group for this entity, see setRenderQueueGroup for full details. */
        auto isRenderQueuePrioritySet() const noexcept -> bool { return mRenderQueuePrioritySet; }

        /** Accessor method to read mesh data.
        */
        auto getSubMesh() noexcept -> SubMesh*;

        /** Accessor to get parent Entity */
        auto getParent() const noexcept -> Entity* { return mParentEntity; }


        auto getMaterial() const noexcept -> const MaterialPtr& override { return mMaterialPtr; }
        auto getTechnique() const noexcept -> Technique* override;
        void getRenderOperation(RenderOperation& op) override;

        /** Tells this SubEntity to draw a subset of the SubMesh by adjusting the index buffer extents.
         * Default value is zero so that the entire index buffer is used when drawing.
         * Valid values are zero to getIndexDataEndIndex()
        */
        void setIndexDataStartIndex(size_t start_index);

        /** Returns the current value of the start index used for drawing.
         * \see setIndexDataStartIndex
        */
        auto getIndexDataStartIndex() const -> size_t;

        /** Tells this SubEntity to draw a subset of the SubMesh by adjusting the index buffer extents.
         * Default value is SubMesh::indexData::indexCount so that the entire index buffer is used when drawing.
         * Valid values are mStartIndex to SubMesh::indexData::indexCount
        */
        void setIndexDataEndIndex(size_t end_index);

        /** Returns the current value of the start index used for drawing.
        */
        auto getIndexDataEndIndex() const -> size_t;

        /** Reset the custom start/end index to the default values.
        */
        void resetIndexDataStartEndIndex();

        void getWorldTransforms(Matrix4* xform) const override;
        auto getNumWorldTransforms() const noexcept -> unsigned short override;
        auto getSquaredViewDepth(const Camera* cam) const -> Real override;
        auto getLights() const noexcept -> const LightList& override;
        auto getCastsShadows() const noexcept -> bool override;
        /** Advanced method to get the temporarily blended vertex information
        for entities which are software skinned. 
        @remarks
            Internal engine will eliminate software animation if possible, this
            information is unreliable unless added request for software animation
            via Entity::addSoftwareAnimationRequest.
        @note
            The positions/normals of the returned vertex data is in object space.
        */
        auto _getSkelAnimVertexData() -> VertexData*;
        /** Advanced method to get the temporarily blended software morph vertex information
        @remarks
            Internal engine will eliminate software animation if possible, this
            information is unreliable unless added request for software animation
            via Entity::addSoftwareAnimationRequest.
        @note
            The positions/normals of the returned vertex data is in object space.
        */
        auto _getSoftwareVertexAnimVertexData() -> VertexData*;
        /** Advanced method to get the hardware morph vertex information
        @note
            The positions/normals of the returned vertex data is in object space.
        */
        auto _getHardwareVertexAnimVertexData() -> VertexData*;
        /** Advanced method to get the temp buffer information for software 
        skeletal animation.
        */
        auto _getSkelAnimTempBufferInfo() -> TempBlendedBufferInfo*;
        /** Advanced method to get the temp buffer information for software 
        morph animation.
        */
        auto _getVertexAnimTempBufferInfo() -> TempBlendedBufferInfo*;
        /// Retrieve the VertexData which should be used for GPU binding
        auto getVertexDataForBinding() noexcept -> VertexData*;

        /** Mark all vertex data as so far unanimated. 
        */
        void _markBuffersUnusedForAnimation();
        /** Mark all vertex data as animated. 
        */
        void _markBuffersUsedForAnimation();
        /** Are buffers already marked as vertex animated? */
        auto _getBuffersMarkedForAnimation() const noexcept -> bool { return mVertexAnimationAppliedThisFrame; }
        /** Internal method to copy original vertex data to the morph structures
        should there be no active animation in use.
        */
        void _restoreBuffersForUnusedAnimation(bool hardwareAnimation);

        /** Overridden from Renderable to provide some custom behaviour. */
        void _updateCustomGpuParameter(
            const GpuProgramParameters::AutoConstantEntry& constantEntry,
            GpuProgramParameters* params) const override;

        /** Invalidate the camera distance cache */
        void _invalidateCameraCache ()
        { mCachedCamera = nullptr; }
    };
    /** @} */
    /** @} */

}
