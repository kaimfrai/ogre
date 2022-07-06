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
#ifndef OGRE_CORE_STATICGEOMETRY_H
#define OGRE_CORE_STATICGEOMETRY_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "OgreAxisAlignedBox.hpp"
#include "OgreCommon.hpp"
#include "OgreIteratorWrapper.hpp"
#include "OgreMaterial.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgreMesh.hpp"
#include "OgreMovableObject.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreQuaternion.hpp"
#include "OgreRenderable.hpp"
#include "OgreShadowCaster.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreVector.hpp"
#include "OgreVertexIndexData.hpp"

namespace Ogre {
class Camera;
class EdgeData;
class Entity;
class Light;
class LodStrategy;
class Matrix4;
class RenderOperation;
class RenderQueue;
class SceneManager;
class SceneNode;
class SubMesh;
class Technique;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */
    /** Pre-transforms and batches up meshes for efficient use as static
        geometry in a scene.
    @remarks
        Modern graphics cards (GPUs) prefer to receive geometry in large
        batches. It is orders of magnitude faster to render 10 batches
        of 10,000 triangles than it is to render 10,000 batches of 10 
        triangles, even though both result in the same number of on-screen
        triangles.
    @par
        Therefore it is important when you are rendering a lot of geometry to 
        batch things up into as few rendering calls as possible. This
        class allows you to build a batched object from a series of entities 
        in order to benefit from this behaviour.
        Batching has implications of it's own though:
        @li Batched geometry cannot be subdivided; that means that the whole
            group will be displayed, or none of it will. This obivously has
            culling issues.
        @li A single world transform must apply to the entire batch. Therefore
            once you have batched things, you can't move them around relative to
            each other. That's why this class is most useful when dealing with 
            static geometry (hence the name). In addition, geometry is 
            effectively duplicated, so if you add 3 entities based on the same 
            mesh in different positions, they will use 3 times the geometry 
            space than the movable version (which re-uses the same geometry). 
            So you trade memory and flexibility of movement for pure speed when
            using this class.
        @li A single material must apply for each batch. In fact this class 
            allows you to use multiple materials, but you should be aware that 
            internally this means that there is one batch per material. 
            Therefore you won't gain as much benefit from the batching if you 
            use many different materials; try to keep the number down.
    @par
        In order to retain some sort of culling, this class will batch up 
        meshes in localised regions. The size and shape of these blocks is
        controlled by the SceneManager which constructs this object, since it
        makes sense to batch things up in the most appropriate way given the 
        existing partitioning of the scene. 
    @par
        The LOD settings of both the Mesh and the Materials used in 
        constructing this static geometry will be respected. This means that 
        if you use meshes/materials which have LOD, batches in the distance 
        will have a lower polygon count or material detail to those in the 
        foreground. Since each mesh might have different LOD distances, during 
        build the furthest distance at each LOD level from all meshes  
        in that region is used. This means all the LOD levels change at the 
        same time, but at the furthest distance of any of them (so quality is 
        not degraded). Be aware that using Mesh LOD in this class will 
        further increase the memory required. Only generated LOD
        is supported for meshes.
    @par
        There are 2 ways you can add geometry to this class; you can add
        Entity objects directly with predetermined positions, scales and 
        orientations, or you can add an entire SceneNode and it's subtree, 
        including all the objects attached to it. Once you've added everything
        you need to, you have to call build() the fix the geometry in place. 
    @note
        This class is not a replacement for world geometry (@see 
        SceneManager::setWorldGeometry). The single most efficient way to 
        render large amounts of static geometry is to use a SceneManager which 
        is specialised for dealing with that particular world structure. 
        However, this class does provide you with a good 'halfway house'
        between generalised movable geometry (Entity) which works with all 
        SceneManagers but isn't efficient when using very large numbers, and 
        highly specialised world geometry which is extremely fast but not 
        generic and typically requires custom world editors.
    @par
        You should not construct instances of this class directly; instead, cal 
        SceneManager::createStaticGeometry, which gives the SceneManager the 
        option of providing you with a specialised version of this class if it
        wishes, and also handles the memory management for you like other 
        classes.
    @note
        Warning: this class only works with indexed triangle lists at the moment,
        do not pass it triangle strips, fans or lines / points, or unindexed geometry.
    */
    class StaticGeometry : public BatchedGeometryAlloc
    {
    public:
        /** Struct holding geometry optimised per SubMesh / LOD level, ready
            for copying to instances. 
        @remarks
            Since we're going to be duplicating geometry lots of times, it's
            far more important that we don't have redundant vertex data. If a 
            SubMesh uses shared geometry, or we're looking at a lower LOD, not
            all the vertices are being referenced by faces on that submesh.
            Therefore to duplicate them, potentially hundreds or even thousands
            of times, would be extremely wasteful. Therefore, if a SubMesh at
            a given LOD has wastage, we create an optimised version of it's
            geometry which is ready for copying with no wastage.
        */
        class OptimisedSubMeshGeometry : public BatchedGeometryAlloc
        {
        public:
            ::std::unique_ptr<VertexData> vertexData{nullptr};
            ::std::unique_ptr<IndexData> indexData{nullptr};
        };
        using OptimisedSubMeshGeometryList = std::list<OptimisedSubMeshGeometry *>;
        /// Saved link between SubMesh at a LOD and vertex/index data
        /// May point to original or optimised geometry
        struct SubMeshLodGeometryLink
        {
            VertexData* vertexData;
            IndexData* indexData;
        };
        using SubMeshLodGeometryLinkList = std::vector<SubMeshLodGeometryLink>;
        using SubMeshGeometryLookup = std::map<SubMesh *, SubMeshLodGeometryLinkList *>;
        /// Structure recording a queued submesh for the build
        struct QueuedSubMesh : public BatchedGeometryAlloc
        {
            SubMesh* submesh;
            MaterialPtr material;
            /// Link to LOD list of geometry, potentially optimised
            SubMeshLodGeometryLinkList* geometryLodList;
            Vector3 position;
            Quaternion orientation;
            Vector3 scale;
            /// Pre-transformed world AABB 
            AxisAlignedBox worldBounds;
        };
        using QueuedSubMeshList = std::vector<QueuedSubMesh *>;
        /// Structure recording a queued geometry for low level builds
        struct QueuedGeometry : public BatchedGeometryAlloc
        {
            SubMeshLodGeometryLink* geometry;
            Vector3 position;
            Quaternion orientation;
            Vector3 scale;
        };
        using QueuedGeometryList = std::vector<QueuedGeometry*>;
        
        // forward declarations
        class LODBucket;
        class MaterialBucket;
        class Region;

        /** A GeometryBucket is a the lowest level bucket where geometry with 
            the same vertex & index format is stored. It also acts as the 
            renderable.
        */
        class GeometryBucket :  public Renderable,  public BatchedGeometryAlloc
        {
            /// Geometry which has been queued up pre-build (not for deallocation)
            QueuedGeometryList mQueuedGeometry;
            /// Pointer to parent bucket
            MaterialBucket* mParent;
            /// Vertex information, includes current number of vertices
            /// committed to be a part of this bucket
            ::std::unique_ptr<VertexData> mVertexData;
            /// Index information, includes index type which limits the max
            /// number of vertices which are allowed in one bucket
            ::std::unique_ptr<IndexData> mIndexData;
            /// Maximum vertex indexable
            size_t mMaxVertexIndex;
        public:
            GeometryBucket(MaterialBucket* parent, const VertexData* vData, const IndexData* iData);
            ~GeometryBucket() override = default;
            auto getParent() noexcept -> MaterialBucket* { return mParent; }
            /// Get the vertex data for this geometry 
            [[nodiscard]] auto getVertexData() const noexcept -> const VertexData* { return mVertexData.get(); }
            /// Get the index data for this geometry 
            [[nodiscard]] auto getIndexData() const noexcept -> const IndexData* { return mIndexData.get(); }
            /// @copydoc Renderable::getMaterial
            [[nodiscard]] auto getMaterial() const noexcept -> const MaterialPtr& override;
            [[nodiscard]] auto getTechnique() const noexcept -> Technique* override;
            void getRenderOperation(RenderOperation& op) override;
            void getWorldTransforms(Matrix4* xform) const override;
            auto getSquaredViewDepth(const Camera* cam) const -> Real override;
            [[nodiscard]] auto getLights() const noexcept -> const LightList& override;
            [[nodiscard]] auto getCastsShadows() const noexcept -> bool override;
            
            /** Try to assign geometry to this bucket.
            @return false if there is no room left in this bucket
            */
            auto assign(QueuedGeometry* qsm) -> bool;
            /// Build
            void build(bool stencilShadows);
            /// Dump contents for diagnostics
            void dump(std::ofstream& of) const;
        };
        /** A MaterialBucket is a collection of smaller buckets with the same 
            Material (and implicitly the same LOD). */
        class MaterialBucket : public BatchedGeometryAlloc
        {
        public:
            /// list of Geometry Buckets in this region
            using GeometryBucketList = std::vector<::std::unique_ptr<GeometryBucket>>;
        private:
            /// Pointer to parent LODBucket
            LODBucket* mParent;
            /// Pointer to material being used
            MaterialPtr mMaterial;
            /// Active technique
            Technique* mTechnique{nullptr};

            /// list of Geometry Buckets in this region
            GeometryBucketList mGeometryBucketList;
            // index to current Geometry Buckets for a given geometry format
            using CurrentGeometryMap = std::map<uint32, GeometryBucket *>;
            CurrentGeometryMap mCurrentGeometryMap;
            
        public:
            MaterialBucket(LODBucket* parent, const MaterialPtr& material);
            virtual ~MaterialBucket() = default;
            auto getParent() noexcept -> LODBucket* { return mParent; }
            /// Get the material name
            [[nodiscard]] auto getMaterialName() const noexcept -> const String& { return mMaterial->getName(); }
            /// Assign geometry to this bucket
            void assign(QueuedGeometry* qsm);
            /// Build
            void build(bool stencilShadows);
            /// Add children to the render queue
            void addRenderables(RenderQueue* queue, uint8 group, 
                Real lodValue);
            /// Get the material for this bucket
            [[nodiscard]] auto getMaterial() const noexcept -> const MaterialPtr& { return mMaterial; }
            /// Override Material without changing the partitioning. For advanced use only.
            void _setMaterial(const MaterialPtr& material);
            /// Iterator over geometry
            using GeometryIterator = VectorIterator<GeometryBucketList>;
            /// Get a list of the contained geometry
            [[nodiscard]] auto getGeometryList() const noexcept -> const GeometryBucketList& { return mGeometryBucketList; }

            /// Get the current Technique
            [[nodiscard]] auto getCurrentTechnique() const noexcept -> Technique* { return mTechnique; }
            /// Dump contents for diagnostics
            void dump(std::ofstream& of) const;
            void visitRenderables(Renderable::Visitor* visitor, bool debugRenderables);
        };
        /** A LODBucket is a collection of smaller buckets with the same LOD. 
        @remarks
            LOD refers to Mesh LOD here. Material LOD can change separately
            at the next bucket down from this.
        */
        class LODBucket : public BatchedGeometryAlloc
        {
        public:
            /// Lookup of Material Buckets in this region
            using MaterialBucketMap = std::map<String, ::std::unique_ptr<MaterialBucket>>;
        private:
            /// Pointer to parent region
            Region* mParent;
            /// LOD level (0 == full LOD)
            unsigned short mLod;
            /// LOD value at which this LOD starts to apply (squared)
            Real mLodValue;
            /// Lookup of Material Buckets in this region
            MaterialBucketMap mMaterialBucketMap;
            /// Geometry queued for a single LOD (deallocated here)
            std::vector<::std::unique_ptr<QueuedGeometry>> mQueuedGeometryList;
            /// Edge list, used if stencil shadow casting is enabled 
            ::std::unique_ptr<EdgeData> mEdgeList{nullptr};
            /// Is a vertex program in use somewhere in this group?
            bool mVertexProgramInUse{false};
            /// List of shadow renderables
            ShadowCaster::ShadowRenderableList mShadowRenderables;
        public:
            LODBucket(Region* parent, unsigned short lod, Real lodValue);
            virtual ~LODBucket();
            auto getParent() noexcept -> Region* { return mParent; }
            /// Get the LOD index
            [[nodiscard]] auto getLod() const noexcept -> ushort { return mLod; }
            /// Get the LOD value
            [[nodiscard]] auto getLodValue() const noexcept -> Real { return mLodValue; }
            /// Assign a queued submesh to this bucket, using specified mesh LOD
            void assign(QueuedSubMesh* qsm, ushort atLod);
            /// Build
            void build(bool stencilShadows);
            /// Add children to the render queue
            void addRenderables(RenderQueue* queue, uint8 group, 
                Real lodValue);
            /// Iterator over the materials in this LOD
            using MaterialIterator = MapIterator<MaterialBucketMap>;
            /// Get an iterator over the materials in this LOD
            [[nodiscard]] auto getMaterialBuckets() const noexcept -> const MaterialBucketMap& { return mMaterialBucketMap; }

            /// Dump contents for diagnostics
            void dump(std::ofstream& of) const;
            void visitRenderables(Renderable::Visitor* visitor, bool debugRenderables);
            [[nodiscard]] auto getEdgeList() const noexcept -> EdgeData* { return mEdgeList.get(); }
            auto getShadowRenderableList() noexcept -> ShadowCaster::ShadowRenderableList& { return mShadowRenderables; }
            [[nodiscard]] auto isVertexProgramInUse() const noexcept -> bool { return mVertexProgramInUse; }
            void updateShadowRenderables(const Vector4& lightPos, const HardwareIndexBufferPtr& indexBuffer,
                                         Real extrusionDistance, int flags = 0);
        };
        /** The details of a topological region which is the highest level of
            partitioning for this class.
        @remarks
            The size & shape of regions entirely depends on the SceneManager
            specific implementation. It is a MovableObject since it will be
            attached to a node based on the local centre - in practice it
            won't actually move (although in theory it could).
        */
        class Region : public MovableObject
        {
            friend class MaterialBucket;
            friend class GeometryBucket;
        public:
            /// list of LOD Buckets in this region
            using LODBucketList = std::vector<::std::unique_ptr<LODBucket>>;
        private:
            /// Parent static geometry
            StaticGeometry* mParent;
            /// Local list of queued meshes (not used for deallocation)
            QueuedSubMeshList mQueuedSubMeshes;
            /// Unique identifier for the region
            uint32 mRegionID;
            /// Center of the region
            Vector3 mCentre;
            /// LOD values as built up - use the max at each level
            Mesh::LodValueList mLodValues;
            /// Local AABB relative to region centre
            AxisAlignedBox mAABB;
            /// Local bounding radius
            Real mBoundingRadius{0.0f};
            /// The current LOD level, as determined from the last camera
            ushort mCurrentLod{0};
            /// Current LOD value, passed on to do material LOD later
            Real mLodValue;
            /// List of LOD buckets         
            LODBucketList mLodBucketList;
            /// List of lights for this region
            mutable LightList mLightList;
            /// LOD strategy reference
            const LodStrategy *mLodStrategy{nullptr};
            /// Current camera
            Camera *mCamera{nullptr};
            /// Cached squared view depth value to avoid recalculation by GeometryBucket
            Real mSquaredViewDepth{0};

        public:
            Region(StaticGeometry* parent, const String& name, SceneManager* mgr, 
                uint32 regionID, const Vector3& centre);
            ~Region() override;
            // more fields can be added in subclasses
            auto getParent() const noexcept -> StaticGeometry* { return mParent;}
            /// Assign a queued mesh to this region, read for final build
            void assign(QueuedSubMesh* qmesh);
            /// Build this region
            void build(bool stencilShadows);
            /// Get the region ID of this region
            auto getID() const noexcept -> uint32 { return mRegionID; }
            /// Get the centre point of the region
            auto getCentre() const noexcept -> const Vector3& { return mCentre; }
            auto getMovableType() const noexcept -> const String& override;
            void _notifyCurrentCamera(Camera* cam) override;
            auto getBoundingBox() const noexcept -> const AxisAlignedBox& override;
            auto getBoundingRadius() const -> Real override;
            void _updateRenderQueue(RenderQueue* queue) override;
            /// @copydoc MovableObject::visitRenderables
            void visitRenderables(Renderable::Visitor* visitor, 
                bool debugRenderables = false) override;
            auto isVisible() const noexcept -> bool override;
            auto getTypeFlags() const noexcept -> uint32 override;

            using LODIterator = VectorIterator<LODBucketList>;

            /// Get an list of the LODs in this region
            auto getLODBuckets() const noexcept -> const LODBucketList& { return mLodBucketList; }
            auto
            getShadowVolumeRenderableList(const Light* light, const HardwareIndexBufferPtr& indexBuffer,
                                          size_t& indexBufferUsedSize, float extrusionDistance,
                                          int flags = 0) -> const ShadowRenderableList& override;
            auto getEdgeList() noexcept -> EdgeData* override;

            void _releaseManualHardwareResources() override;
            void _restoreManualHardwareResources() override;

            /// Dump contents for diagnostics
            void dump(std::ofstream& of) const;
            
        };
        /** Indexed region map based on packed x/y/z region index, 10 bits for
            each axis.
        @remarks
            Regions are indexed 0-1023 in all axes, where for example region 
            0 in the x axis begins at mOrigin.x + (mRegionDimensions.x * -512), 
            and region 1023 ends at mOrigin + (mRegionDimensions.x * 512).
        */
        using RegionMap = std::map<uint32, Region *>;
    private:
        // General state & settings
        SceneManager* mOwner;
        String mName;
        Real mUpperDistance{0.0f};
        Real mSquaredUpperDistance{0.0f};
        bool mCastShadows{false};
        Vector3 mRegionDimensions;
        Vector3 mHalfRegionDimensions;
        Vector3 mOrigin;
        bool mVisible{true};
        /// The render queue to use when rendering this object
        uint8 mRenderQueueID{RENDER_QUEUE_MAIN};
        /// Flags whether the RenderQueue's default should be used.
        bool mRenderQueueIDSet{false};
        /// Stores the visibility flags for the regions
        uint32 mVisibilityFlags;

        QueuedSubMeshList mQueuedSubMeshes;

        /// List of geometry which has been optimised for SubMesh use
        /// This is the primary storage used for cleaning up later
        OptimisedSubMeshGeometryList mOptimisedSubMeshGeometryList;

        /** Cached links from SubMeshes to (potentially optimised) geometry
            This is not used for deletion since the lookup may reference
            original vertex data
        */
        SubMeshGeometryLookup mSubMeshGeometryLookup;
            
        /// Map of regions
        RegionMap mRegionMap;

        /** Virtual method for getting a region most suitable for the
            passed in bounds. Can be overridden by subclasses.
        */
        virtual auto getRegion(const AxisAlignedBox& bounds, bool autoCreate) -> Region*;
        /** Get the region within which a point lies */
        virtual auto getRegion(const Vector3& point, bool autoCreate) -> Region*;
        /** Get the region using indexes */
        virtual auto getRegion(ushort x, ushort y, ushort z, bool autoCreate) -> Region*;
        /** Get the region using a packed index, returns null if it doesn't exist. */
        virtual auto getRegion(uint32 index) -> Region*;
        /** Get the region indexes for a point.
        */
        virtual void getRegionIndexes(const Vector3& point, 
            ushort& x, ushort& y, ushort& z);
        /** Pack 3 indexes into a single index value
        */
        virtual auto packIndex(ushort x, ushort y, ushort z) -> uint32;
        /** Get the volume intersection for an indexed region with some bounds.
        */
        virtual auto getVolumeIntersection(const AxisAlignedBox& box,  
            ushort x, ushort y, ushort z) -> Real;
        /** Get the bounds of an indexed region.
        */
        virtual auto getRegionBounds(ushort x, ushort y, ushort z) -> AxisAlignedBox;
        /** Get the centre of an indexed region.
        */
        virtual auto getRegionCentre(ushort x, ushort y, ushort z) -> Vector3;
        /** Calculate world bounds from a set of vertex data. */
        virtual auto calculateBounds(VertexData* vertexData, 
            const Vector3& position, const Quaternion& orientation, 
            const Vector3& scale) -> AxisAlignedBox;
        /** Look up or calculate the geometry data to use for this SubMesh */
        auto determineGeometry(SubMesh* sm) -> SubMeshLodGeometryLinkList*;
        /** Split some shared geometry into dedicated geometry. */
        void splitGeometry(VertexData* vd, IndexData* id, 
            SubMeshLodGeometryLink* targetGeomLink);

        using IndexRemap = std::map<size_t, size_t>;
        /** Method for figuring out which vertices are used by an index buffer
            and calculating a remap lookup for a vertex buffer just containing
            those vertices. 
        */
        template <typename T>
        void buildIndexRemap(T* pBuffer, size_t numIndexes, IndexRemap& remap)
        {
            remap.clear();
            for (size_t i = 0; i < numIndexes; ++i)
            {
                // use insert since duplicates are silently discarded
                remap.emplace(*pBuffer++, remap.size());
                // this will have mapped oldindex -> new index IF oldindex
                // wasn't already there
            }
        }
        /** Method for altering indexes based on a remap. */
        template <typename T>
        void remapIndexes(T* src, T* dst, const IndexRemap& remap, 
                size_t numIndexes)
        {
            for (size_t i = 0; i < numIndexes; ++i)
            {
                // look up original and map to target
                auto ix = remap.find(*src++);
                assert(ix != remap.end());
                *dst++ = static_cast<T>(ix->second);
            }
        }
        
    public:
        /// Constructor; do not use directly (@see SceneManager::createStaticGeometry)
        StaticGeometry(SceneManager* owner, std::string_view name);
        /// Destructor
        virtual ~StaticGeometry();

        /// Get the name of this object
        [[nodiscard]] auto getName() const noexcept -> const String& { return mName; }
        /** Adds an Entity to the static geometry.
        @remarks
            This method takes an existing Entity and adds its details to the 
            list of elements to include when building. Note that the Entity
            itself is not copied or referenced in this method; an Entity is 
            passed simply so that you can change the materials of attached 
            SubEntity objects if you want. You can add the same Entity 
            instance multiple times with different material settings 
            completely safely, and destroy the Entity before destroying 
            this StaticGeometry if you like. The Entity passed in is simply 
            used as a definition.
        @note Must be called before 'build'.
        @param ent The Entity to use as a definition (the Mesh and Materials 
            referenced will be recorded for the build call).
        @param position The world position at which to add this Entity
        @param orientation The world orientation at which to add this Entity
        @param scale The scale at which to add this entity
        */
        virtual void addEntity(Entity* ent, const Vector3& position,
            const Quaternion& orientation = Quaternion::IDENTITY, 
            const Vector3& scale = Vector3::UNIT_SCALE);

        /** Adds all the Entity objects attached to a SceneNode and all it's
            children to the static geometry.
        @remarks
            This method performs just like addEntity, except it adds all the 
            entities attached to an entire sub-tree to the geometry. 
            The position / orientation / scale parameters are taken from the
            node structure instead of being specified manually. 
        @note
            The SceneNode you pass in will not be automatically detached from 
            it's parent, so if you have this node already attached to the scene
            graph, you will need to remove it if you wish to avoid the overhead
            of rendering <i>both</i> the original objects and their new static
            versions! We don't do this for you incase you are preparing this 
            in advance and so don't want the originals detached yet. 
        @note Must be called before 'build'.
        @param node Pointer to the node to use to provide a set of Entity 
            templates
        */
        virtual void addSceneNode(const SceneNode* node);

        /** Build the geometry. 
        @remarks
            Based on all the entities which have been added, and the batching 
            options which have been set, this method constructs the batched 
            geometry structures required. The batches are added to the scene 
            and will be rendered unless you specifically hide them.
        @note
            Once you have called this method, you can no longer add any more 
            entities.
        */
        virtual void build();

        /** Destroys all the built geometry state (reverse of build). 
        @remarks
            You can call build() again after this and it will pick up all the
            same entities / nodes you queued last time.
        */
        virtual void destroy();

        /** Clears any of the entities / nodes added to this geometry and 
            destroys anything which has already been built.
        */
        virtual void reset();

        /** Sets the distance at which batches are no longer rendered.
        @remarks
            This lets you turn off batches at a given distance. This can be 
            useful for things like detail meshes (grass, foliage etc) and could
            be combined with a shader which fades the geometry out beforehand 
            to lessen the effect.
        @param dist Distance beyond which the batches will not be rendered 
            (the default is 0, which means batches are always rendered).
        */
        virtual void setRenderingDistance(Real dist) { 
            mUpperDistance = dist; 
            mSquaredUpperDistance = mUpperDistance * mUpperDistance;
        }

        /** Gets the distance at which batches are no longer rendered. */
        [[nodiscard]] virtual auto getRenderingDistance() const noexcept -> Real { return mUpperDistance; }

        /** Gets the squared distance at which batches are no longer rendered. */
        [[nodiscard]] virtual auto getSquaredRenderingDistance() const 
        noexcept -> Real { return mSquaredUpperDistance; }

        /** Hides or shows all the batches. */
        virtual void setVisible(bool visible);

        /** Are the batches visible? */
        [[nodiscard]] virtual auto isVisible() const noexcept -> bool { return mVisible; }

        /** Sets whether this geometry should cast shadows.
        @remarks
            No matter what the settings on the original entities,
            the StaticGeometry class defaults to not casting shadows. 
            This is because, being static, unless you have moving lights
            you'd be better to use precalculated shadows of some sort.
            However, if you need them, you can enable them using this
            method. If the SceneManager is set up to use stencil shadows,
            edge lists will be copied from the underlying meshes on build.
            It is essential that all meshes support stencil shadows in this
            case.
        @note If you intend to use stencil shadows, you must set this to 
            true before calling 'build' as well as making sure you set the
            scene's shadow type (that should always be the first thing you do
            anyway). You can turn shadows off temporarily but they can never 
            be turned on if they were not at the time of the build. 
        */
        virtual void setCastShadows(bool castShadows);
        /// Will the geometry from this object cast shadows?
        virtual auto getCastShadows() noexcept -> bool { return mCastShadows; }

        /** Sets the size of a single region of geometry.
        @remarks
            This method allows you to configure the physical world size of 
            each region, so you can balance culling against batch size. Entities
            will be fitted within the batch they most closely fit, and the 
            eventual bounds of each batch may well be slightly larger than this
            if they overlap a little. The default is Vector3(1000, 1000, 1000).
        @note Must be called before 'build'.
        @param size Vector3 expressing the 3D size of each region.
        */
        virtual void setRegionDimensions(const Vector3& size) { 
            mRegionDimensions = size; 
            mHalfRegionDimensions = size * 0.5;
        }
        /** Gets the size of a single batch of geometry. */
        [[nodiscard]] virtual auto getRegionDimensions() const noexcept -> const Vector3& { return mRegionDimensions; }
        /** Sets the origin of the geometry.
        @remarks
            This method allows you to configure the world centre of the geometry,
            thus the place which all regions surround. You probably don't need 
            to mess with this unless you have a seriously large world, since the
            default set up can handle an area 1024 * mRegionDimensions, and 
            the sparseness of population is no issue when it comes to rendering.
            The default is Vector3(0,0,0).
        @note Must be called before 'build'.
        @param origin Vector3 expressing the 3D origin of the geometry.
        */
        virtual void setOrigin(const Vector3& origin) { mOrigin = origin; }
        /** Gets the origin of this geometry. */
        [[nodiscard]] virtual auto getOrigin() const noexcept -> const Vector3& { return mOrigin; }

        /// Sets the visibility flags of all the regions at once
        void setVisibilityFlags(uint32 flags);
        /// Returns the visibility flags of the regions
        [[nodiscard]] auto getVisibilityFlags() const noexcept -> uint32;

        /** Sets the render queue group this object will be rendered through.
        @remarks
            Render queues are grouped to allow you to more tightly control the ordering
            of rendered objects. If you do not call this method, all  objects default
            to the default queue (RenderQueue::getDefaultQueueGroup), which is fine for 
            most objects. You may want to alter this if you want to perform more complex
            rendering.
        @par
            See RenderQueue for more details.
        @param queueID Enumerated value of the queue group to use.
        */
        virtual void setRenderQueueGroup(uint8 queueID);

        /** Gets the queue group for this entity, see setRenderQueueGroup for full details. */
        [[nodiscard]] virtual auto getRenderQueueGroup() const noexcept -> uint8;
        /// @copydoc MovableObject::visitRenderables
        void visitRenderables(Renderable::Visitor* visitor, 
            bool debugRenderables = false);
        
        /// Iterator for iterating over contained regions
        using RegionIterator = MapIterator<RegionMap>;
        /// Get an list of the regions in this geometry
        [[nodiscard]] auto getRegions() const noexcept -> const RegionMap& { return mRegionMap; }

        /** Dump the contents of this StaticGeometry to a file for diagnostic
            purposes.
        */
        virtual void dump(const String& filename) const;


    };

    /** Dummy factory to let Regions adhere to MovableObject protocol */
    class StaticGeometryFactory : public MovableObjectFactory
    {
        auto createInstanceImpl( const String& name, const NameValuePairList* params) noexcept -> MovableObject* override { return nullptr; }
    public:
        StaticGeometryFactory() = default;
        ~StaticGeometryFactory() override = default;

        static String FACTORY_TYPE_NAME;

        [[nodiscard]] auto getType() const noexcept -> const String& override { return FACTORY_TYPE_NAME; }
    };
    /** @} */
    /** @} */

}

#endif
