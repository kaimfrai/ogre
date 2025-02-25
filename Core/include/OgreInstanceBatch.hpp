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

export module Ogre.Core:InstanceBatch;

export import :AxisAlignedBox;
export import :Common;
export import :InstancedEntity;
export import :Matrix4;
export import :Mesh;
export import :MovableObject;
export import :Platform;
export import :Prerequisites;
export import :RenderOperation;
export import :Renderable;
export import :SharedPtr;
export import :Vector;

export import <algorithm>;
export import <memory>;
export import <vector>;

export
namespace Ogre
{
class Camera;
class InstanceManager;
class RenderQueue;
class SubMesh;
class Technique;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */

    /** InstanceBatch forms part of the new Instancing system
        This is an abstract class that must be derived to implement different instancing techniques
        (@see InstanceManager::InstancingTechnique)
        OGRE wasn't truly thought for instancing. OGRE assumes that either:
            a. One MovableObject -> No Renderable
            b. One MovableObject -> One Renderable
            c. One MovableObject -> Many Renderable.
        However, instances work on reverse: Many MovableObject have the same Renderable.
        <b>Instancing is already difficult to cull by a CPU</b>, but the main drawback from this assumption
        is that it makes it even harder to take advantage from OGRE's culling capabilities
        (i.e. @see OctreeSceneManager)
    @par
        To workaround this problem, InstanceBatch updates on almost every frame,
        growing the bounding box to fit all instances that are not being culled individually.
        This helps by avoiding a huge bbox that may cover the whole scene, which decreases shadow
        quality considerably (as it is seen as large shadow receiver)
        Furthermore, if no individual instance is visible, the InstanceBatch switches it's visibility
        (@see MovableObject::setVisible) to avoid sending this Renderable to the GPU. This happens because
        even when no individual instance is visible, their merged bounding box may cause OGRE to think
        the batch is visible (i.e. the camera is looking between object A & B, but A & B aren't visible)
    @par
        <b>As it happens with instancing in general, all instanced entities from the same batch will share
        the same textures and materials</b>
    @par
        Each InstanceBatch preallocates a fixed amount of mInstancesPerBatch instances once it's been
        built (@see build, @see buildFrom).
        @see createInstancedEntity and @see removeInstancedEntity on how to retrieve those instances
        remove them from scene.
        Note that, on GPU side, removing an instance from scene doesn't save GPU cycles on what
        respects vertex shaders, but saves a little fillrate and pixel shaders; unless all instances
        are removed, which saves GPU.
        For more information, @see InstancedEntity
        For information on how Ogre manages multiple Instance batches, @see InstanceManager

    @remarks
        Design discussion webpage
    @author
        Matias N. Goldberg ("dark_sylinc")
    @version
        1.0
    */
    class InstanceBatch : public Renderable, public MovableObject
    {
    public:
        using InstancedEntityVec = std::vector< ::std::unique_ptr<InstancedEntity>>;
        using CustomParamsVec = std::vector<Vector4>;
    protected:
        using Matrix3x4f = TransformBase<3, float>;
        RenderOperation     mRenderOperation;
        size_t              mInstancesPerBatch;

        InstanceManager     *mCreator;

        MaterialPtr         mMaterial;

        MeshPtr             mMeshReference;
        Mesh::IndexMap const *mIndexToBoneMap;

        //InstancedEntities are all allocated at build time and kept as "unused"
        //when they're requested, they're removed from there when requested,
        //and put back again when they're no longer needed
        //Note each InstancedEntity has a unique ID ranging from [0; mInstancesPerBatch)
        InstancedEntityVec  mInstancedEntities;
        std::vector<InstancedEntity*> mUnusedEntities;

        ///@see InstanceManager::setNumCustomParams(). Because this may not even be used,
        ///our implementations keep the params separate from the InstancedEntity to lower
        ///the memory overhead. They default to Vector4::ZERO
        CustomParamsVec     mCustomParams;

        /// This bbox contains all (visible) instanced entities
        AxisAlignedBox      mFullBoundingBox;
        Real                mBoundingRadius{ 0 };
        bool                mBoundsDirty{ false };
        bool                mBoundsUpdated{ false }; //Set to false by derived classes that need it
        Camera              *mCurrentCamera{ nullptr };

        unsigned short      mMaterialLodIndex{ 0 };

        bool                mDirtyAnimation{true}; //Set to false at start of each _updateRenderQueue

        /// False if a technique doesn't support skeletal animation
        bool                mTechnSupportsSkeletal{ true };

        /// Last update camera distance frame number
        mutable unsigned long mCameraDistLastUpdateFrameNumber;
        /// Cached distance to last camera for getSquaredViewDepth
        mutable Real mCachedCameraDist;
        /// The camera for which the cached distance is valid
        mutable const Camera *mCachedCamera{ nullptr };

        /// Tells that the list of entity instances with shared transforms has changed
        bool mTransformSharingDirty{true};

        /// When true remove the memory of the VertexData we've created because no one else will
        bool mRemoveOwnVertexData{false};
        /// When true remove the memory of the IndexData we've created because no one else will
        bool mRemoveOwnIndexData{false};

        virtual void setupVertices( const SubMesh* baseSubMesh ) = 0;
        virtual void setupIndices( const SubMesh* baseSubMesh ) = 0;
        virtual void createAllInstancedEntities();
        virtual void deleteAllInstancedEntities();
        /// Creates a new InstancedEntity instance
        virtual auto generateInstancedEntity(size_t num) -> InstancedEntity*;

        /** Takes an array of 3x4 matrices and makes it camera relative. Note the second argument
            takes number of floats in the array, not number of matrices. Assumes mCachedCamera
            contains the camera which is about to be rendered to.
        */
        void makeMatrixCameraRelative3x4( Matrix3x4f *mat3x4, size_t count );

        /// Returns false on errors that would prevent building this batch from the given submesh
        virtual auto checkSubMeshCompatibility( const SubMesh* baseSubMesh ) -> bool;

        void updateVisibility();

        /** @see _defragmentBatch */
        void defragmentBatchNoCull( InstancedEntityVec &usedEntities, CustomParamsVec &usedParams );

        /** @see _defragmentBatch
            This one takes the entity closest to the minimum corner of the bbox, then starts
            gathering entities closest to this entity. There might be much better algorithms (i.e.
            involving space partition), but this one is simple and works well enough
        */
        void defragmentBatchDoCull( InstancedEntityVec &usedEntities, CustomParamsVec &usedParams );

    public:
        InstanceBatch( InstanceManager *creator, MeshPtr &meshReference, const MaterialPtr &material,
                       size_t instancesPerBatch, const Mesh::IndexMap *indexToBoneMap,
                       std::string_view batchName );
        ~InstanceBatch() override;

        auto _getMeshRef() noexcept -> MeshPtr& { return mMeshReference; }

        /** Raises an exception if trying to change it after being built
        */
        void _setInstancesPerBatch( size_t instancesPerBatch );

        auto _getIndexToBoneMap() const noexcept -> const Mesh::IndexMap* { return mIndexToBoneMap; }

        /** Returns true if this technique supports skeletal animation
        @remarks
            A virtual function could have been used, but using a simple variable overridden
            by the derived class is faster than virtual call overhead. And both are clean
            ways of implementing it.
        */
        auto _supportsSkeletalAnimation() const noexcept -> bool { return mTechnSupportsSkeletal; }

        /** @see InstanceManager::updateDirtyBatches */
        void _updateBounds();

        /** Some techniques have a limit on how many instances can be done.
            Sometimes even depends on the material being used.
        @par
            Note this is a helper function, as such it takes a submesh base to compute
            the parameters, instead of using the object's own. This allows
            querying for a technique without requiering to actually build it.
        @param baseSubMesh The base submesh that will be using to build it.
        @param flags Flags to pass to the InstanceManager. @see InstanceManagerFlags
        @return The max instances limit
        */
        virtual auto calculateMaxNumInstances( const SubMesh *baseSubMesh, InstanceManagerFlags flags ) const -> size_t = 0;

        /** Constructs all the data needed to use this batch, as well as the
            InstanceEntities. Placed here because in the constructor virtual
            tables may not have been yet filled.
        @param baseSubMesh A sub mesh which the instances will be based upon from
        @remarks
            Call this only ONCE. This is done automatically by Ogre::InstanceManager
            Caller is responsable for freeing buffers in this RenderOperation
            Buffers inside the RenderOp may be null if the built failed.
        @return
            A render operation which is very useful to pass to other InstanceBatches
            (@see buildFrom) so that they share the same vertex buffers and indices,
            when possible
        */
        virtual auto build( const SubMesh* baseSubMesh ) -> RenderOperation;

        /** Instancing consumes significantly more GPU memory than regular rendering
            methods. However, multiple batches can share most, if not all, of the
            vertex & index buffers to save memory.
            Derived classes are free to overload this method to manipulate what to
            reference from Render Op.
            For example, Hardware based instancing uses it's own vertex buffer for the
            last source binding, but shares the other sources.
        @param baseSubMesh A sub mesh which the instances will be based upon from
        @param renderOperation The RenderOp to reference.
        @remarks
            Caller is responsable for freeing buffers passed as input arguments
            This function replaces the need to call build()
        */
        virtual void buildFrom( const SubMesh *baseSubMesh, const RenderOperation &renderOperation );

        auto _getMeshReference() const noexcept -> const Ogre::MeshPtr& { return mMeshReference; }

        /** @return true if it can not create more InstancedEntities
            (Num InstancedEntities == mInstancesPerBatch)
        */
        auto isBatchFull() const noexcept -> bool { return mUnusedEntities.empty(); }

        /** Returns true if it no instanced entity has been requested or all of them have been removed
        */
        auto isBatchUnused() const noexcept -> bool { return mUnusedEntities.size() == mInstancedEntities.size(); }

        auto inline getUsedEntityCount() const noexcept -> size_t { return mInstancedEntities.size() - mUnusedEntities.size();  }

        /** Fills the input vector with the instances that are currently being used or were requested.
            Used for defragmentation, @see InstanceManager::defragmentBatches.
            Ownership of instanced entities is transfered to outEntities. mInstancedEntities will be empty afterwards.
        */
        void transferInstancedEntitiesInUse( InstancedEntityVec &outEntities, CustomParamsVec &outParams );

        /** @see InstanceManager::defragmentBatches
            This function takes InstancedEntities and pushes back all entities it can fit here
            Extra entities in mUnusedEntities are destroyed
            (so that used + unused = mInstancedEntities.size())
        @param optimizeCulling true will call the DoCull version, false the NoCull
        @param usedEntities Array of InstancedEntities to parent with this batch. Those reparented
            are removed from this input vector
        @param usedParams Array of Custom parameters correlated with the InstancedEntities in usedEntities.
            They follow the fate of the entities in that vector.
        @remarks:
            This function assumes caller holds data to mInstancedEntities! Otherwise
            you can get memory leaks. Don't call this directly if you don't know what you're doing!
        */
        void _defragmentBatch( bool optimizeCulling, InstancedEntityVec &usedEntities,
                                CustomParamsVec &usedParams );

        /** Called by InstancedEntity(s) to tell us we need to update the bounds
            (we touch the SceneNode so the SceneManager aknowledges such change)
        */
        virtual void _boundsDirty();

        /** Tells this batch to stop updating animations, positions, rotations, and display
            all it's active instances. Currently only InstanceBatchHW & InstanceBatchHW_VTF support it.
            This option makes the batch behave pretty much like Static Geometry, but with the GPU RAM
            memory advantages (less VRAM, less bandwidth) and not LOD support. Very useful for
            billboards of trees, repeating vegetation, etc.
            @remarks
                This function moves a lot of processing time from the CPU to the GPU. If the GPU
                is already a bottleneck, you may see a decrease in performance instead!
                Call this function again (with bStatic=true) if you've made a change to an
                InstancedEntity and wish this change to take effect.
                Be sure to call this after you've set all your instances
                @see InstanceBatchHW::setStaticAndUpdate
        */
        virtual void setStaticAndUpdate( bool bStatic )     {}

        /** Returns true if this batch was set as static. @see setStaticAndUpdate
        */
        virtual auto isStatic() const noexcept -> bool { return false; }

        /** Returns a pointer to a new InstancedEntity ready to use
            Note it's actually preallocated, so no memory allocation happens at
            this point.
            @remarks
                Returns NULL if all instances are being used
        */
        auto createInstancedEntity() -> InstancedEntity*;

        /** Removes an InstancedEntity from the scene retrieved with
            getNewInstancedEntity, putting back into a queue
            @remarks
                Throws an exception if the instanced entity wasn't created by this batch
                Removed instanced entities save little CPU time, but _not_ GPU
        */
        void removeInstancedEntity( InstancedEntity *instancedEntity );

        /** Tells whether world bone matrices need to be calculated.
            This does not include bone matrices which are calculated regardless
        */
        virtual auto useBoneWorldMatrices() const noexcept -> bool { return true; }

        /** Tells that the list of entity instances with shared transforms has changed */
        void _markTransformSharingDirty() { mTransformSharingDirty = true; }

        /** @see InstancedEntity::setCustomParam */
        void _setCustomParam( InstancedEntity *instancedEntity, unsigned char idx, const Vector4 &newParam );

        /** @see InstancedEntity::getCustomParam */
        auto _getCustomParam( InstancedEntity *instancedEntity, unsigned char idx ) -> const Vector4&;

        //Renderable overloads
        /** @copydoc Renderable::getMaterial */
        auto getMaterial() const noexcept -> const MaterialPtr& override { return mMaterial; }
        /** @copydoc Renderable::getRenderOperation */
        void getRenderOperation( RenderOperation& op ) override  { op = mRenderOperation; }

        /** @copydoc Renderable::getSquaredViewDepth */
        auto getSquaredViewDepth( const Camera* cam ) const -> Real override;
        /** @copydoc Renderable::getLights */
        auto getLights( ) const noexcept -> const LightList& override;
        /** @copydoc Renderable::getTechnique */
        auto getTechnique() const noexcept -> Technique* override;

        /** @copydoc MovableObject::getMovableType */
        auto getMovableType() const noexcept -> std::string_view override;
        /** @copydoc MovableObject::_notifyCurrentCamera */
        void _notifyCurrentCamera( Camera* cam ) override;
        /** @copydoc MovableObject::getBoundingBox */
        auto getBoundingBox() const noexcept -> const AxisAlignedBox& override;
        /** @copydoc MovableObject::getBoundingRadius */
        auto getBoundingRadius() const -> Real override;

        void _updateRenderQueue(RenderQueue* queue) override;
        void visitRenderables( Renderable::Visitor* visitor, bool debugRenderables = false ) override;
    };
} // namespace Ogre
