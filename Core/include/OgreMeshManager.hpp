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

export module Ogre.Core:MeshManager;

export import :Common;
export import :HardwareBuffer;
export import :HardwareVertexBuffer;
export import :PatchSurface;
export import :Plane;
export import :Prerequisites;
export import :Quaternion;
export import :Resource;
export import :ResourceGroupManager;
export import :ResourceManager;
export import :Singleton;
export import :Vector;

export import <map>;
export import <memory>;

export
namespace Ogre {

    class MeshSerializerListener;
class Codec;
class Mesh;
class SubMesh;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    /** Handles the management of mesh resources.
        @remarks
            This class deals with the runtime management of
            mesh data; like other resource managers it handles
            the creation of resources (in this case mesh data),
            working within a fixed memory budget.
        @remarks
            Ogre loads model files from it's own proprietary
            format called .mesh. This is because having a single file
            format is better for runtime performance, and we also have
            control over pre-processed data (such as
            collision boxes, LOD reductions etc). 
    */
    class MeshManager: public ResourceManager, public Singleton<MeshManager>
    {
    public:
        MeshManager();
        ~MeshManager() override;

        /** Initialises the manager, only to be called by OGRE internally. */
        void _initialise();

        /// @copydoc ResourceManager::getResourceByName
        auto getByName(std::string_view name, std::string_view groupName = RGN_DEFAULT) const -> MeshPtr;

        /// Create a new mesh
        /// @copydetails ResourceManager::createResource
        auto create (std::string_view name, std::string_view group,
                            bool isManual = false, ManualResourceLoader* loader = nullptr,
                            const NameValuePairList* createParams = nullptr) -> MeshPtr;

        using ResourceManager::createOrRetrieve;

        /** Create a new mesh, or retrieve an existing one with the same
            name if it already exists.
            @copydetails ResourceManager::createResource
            @param vertexBufferUsage The usage flags with which the vertex buffer(s)
                will be created
            @param indexBufferUsage The usage flags with which the index buffer(s) created for 
                this mesh will be created with.
            @param vertexBufferShadowed If true, the vertex buffers will be shadowed by system memory 
                copies for faster read access
            @param indexBufferShadowed If true, the index buffers will be shadowed by system memory 
                copies for faster read access
        */
        auto createOrRetrieve(
            std::string_view name,
            std::string_view group,
            bool isManual, ManualResourceLoader* loader,
            const NameValuePairList* createParams,
            HardwareBuffer::Usage vertexBufferUsage,
            HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            bool vertexBufferShadowed = false, bool indexBufferShadowed = false) -> ResourceCreateOrRetrieveResult;

        /** Prepares a mesh for loading from a file.  This does the IO in advance of the call to load().
            @note
                If the model has already been created (prepared or loaded), the existing instance
                will be returned.
            @param filename The name of the .mesh file
            @param groupName The name of the resource group to assign the mesh to 
            @param vertexBufferUsage The usage flags with which the vertex buffer(s)
                will be created
            @param indexBufferUsage The usage flags with which the index buffer(s) created for 
                this mesh will be created with.
            @param vertexBufferShadowed If true, the vertex buffers will be shadowed by system memory 
                copies for faster read access
            @param indexBufferShadowed If true, the index buffers will be shadowed by system memory 
                copies for faster read access
        */
        auto prepare( std::string_view filename, std::string_view groupName,
            HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            bool vertexBufferShadowed = false, bool indexBufferShadowed = false) -> MeshPtr;

        /** Loads a mesh from a file, making it immediately available for use.
            @copydetails MeshManager::prepare
        */
        auto load( std::string_view filename, std::string_view groupName,
            HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            bool vertexBufferShadowed = false, bool indexBufferShadowed = false) -> MeshPtr;


        /** Creates a new Mesh specifically for manual definition rather
            than loading from an object file. 

            Note that once you've defined your mesh, you must call Mesh::_setBounds
            in order to define the bounds of your mesh. In previous
            versions of OGRE could auto-compute that, but OGRE's support of 
            write-only vertex buffers makes this no longer appropriate.
        @param name The name to give the new mesh
        @param groupName The name of the resource group to assign the mesh to 
        @param loader ManualResourceLoader which will be called to load this mesh
            when the time comes. It is recommended that you populate this field
            in order that the mesh can be rebuilt should the need arise
        */
        auto createManual( std::string_view name, std::string_view groupName, 
            ManualResourceLoader* loader = nullptr) -> MeshPtr;

        /** Creates a basic plane, by default majoring on the x/y axes facing positive Z.
            @param
                name The name to give the resulting mesh
            @param 
                groupName The name of the resource group to assign the mesh to 
            @param
                plane The orientation of the plane and distance from the origin
            @param
                width The width of the plane in world coordinates
            @param
                height The height of the plane in world coordinates
            @param
                xsegments The number of segments to the plane in the x direction
            @param
                ysegments The number of segments to the plane in the y direction
            @param
                normals If true, normals are created perpendicular to the plane
            @param
                numTexCoordSets The number of 2D texture coordinate sets created - by default the corners
                are created to be the corner of the texture.
            @param
                uTile The number of times the texture should be repeated in the u direction
            @param
                vTile The number of times the texture should be repeated in the v direction
            @param
                upVector The 'Up' direction of the plane texture coordinates.
            @param
                vertexBufferUsage The usage flag with which the vertex buffer for this plane will be created
            @param
                indexBufferUsage The usage flag with which the index buffer for this plane will be created
            @param
                vertexShadowBuffer If this flag is set to true, the vertex buffer will be created 
                with a system memory shadow buffer,
                allowing you to read it back more efficiently than if it is in hardware
            @param
                indexShadowBuffer If this flag is set to true, the index buffer will be 
                created with a system memory shadow buffer,
                allowing you to read it back more efficiently than if it is in hardware
        */
        auto createPlane(
            std::string_view name, std::string_view groupName, const Plane& plane,
            Real width, Real height,
            int xsegments = 1, int ysegments = 1,
            bool normals = true, unsigned short numTexCoordSets = 1,
            Real uTile = 1.0f, Real vTile = 1.0f, const Vector3& upVector = Vector3::UNIT_Y,
            HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            bool vertexShadowBuffer = false, bool indexShadowBuffer = false) -> MeshPtr;

        
        /** Creates a plane, which because of it's texture coordinates looks like a curved
            surface, useful for skies in a skybox. 
            @param name
                The name to give the resulting mesh
            @param groupName
                The name of the resource group to assign the mesh to 
            @param plane
                The orientation of the plane and distance from the origin
            @param width
                The width of the plane in world coordinates
            @param height
                The height of the plane in world coordinates
            @param curvature
                The curvature of the plane. Good values are
                between 2 and 65. Higher values are more curved leading to
                a smoother effect, lower values are less curved meaning
                more distortion at the horizons but a better distance effect.
            @param xsegments
                The number of segments to the plane in the x direction
            @param ysegments
                The number of segments to the plane in the y direction
            @param normals
                If true, normals are created perpendicular to the plane
            @param numTexCoordSets
                The number of 2D texture coordinate sets created - by default the corners
                are created to be the corner of the texture.
            @param uTile
                The number of times the texture should be repeated in the u direction
            @param vTile
                The number of times the texture should be repeated in the v direction
            @param upVector
                The 'Up' direction of the plane.
            @param orientation
                The orientation of the overall sphere that's used to create the illusion
            @param vertexBufferUsage
                The usage flag with which the vertex buffer for this plane will be created
            @param indexBufferUsage
                The usage flag with which the index buffer for this plane will be created
            @param vertexShadowBuffer
                If this flag is set to true, the vertex buffer will be created 
                with a system memory shadow buffer,
                allowing you to read it back more efficiently than if it is in hardware
            @param indexShadowBuffer
                If this flag is set to true, the index buffer will be 
                created with a system memory shadow buffer,
                allowing you to read it back more efficiently than if it is in hardware
            @param ySegmentsToKeep The number of segments from the top of the dome
                downwards to keep. -1 keeps all of them. This can save fillrate if
                you cannot see much of the sky lower down.
        */
        auto createCurvedIllusionPlane(
            std::string_view name, std::string_view groupName, const Plane& plane,
            Real width, Real height, Real curvature,
            int xsegments = 1, int ysegments = 1,
            bool normals = true, unsigned short numTexCoordSets = 1,
            Real uTile = 1.0f, Real vTile = 1.0f, const Vector3& upVector = Vector3::UNIT_Y,
            const Quaternion& orientation = Quaternion::IDENTITY,
            HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            bool vertexShadowBuffer = false, bool indexShadowBuffer = false, 
            int ySegmentsToKeep = -1) -> MeshPtr;

        /** Creates a genuinely curved plane, by default majoring on the x/y axes facing positive Z.
            @param name
                The name to give the resulting mesh
            @param groupName
                The name of the resource group to assign the mesh to 
            @param plane
                The orientation of the plane and distance from the origin
            @param width
                The width of the plane in world coordinates
            @param height
                The height of the plane in world coordinates
            @param bow
                The amount of 'bow' in the curved plane.  (Could also be considered the depth.)
            @param xsegments
                The number of segments to the plane in the x direction
            @param ysegments
                The number of segments to the plane in the y direction
            @param normals
                If true, normals are created perpendicular to the plane
            @param numTexCoordSets
                The number of 2D texture coordinate sets created - by default the corners
                are created to be the corner of the texture.
            @param uTile
                The number of times the texture should be repeated in the u direction
            @param vTile
                The number of times the texture should be repeated in the v direction
            @param upVector
                The 'Up' direction of the plane.
            @param vertexBufferUsage
                The usage flag with which the vertex buffer for this plane will be created
            @param indexBufferUsage
                The usage flag with which the index buffer for this plane will be created
            @param vertexShadowBuffer
                If this flag is set to true, the vertex buffer will be created 
                with a system memory shadow buffer,
                allowing you to read it back more efficiently than if it is in hardware
            @param indexShadowBuffer
                If this flag is set to true, the index buffer will be 
                created with a system memory shadow buffer,
                allowing you to read it back more efficiently than if it is in hardware
        */
        auto createCurvedPlane( 
            std::string_view name, std::string_view groupName, const Plane& plane, 
            Real width, Real height, Real bow = 0.5f, 
            int xsegments = 1, int ysegments = 1,
            bool normals = false, unsigned short numTexCoordSets = 1, 
            Real uTile = 1.0f, Real vTile = 1.0f, const Vector3& upVector = Vector3::UNIT_Y,
            HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            bool vertexShadowBuffer = false, bool indexShadowBuffer = false) -> MeshPtr;

        /** Creates a Bezier patch based on an array of control vertices.
            @param name
                The name to give the newly created mesh. 
            @param groupName
                The name of the resource group to assign the mesh to 
            @param controlPointBuffer
                A pointer to a buffer containing the vertex data which defines control points 
                of the curves rather than actual vertices. Note that you are expected to provide not
                just position information, but potentially normals and texture coordinates too. The
                format of the buffer is defined in the VertexDeclaration parameter
            @param declaration
                VertexDeclaration describing the contents of the buffer. 
                Note this declaration must _only_ draw on buffer source 0!
            @param width
                Specifies the width of the patch in control points.
                Note this parameter must greater than or equal to 3.
            @param height
                Specifies the height of the patch in control points. 
                Note this parameter must greater than or equal to 3.
            @param uMaxSubdivisionLevel, vMaxSubdivisionLevel
                If you want to manually set the top level of subdivision, 
                do it here, otherwise let the system decide.
            @param visibleSide 
                Determines which side of the patch (or both) triangles are generated for.
            @param vbUsage
                Vertex buffer usage flags. Recommend the default since vertex buffer should be static.
            @param ibUsage
                Index buffer usage flags. Recommend the default since index buffer should 
                be dynamic to change levels but not readable.
            @param vbUseShadow
                Flag to determine if a shadow buffer is generated for the vertex buffer. See
                HardwareBuffer for full details.
            @param ibUseShadow
                Flag to determine if a shadow buffer is generated for the index buffer. See
                HardwareBuffer for full details.
        */
        auto createBezierPatch(
            std::string_view name, std::string_view groupName, void* controlPointBuffer, 
            VertexDeclaration *declaration, size_t width, size_t height,
            size_t uMaxSubdivisionLevel = PatchSurface::AUTO_LEVEL, 
            size_t vMaxSubdivisionLevel = PatchSurface::AUTO_LEVEL,
            PatchSurface::VisibleSide visibleSide = PatchSurface::VisibleSide::FRONT,
            HardwareBuffer::Usage vbUsage = HardwareBuffer::STATIC_WRITE_ONLY,
            HardwareBuffer::Usage ibUsage = HardwareBuffer::DYNAMIC_WRITE_ONLY,
            bool vbUseShadow = true, bool ibUseShadow = true) -> PatchMeshPtr;
        
        /** Tells the mesh manager that all future meshes should prepare themselves for
            shadow volumes on loading.
        */
        void setPrepareAllMeshesForShadowVolumes(bool enable);
        /** Retrieves whether all Meshes should prepare themselves for shadow volumes. */
        auto getPrepareAllMeshesForShadowVolumes() noexcept -> bool;

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> MeshManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> MeshManager*;

        /** Gets the base element type used for blend weights in vertex buffers.
        @remarks
        See the remarks below for SetBlendWeightsBaseElementType().
        */
        auto getBlendWeightsBaseElementType() const -> VertexElementType;

        /** sets the base element type used for blend weights in vertex buffers.
        @remarks
        This takes effect when meshes are loaded.  Default is VertexElementType::FLOAT1.
        Valid values are:
        VertexElementType::UBYTE4_NORM:   8-bit blend weights.  Lowest memory cost but may have precision issues.  Requires SM2.0+ vertex shader.  No software skinning.
        VertexElementType::USHORT2_NORM:  16-bit blend weights.  Requires SM2.0+ vertex shader.  No software skinning.
        VertexElementType::FLOAT1:        32-bit blend weights.  Highest memory cost.  Supports hardware and software skinning.
        */
        void setBlendWeightsBaseElementType( VertexElementType vet );

        /** Gets the factor by which the bounding box of an entity is padded.
            Default is 0.01
        */
        auto getBoundsPaddingFactor() -> Real;
    
        /** Sets the factor by which the bounding box of an entity is padded
        */
        void setBoundsPaddingFactor(Real paddingFactor);

        /** Sets the listener used to control mesh loading through the serializer.
        */
        void setListener(MeshSerializerListener *listener);
        
        /** Gets the listener used to control mesh loading through the serializer.
        */
        auto getListener() -> MeshSerializerListener *;

    private:

        /// @copydoc ResourceManager::createImpl
        auto createImpl(std::string_view name, ResourceHandle handle, 
            std::string_view group, bool isManual, ManualResourceLoader* loader, 
            const NameValuePairList* createParams) -> Resource* override;
    
        /** Enum identifying the types of manual mesh built by this manager */
        enum class MeshBuildType
        {
            PLANE,
            CURVED_ILLUSION_PLANE,
            CURVED_PLANE
        };
        /** Saved parameters used to (re)build a manual mesh built by this class */
        struct MeshBuildParams
        {
            MeshBuildType type;
            Plane plane;
            Real width;
            Real height;
            Real curvature;
            int xsegments;
            int ysegments;
            bool normals;
            unsigned short numTexCoordSets;
            Real xTile;
            Real yTile;
            Vector3 upVector;
            Quaternion orientation;
            HardwareBuffer::Usage vertexBufferUsage;
            HardwareBuffer::Usage indexBufferUsage;
            bool vertexShadowBuffer;
            bool indexShadowBuffer;
            int ySegmentsToKeep;
        };

        struct PrefabLoader : public ManualResourceLoader
        {
            /** Map from resource pointer to parameter set */
            using MeshBuildParamsMap = std::map<Resource *, MeshBuildParams>;
            MeshBuildParamsMap mMeshBuildParams;

            /** Utility method for tessellating 2D meshes.
            */
            static void tesselate2DMesh(SubMesh* pSub, unsigned short meshWidth, unsigned short meshHeight,
                bool doubleSided = false,
                HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::STATIC_WRITE_ONLY,
                bool indexSysMem = false);
            /** Utility method for manual loading a plane */
            static void loadManualPlane(Mesh* pMesh, MeshBuildParams& params);
            /** Utility method for manual loading a curved plane */
            static void loadManualCurvedPlane(Mesh* pMesh, MeshBuildParams& params);
            /** Utility method for manual loading a curved illusion plane */
            static void loadManualCurvedIllusionPlane(Mesh* pMesh, MeshBuildParams& params);

            void loadResource(Resource* res) override;
        } mPrefabLoader;

        // element type for blend weights in vertex buffer (VertexElementType::UBYTE4, VertexElementType::USHORT1, or VertexElementType::FLOAT1)
        VertexElementType mBlendWeightsBaseElementType;

        bool mPrepAllMeshesForShadowVolumes;
    
        //the factor by which the bounding box of an entity is padded   
        Real mBoundsPaddingFactor{0.01};

        // The listener to pass to serializers
        MeshSerializerListener *mListener{nullptr};

    private:
        std::unique_ptr<Codec> mMeshCodec;
    };

    /** @} */
    /** @} */

} //namespace
