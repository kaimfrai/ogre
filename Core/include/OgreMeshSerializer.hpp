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
export module Ogre.Core:MeshSerializer;

export import :MeshSerializerImpl;
export import :Prerequisites;
export import :Serializer;

export import <memory>;
export import <utility>;
export import <vector>;

export
namespace Ogre {
    
    class MeshSerializerListener;
    class Mesh;
    
    /// Mesh compatibility versions
    enum class MeshVersion 
    {
        /// Latest version available
        LATEST,
        
        /// OGRE version v1.10+
        _1_10,
        /// OGRE version v1.8+
        _1_8,
        /// OGRE version v1.7+
        _1_7,
        /// OGRE version v1.4+
        _1_4,
        /// OGRE version v1.0+
        _1_0,
        
        /// Legacy versions, DO NOT USE for writing
        LEGACY
    };

    class MeshVersionData : public SerializerAlloc
    {
    public:
        MeshVersion version;
        String versionString;
        ::std::unique_ptr<MeshSerializerImpl> impl;

        MeshVersionData(MeshVersion _ver, std::string_view _string, ::std::unique_ptr<MeshSerializerImpl> _impl)
        : version(_ver), versionString(_string), impl(::std::move(_impl)) {}

    };

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    /** Class for serialising mesh data to/from an OGRE .mesh file.
    @remarks
        This class allows exporters to write OGRE .mesh files easily, and allows the
        OGRE engine to import .mesh files into instantiated OGRE Meshes.
        Note that a .mesh file can include not only the Mesh, but also definitions of
        any Materials it uses (although this is optional, the .mesh can rely on the
        Material being loaded from another source, especially useful if you want to
        take advantage of OGRE's advanced Material properties which may not be available
        in your modeller).
    @par
        To export a Mesh:<OL>
        <LI>Use the MaterialManager methods to create any dependent Material objects, if you want
            to export them with the Mesh.</LI>
        <LI>Create a Mesh object and populate it using it's methods.</LI>
        <LI>Call the exportMesh method</LI>
        </OL>
    @par
        It's important to realise that this exporter uses OGRE terminology. In this context,
        'Mesh' means a top-level mesh structure which can actually contain many SubMeshes, each
        of which has only one Material. Modelling packages may refer to these differently, for
        example in Milkshape, it says 'Model' instead of 'Mesh' and 'Mesh' instead of 'SubMesh', 
        but the theory is the same.
    */
    class MeshSerializer : public Serializer
    {
    public:
        MeshSerializer();
        virtual ~MeshSerializer() = default;


        /** Exports a mesh to the file specified, in the latest format
        @remarks
            This method takes an externally created Mesh object, and exports it
            to a .mesh file in the latest format version available.
        @param pMesh Pointer to the Mesh to export
        @param filename The destination filename
        @param endianMode The endian mode of the written file
        */
        void exportMesh(const Mesh* pMesh, std::string_view filename,
            std::endian endianMode = std::endian::native);

        /** Exports a mesh to the file specified, in a specific version format. 
         @remarks
         This method takes an externally created Mesh object, and exports it
         to a .mesh file in the specified format version. Note that picking a
         format version other that the latest will cause some information to be
         lost.
         @param pMesh Pointer to the Mesh to export
         @param filename The destination filename
         @param version Mesh version to write
         @param endianMode The endian mode of the written file
         */
        void exportMesh(const Mesh* pMesh, std::string_view filename,
                        MeshVersion version,
                        std::endian endianMode = std::endian::native);

        /** Exports a mesh to the stream specified, in the latest format. 
        @remarks
         This method takes an externally created Mesh object, and exports it
         to a .mesh file in the latest format version. 
        @param pMesh Pointer to the Mesh to export
        @param stream Writeable stream
        @param endianMode The endian mode of the written file
        */
        void exportMesh(const Mesh* pMesh, DataStreamPtr stream,
            std::endian endianMode = std::endian::native);

        /** Exports a mesh to the stream specified, in a specific version format. 
         @remarks
         This method takes an externally created Mesh object, and exports it
         to a .mesh file in the specified format version. Note that picking a
         format version other that the latest will cause some information to be
         lost.
         @param pMesh Pointer to the Mesh to export
         @param stream Writeable stream
         @param version Mesh version to write
         @param endianMode The endian mode of the written file
         */
        void exportMesh(const Mesh* pMesh, DataStreamPtr stream,
                        MeshVersion version,
                        std::endian endianMode = std::endian::native);
        
        /** Imports Mesh and (optionally) Material data from a .mesh file DataStream.
        @remarks
            This method imports data from a DataStream opened from a .mesh file and places it's
            contents into the Mesh object which is passed in. 
        @param stream The DataStream holding the .mesh data. Must be initialised (pos at the start of the buffer).
        @param pDest Pointer to the Mesh object which will receive the data. Should be blank already.
        */
        void importMesh(const DataStreamPtr& stream, Mesh* pDest);

        /// Sets the listener for this serializer
        void setListener(MeshSerializerListener *listener);
        /// Returns the current listener
        auto getListener() -> MeshSerializerListener *;
        
    private:
        using MeshVersionDataList = std::vector<::std::unique_ptr<MeshVersionData>>;
        MeshVersionDataList mVersionData;

        MeshSerializerListener *mListener{nullptr};

    };

    /** 
     @remarks
        This class allows users to hook into the mesh loading process and
        modify references within the mesh as they are loading. Material and
        skeletal references can be processed using this interface which allows
        finer control over resources.
    */
    class MeshSerializerListener
    {
    public:
        virtual ~MeshSerializerListener() = default;
        /// Called to override the loading of the given named material
        virtual void processMaterialName(Mesh *mesh, String *name) = 0;
        /// Called to override the reference to a skeleton
        virtual void processSkeletonName(Mesh *mesh, String *name) = 0;
        /// Allows to do changes on mesh after it's completely loaded. For example you can generate LOD levels here.
        virtual void processMeshCompleted(Mesh *mesh) = 0;
    };
    /** @} */
    /** @} */
}
