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

export module Ogre.Core:SubMesh;

export import :AnimationTrack;
export import :Common;
export import :IteratorWrapper;
export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :RenderOperation;
export import :ResourceGroupManager;
export import :SharedPtr;
export import :Vector;
export import :VertexBoneAssignment;

export import <algorithm>;
export import <map>;
export import <memory>;
export import <vector>;

export
namespace Ogre {
class IndexData;
class Mesh;
class VertexData;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    /** Defines a part of a complete mesh.
        @remarks
            Meshes which make up the definition of a discrete 3D object
            are made up of potentially multiple parts. This is because
            different parts of the mesh may use different materials or
            use different vertex formats, such that a rendering state
            change is required between them.
        @par
            Like the Mesh class, instantiations of 3D objects in the scene
            share the SubMesh instances, and have the option of overriding
            their material differences on a per-object basis if required.
            See the SubEntity class for more information.
    */
    class SubMesh : public SubMeshAlloc
    {
        friend class Mesh;
        friend class MeshSerializerImpl;
        friend class MeshSerializerImpl_v1_2;
        friend class MeshSerializerImpl_v1_1;
    public:
        SubMesh();
        ~SubMesh();

        /** Dedicated vertex data (only valid if useSharedVertices = false).
            @remarks
                This data is completely owned by this submesh.
            @par
                The use of shared or non-shared buffers is determined when
                model data is converted to the OGRE .mesh format.
        */
        ::std::unique_ptr<VertexData> vertexData{nullptr};

        /// Face index data
        ::std::unique_ptr<IndexData> indexData;

        /** Dedicated index map for translate blend index to bone index (only valid if useSharedVertices = false).
            @remarks
                This data is completely owned by this submesh.
            @par
                We collect actually used bones of all bone assignments, and build the
                blend index in 'packed' form, then the range of the blend index in vertex
                data VertexElementSemantic::BLEND_INDICES element is continuous, with no gaps. Thus, by
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
        using IndexMap = std::vector<unsigned short>;
        IndexMap blendIndexToBoneIndexMap;

        using LODFaceList = std::vector<IndexData *>;
        LODFaceList mLodFaceList;

        /** A list of extreme points on the submesh (optional).
            @remarks
                These points are some arbitrary points on the mesh that are used
                by engine to better sort submeshes by depth. This doesn't matter
                much for non-transparent submeshes, as Z-buffer takes care of invisible
                surface culling anyway, but is pretty useful for semi-transparent
                submeshes because the order in which transparent submeshes must be
                rendered cannot be always correctly deduced from entity position.
            @par
                These points are intelligently chosen from the points that make up
                the submesh, the criteria for choosing them should be that these points
                somewhat characterize the submesh outline, e.g. they should not be
                close to each other, and they should be on the outer hull of the submesh.
                They can be stored in the .mesh file, or generated at runtime
                (see generateExtremes ()).
            @par
                If this array is empty, submesh sorting is done like in older versions -
                by comparing the positions of the owning entity.
         */
        std::vector<Vector3> extremityPoints;

        /// Reference to parent Mesh (not a smart pointer so child does not keep parent alive).
        Mesh* parent{nullptr};

        /// Indicates if this submesh shares vertex data with other meshes or whether it has it's own vertices.
        bool useSharedVertices{true};

        /// The render operation type used to render this submesh
        RenderOperation::OperationType operationType{RenderOperation::OperationType::TRIANGLE_LIST};

        /// Sets the name of the Material which this SubMesh will use
        void setMaterialName(std::string_view matName, std::string_view groupName = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
        auto getMaterialName() const noexcept -> std::string_view ;

        void setMaterial(const MaterialPtr& mat) { mMaterial = mat; }
        auto getMaterial() const noexcept -> const MaterialPtr& { return mMaterial; }

        /** Returns a RenderOperation structure required to render this mesh.
            @param 
                rend Reference to a RenderOperation structure to populate.
            @param
                lodIndex The index of the LOD to use. 
        */
        void _getRenderOperation(RenderOperation& rend, ushort lodIndex = 0);

        /** Assigns a vertex to a bone with a given weight, for skeletal animation. 
        @remarks    
            This method is only valid after calling setSkeletonName.
            Since this is a one-off process there exists only 'addBoneAssignment' and
            'clearBoneAssignments' methods, no 'editBoneAssignment'. You should not need
            to modify bone assignments during rendering (only the positions of bones) and OGRE
            reserves the right to do some internal data reformatting of this information, depending
            on render system requirements.
        @par
            This method is for assigning weights to the dedicated geometry of the SubMesh. To assign
            weights to the shared Mesh geometry, see the equivalent methods on Mesh.
        */
        void addBoneAssignment(const VertexBoneAssignment& vertBoneAssign);

        /** Removes all bone assignments for this mesh. 
        @par
            This method is for assigning weights to the dedicated geometry of the SubMesh. To assign
            weights to the shared Mesh geometry, see the equivalent methods on Mesh.
        */
        void clearBoneAssignments();

        /// Multimap of verex bone assignments (orders by vertex index)
        using VertexBoneAssignmentList = std::multimap<size_t, VertexBoneAssignment>;
        using BoneAssignmentIterator = MapIterator<VertexBoneAssignmentList>;

        /** Gets a const reference to the list of bone assignments
        */
        auto getBoneAssignments() const noexcept -> const VertexBoneAssignmentList& { return mBoneAssignments; }


        /** Must be called once to compile bone assignments into geometry buffer. */
        void _compileBoneAssignments();

        using AliasTextureIterator = ConstMapIterator<AliasTextureNamePairList>;
        /// @deprecated do not use
        auto getAliasTextureIterator() const -> AliasTextureIterator;
        /// @deprecated do not use
        void addTextureAlias(std::string_view aliasName, std::string_view textureName);

        /// @deprecated do not use
        auto hasTextureAliases() const -> bool { return !mTextureAliases.empty(); }

        /** Get the type of any vertex animation used by dedicated geometry.
        */
        auto getVertexAnimationType() const -> VertexAnimationType;
        
        /// Returns whether animation on dedicated vertex data includes normals
        auto getVertexAnimationIncludesNormals() const noexcept -> bool { return mVertexAnimationIncludesNormals; }


        /** Generate the submesh extremes (@see extremityPoints).
        @param count
            Number of extreme points to compute for the submesh.
        */
        void generateExtremes(size_t count);

        /** Returns true(by default) if the submesh should be included in the mesh EdgeList, otherwise returns false.
        */      
        auto isBuildEdgesEnabled() const noexcept -> bool { return mBuildEdgesEnabled; }
        void setBuildEdgesEnabled(bool b);
        /** Makes a copy of this submesh object and gives it a new name.
         @param newName
         The name to give the clone.
         @param parentMesh
         Optional mesh to make the parent of the newly created clone.
         If you leave this blank, the clone will be parented to the same Mesh as the original.
         */
        auto clone(std::string_view newName, Mesh *parentMesh = nullptr) -> SubMesh *;

    private:

        /// Flag indicating that bone assignments need to be recompiled
        bool mBoneAssignmentsOutOfDate{false};

        /// Type of vertex animation for dedicated vertex data (populated by Mesh)
        mutable VertexAnimationType mVertexAnimationType{VertexAnimationType::NONE};

        /// Whether normals are included in vertex animation keyframes
        mutable bool mVertexAnimationIncludesNormals{false};

        /// Is Build Edges Enabled
        bool mBuildEdgesEnabled{true};

        /// the material this SubMesh uses.
        MaterialPtr mMaterial;

        /// paired list of texture aliases and texture names
        AliasTextureNamePairList mTextureAliases;

        VertexBoneAssignmentList mBoneAssignments;

        /// Internal method for removing LOD data
        void removeLodLevels();


    };
    /** @} */
    /** @} */

} // namespace
