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
#ifndef OGRE_CORE_MESH_H
#define OGRE_CORE_MESH_H

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "OgreAnimation.hpp"
#include "OgreAnimationTrack.hpp"
#include "OgreAxisAlignedBox.hpp"
#include "OgreCommon.hpp"
#include "OgreHardwareBuffer.hpp"
#include "OgreHardwareVertexBuffer.hpp"
#include "OgreIteratorWrapper.hpp"
#include "OgrePose.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResource.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreVertexBoneAssignment.hpp"


namespace Ogre {


    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    struct MeshLodUsage;
    class LodStrategy;
class Affine3;
class AnimationStateSet;
class EdgeData;
class HardwareBufferManagerBase;
class IndexData;
class ResourceManager;
class SubMesh;
class VertexData;

    /** Resource holding data about 3D mesh.
    @remarks
        This class holds the data used to represent a discrete
        3-dimensional object. Mesh data usually contains more
        than just vertices and triangle information; it also
        includes references to materials (and the faces which use them),
        level-of-detail reduction information, convex hull definition,
        skeleton/bones information, keyframe animation etc.
        However, it is important to note the emphasis on the word
        'discrete' here. This class does not cover the large-scale
        sprawling geometry found in level / landscape data.
    @par
        Multiple world objects can (indeed should) be created from a
        single mesh object - see the Entity class for more info.
        The mesh object will have it's own default
        material properties, but potentially each world instance may
        wish to customise the materials from the original. When the object
        is instantiated into a scene node, the mesh material properties
        will be taken by default but may be changed. These properties
        are actually held at the SubMesh level since a single mesh may
        have parts with different materials.
    @par
        As described above, because the mesh may have sections of differing
        material properties, a mesh is inherently a compound construct,
        consisting of one or more SubMesh objects.
        However, it strongly 'owns' it's SubMeshes such that they
        are loaded / unloaded at the same time. This is contrary to
        the approach taken to hierarchically related (but loosely owned)
        scene nodes, where data is loaded / unloaded separately. Note
        also that mesh sub-sections (when used in an instantiated object)
        share the same scene node as the parent.
    */
    class Mesh: public Resource, public AnimationContainer
    {
        friend class SubMesh;
        friend class MeshSerializerImpl;
        friend class MeshSerializerImpl_v1_8;
        friend class MeshSerializerImpl_v1_4;
        friend class MeshSerializerImpl_v1_3;
        friend class MeshSerializerImpl_v1_2;
        friend class MeshSerializerImpl_v1_1;

    public:
        using LodValueList = std::vector<Real>;
        using MeshLodUsageList = std::vector<MeshLodUsage>;
        /// Multimap of vertex bone assignments (orders by vertex index).
        using VertexBoneAssignmentList = std::multimap<size_t, VertexBoneAssignment>;
        using BoneAssignmentIterator = MapIterator<VertexBoneAssignmentList>;
        using SubMeshList = std::vector<SubMesh *>;
        using IndexMap = std::vector<unsigned short>;

    private:
        /** A list of submeshes which make up this mesh.
            Each mesh is made up of 1 or more submeshes, which
            are each based on a single material and can have their
            own vertex data (they may not - they can share vertex data
            from the Mesh, depending on preference).
        */
        SubMeshList mSubMeshList;
    
        /** Internal method for making the space for a vertex element to hold tangents. */
        void organiseTangentsBuffer(VertexData *vertexData, 
            VertexElementSemantic targetSemantic, unsigned short index, 
            unsigned short sourceTexCoordSet);

    public:
        /** A hashmap used to store optional SubMesh names.
            Translates a name into SubMesh index.
        */
        using SubMeshNameMap = std::unordered_map<std::string_view, ushort> ;

        
    private:

        DataStreamPtr mFreshFromDisk;

        SubMeshNameMap mSubMeshNameMap ;

        /// Local bounding box volume.
        AxisAlignedBox mAABB;
        /// Local bounding sphere radius (centered on object).
        Real mBoundRadius{0.0f};
        /// Largest bounding radius of any bone in the skeleton (centered on each bone, only considering verts weighted to the bone)
        Real mBoneBoundingRadius{0.0f};

        /// Optional linked skeleton.
        SkeletonPtr mSkeleton;
       
        VertexBoneAssignmentList mBoneAssignments;

        /// Flag indicating that bone assignments need to be recompiled.
        bool mBoneAssignmentsOutOfDate{false};

        /** Build the index map between bone index and blend index. */
        void buildIndexMap(const VertexBoneAssignmentList& boneAssignments,
            IndexMap& boneIndexToBlendIndexMap, IndexMap& blendIndexToBoneIndexMap);
        /** Compile bone assignments into blend index and weight buffers. */
        void compileBoneAssignments(const VertexBoneAssignmentList& boneAssignments,
            unsigned short numBlendWeightsPerVertex, 
            IndexMap& blendIndexToBoneIndexMap,
            VertexData* targetVertexData);

        const LodStrategy *mLodStrategy;
        bool mHasManualLodLevel{false};
        ushort mNumLods{1};
        MeshLodUsageList mMeshLodUsageList;

        HardwareBufferManagerBase* mBufferManager{nullptr};
        HardwareBuffer::Usage mVertexBufferUsage{HardwareBuffer::HBU_STATIC_WRITE_ONLY};
        HardwareBuffer::Usage mIndexBufferUsage{HardwareBuffer::HBU_STATIC_WRITE_ONLY};
        bool mVertexBufferShadowBuffer{false};
        bool mIndexBufferShadowBuffer{false};


        bool mPreparedForShadowVolumes{false};
        bool mEdgeListsBuilt{false};
        bool mAutoBuildEdgeLists{true};

        /// Storage of morph animations, lookup by name
        using AnimationList = std::map<std::string_view, Animation *>;
        AnimationList mAnimationsList;
        /// The vertex animation type associated with the shared vertex data
        mutable VertexAnimationType mSharedVertexDataAnimationType{VAT_NONE};
        /// Whether vertex animation includes normals
        mutable bool mSharedVertexDataAnimationIncludesNormals{false};
        /// Do we need to scan animations for animation types?
        mutable bool mAnimationTypesDirty{true};

        /// List of available poses for shared and dedicated geometryPoseList
        PoseList mPoseList;
        mutable bool mPosesIncludeNormals{false};


        /** Loads the mesh from disk.  This call only performs IO, it
            does not parse the bytestream or check for any errors therein.
            It also does not set up submeshes, etc.  You have to call load()
            to do that.
         */
        void prepareImpl() override;
        /** Destroys data cached by prepareImpl.
         */
        void unprepareImpl() override;
        /// @copydoc Resource::loadImpl
        void loadImpl() override;
        /// @copydoc Resource::postLoadImpl
        void postLoadImpl() override;
        /// @copydoc Resource::unloadImpl
        void unloadImpl() override;
        /// @copydoc Resource::calculateSize
        auto calculateSize() const -> size_t override;

        void mergeAdjacentTexcoords( unsigned short finalTexCoordSet,
                                     unsigned short texCoordSetToDestroy, VertexData *vertexData );


    public:
        /** Default constructor - used by MeshManager
        @warning
            Do not call this method directly.
        */
        Mesh(ResourceManager* creator, std::string_view name, ResourceHandle handle,
            std::string_view group, bool isManual = false, ManualResourceLoader* loader = nullptr);
        ~Mesh() override;

        // NB All methods below are non-virtual since they will be
        // called in the rendering loop - speed is of the essence.

        /** Creates a new SubMesh.
        @remarks
            Method for manually creating geometry for the mesh.
            Note - use with extreme caution - you must be sure that
            you have set up the geometry properly.
        */
        auto createSubMesh() -> SubMesh*;

        /** Creates a new SubMesh and gives it a name
        */
        auto createSubMesh(std::string_view name) -> SubMesh*;
        
        /** Gives a name to a SubMesh
        */
        void nameSubMesh(std::string_view name, ushort index);

        /** Removes a name from a SubMesh
        */
        void unnameSubMesh(std::string_view name);
        
        /** Gets the index of a submesh with a given name.
        @remarks
            Useful if you identify the SubMeshes by name (using nameSubMesh)
            but wish to have faster repeat access.
        */
        auto _getSubMeshIndex(std::string_view name) const -> ushort;

        /** Gets the number of sub meshes which comprise this mesh.
        *  @deprecated use getSubMeshes() instead
        */
        auto getNumSubMeshes() const -> size_t {
            return mSubMeshList.size();
        }

        /** Gets a pointer to the submesh indicated by the index.
        *  @deprecated use getSubMeshes() instead
        */
        auto getSubMesh(size_t index) const -> SubMesh* {
            return mSubMeshList[index];
        }

        /** Gets a SubMesh by name
        */
        auto getSubMesh(std::string_view name) const -> SubMesh* ;
        
        /** Destroy a SubMesh with the given index. 
        @note
            This will invalidate the contents of any existing Entity, or
            any other object that is referring to the SubMesh list. Entity will
            detect this and reinitialise, but it is still a disruptive action.
        */
        void destroySubMesh(unsigned short index);

        /** Destroy a SubMesh with the given name. 
        @note
            This will invalidate the contents of any existing Entity, or
            any other object that is referring to the SubMesh list. Entity will
            detect this and reinitialise, but it is still a disruptive action.
        */
        void destroySubMesh(std::string_view name);
        
        using SubMeshIterator = VectorIterator<SubMeshList>;
      
        /// Gets the available submeshes
        auto getSubMeshes() const noexcept -> const SubMeshList& {
            return mSubMeshList;
        }

        /** Shared vertex data.
        @remarks
            This vertex data can be shared among multiple submeshes. SubMeshes may not have
            their own VertexData, they may share this one.
        @par
            The use of shared or non-shared buffers is determined when
            model data is converted to the OGRE .mesh format.
        */
        VertexData *sharedVertexData{nullptr};

        /** Shared index map for translating blend index to bone index.
        @remarks
            This index map can be shared among multiple submeshes. SubMeshes might not have
            their own IndexMap, they might share this one.
        @par
            We collect actually used bones of all bone assignments, and build the
            blend index in 'packed' form, then the range of the blend index in vertex
            data VES_BLEND_INDICES element is continuous, with no gaps. Thus, by
            minimising the world matrix array constants passing to GPU, we can support
            more bones for a mesh when hardware skinning is used. The hardware skinning
            support limit is applied to each set of vertex data in the mesh, in other words, the
            hardware skinning support limit is applied only to the actually used bones of each
            SubMeshes, not all bones across the entire Mesh.
        @par
            Because the blend index is different to the bone index, therefore, we use
            the index map to translate the blend index to bone index.
        @par
            The use of shared or non-shared index map is determined when
            model data is converted to the OGRE .mesh format.
        */
        IndexMap sharedBlendIndexToBoneIndexMap;

        /** Makes a copy of this mesh object and gives it a new name.
        @remarks
            This is useful if you want to tweak an existing mesh without affecting the original one. The
            newly cloned mesh is registered with the MeshManager under the new name.
        @param newName
            The name to give the clone.
        @param newGroup
            Optional name of the new group to assign the clone to;
            if you leave this blank, the clone will be assigned to the same
            group as this Mesh.
        */
        auto clone(std::string_view newName, std::string_view newGroup = "") -> MeshPtr;

        /** @copydoc Resource::reload */
        void reload(LoadingFlags flags = LF_DEFAULT) override;

        /** Get the axis-aligned bounding box for this mesh.
        */
        auto getBounds() const noexcept -> const AxisAlignedBox&;

        /** Gets the radius of the bounding sphere surrounding this mesh. */
        auto getBoundingSphereRadius() const -> Real;

        /** Gets the radius used to inflate the bounding box around the bones. */
        auto getBoneBoundingRadius() const -> Real;

        /** Manually set the bounding box for this Mesh.
        @remarks
            Calling this method is required when building manual meshes now, because OGRE can no longer 
            update the bounds for you, because it cannot necessarily read vertex data back from 
            the vertex buffers which this mesh uses (they very well might be write-only, and even
            if they are not, reading data from a hardware buffer is a bottleneck).
            @param bounds The axis-aligned bounding box for this mesh
            @param pad If true, a certain padding will be added to the bounding box to separate it from the mesh
        */
        void _setBounds(const AxisAlignedBox& bounds, bool pad = true);

        /** Manually set the bounding radius. 
        @remarks
            Calling this method is required when building manual meshes now, because OGRE can no longer 
            update the bounds for you, because it cannot necessarily read vertex data back from 
            the vertex buffers which this mesh uses (they very well might be write-only, and even
            if they are not, reading data from a hardware buffer is a bottleneck).
        */
        void _setBoundingSphereRadius(Real radius);

        /** Manually set the bone bounding radius. 
        @remarks
            This value is normally computed automatically, however it can be overriden with this method.
        */
        void _setBoneBoundingRadius(Real radius);

        /** Compute the bone bounding radius by looking at the vertices, vertex-bone-assignments, and skeleton bind pose.
        @remarks
            This is automatically called by Entity if necessary.  Only does something if the boneBoundingRadius is zero to
            begin with.  Only works if vertex data is readable (i.e. not WRITE_ONLY).
        */
        void _computeBoneBoundingRadius();

        /** Automatically update the bounding radius and bounding box for this Mesh.
        @remarks
        Calling this method is required when building manual meshes. However it is recommended to
        use _setBounds and _setBoundingSphereRadius instead, because the vertex buffer may not have
        a shadow copy in the memory. Reading back the buffer from video memory is very slow!
        @param pad If true, a certain padding will be added to the bounding box to separate it from the mesh
        */
        void _updateBoundsFromVertexBuffers(bool pad = false);

        /** Calculates 
        @remarks
        Calling this method is required when building manual meshes. However it is recommended to
        use _setBounds and _setBoundingSphereRadius instead, because the vertex buffer may not have
        a shadow copy in the memory. Reading back the buffer from video memory is very slow!
        */
        void _calcBoundsFromVertexBuffer(VertexData* vertexData, AxisAlignedBox& outAABB, Real& outRadius, bool updateOnly = false);
        /** Sets the name of the skeleton this Mesh uses for animation.
        @remarks
            Meshes can optionally be assigned a skeleton which can be used to animate
            the mesh through bone assignments. The default is for the Mesh to use no
            skeleton. Calling this method with a valid skeleton filename will cause the
            skeleton to be loaded if it is not already (a single skeleton can be shared
            by many Mesh objects).
        @param skelName
            The name of the .skeleton file to use, or an empty string to use
            no skeleton
        */
        void setSkeletonName(std::string_view skelName);

        /** Returns true if this Mesh has a linked Skeleton. */
        auto hasSkeleton() const -> bool { return mSkeleton != nullptr; }

        /** Returns whether or not this mesh has some kind of vertex animation. 
        */
        auto hasVertexAnimation() const -> bool;
        
        /** Gets a pointer to any linked Skeleton. 
        @return
            Weak reference to the skeleton - copy this if you want to hold a strong pointer.
        */
        auto getSkeleton() const noexcept -> const SkeletonPtr& { return mSkeleton; }

        /** Gets the name of any linked Skeleton */
        auto getSkeletonName() const noexcept -> std::string_view;
        /** Initialise an animation set suitable for use with this mesh. 
        @remarks
            Only recommended for use inside the engine, not by applications.
        */
        void _initAnimationState(AnimationStateSet* animSet);

        /** Refresh an animation set suitable for use with this mesh. 
        @remarks
            Only recommended for use inside the engine, not by applications.
        */
        void _refreshAnimationState(AnimationStateSet* animSet);
        /** Assigns a vertex to a bone with a given weight, for skeletal animation. 
        @remarks    
            This method is only valid after calling setSkeletonName.
            Since this is a one-off process there exists only 'addBoneAssignment' and
            'clearBoneAssignments' methods, no 'editBoneAssignment'. You should not need
            to modify bone assignments during rendering (only the positions of bones) and OGRE
            reserves the right to do some internal data reformatting of this information, depending
            on render system requirements.
        @par
            This method is for assigning weights to the shared geometry of the Mesh. To assign
            weights to the per-SubMesh geometry, see the equivalent methods on SubMesh.
        */
        void addBoneAssignment(const VertexBoneAssignment& vertBoneAssign);

        /** Removes all bone assignments for this mesh. 
        @remarks
            This method is for modifying weights to the shared geometry of the Mesh. To assign
            weights to the per-SubMesh geometry, see the equivalent methods on SubMesh.
        */
        void clearBoneAssignments();

        /** Internal notification, used to tell the Mesh which Skeleton to use without loading it. 
        @remarks
            This is only here for unusual situation where you want to manually set up a
            Skeleton. Best to let OGRE deal with this, don't call it yourself unless you
            really know what you're doing.
        */
        void _notifySkeleton(const SkeletonPtr& pSkel);

        /** Gets a const reference to the list of bone assignments
        */
        auto getBoneAssignments() const noexcept -> const VertexBoneAssignmentList& { return mBoneAssignments; }

        /** Returns the number of levels of detail that this mesh supports. 
        @remarks
            This number includes the original model.
        */
        auto getNumLodLevels() const noexcept -> ushort { return mNumLods; }
        /** Gets details of the numbered level of detail entry. */
        auto getLodLevel(ushort index) const -> const MeshLodUsage&;

        /** Retrieves the level of detail index for the given LOD value. 
        @note
            The value passed in is the 'transformed' value. If you are dealing with
            an original source value (e.g. distance), use LodStrategy::transformUserValue
            to turn this into a lookup value.
        */
        auto getLodIndex(Real value) const -> ushort;

        /** Returns true if this mesh has a manual LOD level.
        @remarks
            A mesh can either use automatically generated LOD, or it can use alternative
            meshes as provided by an artist.
        */
        auto hasManualLodLevel() const noexcept -> bool { return mHasManualLodLevel; }

        /** Changes the alternate mesh to use as a manual LOD at the given index.
        @remarks
            Note that the index of a LOD may change if you insert other LODs. If in doubt,
            use getLodIndex().
        @param index
            The index of the level to be changed.
        @param meshName
            The name of the mesh which will be the lower level detail version.
        */
        void updateManualLodLevel(ushort index, std::string_view meshName);

        /** Internal methods for loading LOD, do not use. */
        void _setLodInfo(unsigned short numLevels);
        /** Internal methods for loading LOD, do not use. */
        void _setLodUsage(unsigned short level, const MeshLodUsage& usage);
        /** Internal methods for loading LOD, do not use. */
        void _setSubMeshLodFaceList(unsigned short subIdx, unsigned short level, IndexData* facedata);

        /** Internal methods for loading LOD, do not use. */
        auto _isManualLodLevel(unsigned short level) const -> bool;


        /** Removes all LOD data from this Mesh. */
        void removeLodLevels();

        /** Sets the manager for the vertex and index buffers to be used when loading
            this Mesh.
        @remarks
            By default, when loading the Mesh, static, write-only vertex and index buffers 
            will be used where possible in order to improve rendering performance. 
            However, such buffers cannot be manipulated on the fly by CPU code 
            (although shader code can). If you wish to use the CPU to modify these buffers
            and will never use it with GPU, you should call this method. Note,
            however, that it only takes effect after the Mesh has been reloaded. Note that you
            still have the option of manually repacing the buffers in this mesh with your
            own if you see fit too, in which case you don't need to call this method since it
            only affects buffers created by the mesh itself.
        @par
            You can define the approach to a Mesh by changing the default parameters to 
            MeshManager::load if you wish; this means the Mesh is loaded with those options
            the first time instead of you having to reload the mesh after changing these options.
        @param bufferManager
            If set to @c DefaultHardwareBufferManager, the buffers will be created in system memory
            only, without hardware counterparts. Such mesh could not be rendered, but LODs could be
            generated for such mesh, it could be cloned, transformed and serialized.
        */
        void setHardwareBufferManager(HardwareBufferManagerBase* bufferManager) { mBufferManager = bufferManager; }
        auto getHardwareBufferManager() noexcept -> HardwareBufferManagerBase*;
        /** Sets the policy for the vertex buffers to be used when loading
            this Mesh.
        @remarks
            By default, when loading the Mesh, static, write-only vertex and index buffers 
            will be used where possible in order to improve rendering performance. 
            However, such buffers
            cannot be manipulated on the fly by CPU code (although shader code can). If you
            wish to use the CPU to modify these buffers, you should call this method. Note,
            however, that it only takes effect after the Mesh has been reloaded. Note that you
            still have the option of manually repacing the buffers in this mesh with your
            own if you see fit too, in which case you don't need to call this method since it
            only affects buffers created by the mesh itself.
        @par
            You can define the approach to a Mesh by changing the default parameters to 
            MeshManager::load if you wish; this means the Mesh is loaded with those options
            the first time instead of you having to reload the mesh after changing these options.
        @param usage
            The usage flags, which by default are 
            HardwareBuffer::HBU_STATIC_WRITE_ONLY
        @param shadowBuffer
            If set to @c true, the vertex buffers will be created with a
            system memory shadow buffer. You should set this if you want to be able to
            read from the buffer, because reading from a hardware buffer is a no-no.
        */
        void setVertexBufferPolicy(HardwareBuffer::Usage usage, bool shadowBuffer = false);
        /** Sets the policy for the index buffers to be used when loading
            this Mesh.
        @remarks
            By default, when loading the Mesh, static, write-only vertex and index buffers 
            will be used where possible in order to improve rendering performance. 
            However, such buffers
            cannot be manipulated on the fly by CPU code (although shader code can). If you
            wish to use the CPU to modify these buffers, you should call this method. Note,
            however, that it only takes effect after the Mesh has been reloaded. Note that you
            still have the option of manually repacing the buffers in this mesh with your
            own if you see fit too, in which case you don't need to call this method since it
            only affects buffers created by the mesh itself.
        @par
            You can define the approach to a Mesh by changing the default parameters to 
            MeshManager::load if you wish; this means the Mesh is loaded with those options
            the first time instead of you having to reload the mesh after changing these options.
        @param usage
            The usage flags, which by default are 
            HardwareBuffer::HBU_STATIC_WRITE_ONLY
        @param shadowBuffer
            If set to @c true, the index buffers will be created with a
            system memory shadow buffer. You should set this if you want to be able to
            read from the buffer, because reading from a hardware buffer is a no-no.
        */
        void setIndexBufferPolicy(HardwareBuffer::Usage usage, bool shadowBuffer = false);
        /** Gets the usage setting for this meshes vertex buffers. */
        auto getVertexBufferUsage() const noexcept -> HardwareBuffer::Usage { return mVertexBufferUsage; }
        /** Gets the usage setting for this meshes index buffers. */
        auto getIndexBufferUsage() const noexcept -> HardwareBuffer::Usage { return mIndexBufferUsage; }
        /** Gets whether or not this meshes vertex buffers are shadowed. */
        auto isVertexBufferShadowed() const noexcept -> bool { return mVertexBufferShadowBuffer; }
        /** Gets whether or not this meshes index buffers are shadowed. */
        auto isIndexBufferShadowed() const noexcept -> bool { return mIndexBufferShadowBuffer; }
       

        /** Rationalises the passed in bone assignment list.
        @remarks
            OGRE supports up to 4 bone assignments per vertex. The reason for this limit
            is that this is the maximum number of assignments that can be passed into
            a hardware-assisted blending algorithm. This method identifies where there are
            more than 4 bone assignments for a given vertex, and eliminates the bone
            assignments with the lowest weights to reduce to this limit. The remaining
            weights are then re-balanced to ensure that they sum to 1.0.
        @param vertexCount
            The number of vertices.
        @param assignments
            The bone assignment list to rationalise. This list will be modified and
            entries will be removed where the limits are exceeded.
        @return
            The maximum number of bone assignments per vertex found, clamped to [1-4]
        */
        auto _rationaliseBoneAssignments(size_t vertexCount, VertexBoneAssignmentList& assignments) -> unsigned short;

        /** Internal method, be called once to compile bone assignments into geometry buffer. 
        @remarks
            The OGRE engine calls this method automatically. It compiles the information 
            submitted as bone assignments into a format usable in realtime. It also 
            eliminates excessive bone assignments (max is OGRE_MAX_BLEND_WEIGHTS)
            and re-normalises the remaining assignments.
        */
        void _compileBoneAssignments();

        /** Internal method, be called once to update the compiled bone assignments.
        @remarks
            The OGRE engine calls this method automatically. It updates the compiled bone
            assignments if requested.
        */
        void _updateCompiledBoneAssignments();

        /** This method collapses two texcoords into one for all submeshes where this is possible.
        @remarks
            Often a submesh can have two tex. coords. (i.e. TEXCOORD0 & TEXCOORD1), being both
            composed of two floats. There are many practical reasons why it would be more convenient
            to merge both of them into one TEXCOORD0 of 4 floats. This function does exactly that
            The finalTexCoordSet must have enough space for the merge, or else the submesh will be
            skipped. (i.e. you can't merge a tex. coord with 3 floats with one having 2 floats)

            finalTexCoordSet & texCoordSetToDestroy must be in the same buffer source, and must
            be adjacent.
        @param finalTexCoordSet The tex. coord index to merge to. Should have enough space to
            actually work.
        @param texCoordSetToDestroy The texture coordinate index that will disappear on
            successful merges.
        */
        void mergeAdjacentTexcoords( unsigned short finalTexCoordSet, unsigned short texCoordSetToDestroy );

        /** This method builds a set of tangent vectors for a given mesh into a 3D texture coordinate buffer.
        @remarks
            Tangent vectors are vectors representing the local 'X' axis for a given vertex based
            on the orientation of the 2D texture on the geometry. They are built from a combination
            of existing normals, and from the 2D texture coordinates already baked into the model.
            They can be used for a number of things, but most of all they are useful for 
            vertex and fragment programs, when you wish to arrive at a common space for doing
            per-pixel calculations.
        @par
            The prerequisites for calling this method include that the vertex data used by every
            SubMesh has both vertex normals and 2D texture coordinates.
        @param targetSemantic
            The semantic to store the tangents in. Defaults to 
            the explicit tangent binding, but note that this is only usable on more
            modern hardware (Shader Model 2), so if you need portability with older
            cards you should change this to a texture coordinate binding instead.
        @param sourceTexCoordSet
            The texture coordinate index which should be used as the source
            of 2D texture coordinates, with which to calculate the tangents.
        @param index
            The element index, ie the texture coordinate set which should be used to store the 3D
            coordinates representing a tangent vector per vertex, if targetSemantic is 
            VES_TEXTURE_COORDINATES. If this already exists, it will be overwritten.
        @param splitMirrored
            Sets whether or not to split vertices when a mirrored tangent space
            transition is detected (matrix parity differs). @see TangentSpaceCalc::setSplitMirrored
        @param splitRotated
            Sets whether or not to split vertices when a rotated tangent space
            is detected. @see TangentSpaceCalc::setSplitRotated
        @param storeParityInW
            If @c true, store tangents as a 4-vector and include parity in w.
        */
        void buildTangentVectors(VertexElementSemantic targetSemantic = VES_TANGENT,
            unsigned short sourceTexCoordSet = 0, unsigned short index = 0, 
            bool splitMirrored = false, bool splitRotated = false, bool storeParityInW = false);

        /** Ask the mesh to suggest parameters to a future buildTangentVectors call, 
            should you wish to use texture coordinates to store the tangents. 
        @remarks
            This helper method will suggest source and destination texture coordinate sets
            for a call to buildTangentVectors. It will detect when there are inappropriate
            conditions (such as multiple geometry sets which don't agree). 
            Moreover, it will return 'true' if it detects that there are aleady 3D 
            coordinates in the mesh, and therefore tangents may have been prepared already.
        @param targetSemantic
            The semantic you intend to use to store the tangents
            if they are not already present;
            most likely options are VES_TEXTURE_COORDINATES or VES_TANGENT; you should
            use texture coordinates if you want compatibility with older, pre-SM2
            graphics cards, and the tangent binding otherwise.
        @param outSourceCoordSet
            Reference to a source texture coordinate set which 
            will be populated.
        @param outIndex
            Reference to a destination element index (e.g. texture coord set)
            which will be populated
        */
        auto suggestTangentVectorBuildParams(VertexElementSemantic targetSemantic,
            unsigned short& outSourceCoordSet, unsigned short& outIndex) -> bool;

        /** Builds an edge list for this mesh, which can be used for generating a shadow volume
            among other things.
        */
        void buildEdgeList();
        /** Destroys and frees the edge lists this mesh has built. */
        void freeEdgeList();

        /// @copydoc VertexData::prepareForShadowVolume
        void prepareForShadowVolume();

        /** Return the edge list for this mesh, building it if required. 
        @remarks
            You must ensure that the Mesh as been prepared for shadow volume 
            rendering if you intend to use this information for that purpose.
        @param lodIndex
            The LOD at which to get the edge list, 0 being the highest.
        */
        auto getEdgeList(unsigned short lodIndex = 0) -> EdgeData*;

        /** Return the edge list for this mesh, building it if required. 
        @remarks
            You must ensure that the Mesh as been prepared for shadow volume 
            rendering if you intend to use this information for that purpose.
        @param lodIndex
            The LOD at which to get the edge list, 0 being the highest.
        */
        auto getEdgeList(unsigned short lodIndex = 0) const -> const EdgeData*;

        /** Returns whether this mesh has already had it's geometry prepared for use in 
            rendering shadow volumes. */
        auto isPreparedForShadowVolumes() const noexcept -> bool { return mPreparedForShadowVolumes; }

        /** Returns whether this mesh has an attached edge list. */
        auto isEdgeListBuilt() const noexcept -> bool { return mEdgeListsBuilt; }

        /** Prepare matrices for software indexed vertex blend.
        @remarks
            This function organise bone indexed matrices to blend indexed matrices,
            so software vertex blending can access to the matrix via blend index
            directly.
        @param blendMatrices
            Pointer to an array of matrix pointers to store
            prepared results, which indexed by blend index.
        @param boneMatrices
            Pointer to an array of matrices to be used to blend,
            which indexed by bone index.
        @param indexMap
            The index map used to translate blend index to bone index.
        */
        static void prepareMatricesForVertexBlend(const Affine3** blendMatrices,
            const Affine3* boneMatrices, const IndexMap& indexMap);

        /** Performs a software indexed vertex blend, of the kind used for
            skeletal animation although it can be used for other purposes. 
        @remarks
            This function is supplied to update vertex data with blends 
            done in software, either because no hardware support is available, 
            or that you need the results of the blend for some other CPU operations.
        @param sourceVertexData
            VertexData class containing positions, normals,
            blend indices and blend weights.
        @param targetVertexData
            VertexData class containing target position
            and normal buffers which will be updated with the blended versions.
            Note that the layout of the source and target position / normal 
            buffers must be identical, ie they must use the same buffer indexes
        @param blendMatrices
            Pointer to an array of matrix pointers to be used to blend,
            indexed by blend indices in the sourceVertexData
        @param numMatrices
            Number of matrices in the blendMatrices, it might be used
            as a hint for optimisation.
        @param blendNormals
            If @c true, normals are blended as well as positions.
        */
        static void softwareVertexBlend(const VertexData* sourceVertexData, 
            const VertexData* targetVertexData,
            const Affine3* const* blendMatrices, size_t numMatrices,
            bool blendNormals);

        /** Performs a software vertex morph, of the kind used for
            morph animation although it can be used for other purposes. 
        @remarks
            This function will linearly interpolate positions between two
            source buffers, into a third buffer.
        @param t
            Parametric distance between the start and end buffer positions.
        @param b1
            Vertex buffer containing VET_FLOAT3 entries for the start positions.
        @param b2
            Vertex buffer containing VET_FLOAT3 entries for the end positions.
        @param targetVertexData
            VertexData destination; assumed to have a separate position
            buffer already bound, and the number of vertices must agree with the
            number in start and end
        */
        static void softwareVertexMorph(Real t, 
            const HardwareVertexBufferSharedPtr& b1, 
            const HardwareVertexBufferSharedPtr& b2, 
            VertexData* targetVertexData);

        /** Performs a software vertex pose blend, of the kind used for
            morph animation although it can be used for other purposes. 
        @remarks
            This function will apply a weighted offset to the positions in the 
            incoming vertex data (therefore this is a read/write operation, and 
            if you expect to call it more than once with the same data, then
            you would be best to suppress hardware uploads of the position buffer
            for the duration).
        @param weight
            Parametric weight to scale the offsets by.
        @param vertexOffsetMap
            Potentially sparse map of vertex index -> offset.
        @param normalsMap
            Potentially sparse map of vertex index -> normal.
        @param targetVertexData 
            VertexData destination; assumed to have a separate position
            buffer already bound, and the number of vertices must agree with the
            number in start and end.
        */
        static void softwareVertexPoseBlend(Real weight, 
            const std::map<size_t, Vector3>& vertexOffsetMap,
            const std::map<size_t, Vector3>& normalsMap,
            VertexData* targetVertexData);
        /** Gets a reference to the optional name assignments of the SubMeshes. */
        auto getSubMeshNameMap() const noexcept -> const SubMeshNameMap& { return mSubMeshNameMap; }

        /** Sets whether or not this Mesh should automatically build edge lists
            when asked for them, or whether it should never build them if
            they are not already provided.
        @remarks
            This allows you to create meshes which do not have edge lists calculated, 
            because you never want to use them. This value defaults to 'true'
            for mesh formats which did not include edge data, and 'false' for 
            newer formats, where edge lists are expected to have been generated
            in advance.
        */
        void setAutoBuildEdgeLists(bool autobuild) { mAutoBuildEdgeLists = autobuild; }
        /** Sets whether or not this Mesh should automatically build edge lists
            when asked for them, or whether it should never build them if
            they are not already provided.
        */
        auto getAutoBuildEdgeLists() const noexcept -> bool { return mAutoBuildEdgeLists; }

        /** Gets the type of vertex animation the shared vertex data of this mesh supports.
        */
        virtual auto getSharedVertexDataAnimationType() const -> VertexAnimationType;

        /// Returns whether animation on shared vertex data includes normals.
        auto getSharedVertexDataAnimationIncludesNormals() const noexcept -> bool { return mSharedVertexDataAnimationIncludesNormals; }

        /** Creates a new Animation object for vertex animating this mesh. 
        @param name
            The name of this animation.
        @param length
            The length of the animation in seconds.
        */
        auto createAnimation(std::string_view name, Real length) -> Animation* override;

        /** Returns the named vertex Animation object. 
        @param name
            The name of the animation.
        */
        auto getAnimation(std::string_view name) const -> Animation* override;

        /** Internal access to the named vertex Animation object - returns null 
            if it does not exist. 
        @param name
            The name of the animation.
        */
        virtual auto _getAnimationImpl(std::string_view name) const -> Animation*;

        /** Returns whether this mesh contains the named vertex animation. */
        auto hasAnimation(std::string_view name) const -> bool override;

        /** Removes vertex Animation from this mesh. */
        void removeAnimation(std::string_view name) override;

        /** Gets the number of morph animations in this mesh. */
        auto getNumAnimations() const noexcept -> unsigned short override;

        /** Gets a single morph animation by index. 
        */
        auto getAnimation(unsigned short index) const -> Animation* override;

        /** Removes all morph Animations from this mesh. */
        virtual void removeAllAnimations();
        /** Gets a pointer to a vertex data element based on a morph animation 
            track handle.
        @remarks
            0 means the shared vertex data, 1+ means a submesh vertex data (index+1)
        */
        auto getVertexDataByTrackHandle(unsigned short handle) -> VertexData*;

        /** Internal method which, if animation types have not been determined,
            scans any vertex animations and determines the type for each set of
            vertex data (cannot have 2 different types).
        */
        void _determineAnimationTypes() const;
        /** Are the derived animation types out of date? */
        auto _getAnimationTypesDirty() const noexcept -> bool { return mAnimationTypesDirty; }

        /** Create a new Pose for this mesh or one of its submeshes.
        @param target
            The target geometry index; 0 is the shared Mesh geometry, 1+ is the
            dedicated SubMesh geometry belonging to submesh index + 1.
        @param name
            Name to give the pose, which is optional.
        @return
            A new Pose ready for population.
        */
        auto createPose(ushort target, std::string_view name = "") -> Pose*;
        /** Get the number of poses */
        auto getPoseCount() const -> size_t { return mPoseList.size(); }
        /** Retrieve an existing Pose by index */
        auto getPose(size_t index) const -> Pose* { return mPoseList.at(index); }
        /** Retrieve an existing Pose by name.*/
        auto getPose(std::string_view name) const -> Pose*;
        /** Destroy a pose by index.
        @note
            This will invalidate any animation tracks referring to this pose or those after it.
        */
        void removePose(ushort index);
        /** Destroy a pose by name.
        @note
            This will invalidate any animation tracks referring to this pose or those after it.
        */
        void removePose(std::string_view name);
        /** Destroy all poses. */
        void removeAllPoses();

        using PoseIterator = VectorIterator<PoseList>;
        using ConstPoseIterator = ConstVectorIterator<PoseList>;

        /** Get pose list. */
        auto getPoseList() const noexcept -> const PoseList&;

        /** Get LOD strategy used by this mesh. */
        auto getLodStrategy() const -> const LodStrategy *;

        /** Set the lod strategy used by this mesh. */
        void setLodStrategy(LodStrategy *lodStrategy);
    };

    /** A way of recording the way each LODs is recorded this Mesh. */
    struct MeshLodUsage
    {
        /** User-supplied values used to determine on which distance the lod is applies.
        @remarks
            This is required in case the LOD strategy changes.
        */
        Real userValue{0.0};

        /** Value used by to determine when this LOD applies.
        @remarks
            May be interpreted differently by different strategies.
            Transformed from user-supplied values with LodStrategy::transformUserValue.
        */
        Real value{0.0};
        

        /// Only relevant if mIsLodManual is true, the name of the alternative mesh to use.
        String manualName;
        /// Hard link to mesh to avoid looking up each time.
        mutable MeshPtr manualMesh;
        /// Edge list for this LOD level (may be derived from manual mesh).
        mutable EdgeData* edgeData{nullptr};

        MeshLodUsage()  = default;
    };

    /** @} */
    /** @} */


} // namespace Ogre

#endif // OGRE_CORE_MESH_H
