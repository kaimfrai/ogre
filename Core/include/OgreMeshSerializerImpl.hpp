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

#ifndef OGRE_CORE_MESHSERIALIZERIMPL_H
#define OGRE_CORE_MESHSERIALIZERIMPL_H

#include <cstddef>

#include "OgreEdgeListBuilder.hpp"
#include "OgreHardwareVertexBuffer.hpp"
#include "OgreKeyFrame.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSerializer.hpp"
#include "OgreVertexBoneAssignment.hpp"

namespace Ogre {
    
    class MeshSerializerListener;
    struct MeshLodUsage;
class Animation;
class Mesh;
class Pose;
class SubMesh;
class VertexAnimationTrack;
class VertexData;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    /** Internal implementation of Mesh reading / writing for the latest version of the
    .mesh format.
    @remarks
    In order to maintain compatibility with older versions of the .mesh format, there
    will be alternative subclasses of this class to load older versions, whilst this class
    will remain to load the latest version.

     @note
        This mesh format was used from Ogre v1.10.

    */
    class MeshSerializerImpl : public Serializer
    {
    public:
        MeshSerializerImpl();
        virtual ~MeshSerializerImpl();
        /** Exports a mesh to the file specified. 
        @remarks
        This method takes an externally created Mesh object, and exports both it
        and optionally the Materials it uses to a .mesh file.
        @param pMesh Pointer to the Mesh to export
        @param stream The destination stream
        @param endianMode The endian mode for the written file
        */
        void exportMesh(const Mesh* pMesh, const DataStreamPtr stream,
            std::endian endianMode = std::endian::native);

        /** Imports Mesh and (optionally) Material data from a .mesh file DataStream.
        @remarks
        This method imports data from a DataStream opened from a .mesh file and places it's
        contents into the Mesh object which is passed in. 
        @param stream The DataStream holding the .mesh data. Must be initialised (pos at the start of the buffer).
        @param pDest Pointer to the Mesh object which will receive the data. Should be blank already.
        */
        void importMesh(const DataStreamPtr& stream, Mesh* pDest, MeshSerializerListener *listener);

    protected:

        // Internal methods
        virtual void writeSubMeshNameTable(const Mesh* pMesh);
        virtual void writeMesh(const Mesh* pMesh);
        virtual void writeSubMesh(const SubMesh* s);
        virtual void writeSubMeshOperation(const SubMesh* s);
        virtual void writeSubMeshTextureAliases(const SubMesh* s);
        virtual void writeGeometry(const VertexData* pGeom);
        virtual void writeSkeletonLink(std::string_view skelName);
        virtual void writeMeshBoneAssignment(const VertexBoneAssignment& assign);
        virtual void writeSubMeshBoneAssignment(const VertexBoneAssignment& assign);

        virtual void writeLodLevel(const Mesh* pMesh);
        virtual void writeLodUsageManual(const MeshLodUsage& usage);
        virtual void writeLodUsageGenerated(const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum);
        virtual void writeLodUsageGeneratedSubmesh(const SubMesh* submesh, unsigned short lodNum);

        virtual void writeBoundsInfo(const Mesh* pMesh);
        virtual void writeEdgeList(const Mesh* pMesh);
        virtual void writeAnimations(const Mesh* pMesh);
        virtual void writeAnimation(const Animation* anim);
        virtual void writePoses(const Mesh* pMesh);
        virtual void writePose(const Pose* pose);
        virtual void writeAnimationTrack(const VertexAnimationTrack* track);
        virtual void writeMorphKeyframe(const VertexMorphKeyFrame* kf, size_t vertexCount);
        virtual void writePoseKeyframe(const VertexPoseKeyFrame* kf);
        virtual void writePoseKeyframePoseRef(const VertexPoseKeyFrame::PoseRef& poseRef);
        virtual void writeExtremes(const Mesh *pMesh);
        virtual void writeSubMeshExtremes(unsigned short idx, const SubMesh* s);

        virtual auto calcMeshSize(const Mesh* pMesh) -> size_t;
        virtual auto calcSubMeshSize(const SubMesh* pSub) -> size_t;
        virtual auto calcGeometrySize(const VertexData* pGeom) -> size_t;
        virtual auto calcSkeletonLinkSize(std::string_view skelName) -> size_t;
        virtual auto calcBoneAssignmentSize() -> size_t;
        virtual auto calcSubMeshOperationSize(const SubMesh* pSub) -> size_t;
        virtual auto calcSubMeshNameTableSize(const Mesh* pMesh) -> size_t;
        virtual auto calcLodLevelSize(const Mesh* pMesh) -> size_t;
        virtual auto calcLodUsageManualSize(const MeshLodUsage& usage) -> size_t;
        virtual auto calcLodUsageGeneratedSize(const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum) -> size_t;
        virtual auto calcLodUsageGeneratedSubmeshSize(const SubMesh* submesh, unsigned short lodNum) -> size_t;
        virtual auto calcEdgeListSize(const Mesh* pMesh) -> size_t;
        virtual auto calcEdgeListLodSize(const EdgeData* data, bool isManual) -> size_t;
        virtual auto calcEdgeGroupSize(const EdgeData::EdgeGroup& group) -> size_t;
        virtual auto calcPosesSize(const Mesh* pMesh) -> size_t;
        virtual auto calcPoseSize(const Pose* pose) -> size_t;
        virtual auto calcAnimationsSize(const Mesh* pMesh) -> size_t;
        virtual auto calcAnimationSize(const Animation* anim) -> size_t;
        virtual auto calcAnimationTrackSize(const VertexAnimationTrack* track) -> size_t;
        virtual auto calcMorphKeyframeSize(const VertexMorphKeyFrame* kf, size_t vertexCount) -> size_t;
        virtual auto calcPoseKeyframeSize(const VertexPoseKeyFrame* kf) -> size_t;
        virtual auto calcPoseKeyframePoseRefSize() -> size_t;
        virtual auto calcPoseVertexSize(const Pose* pose) -> size_t;
        virtual auto calcSubMeshTextureAliasesSize(const SubMesh* pSub) -> size_t;
        virtual auto calcBoundsInfoSize(const Mesh* pMesh) -> size_t;
        virtual auto calcExtremesSize(const Mesh* pMesh) -> size_t;
        virtual auto calcSubMeshExtremesSize(unsigned short idx, const SubMesh* s) -> size_t;

        virtual void readTextureLayer(const DataStreamPtr& stream, Mesh* pMesh, MaterialPtr& pMat);
        virtual void readSubMeshNameTable(const DataStreamPtr& stream, Mesh* pMesh);
        virtual void readMesh(const DataStreamPtr& stream, Mesh* pMesh, MeshSerializerListener *listener);
        virtual void readSubMesh(const DataStreamPtr& stream, Mesh* pMesh, MeshSerializerListener *listener);
        virtual void readSubMeshOperation(const DataStreamPtr& stream, Mesh* pMesh, SubMesh* sub);
        virtual void readSubMeshTextureAlias(const DataStreamPtr& stream, Mesh* pMesh, SubMesh* sub);
        virtual void readGeometry(const DataStreamPtr& stream, Mesh* pMesh, VertexData* dest);
        virtual void readGeometryVertexDeclaration(const DataStreamPtr& stream, Mesh* pMesh, VertexData* dest);
        virtual void readGeometryVertexElement(const DataStreamPtr& stream, Mesh* pMesh, VertexData* dest);
        virtual void readGeometryVertexBuffer(const DataStreamPtr& stream, Mesh* pMesh, VertexData* dest);

        virtual void readSkeletonLink(const DataStreamPtr& stream, Mesh* pMesh, MeshSerializerListener *listener);
        virtual void readMeshBoneAssignment(const DataStreamPtr& stream, Mesh* pMesh);
        virtual void readSubMeshBoneAssignment(const DataStreamPtr& stream, Mesh* pMesh,
            SubMesh* sub);
        virtual void readMeshLodLevel(const DataStreamPtr& stream, Mesh* pMesh);

        virtual void readMeshLodUsageManual(const DataStreamPtr& stream, Mesh* pMesh, unsigned short lodNum, MeshLodUsage& usage);
        virtual void readMeshLodUsageGenerated(const DataStreamPtr& stream, Mesh* pMesh, unsigned short lodNum, MeshLodUsage& usage);

        virtual void readBoundsInfo(const DataStreamPtr& stream, Mesh* pMesh);
        virtual void readEdgeList(const DataStreamPtr& stream, Mesh* pMesh);
        virtual void readEdgeListLodInfo(const DataStreamPtr& stream, EdgeData* edgeData);
        virtual void readPoses(const DataStreamPtr& stream, Mesh* pMesh);
        virtual void readPose(const DataStreamPtr& stream, Mesh* pMesh);
        virtual void readAnimations(const DataStreamPtr& stream, Mesh* pMesh);
        virtual void readAnimation(const DataStreamPtr& stream, Mesh* pMesh);
        virtual void readAnimationTrack(const DataStreamPtr& stream, Animation* anim,
            Mesh* pMesh);
        virtual void readMorphKeyFrame(const DataStreamPtr& stream, Mesh* pMesh, VertexAnimationTrack* track);
        virtual void readPoseKeyFrame(const DataStreamPtr& stream, VertexAnimationTrack* track);
        virtual void readExtremes(const DataStreamPtr& stream, Mesh *pMesh);


        /// Flip an entire vertex buffer from little endian
        virtual void flipFromLittleEndian(void* pData, size_t vertexCount, size_t vertexSize, const VertexDeclaration::VertexElementList& elems);
        /// Flip an entire vertex buffer to little endian
        virtual void flipToLittleEndian(void* pData, size_t vertexCount, size_t vertexSize, const VertexDeclaration::VertexElementList& elems);
        /// Flip the endianness of an entire vertex buffer, passed in as a 
        /// pointer to locked or temporary memory 
        virtual void flipEndian(void* pData, size_t vertexCount, size_t vertexSize, const VertexDeclaration::VertexElementList& elems);
        
        /// This function can be overloaded to disable validation in debug builds.
        virtual void enableValidation();

        ushort exportedLodCount; // Needed to limit exported Edge data, when exporting
    };


    /** Class for providing backwards-compatibility for loading version 1.8 of the .mesh format. 
     This mesh format was used from Ogre v1.8.
     */
    class MeshSerializerImpl_v1_8 : public MeshSerializerImpl
    {
    public:
        MeshSerializerImpl_v1_8();
        ~MeshSerializerImpl_v1_8() override;
    protected:
        // In the past we could select to use manual or automatic generated Lod levels,
        // but now we can mix them. If it is mixed, we can't export it to older mesh formats.
        auto compatibleLodStrategyName(std::string_view lodStrategyName) -> std::string_view;
        virtual auto isLodMixed(const Mesh* pMesh) -> bool;
        auto calcLodLevelSize(const Mesh* pMesh) -> size_t override;
        auto calcLodUsageManualSize(const MeshLodUsage& usage) -> size_t override;
        auto calcLodUsageGeneratedSize(const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum) -> size_t override;
        auto calcLodUsageGeneratedSubmeshSize(const SubMesh* submesh, unsigned short lodNum) -> size_t override;

        void writeLodLevel(const Mesh* pMesh) override;
        void writeLodUsageGenerated(const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum) override;
        void writeLodUsageGeneratedSubmesh(const SubMesh* submesh, unsigned short lodNum) override;
        void writeLodUsageManual(const MeshLodUsage& usage) override;

        void readMeshLodUsageGenerated(const DataStreamPtr& stream, Mesh* pMesh,
            unsigned short lodNum, MeshLodUsage& usage) override;
        void readMeshLodUsageManual(const DataStreamPtr& stream, Mesh* pMesh, unsigned short lodNum, MeshLodUsage& usage) override;

        void readMeshLodLevel(const DataStreamPtr& stream, Mesh* pMesh) override;
        void enableValidation() override;
    };

    /** Class for providing backwards-compatibility for loading version 1.41 of the .mesh format. 
     This mesh format was used from Ogre v1.7.
     */
    class MeshSerializerImpl_v1_41 : public MeshSerializerImpl_v1_8
    {
    public:
        MeshSerializerImpl_v1_41();
        ~MeshSerializerImpl_v1_41() override;
    protected:
        void writeMorphKeyframe(const VertexMorphKeyFrame* kf, size_t vertexCount) override;
        void readMorphKeyFrame(const DataStreamPtr& stream, Mesh* pMesh, VertexAnimationTrack* track) override;
        void writePose(const Pose* pose) override;
        void readPose(const DataStreamPtr& stream, Mesh* pMesh) override;
        auto calcMorphKeyframeSize(const VertexMorphKeyFrame* kf, size_t vertexCount) -> size_t override;
        auto calcPoseSize(const Pose* pose) -> size_t override;
        auto calcPoseVertexSize() -> size_t;
        using MeshSerializerImpl::calcPoseVertexSize;
    };

    /** Class for providing backwards-compatibility for loading version 1.4 of the .mesh format. 
     This mesh format was used from Ogre v1.4.
     */
    class MeshSerializerImpl_v1_4 : public MeshSerializerImpl_v1_41
    {
    public:
        MeshSerializerImpl_v1_4();
        ~MeshSerializerImpl_v1_4() override;
    protected:
        auto calcLodLevelSize(const Mesh* pMesh) -> size_t override;
        void readMeshLodLevel(const DataStreamPtr& stream, Mesh* pMesh) override;

        void writeLodLevel(const Mesh* pMesh) override;
        void writeLodUsageGenerated(const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum) override;
    };

    /** Class for providing backwards-compatibility for loading version 1.3 of the .mesh format. 
     This mesh format was used from Ogre v1.0 (and some pre-releases)
     */
    class MeshSerializerImpl_v1_3 : public MeshSerializerImpl_v1_4
    {
    public:
        MeshSerializerImpl_v1_3();
        ~MeshSerializerImpl_v1_3() override;
    protected:
        void readEdgeListLodInfo(const DataStreamPtr& stream, EdgeData* edgeData) override;

        /// Reorganise triangles of the edge list to group by vertex set
        virtual void reorganiseTriangles(EdgeData* edgeData);
        
        void writeEdgeList(const Mesh* pMesh) override;
        auto calcEdgeListLodSize(const EdgeData* edgeData, bool isManual) -> size_t override;
        auto calcEdgeGroupSize(const EdgeData::EdgeGroup& group) -> size_t override;
    };

    /** Class for providing backwards-compatibility for loading version 1.2 of the .mesh format. 
     This is a LEGACY FORMAT that pre-dates version Ogre 1.0
     */
    class MeshSerializerImpl_v1_2 : public MeshSerializerImpl_v1_3
    {
    public:
        MeshSerializerImpl_v1_2();
        ~MeshSerializerImpl_v1_2() override;
    protected:
        void readMesh(const DataStreamPtr& stream, Mesh* pMesh, MeshSerializerListener *listener) override;
        void readGeometry(const DataStreamPtr& stream, Mesh* pMesh, VertexData* dest) override;
        virtual void readGeometryPositions(unsigned short bindIdx, const DataStreamPtr& stream,
            Mesh* pMesh, VertexData* dest);
        virtual void readGeometryNormals(unsigned short bindIdx, const DataStreamPtr& stream,
            Mesh* pMesh, VertexData* dest);
        virtual void readGeometryColours(unsigned short bindIdx, const DataStreamPtr& stream,
            Mesh* pMesh, VertexData* dest);
        virtual void readGeometryTexCoords(unsigned short bindIdx, const DataStreamPtr& stream,
            Mesh* pMesh, VertexData* dest, unsigned short set);
    };

    /** Class for providing backwards-compatibility for loading version 1.1 of the .mesh format. 
     This is a LEGACY FORMAT that pre-dates version Ogre 1.0
     */
    class MeshSerializerImpl_v1_1 : public MeshSerializerImpl_v1_2
    {
    public:
        MeshSerializerImpl_v1_1();
        ~MeshSerializerImpl_v1_1() override;
    protected:
        void readGeometryTexCoords(unsigned short bindIdx, const DataStreamPtr& stream,
            Mesh* pMesh, VertexData* dest, unsigned short set) override;
    };

    /** @} */
    /** @} */

}

#endif
