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

#include <cassert>
#include <cstring>

module Ogre.Core;

import :Animation;
import :AnimationTrack;
import :AxisAlignedBox;
import :Bitwise;
import :ColourValue;
import :Common;
import :DataStream;
import :DistanceLodStrategy;
import :Exception;
import :HardwareBuffer;
import :HardwareBufferManager;
import :HardwareIndexBuffer;
import :LodStrategy;
import :LodStrategyManager;
import :Log;
import :LogManager;
import :MaterialManager;
import :Math;
import :Mesh;
import :MeshFileFormat;
import :MeshSerializer;
import :MeshSerializerImpl;
import :Platform;
import :Pose;
import :RenderOperation;
import :SharedPtr;
import :SubMesh;
import :Vector;
import :VertexIndexData;

import <format>;
import <list>;
import <map>;
import <memory>;
import <string>;
import <unordered_map>;
import <utility>;
import <vector>;

namespace Ogre {

    /// stream overhead = ID + size
    const long MSTREAM_OVERHEAD_SIZE = sizeof(uint16) + sizeof(uint32);
    //---------------------------------------------------------------------
    MeshSerializerImpl::MeshSerializerImpl()
    {
        // Version number
        mVersion = "[MeshSerializer_v1.100]";
    }
    //---------------------------------------------------------------------
    MeshSerializerImpl::~MeshSerializerImpl()
    = default;
    //---------------------------------------------------------------------
    void MeshSerializerImpl::exportMesh(const Mesh* pMesh, 
        const DataStreamPtr stream, std::endian endianMode)
    {
        LogManager::getSingleton().logMessage(::std::format("MeshSerializer writing mesh data to stream {}...", stream->getName() ));

        // Decide on endian mode
        determineEndianness(endianMode);

        // Check that the mesh has it's bounds set
        if (pMesh->getBounds().isNull() || pMesh->getBoundingSphereRadius() == 0.0f)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "The Mesh you have supplied does not have its"
                " bounds completely defined. Define them first before exporting.",
                "MeshSerializerImpl::exportMesh");
        }
        mStream = stream;
        if (!mStream->isWriteable())
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                ::std::format("Unable to use stream {} for writing", mStream->getName() ),
                "MeshSerializerImpl::exportMesh");
        }

        writeFileHeader();
        LogManager::getSingleton().logMessage("File header written.");


        LogManager::getSingleton().logMessage("Writing mesh data...");
        pushInnerChunk(mStream);
        writeMesh(pMesh);
        popInnerChunk(mStream);
        LogManager::getSingleton().logMessage("Mesh data exported.");

        LogManager::getSingleton().logMessage("MeshSerializer export successful.");
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::importMesh(const DataStreamPtr& stream, Mesh* pMesh, MeshSerializerListener *listener)
    {
        // Determine endianness (must be the first thing we do!)
        determineEndianness(stream);

        // Check header
        readFileHeader(stream);
        pushInnerChunk(stream);
        auto streamID = static_cast<MeshChunkID>(readChunk(stream));

        while(!stream->eof())
        {
            using enum MeshChunkID;
            switch (streamID)
            {
            case MESH:
                readMesh(stream, pMesh, listener);
                break;
            default:
                break;
            }

            streamID = static_cast<MeshChunkID>(readChunk(stream));
        }
        popInnerChunk(stream);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeMesh(const Mesh* pMesh)
    {
        exportedLodCount = 1; // generate edge data for original mesh

        // Header
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH), calcMeshSize(pMesh));
        {
        // bool skeletallyAnimated
        bool skelAnim = pMesh->hasSkeleton();
        writeBools(&skelAnim, 1);

            pushInnerChunk(mStream);
            
        // Write shared geometry
        if (pMesh->sharedVertexData)
            writeGeometry(pMesh->sharedVertexData);

        // Write Submeshes
        for (unsigned short i = 0; i < pMesh->getNumSubMeshes(); ++i)
        {
            LogManager::getSingleton().logMessage("Writing submesh...");
            writeSubMesh(pMesh->getSubMesh(i));
            LogManager::getSingleton().logMessage("Submesh exported.");
        }

        // Write skeleton info if required
        if (pMesh->hasSkeleton())
        {
            LogManager::getSingleton().logMessage("Exporting skeleton link...");
            // Write skeleton link
            writeSkeletonLink(pMesh->getSkeletonName());
            LogManager::getSingleton().logMessage("Skeleton link exported.");

            // Write bone assignments
            if (!pMesh->mBoneAssignments.empty())
            {
                LogManager::getSingleton().logMessage("Exporting shared geometry bone assignments...");

                for (const auto & mBoneAssignment : pMesh->mBoneAssignments)
                {
                    writeMeshBoneAssignment(mBoneAssignment.second);
                }

                LogManager::getSingleton().logMessage("Shared geometry bone assignments exported.");
            }
        }

        // Write LOD data if any
        if (pMesh->getNumLodLevels() > 1)
        {
            LogManager::getSingleton().logMessage("Exporting LOD information....");
                writeLodLevel(pMesh);
            LogManager::getSingleton().logMessage("LOD information exported.");
        }
        
        // Write bounds information
        LogManager::getSingleton().logMessage("Exporting bounds information....");
        writeBoundsInfo(pMesh);
        LogManager::getSingleton().logMessage("Bounds information exported.");

        // Write submesh name table
        LogManager::getSingleton().logMessage("Exporting submesh name table...");
        writeSubMeshNameTable(pMesh);
        LogManager::getSingleton().logMessage("Submesh name table exported.");

        // Write edge lists
        if (pMesh->isEdgeListBuilt())
        {
            LogManager::getSingleton().logMessage("Exporting edge lists...");
            writeEdgeList(pMesh);
            LogManager::getSingleton().logMessage("Edge lists exported");
        }

        // Write morph animation
        writePoses(pMesh);
        if (pMesh->hasVertexAnimation())
        {
            writeAnimations(pMesh);
        }

        // Write submesh extremes
        writeExtremes(pMesh);
            popInnerChunk(mStream);
        }
    }
    //---------------------------------------------------------------------
    // Added by DrEvil
    void MeshSerializerImpl::writeSubMeshNameTable(const Mesh* pMesh)
    {
        // Header
        writeChunkHeader(std::to_underlying(MeshChunkID::SUBMESH_NAME_TABLE), calcSubMeshNameTableSize(pMesh));

        // Loop through and save out the index and names.
        pushInnerChunk(mStream);
        for (auto const& it : pMesh->mSubMeshNameMap)
        {
            // Header
            writeChunkHeader(std::to_underlying(MeshChunkID::SUBMESH_NAME_TABLE_ELEMENT), MSTREAM_OVERHEAD_SIZE +
                sizeof(unsigned short) + calcStringSize(it.first));

            // write the index
            writeShorts(&it.second, 1);
            // name
            writeString(it.first);
        }
        popInnerChunk(mStream);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeSubMesh(const SubMesh* s)
    {
        // Header
        writeChunkHeader(std::to_underlying(MeshChunkID::SUBMESH), calcSubMeshSize(s));

        // char* materialName
        writeString(s->getMaterialName());

        // bool useSharedVertices
        writeBools(&s->useSharedVertices, 1);

        auto indexCount = static_cast<unsigned int>(s->indexData->indexCount);
        writeInts(&indexCount, 1);

        // bool indexes32Bit
        bool idx32bit = (s->indexData->indexBuffer &&
            s->indexData->indexBuffer->getType() == HardwareIndexBuffer::IndexType::_32BIT);
        writeBools(&idx32bit, 1);

        if (indexCount > 0)
        {
            // unsigned short* faceVertexIndices ((indexCount)
            HardwareIndexBufferSharedPtr ibuf = s->indexData->indexBuffer;
            HardwareBufferLockGuard ibufLock(ibuf, HardwareBuffer::LockOptions::READ_ONLY);
            if (idx32bit)
            {
                auto* pIdx32 = static_cast<unsigned int*>(ibufLock.pData);
                writeInts(pIdx32, s->indexData->indexCount);
            }
            else
            {
                auto* pIdx16 = static_cast<unsigned short*>(ibufLock.pData);
                writeShorts(pIdx16, s->indexData->indexCount);
            }
        }

        pushInnerChunk(mStream);
        
        // MeshChunkID::GEOMETRY stream (Optional: present only if useSharedVertices = false)
        if (!s->useSharedVertices)
        {
            writeGeometry(s->vertexData.get());
        }

        // write out texture alias chunks
        writeSubMeshTextureAliases(s);

        // Operation type
        writeSubMeshOperation(s);

        // Bone assignments
        if (!s->mBoneAssignments.empty())
        {
            LogManager::getSingleton().logMessage("Exporting dedicated geometry bone assignments...");
            for (const auto & mBoneAssignment : s->mBoneAssignments)
            {
                writeSubMeshBoneAssignment(mBoneAssignment.second);
            }
            LogManager::getSingleton().logMessage("Dedicated geometry bone assignments exported.");
        }
        popInnerChunk(mStream);

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeExtremes(const Mesh *pMesh)
    {
        bool has_extremes = false;
        for (unsigned short i = 0; i < pMesh->getNumSubMeshes(); ++i)
        {
            SubMesh *sm = pMesh->getSubMesh(i);
            if (sm->extremityPoints.empty())
                continue;
            if (!has_extremes)
            {
                has_extremes = true;
                LogManager::getSingleton().logMessage("Writing submesh extremes...");
            }
            writeSubMeshExtremes(i, sm);
        }
        if (has_extremes)
            LogManager::getSingleton().logMessage("Extremes exported.");
    }
    auto MeshSerializerImpl::calcExtremesSize(const Mesh* pMesh) -> size_t
    {
        size_t size = 0;
        for (unsigned short i = 0; i < pMesh->getNumSubMeshes(); ++i)
        {
            SubMesh *sm = pMesh->getSubMesh(i);
            if (!sm->extremityPoints.empty()){
                size += calcSubMeshExtremesSize(i, sm);
            }
        }
        return size;
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeSubMeshExtremes(unsigned short idx, const SubMesh* s)
    {
        
        writeChunkHeader(std::to_underlying(MeshChunkID::TABLE_EXTREMES), calcSubMeshExtremesSize(idx, s));

        writeShorts(&idx, 1);

        auto *vertices = new float[s->extremityPoints.size() * 3];
        float *pVert = vertices;

        for (auto extremityPoint : s->extremityPoints)
        {
            *pVert++ = extremityPoint.x;
            *pVert++ = extremityPoint.y;
            *pVert++ = extremityPoint.z;
        }

        writeFloats(vertices, s->extremityPoints.size () * 3);
        delete[] vertices;
    }

    auto MeshSerializerImpl::calcSubMeshExtremesSize(unsigned short idx, const SubMesh* s) -> size_t
    {
        return MSTREAM_OVERHEAD_SIZE + sizeof (unsigned short) +
            s->extremityPoints.size() * sizeof (float)* 3;
    }


    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeSubMeshTextureAliases(const SubMesh* s)
    {
        size_t chunkSize;

        LogManager::getSingleton().logMessage("Exporting submesh texture aliases...");

        // iterate through texture aliases and write them out as a chunk
        for (const auto & mTextureAliase : s->mTextureAliases)
        {
            // calculate chunk size based on string length + 1.  Add 1 for the line feed.
            chunkSize = MSTREAM_OVERHEAD_SIZE + calcStringSize(mTextureAliase.first) + calcStringSize(mTextureAliase.second);
            writeChunkHeader(std::to_underlying(MeshChunkID::SUBMESH_TEXTURE_ALIAS), chunkSize);
            // write out alias name
            writeString(mTextureAliase.first);
            // write out texture name
            writeString(mTextureAliase.second);
        }

        LogManager::getSingleton().logMessage("Submesh texture aliases exported.");
    }

    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeSubMeshOperation(const SubMesh* sm)
    {
        // Header
        writeChunkHeader(std::to_underlying(MeshChunkID::SUBMESH_OPERATION), calcSubMeshOperationSize(sm));

        // unsigned short operationType
        auto opType = static_cast<unsigned short>(sm->operationType);
        writeShorts(&opType, 1);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeGeometry(const VertexData* vertexData)
    {
        const VertexDeclaration::VertexElementList& elemList =
            vertexData->vertexDeclaration->getElements();
        const VertexBufferBinding::VertexBufferBindingMap& bindings =
            vertexData->vertexBufferBinding->getBindings();

        // Header
        writeChunkHeader(std::to_underlying(MeshChunkID::GEOMETRY), calcGeometrySize(vertexData));

        auto vertexCount = static_cast<unsigned int>(vertexData->vertexCount);
        writeInts(&vertexCount, 1);

        pushInnerChunk(mStream);
        {
            // Vertex declaration
            size_t size = MSTREAM_OVERHEAD_SIZE + elemList.size() * (MSTREAM_OVERHEAD_SIZE + sizeof(unsigned short)* 5);
            writeChunkHeader(std::to_underlying(MeshChunkID::GEOMETRY_VERTEX_DECLARATION), size);

            pushInnerChunk(mStream);
            {
                unsigned short tmp;
                size = MSTREAM_OVERHEAD_SIZE + sizeof(unsigned short) * 5;
                for (const auto & elem : elemList)
                {
                    writeChunkHeader(std::to_underlying(MeshChunkID::GEOMETRY_VERTEX_ELEMENT), size);
                    // unsigned short source;   // buffer bind source
                    tmp = elem.getSource();
                    writeShorts(&tmp, 1);
                    // unsigned short type;     // VertexElementType
                    tmp = static_cast<unsigned short>(elem.getType());
                    writeShorts(&tmp, 1);
                    // unsigned short semantic; // VertexElementSemantic
                    tmp = static_cast<unsigned short>(elem.getSemantic());
                    writeShorts(&tmp, 1);
                    // unsigned short offset;   // start offset in buffer in bytes
                    tmp = static_cast<unsigned short>(elem.getOffset());
                    writeShorts(&tmp, 1);
                    // unsigned short index;    // index of the semantic (for colours and texture coords)
                    tmp = elem.getIndex();
                    writeShorts(&tmp, 1);

                }
            }
            popInnerChunk(mStream);


            // Buffers and bindings
            for (auto const& [key, vbuf] : bindings)
            {
                size_t vbufSizeInBytes = vbuf->getVertexSize() * vertexData->vertexCount; // vbuf->getSizeInBytes() is too large for meshes prepared for shadow volumes
                size = (MSTREAM_OVERHEAD_SIZE * 2) + (sizeof(unsigned short) * 2) + vbufSizeInBytes;
                writeChunkHeader(std::to_underlying(MeshChunkID::GEOMETRY_VERTEX_BUFFER),  size);
                // unsigned short bindIndex;    // Index to bind this buffer to
                unsigned short tmp = key;
                writeShorts(&tmp, 1);
                // unsigned short vertexSize;   // Per-vertex size, must agree with declaration at this index
                tmp = (unsigned short)vbuf->getVertexSize();
                writeShorts(&tmp, 1);
                pushInnerChunk(mStream);
                {
                    // Data
                    size = MSTREAM_OVERHEAD_SIZE + vbufSizeInBytes;
                    writeChunkHeader(std::to_underlying(MeshChunkID::GEOMETRY_VERTEX_BUFFER_DATA), size);
                    HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::READ_ONLY);

                    if (mFlipEndian)
                    {
                        // endian conversion
                        // Copy data
                        auto* tempData = new unsigned char[vbufSizeInBytes];
                        memcpy(tempData, vbufLock.pData, vbufSizeInBytes);
                        flipToLittleEndian(
                            tempData,
                            vertexData->vertexCount,
                            vbuf->getVertexSize(),
                            vertexData->vertexDeclaration->findElementsBySource(key));
                        writeData(tempData, vbuf->getVertexSize(), vertexData->vertexCount);
                        delete[] tempData;
                    }
                    else
                    {
                        writeData(vbufLock.pData, vbuf->getVertexSize(), vertexData->vertexCount);
                    }
                }
                popInnerChunk(mStream);
            }
        }
        popInnerChunk(mStream);
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcSubMeshNameTableSize(const Mesh* pMesh) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;
        // Figure out the size of the Name table.
        // Iterate through the subMeshList & add up the size of the indexes and names.
        for (auto const& it : pMesh->mSubMeshNameMap)
        {
            // size of the index + header size for each element chunk
            size += MSTREAM_OVERHEAD_SIZE + sizeof(uint16);
            // name
            size += calcStringSize(it.first);
        }

        // size of the sub-mesh name table.
        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcMeshSize(const Mesh* pMesh) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // bool hasSkeleton
        size += sizeof(bool);

        // Geometry
        if (pMesh->sharedVertexData)
        {
            size += calcGeometrySize(pMesh->sharedVertexData);
        }

        // Submeshes
        for (unsigned short i = 0; i < pMesh->getNumSubMeshes(); ++i)
        {
            size += calcSubMeshSize(pMesh->getSubMesh(i));
        }

        // Skeleton link
        if (pMesh->hasSkeleton())
        {
            size += calcSkeletonLinkSize(pMesh->getSkeletonName());
            // Write bone assignments
            size += pMesh->mBoneAssignments.size() * calcBoneAssignmentSize();
        }
        
        // Write LOD data if any
        if (pMesh->getNumLodLevels() > 1)
        {
            size += calcLodLevelSize(pMesh);
        }
        
        size += calcBoundsInfoSize(pMesh);

        // Submesh name table
        size += calcSubMeshNameTableSize(pMesh);

        // Edge list
        if (pMesh->isEdgeListBuilt())
        {
            size += calcEdgeListSize(pMesh);
        }

        // Morph animation
        size += calcPosesSize(pMesh);

        // Vertex animation
        if (pMesh->hasVertexAnimation())
        {
            size += calcAnimationsSize(pMesh);
        }

        size += calcExtremesSize(pMesh);

        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcSubMeshSize(const SubMesh* pSub) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;
        
        // Material name
        size += calcStringSize(pSub->getMaterialName());

        // bool useSharedVertices
        size += sizeof(bool);
        // unsigned int indexCount
        size += sizeof(unsigned int);
        // bool indexes32bit
        size += sizeof(bool);

        bool idx32bit = (pSub->indexData->indexBuffer &&
            pSub->indexData->indexBuffer->getType() == HardwareIndexBuffer::IndexType::_32BIT);
        // unsigned int* / unsigned short* faceVertexIndices
        if (idx32bit)
            size += sizeof(unsigned int) * pSub->indexData->indexCount;
        else
            size += sizeof(unsigned short) * pSub->indexData->indexCount;

        // Geometry
        if (!pSub->useSharedVertices)
        {
            size += calcGeometrySize(pSub->vertexData.get());
        }

        size += calcSubMeshTextureAliasesSize(pSub);
        size += calcSubMeshOperationSize(pSub);

        // Bone assignments
        if (!pSub->mBoneAssignments.empty())
        {
            for (auto vi = pSub->mBoneAssignments.begin();
                 vi != pSub->mBoneAssignments.end(); ++vi)
            {
                size += calcBoneAssignmentSize();
            }
        }

        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcSubMeshOperationSize(const SubMesh* pSub) -> size_t
    {
        return MSTREAM_OVERHEAD_SIZE + sizeof(uint16);
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcSubMeshTextureAliasesSize(const SubMesh* pSub) -> size_t
    {
        size_t chunkSize = 0;

        // iterate through texture alias map and calc size of strings
        for (const auto & mTextureAliase : pSub->mTextureAliases)
        {
            // calculate chunk size based on string length + 1.  Add 1 for the line feed.
            chunkSize += MSTREAM_OVERHEAD_SIZE + calcStringSize(mTextureAliase.first) + calcStringSize(mTextureAliase.second);
        }

        return chunkSize;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcGeometrySize(const VertexData* vertexData) -> size_t
    {
        const VertexDeclaration::VertexElementList& elemList =
            vertexData->vertexDeclaration->getElements();
        const VertexBufferBinding::VertexBufferBindingMap& bindings =
            vertexData->vertexBufferBinding->getBindings();
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // Vertex count
        size += sizeof(unsigned int);

        // Vertex declaration
        size += MSTREAM_OVERHEAD_SIZE + elemList.size() * (MSTREAM_OVERHEAD_SIZE + sizeof(unsigned short)* 5);
        
        // Buffers and bindings
        size += bindings.size() * ((MSTREAM_OVERHEAD_SIZE * 2) + (sizeof(unsigned short)* 2));

        // Buffer data
        for (auto const& [key, vbuf] : bindings)
        {
            size += vbuf->getVertexSize() * vertexData->vertexCount; // vbuf->getSizeInBytes() is too large for meshes prepared for shadow volumes
        }
        return size;
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readGeometry(const DataStreamPtr& stream, Mesh* pMesh,
        VertexData* dest)
    {

        dest->vertexStart = 0;

        unsigned int vertexCount = 0;
        readInts(stream, &vertexCount, 1);
        dest->vertexCount = vertexCount;
        // Find optional geometry streams
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            while(!stream->eof() &&
                (streamID == MeshChunkID::GEOMETRY_VERTEX_DECLARATION ||
                 streamID == MeshChunkID::GEOMETRY_VERTEX_BUFFER ))
            {
                using enum MeshChunkID;
                switch (streamID)
                {
                case GEOMETRY_VERTEX_DECLARATION:
                    readGeometryVertexDeclaration(stream, pMesh, dest);
                    break;
                case GEOMETRY_VERTEX_BUFFER:
                    readGeometryVertexBuffer(stream, pMesh, dest);
                    break;
                default:
                    break;
                }
                // Get next stream
                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }
            }
            if (!stream->eof())
            {
                // Backpedal back to start of non-submesh stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }

        // Perform any necessary colour conversions from ARGB to ABGR (UBYTE4)
        dest->convertPackedColour(VertexElementType::_DETAIL_SWAP_RB, VertexElementType::UBYTE4_NORM);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readGeometryVertexDeclaration(const DataStreamPtr& stream,
        Mesh* pMesh, VertexData* dest)
    {
        // Find optional geometry streams
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            while(!stream->eof() &&
                (streamID == MeshChunkID::GEOMETRY_VERTEX_ELEMENT ))
            {
                using enum MeshChunkID;
                switch (streamID)
                {
                case GEOMETRY_VERTEX_ELEMENT:
                    readGeometryVertexElement(stream, pMesh, dest);
                    break;
                default:
                    break;
                }
                // Get next stream
                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }
            }
            if (!stream->eof())
            {
                // Backpedal back to start of non-submesh stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readGeometryVertexElement(const DataStreamPtr& stream,
        Mesh* pMesh, VertexData* dest)
    {
        unsigned short source, offset, index, tmp;
        VertexElementType vType;
        VertexElementSemantic vSemantic;
        // unsigned short source;   // buffer bind source
        readShorts(stream, &source, 1);
        // unsigned short type;     // VertexElementType
        readShorts(stream, &tmp, 1);
        if(tmp == 4 || tmp == 11)
            vType = VertexElementType::UBYTE4_NORM;
        else
            vType = static_cast<VertexElementType>(tmp);
        // unsigned short semantic; // VertexElementSemantic
        readShorts(stream, &tmp, 1);
        vSemantic = static_cast<VertexElementSemantic>(tmp);
        // unsigned short offset;   // start offset in buffer in bytes
        readShorts(stream, &offset, 1);
        // unsigned short index;    // index of the semantic
        readShorts(stream, &index, 1);

        dest->vertexDeclaration->addElement(source, offset, vType, vSemantic, index);

        if (vType == VertexElementType::_DETAIL_SWAP_RB)
        {
            LogManager::getSingleton().stream(LogMessageLevel::Warning)
                << "Warning: VertexElementType::COLOUR_ARGB element type is deprecated and incurs conversion on load. "
                << "Use OgreMeshUpgrader on '" << pMesh->getName() << "' as soon as possible.";
        }

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readGeometryVertexBuffer(const DataStreamPtr& stream,
        Mesh* pMesh, VertexData* dest)
    {
        unsigned short bindIndex, vertexSize;
        // unsigned short bindIndex;    // Index to bind this buffer to
        readShorts(stream, &bindIndex, 1);
        // unsigned short vertexSize;   // Per-vertex size, must agree with declaration at this index
        readShorts(stream, &vertexSize, 1);
        pushInnerChunk(stream);
        {
        // Check for vertex data header
        auto headerID = static_cast<MeshChunkID>(readChunk(stream));
        if (headerID != MeshChunkID::GEOMETRY_VERTEX_BUFFER_DATA)
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, "Can't find vertex buffer data area",
                "MeshSerializerImpl::readGeometryVertexBuffer");
        }
        // Check that vertex size agrees
        if (dest->vertexDeclaration->getVertexSize(bindIndex) != vertexSize)
        {
            OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, "Buffer vertex size does not agree with vertex declaration",
                "MeshSerializerImpl::readGeometryVertexBuffer");
        }

        // Create / populate vertex buffer
        HardwareVertexBufferSharedPtr vbuf;
        vbuf = pMesh->getHardwareBufferManager()->createVertexBuffer(
            vertexSize,
            dest->vertexCount,
            pMesh->mVertexBufferUsage,
            pMesh->mVertexBufferShadowBuffer);
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        stream->read(vbufLock.pData, dest->vertexCount * vertexSize);

        // endian conversion for OSX
        flipFromLittleEndian(
            vbufLock.pData,
            dest->vertexCount,
            vertexSize,
            dest->vertexDeclaration->findElementsBySource(bindIndex));

        // Set binding
        dest->vertexBufferBinding->setBinding(bindIndex, vbuf);
        }
        popInnerChunk(stream);

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readSubMeshNameTable(const DataStreamPtr& stream, Mesh* pMesh)
    {
        // The map for
        std::map<unsigned short, String> subMeshNames;
        unsigned short subMeshIndex;

        // Need something to store the index, and the objects name
        // This table is a method that imported meshes can retain their naming
        // so that the names established in the modelling software can be used
        // to get the sub-meshes by name. The exporter must support exporting
        // the optional stream MeshChunkID::SUBMESH_NAME_TABLE.

        // Read in all the sub-streams. Each sub-stream should contain an index and Ogre::String for the name.
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            while(!stream->eof() && (streamID == MeshChunkID::SUBMESH_NAME_TABLE_ELEMENT ))
            {
                // Read in the index of the submesh.
                readShorts(stream, &subMeshIndex, 1);
                // Read in the String and map it to its index.
                subMeshNames[subMeshIndex] = readString(stream);

                // If we're not end of file get the next stream ID
                if (!stream->eof())
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
            }
            if (!stream->eof())
            {
                // Backpedal back to start of stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }

        // Set all the submeshes names
        // ?

        // Loop through and save out the index and names.
        for (auto const& it : subMeshNames)
        {
            // Name this submesh to the stored name.
            pMesh->nameSubMesh(it.second, it.first);
        }



    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readMesh(const DataStreamPtr& stream, Mesh* pMesh, MeshSerializerListener *listener)
    {
        // Never automatically build edge lists for this version
        // expect them in the file or not at all
        pMesh->mAutoBuildEdgeLists = false;

        // bool skeletallyAnimated
        bool skeletallyAnimated;
        readBools(stream, &skeletallyAnimated, 1);

        // Find all substreams
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            using enum MeshChunkID;
            while(!stream->eof() &&
                (streamID == GEOMETRY ||
                 streamID == SUBMESH ||
                 streamID == MESH_SKELETON_LINK ||
                 streamID == MESH_BONE_ASSIGNMENT ||
                 streamID == MESH_LOD_LEVEL ||
                 streamID == MESH_BOUNDS ||
                 streamID == SUBMESH_NAME_TABLE ||
                 streamID == EDGE_LISTS ||
                 streamID == POSES ||
                 streamID == ANIMATIONS ||
                 streamID == TABLE_EXTREMES))
            {
                switch(streamID)
                {
                case GEOMETRY:
                    pMesh->sharedVertexData = new VertexData();
                    try {
                        readGeometry(stream, pMesh, pMesh->sharedVertexData);
                    }
                    catch (ItemIdentityException&)
                    {
                        // duff geometry data entry with 0 vertices
                        delete pMesh->sharedVertexData;
                        pMesh->sharedVertexData = nullptr;
                        // Skip this stream (pointer will have been returned to just after header)
                        stream->skip(mCurrentstreamLen - MSTREAM_OVERHEAD_SIZE);
                    }
                    break;
                case SUBMESH:
                    readSubMesh(stream, pMesh, listener);
                    break;
                case MESH_SKELETON_LINK:
                    readSkeletonLink(stream, pMesh, listener);
                    break;
                case MESH_BONE_ASSIGNMENT:
                    readMeshBoneAssignment(stream, pMesh);
                    break;
                case MESH_LOD_LEVEL:
                    readMeshLodLevel(stream, pMesh);
                    break;
                case MESH_BOUNDS:
                    readBoundsInfo(stream, pMesh);
                    break;
                case SUBMESH_NAME_TABLE:
                    readSubMeshNameTable(stream, pMesh);
                    break;
                case EDGE_LISTS:
                    readEdgeList(stream, pMesh);
                    break;
                case POSES:
                    readPoses(stream, pMesh);
                    break;
                case ANIMATIONS:
                    readAnimations(stream, pMesh);
                    break;
                case TABLE_EXTREMES:
                    readExtremes(stream, pMesh);
                    break;
                default:
                    break;
                }

                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }

            }
            if (!stream->eof())
            {
                // Backpedal back to start of stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readSubMesh(const DataStreamPtr& stream, Mesh* pMesh, MeshSerializerListener *listener)
    {
        SubMesh* sm = pMesh->createSubMesh();

        // char* materialName
        String materialName = readString(stream);
        if(listener)
            listener->processMaterialName(pMesh, &materialName);
        if (auto material = MaterialManager::getSingleton().getByName(materialName, pMesh->getGroup()))
        {
            sm->setMaterial(material);
        }
        else
        {
            LogManager::getSingleton().logWarning(
                ::std::format("Can't assign material '"
                "' to SubMesh of '{}' because this "
                "Material does not exist in group '{}'. Have you forgotten to define it in a "
                ".material script?", materialName, pMesh->getName(), pMesh->getGroup()));
        }

        // bool useSharedVertices
        readBools(stream,&sm->useSharedVertices, 1);

        sm->indexData->indexStart = 0;
        unsigned int indexCount = 0;
        readInts(stream, &indexCount, 1);
        sm->indexData->indexCount = indexCount;

        HardwareIndexBufferSharedPtr ibuf;
        // bool indexes32Bit
        bool idx32bit;
        readBools(stream, &idx32bit, 1);
        if (indexCount > 0)
        {
            if (idx32bit)
            {
                ibuf = pMesh->getHardwareBufferManager()->createIndexBuffer(
                        HardwareIndexBuffer::IndexType::_32BIT,
                        sm->indexData->indexCount,
                        pMesh->mIndexBufferUsage,
                        pMesh->mIndexBufferShadowBuffer);
                HardwareBufferLockGuard ibufLock(ibuf, HardwareBuffer::LockOptions::DISCARD);
                readInts(stream, static_cast<unsigned int*>(ibufLock.pData), sm->indexData->indexCount);

            }
            else // 16-bit
            {
                ibuf = pMesh->getHardwareBufferManager()->createIndexBuffer(
                        HardwareIndexBuffer::IndexType::_16BIT,
                        sm->indexData->indexCount,
                        pMesh->mIndexBufferUsage,
                        pMesh->mIndexBufferShadowBuffer);
                HardwareBufferLockGuard ibufLock(ibuf, HardwareBuffer::LockOptions::DISCARD);
                readShorts(stream, static_cast<unsigned short*>(ibufLock.pData), sm->indexData->indexCount);
            }
        }
        sm->indexData->indexBuffer = ibuf;

        pushInnerChunk(stream);
        {
        // MeshChunkID::GEOMETRY stream (Optional: present only if useSharedVertices = false)
        if (!sm->useSharedVertices)
        {
            auto const streamID = static_cast<MeshChunkID>(readChunk(stream));
            if (streamID != MeshChunkID::GEOMETRY)
            {
                OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, "Missing geometry data in mesh file",
                    "MeshSerializerImpl::readSubMesh");
            }
            sm->vertexData = ::std::make_unique<VertexData>();
            readGeometry(stream, pMesh, sm->vertexData.get());
        }


        // Find all bone assignments, submesh operation, and texture aliases (if present)
        if (!stream->eof())
        {
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            bool seenTexAlias = false;
            using enum MeshChunkID;
            while(!stream->eof() &&
                (streamID == SUBMESH_BONE_ASSIGNMENT ||
                 streamID == SUBMESH_OPERATION ||
                 streamID == SUBMESH_TEXTURE_ALIAS))
            {
                switch(streamID)
                {
                case SUBMESH_OPERATION:
                    readSubMeshOperation(stream, pMesh, sm);
                    break;
                case SUBMESH_BONE_ASSIGNMENT:
                    readSubMeshBoneAssignment(stream, pMesh, sm);
                    break;
                case SUBMESH_TEXTURE_ALIAS:
                    seenTexAlias = true;
                    readSubMeshTextureAlias(stream, pMesh, sm);
                    break;
                default:
                    break;
                }

                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }

            }

            if (seenTexAlias)
                LogManager::getSingleton().logWarning(std::format("texture aliases for SubMeshes are deprecated - {}",
                                                      stream->getName()));

            if (!stream->eof())
            {
                // Backpedal back to start of stream
                    backpedalChunkHeader(stream);
                }
            }
        }
        popInnerChunk(stream);

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readSubMeshOperation(const DataStreamPtr& stream,
        Mesh* pMesh, SubMesh* sm)
    {
        // unsigned short operationType
        unsigned short opType;
        readShorts(stream, &opType, 1);
        sm->operationType = static_cast<RenderOperation::OperationType>(opType);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readSubMeshTextureAlias(const DataStreamPtr& stream, Mesh* pMesh, SubMesh* sub)
    {
        String aliasName = readString(stream);
        String textureName = readString(stream);
        sub->addTextureAlias(aliasName, textureName);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeSkeletonLink(std::string_view skelName)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_SKELETON_LINK), calcSkeletonLinkSize(skelName));

        writeString(skelName);

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readSkeletonLink(const DataStreamPtr& stream, Mesh* pMesh, MeshSerializerListener *listener)
    {
        String skelName = readString(stream);

        if(listener)
            listener->processSkeletonName(pMesh, &skelName);

        pMesh->setSkeletonName(skelName);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readTextureLayer(const DataStreamPtr& stream, Mesh* pMesh,
        MaterialPtr& pMat)
    {
        // Material definition section phased out of 1.1
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcSkeletonLinkSize(std::string_view skelName) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        size += calcStringSize(skelName);

        return size;

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeMeshBoneAssignment(const VertexBoneAssignment& assign)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_BONE_ASSIGNMENT), calcBoneAssignmentSize());

        // unsigned int vertexIndex;
        writeInts(&(assign.vertexIndex), 1);
        // unsigned short boneIndex;
        writeShorts(&(assign.boneIndex), 1);
        // float weight;
        writeFloats(&(assign.weight), 1);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeSubMeshBoneAssignment(const VertexBoneAssignment& assign)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::SUBMESH_BONE_ASSIGNMENT), calcBoneAssignmentSize());

        // unsigned int vertexIndex;
        writeInts(&(assign.vertexIndex), 1);
        // unsigned short boneIndex;
        writeShorts(&(assign.boneIndex), 1);
        // float weight;
        writeFloats(&(assign.weight), 1);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readMeshBoneAssignment(const DataStreamPtr& stream, Mesh* pMesh)
    {
        VertexBoneAssignment assign;

        // unsigned int vertexIndex;
        readInts(stream, &(assign.vertexIndex),1);
        // unsigned short boneIndex;
        readShorts(stream, &(assign.boneIndex),1);
        // float weight;
        readFloats(stream, &(assign.weight), 1);

        pMesh->addBoneAssignment(assign);

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readSubMeshBoneAssignment(const DataStreamPtr& stream,
        Mesh* pMesh, SubMesh* sub)
    {
        VertexBoneAssignment assign;

        // unsigned int vertexIndex;
        readInts(stream, &(assign.vertexIndex),1);
        // unsigned short boneIndex;
        readShorts(stream, &(assign.boneIndex),1);
        // float weight;
        readFloats(stream, &(assign.weight), 1);

        sub->addBoneAssignment(assign);

    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcBoneAssignmentSize() -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // Vert index
        size += sizeof(unsigned int);
        // Bone index
        size += sizeof(unsigned short);
        // weight
        size += sizeof(float);

        return size;
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeLodLevel(const Mesh* pMesh)
    {
        const LodStrategy *strategy = pMesh->getLodStrategy();
        exportedLodCount = pMesh->getNumLodLevels();
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_LEVEL), calcLodLevelSize(pMesh));
        writeString(strategy->getName()); // string strategyName;
        writeShorts(&exportedLodCount, 1); // unsigned short numLevels;

        pushInnerChunk(mStream);
        // Loop from LOD 1 (not 0, this is full detail)
        for (ushort i = 1; i < exportedLodCount; ++i)
        {
            const MeshLodUsage& usage = pMesh->mMeshLodUsageList[i];
            if (pMesh->_isManualLodLevel(i))
            {
                writeLodUsageManual(usage);
            }
            else
            {
                writeLodUsageGenerated(pMesh, usage, i);
            }
        }
        popInnerChunk(mStream);
    }

    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeLodUsageManual(const MeshLodUsage& usage)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_MANUAL), calcLodUsageManualSize(usage));
        auto userValue = static_cast<float>(usage.userValue);
        writeFloats(&userValue, 1);
        writeString(usage.manualName);
    }

    void MeshSerializerImpl::writeLodUsageGeneratedSubmesh( const SubMesh* submesh, unsigned short lodNum )
    {
        const IndexData* indexData = submesh->mLodFaceList[lodNum-1];
        HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;
        assert(ibuf);
        unsigned int bufferIndex = -1;
        for(ushort i = 1; i < lodNum; i++){
            // it will check any previous Lod levels for the same buffer.
            // This will allow to use merged/shared/compressed buffers.
            const IndexData* prevIndexData = submesh->mLodFaceList[i-1];
            if(prevIndexData->indexCount != 0 && prevIndexData->indexBuffer == indexData->indexBuffer){
                bufferIndex = i;
            }
        }

        auto indexCount = static_cast<unsigned int>(indexData->indexCount);
        writeInts(&indexCount, 1);
        auto indexStart = static_cast<unsigned int>(indexData->indexStart);
        writeInts(&indexStart, 1);
        writeInts(&bufferIndex, 1);

        if(bufferIndex == (unsigned int)-1) { // It has its own buffer (Not compressed).
            bool is32BitIndices = (ibuf->getType() == HardwareIndexBuffer::IndexType::_32BIT);
            writeBools(&is32BitIndices, 1);

            auto bufIndexCount = static_cast<unsigned int>(ibuf->getNumIndexes());
            writeInts(&bufIndexCount, 1);

            if (bufIndexCount > 0)
            {
                if (is32BitIndices)
                {
                    HardwareBufferLockGuard ibufLock(ibuf, HardwareBuffer::LockOptions::READ_ONLY);
                    writeInts(static_cast<unsigned int*>(ibufLock.pData), bufIndexCount);
                }
                else
                {
                    HardwareBufferLockGuard ibufLock(ibuf, HardwareBuffer::LockOptions::READ_ONLY);
                    writeShorts(static_cast<unsigned short*>(ibufLock.pData), bufIndexCount);
                }
            }
        }
    }
    void MeshSerializerImpl::writeLodUsageGenerated(const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_GENERATED), calcLodUsageGeneratedSize(pMesh, usage, lodNum));
        auto userValue = static_cast<float>(usage.userValue);
        writeFloats(&userValue, 1);
        for (ushort i = 0; i < pMesh->getNumSubMeshes(); i++)
        {
            SubMesh* submesh = pMesh->getSubMesh(i);
            writeLodUsageGeneratedSubmesh(submesh, lodNum);
        }
    }

    auto MeshSerializerImpl::calcLodLevelSize(const Mesh* pMesh) -> size_t
    {
        exportedLodCount = pMesh->getNumLodLevels();
        size_t size = MSTREAM_OVERHEAD_SIZE; // Header
        size += calcStringSize(pMesh->getLodStrategy()->getName()); // string strategyName;
        size += sizeof(unsigned short); // unsigned short numLevels;
        //size += sizeof(bool); // bool manual; <== this is removed in v1_9

        // Loop from LOD 1 (not 0, this is full detail)
        for (ushort i = 1; i < exportedLodCount; ++i)
        {
            const MeshLodUsage& usage = pMesh->mMeshLodUsageList[i];
            if (pMesh->_isManualLodLevel(i))
            {
                size += calcLodUsageManualSize(usage);
            }
            else
            {
                size += calcLodUsageGeneratedSize(pMesh, usage, i);
            }
        }
        return size;
    }

    auto MeshSerializerImpl::calcLodUsageManualSize(const MeshLodUsage& usage) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;// Header
        size += sizeof(float);// float usage.userValue;
        size += calcStringSize(usage.manualName);
        return size;
    }

    auto MeshSerializerImpl::calcLodUsageGeneratedSize(const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;
        size += sizeof(float); // float usage.userValue;
        for (ushort i = 0; i < pMesh->getNumSubMeshes(); i++)
        {
            SubMesh* submesh = pMesh->getSubMesh(i);
            size += calcLodUsageGeneratedSubmeshSize(submesh, lodNum);
        }
        return size;
    }
    auto MeshSerializerImpl::calcLodUsageGeneratedSubmeshSize( const SubMesh* submesh, unsigned short lodNum ) -> size_t
    {
        size_t size = 0;
        
        const IndexData* indexData = submesh->mLodFaceList[lodNum-1];
        HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;
        assert(ibuf);
        unsigned int bufferIndex = -1;
        for(ushort i = 1; i < lodNum; i++){
            // it will check any previous Lod levels for the same buffer.
            // This will allow to use merged/shared/compressed buffers.
            const IndexData* prevIndexData = submesh->mLodFaceList[i-1];
            if(prevIndexData->indexCount != 0 && prevIndexData->indexBuffer == indexData->indexBuffer){
                bufferIndex = i;
            }
        }
        
        size += sizeof(unsigned int); // unsigned int indexData->indexCount;
        size += sizeof(unsigned int); // unsigned int indexData->indexStart;
        size += sizeof(unsigned int); // unsigned int bufferIndex;
        if(bufferIndex == (unsigned int)-1) {
            size += sizeof(bool); // bool indexes32Bit
            size += sizeof(unsigned int); // unsigned int ibuf->getNumIndexes()
            size += !ibuf ? 0 : static_cast<unsigned long>(ibuf->getIndexSize() * ibuf->getNumIndexes()); // faces
        }
        return size;
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeBoundsInfo(const Mesh* pMesh)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_BOUNDS), calcBoundsInfoSize(pMesh));

        // float minx, miny, minz
        const Vector3& min = pMesh->mAABB.getMinimum();
        const Vector3& max = pMesh->mAABB.getMaximum();
        writeFloats(&min.x, 1);
        writeFloats(&min.y, 1);
        writeFloats(&min.z, 1);
        // float maxx, maxy, maxz
        writeFloats(&max.x, 1);
        writeFloats(&max.y, 1);
        writeFloats(&max.z, 1);
        // float radius
        writeFloats(&pMesh->mBoundRadius, 1);

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readBoundsInfo(const DataStreamPtr& stream, Mesh* pMesh)
    {
        Vector3 min, max;
        // float minx, miny, minz
        readFloats(stream, &min.x, 1);
        readFloats(stream, &min.y, 1);
        readFloats(stream, &min.z, 1);
        // float maxx, maxy, maxz
        readFloats(stream, &max.x, 1);
        readFloats(stream, &max.y, 1);
        readFloats(stream, &max.z, 1);

        using enum AxisAlignedBox::Extent;
        AxisAlignedBox box{Finite, min, max};
        pMesh->_setBounds(box, false);
        // float radius
        float radius;
        readFloats(stream, &radius, 1);
        pMesh->_setBoundingSphereRadius(radius);
    }
    auto MeshSerializerImpl::calcBoundsInfoSize(const Mesh* pMesh) -> size_t
    {
        unsigned long size = MSTREAM_OVERHEAD_SIZE;
        size += sizeof(float) * 7;
        return size;
    }
    //---------------------------------------------------------------------

    void MeshSerializerImpl::readMeshLodLevel(const DataStreamPtr& stream, Mesh* pMesh)
    {
        // Read the strategy to be used for this mesh
        String strategyName = readString(stream);
        LodStrategy *strategy = LodStrategyManager::getSingleton().getStrategy(strategyName);

        // Check that valid strategy name was given, otherwise use default
        if (strategy == nullptr)
            strategy = LodStrategyManager::getSingleton().getDefaultStrategy();

        pMesh->setLodStrategy(strategy);

        // unsigned short numLevels;
        readShorts(stream, &(pMesh->mNumLods), 1);

        pMesh->mMeshLodUsageList.resize(pMesh->mNumLods);
        ushort numSubs, i;
        numSubs = pMesh->getNumSubMeshes();
        for (i = 0; i < numSubs; ++i)
        {
            SubMesh* sm = pMesh->getSubMesh(i);
            assert(sm->mLodFaceList.empty());
            sm->mLodFaceList.resize(pMesh->mNumLods-1);
        }
        pushInnerChunk(stream);
        // lodID=0 is the original mesh. We need to skip it.
        for(int lodID = 1; lodID < pMesh->mNumLods; lodID++){
            // Read depth
            MeshLodUsage& usage = pMesh->mMeshLodUsageList[lodID];
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            readFloats(stream, &(usage.userValue), 1);
            using enum MeshChunkID;
            switch(streamID){
            case MESH_LOD_MANUAL:
                readMeshLodUsageManual(stream, pMesh, lodID, usage);
                break;
            case MESH_LOD_GENERATED:
                readMeshLodUsageGenerated(stream, pMesh, lodID, usage);
                break;
            default:
                OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                ::std::format("Invalid Lod Usage type in {}", pMesh->getName()),
                    "MeshSerializerImpl::readMeshLodInfo");
            }
            usage.manualMesh.reset(); // will trigger load later with manual Lod
            usage.edgeData = nullptr;
        }
        popInnerChunk(stream);
    }

    //---------------------------------------------------------------------
    void MeshSerializerImpl::readMeshLodUsageManual( const DataStreamPtr& stream, Mesh* pMesh, unsigned short lodNum, MeshLodUsage& usage )
    {
        pMesh->mHasManualLodLevel = true;
        usage.manualName = readString(stream);
        
        // Generate for mixed
        ushort numSubs, i;
        numSubs = pMesh->getNumSubMeshes();
        for (i = 0; i < numSubs; ++i)
        {
            
            SubMesh* sm = pMesh->getSubMesh(i);
            sm->mLodFaceList[lodNum-1] = new IndexData();
        }
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readMeshLodUsageGenerated( const DataStreamPtr& stream, Mesh* pMesh, unsigned short lodNum, MeshLodUsage& usage )
    {
        usage.manualName = "";

        // Get one set of detail per SubMesh
        unsigned short numSubs, i;
        numSubs = pMesh->getNumSubMeshes();
        for (i = 0; i < numSubs; ++i)
        {
            SubMesh* sm = pMesh->getSubMesh(i);
            sm->mLodFaceList[lodNum-1] = new IndexData();
            IndexData* indexData = sm->mLodFaceList[lodNum-1];

            unsigned int numIndexes;
            readInts(stream, &numIndexes, 1);
            indexData->indexCount = static_cast<size_t>(numIndexes);

            unsigned int offset;
            readInts(stream, &offset, 1);
            indexData->indexStart = static_cast<size_t>(offset);

            // For merged buffers, you can pass the index of previous Lod.
            // To create buffer it should be -1.
            unsigned int bufferIndex;
            readInts(stream, &bufferIndex, 1);
            if(bufferIndex != (unsigned int)-1) {
                // copy buffer pointer
                indexData->indexBuffer = sm->mLodFaceList[bufferIndex-1]->indexBuffer;
                assert(indexData->indexBuffer);
            } else {
                // generate buffers

                // bool indexes32Bit
                bool idx32Bit;
                readBools(stream, &idx32Bit, 1);
                
                unsigned int buffIndexCount;
                readInts(stream, &buffIndexCount, 1);

                indexData->indexBuffer = pMesh->getHardwareBufferManager()->createIndexBuffer(
                    idx32Bit ? HardwareIndexBuffer::IndexType::_32BIT : HardwareIndexBuffer::IndexType::_16BIT,
                    buffIndexCount, pMesh->mIndexBufferUsage, pMesh->mIndexBufferShadowBuffer);
                HardwareBufferLockGuard ibufLock(indexData->indexBuffer, HardwareBuffer::LockOptions::DISCARD);

                if (idx32Bit)
                {
                    readInts(stream, (uint32*)ibufLock.pData, buffIndexCount);
                }
                else
                {
                    readShorts(stream, (uint16*)ibufLock.pData, buffIndexCount);
                }
            }
        }
    }

    //---------------------------------------------------------------------
    void MeshSerializerImpl::flipFromLittleEndian(void* pData, size_t vertexCount,
        size_t vertexSize, const VertexDeclaration::VertexElementList& elems)
    {
        if (mFlipEndian)
        {
            flipEndian(pData, vertexCount, vertexSize, elems);
        }
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::flipToLittleEndian(void* pData, size_t vertexCount,
            size_t vertexSize, const VertexDeclaration::VertexElementList& elems)
    {
        if (mFlipEndian)
        {
            flipEndian(pData, vertexCount, vertexSize, elems);
        }
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::flipEndian(void* pData, size_t vertexCount,
        size_t vertexSize, const VertexDeclaration::VertexElementList& elems)
    {
        void *pBase = pData;
        for (size_t v = 0; v < vertexCount; ++v)
        {
            for (const auto & elem : elems)
            {
                void *pElem;
                // re-base pointer to the element
                elem.baseVertexPointerToElement(pBase, &pElem);
                // Flip the endian based on the type
                size_t typeSize = 0;
                switch (VertexElement::getBaseType(elem.getType()))
                {
                    using enum VertexElementType;
                    case FLOAT1:
                        typeSize = sizeof(float);
                        break;
                    case DOUBLE1:
                        typeSize = sizeof(double);
                        break;
                    case SHORT1:
                        typeSize = sizeof(short);
                        break;
                    case USHORT1:
                        typeSize = sizeof(unsigned short);
                        break;
                    case INT1:
                        typeSize = sizeof(int);
                        break;
                    case UINT1:
                        typeSize = sizeof(unsigned int);
                        break;
                    case UBYTE4_NORM:
                    case UBYTE4:
                        typeSize = 0; // NO FLIPPING
                        break;
                    default:
                        assert(false); // Should never happen
                };
				Bitwise::bswapChunks(pElem, typeSize,
                    VertexElement::getTypeCount(elem.getType()));

            }

            pBase = static_cast<void*>(
                static_cast<unsigned char*>(pBase) + vertexSize);

        }
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcEdgeListSize(const Mesh* pMesh) -> size_t
    {

        size_t size = MSTREAM_OVERHEAD_SIZE;

        for (ushort i = 0; i < exportedLodCount; ++i)
        {

            const EdgeData* edgeData = pMesh->getEdgeList(i);
            bool isManual = !pMesh->mMeshLodUsageList[i].manualName.empty();

            size += calcEdgeListLodSize(edgeData, isManual);

        }

        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcEdgeListLodSize(const EdgeData* edgeData, bool isManual) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // unsigned short lodIndex
        size += sizeof(uint16);

        // bool isManual            // If manual, no edge data here, loaded from manual mesh
        size += sizeof(bool);
        if (!isManual)
        {
            // bool isClosed
            size += sizeof(bool);
            // unsigned long numTriangles
            size += sizeof(uint32);
            // unsigned long numEdgeGroups
            size += sizeof(uint32);
            // Triangle* triangleList
            size_t triSize = 0;
            // unsigned long indexSet
            // unsigned long vertexSet
            // unsigned long vertIndex[3]
            // unsigned long sharedVertIndex[3]
            // float normal[4]
            triSize += sizeof(uint32) * 8
                    + sizeof(float) * 4;

            size += triSize * edgeData->triangles.size();
            // Write the groups
            for (const auto & edgeGroup : edgeData->edgeGroups)
            {
                size += calcEdgeGroupSize(edgeGroup);
            }

        }

        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcEdgeGroupSize(const EdgeData::EdgeGroup& group) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // unsigned long vertexSet
        size += sizeof(uint32);
        // unsigned long triStart
        size += sizeof(uint32);
        // unsigned long triCount
        size += sizeof(uint32);
        // unsigned long numEdges
        size += sizeof(uint32);
        // Edge* edgeList
        size_t edgeSize = 0;
        // unsigned long  triIndex[2]
        // unsigned long  vertIndex[2]
        // unsigned long  sharedVertIndex[2]
        // bool degenerate
        edgeSize += sizeof(uint32) * 6 + sizeof(bool);
        size += edgeSize * group.edges.size();

        return size;
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeEdgeList(const Mesh* pMesh)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::EDGE_LISTS), calcEdgeListSize(pMesh));
        pushInnerChunk(mStream);
        {
        for (ushort i = 0; i < exportedLodCount; ++i)
        {
            const EdgeData* edgeData = pMesh->getEdgeList(i);
                bool isManual = !pMesh->mMeshLodUsageList[i].manualName.empty();
            writeChunkHeader(std::to_underlying(MeshChunkID::EDGE_LIST_LOD), calcEdgeListLodSize(edgeData, isManual));

            // unsigned short lodIndex
            writeShorts(&i, 1);

            // bool isManual            // If manual, no edge data here, loaded from manual mesh
            writeBools(&isManual, 1);
            if (!isManual)
            {
                // bool isClosed
                writeBools(&edgeData->isClosed, 1);
                // unsigned long  numTriangles
                auto count = static_cast<uint32>(edgeData->triangles.size());
                writeInts(&count, 1);
                // unsigned long numEdgeGroups
                count = static_cast<uint32>(edgeData->edgeGroups.size());
                writeInts(&count, 1);
                // Triangle* triangleList
                // Iterate rather than writing en-masse to allow endian conversion
                for (auto fni = edgeData->triangleFaceNormals.begin();
                     const EdgeData::Triangle& tri : edgeData->triangles)
                {
                    // unsigned long indexSet;
                    uint32 tmp[3];
                    tmp[0] = static_cast<uint32>(tri.indexSet);
                    writeInts(tmp, 1);
                    // unsigned long vertexSet;
                    tmp[0] = static_cast<uint32>(tri.vertexSet);
                    writeInts(tmp, 1);
                    // unsigned long vertIndex[3];
                    tmp[0] = static_cast<uint32>(tri.vertIndex[0]);
                    tmp[1] = static_cast<uint32>(tri.vertIndex[1]);
                    tmp[2] = static_cast<uint32>(tri.vertIndex[2]);
                    writeInts(tmp, 3);
                    // unsigned long sharedVertIndex[3];
                    tmp[0] = static_cast<uint32>(tri.sharedVertIndex[0]);
                    tmp[1] = static_cast<uint32>(tri.sharedVertIndex[1]);
                    tmp[2] = static_cast<uint32>(tri.sharedVertIndex[2]);
                    writeInts(tmp, 3);
                    // float normal[4];
                    writeFloats(&(fni->x), 4);

                    ++fni;
                }
                    pushInnerChunk(mStream);
                    {
                // Write the groups
                for (const auto & edgeGroup : edgeData->edgeGroups)
                {
                    writeChunkHeader(std::to_underlying(MeshChunkID::EDGE_GROUP), calcEdgeGroupSize(edgeGroup));
                    // unsigned long vertexSet
                    auto vertexSet = static_cast<uint32>(edgeGroup.vertexSet);
                    writeInts(&vertexSet, 1);
                    // unsigned long triStart
                    auto triStart = static_cast<uint32>(edgeGroup.triStart);
                    writeInts(&triStart, 1);
                    // unsigned long triCount
                    auto triCount = static_cast<uint32>(edgeGroup.triCount);
                    writeInts(&triCount, 1);
                    // unsigned long numEdges
                    count = static_cast<uint32>(edgeGroup.edges.size());
                    writeInts(&count, 1);
                    // Edge* edgeList
                    // Iterate rather than writing en-masse to allow endian conversion
                    for (const auto & edge : edgeGroup.edges)
                    {
                        uint32 tmp[2];
                        // unsigned long  triIndex[2]
                        tmp[0] = static_cast<uint32>(edge.triIndex[0]);
                        tmp[1] = static_cast<uint32>(edge.triIndex[1]);
                        writeInts(tmp, 2);
                        // unsigned long  vertIndex[2]
                        tmp[0] = static_cast<uint32>(edge.vertIndex[0]);
                        tmp[1] = static_cast<uint32>(edge.vertIndex[1]);
                        writeInts(tmp, 2);
                        // unsigned long  sharedVertIndex[2]
                        tmp[0] = static_cast<uint32>(edge.sharedVertIndex[0]);
                        tmp[1] = static_cast<uint32>(edge.sharedVertIndex[1]);
                        writeInts(tmp, 2);
                        // bool degenerate
                        writeBools(&(edge.degenerate), 1);
                    }

                }
                    }
                    popInnerChunk(mStream);

                }

            }
        }
        popInnerChunk(mStream);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readEdgeList(const DataStreamPtr& stream, Mesh* pMesh)
    {
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            while(!stream->eof() &&
                streamID == MeshChunkID::EDGE_LIST_LOD)
            {
                // Process single LOD

                // unsigned short lodIndex
                unsigned short lodIndex;
                readShorts(stream, &lodIndex, 1);

                // bool isManual            // If manual, no edge data here, loaded from manual mesh
                bool isManual;
                readBools(stream, &isManual, 1);
                // Only load in non-manual levels; others will be connected up by Mesh on demand

                if (!isManual) {
                    MeshLodUsage& usage = pMesh->mMeshLodUsageList[lodIndex];

                    usage.edgeData = new EdgeData();

                    // Read detail information of the edge list
                    readEdgeListLodInfo(stream, usage.edgeData);

                    // Postprocessing edge groups
                    for (auto & edgeGroup : usage.edgeData->edgeGroups)
                    {
                        // Populate edgeGroup.vertexData pointers
                        // If there is shared vertex data, vertexSet 0 is that,
                        // otherwise 0 is first dedicated
                        if (pMesh->sharedVertexData)
                        {
                            if (edgeGroup.vertexSet == 0)
                            {
                                edgeGroup.vertexData = pMesh->sharedVertexData;
                            }
                            else
                            {
                                edgeGroup.vertexData = pMesh->getSubMesh(
                                    (unsigned short)edgeGroup.vertexSet-1)->vertexData.get();
                            }
                        }
                        else
                        {
                            edgeGroup.vertexData = pMesh->getSubMesh(
                                (unsigned short)edgeGroup.vertexSet)->vertexData.get();
                        }
                    }
                }

                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }

            }
            if (!stream->eof())
            {
                // Backpedal back to start of stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }

        pMesh->mEdgeListsBuilt = true;
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readEdgeListLodInfo(const DataStreamPtr& stream,
        EdgeData* edgeData)
    {
        // bool isClosed
        readBools(stream, &edgeData->isClosed, 1);
        // unsigned long numTriangles
        uint32 numTriangles;
        readInts(stream, &numTriangles, 1);
        // Allocate correct amount of memory
        edgeData->triangles.resize(numTriangles);
        edgeData->triangleFaceNormals.resize(numTriangles);
        edgeData->triangleLightFacings.resize(numTriangles);
        // unsigned long numEdgeGroups
        uint32 numEdgeGroups;
        readInts(stream, &numEdgeGroups, 1);
        // Allocate correct amount of memory
        edgeData->edgeGroups.resize(numEdgeGroups);
        // Triangle* triangleList
        uint32 tmp[3];
        for (size_t t = 0; t < numTriangles; ++t)
        {
            EdgeData::Triangle& tri = edgeData->triangles[t];
            // unsigned long indexSet
            readInts(stream, tmp, 1);
            tri.indexSet = tmp[0];
            // unsigned long vertexSet
            readInts(stream, tmp, 1);
            tri.vertexSet = tmp[0];
            // unsigned long vertIndex[3]
            readInts(stream, tmp, 3);

            tri.vertIndex[0] = tmp[0];
            tri.vertIndex[1] = tmp[1];
            tri.vertIndex[2] = tmp[2];
            // unsigned long sharedVertIndex[3]
            readInts(stream, tmp, 3);
            tri.sharedVertIndex[0] = tmp[0];
            tri.sharedVertIndex[1] = tmp[1];
            tri.sharedVertIndex[2] = tmp[2];
            // float normal[4]
            readFloats(stream, &(edgeData->triangleFaceNormals[t].x), 4);

        }
        pushInnerChunk(stream);
        for (uint32 eg = 0; eg < numEdgeGroups; ++eg)
        {
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            if (streamID != MeshChunkID::EDGE_GROUP)
            {
                OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR,
                    "Missing MeshChunkID::EDGE_GROUP stream",
                    "MeshSerializerImpl::readEdgeListLodInfo");
            }
            EdgeData::EdgeGroup& edgeGroup = edgeData->edgeGroups[eg];

            // unsigned long vertexSet
            readInts(stream, tmp, 1);
            edgeGroup.vertexSet = tmp[0];
            // unsigned long triStart
            readInts(stream, tmp, 1);
            edgeGroup.triStart = tmp[0];
            // unsigned long triCount
            readInts(stream, tmp, 1);
            edgeGroup.triCount = tmp[0];
            // unsigned long numEdges
            uint32 numEdges;
            readInts(stream, &numEdges, 1);
            edgeGroup.edges.resize(numEdges);
            // Edge* edgeList
            for (uint32 e = 0; e < numEdges; ++e)
            {
                EdgeData::Edge& edge = edgeGroup.edges[e];
                // unsigned long  triIndex[2]
                readInts(stream, tmp, 2);
                edge.triIndex[0] = tmp[0];
                edge.triIndex[1] = tmp[1];
                // unsigned long  vertIndex[2]
                readInts(stream, tmp, 2);
                edge.vertIndex[0] = tmp[0];
                edge.vertIndex[1] = tmp[1];
                // unsigned long  sharedVertIndex[2]
                readInts(stream, tmp, 2);
                edge.sharedVertIndex[0] = tmp[0];
                edge.sharedVertIndex[1] = tmp[1];
                // bool degenerate
                readBools(stream, &(edge.degenerate), 1);
            }
        }
        popInnerChunk(stream);
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcAnimationsSize(const Mesh* pMesh) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        for (unsigned short a = 0; a < pMesh->getNumAnimations(); ++a)
        {
            Animation* anim = pMesh->getAnimation(a);
            size += calcAnimationSize(anim);
        }
        return size;

    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcAnimationSize(const Animation* anim) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;
        // char* name
        size += anim->getName().length() + 1;

        // float length
        size += sizeof(float);

        for (const auto& trackIt : anim->_getVertexTrackList())
        {
            size += calcAnimationTrackSize(trackIt.second);
        }

        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcAnimationTrackSize(const VertexAnimationTrack* track) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;
        // uint16 type
        size += sizeof(uint16);
        // unsigned short target        // 0 for shared geometry,
        size += sizeof(unsigned short);

        if (track->getAnimationType() == VertexAnimationType::MORPH)
        {
            for (unsigned short i = 0; i < track->getNumKeyFrames(); ++i)
            {
                VertexMorphKeyFrame* kf = track->getVertexMorphKeyFrame(i);
                size += calcMorphKeyframeSize(kf, track->getAssociatedVertexData()->vertexCount);
            }
        }
        else
        {
            for (unsigned short i = 0; i < track->getNumKeyFrames(); ++i)
            {
                VertexPoseKeyFrame* kf = track->getVertexPoseKeyFrame(i);
                size += calcPoseKeyframeSize(kf);
            }
        }
        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcMorphKeyframeSize(const VertexMorphKeyFrame* kf,
        size_t vertexCount) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;
        // float time
        size += sizeof(float);
        // float x,y,z[,nx,ny,nz]
        bool includesNormals = kf->getVertexBuffer()->getVertexSize() > (sizeof(float) * 3);
        size += sizeof(float) * (includesNormals ? 6 : 3) * vertexCount;

        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcPoseKeyframeSize(const VertexPoseKeyFrame* kf) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // float time
        size += sizeof(float);

        size += calcPoseKeyframePoseRefSize() * kf->getPoseReferences().size();

        return size;

    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcPoseKeyframePoseRefSize() -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;
        // unsigned short poseIndex
        size += sizeof(uint16);
        // float influence
        size += sizeof(float);

        return size;

    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcPosesSize(const Mesh* pMesh) -> size_t
    {
        size_t size = 0;

        if (!pMesh->getPoseList().empty())
        {
            size += MSTREAM_OVERHEAD_SIZE;
            for(auto const& it : pMesh->getPoseList())
            {
                size += calcPoseSize(it);
            }
        }
        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcPoseSize(const Pose* pose) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // char* name (may be blank)
        size += pose->getName().length() + 1;
        // unsigned short target
        size += sizeof(uint16);
        // bool includesNormals
        size += sizeof(bool);

        // vertex offsets
        size += pose->getVertexOffsets().size() * calcPoseVertexSize(pose);

        return size;

    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl::calcPoseVertexSize(const Pose* pose) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;
        // unsigned long vertexIndex
        size += sizeof(uint32);
        // float xoffset, yoffset, zoffset
        size += sizeof(float) * 3;
        // optional normals
        if (!pose->getNormals().empty())
            size += sizeof(float) * 3;

        return size;
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writePoses(const Mesh* pMesh)
    {
        if (!pMesh->getPoseList().empty())
        {
            writeChunkHeader(std::to_underlying(MeshChunkID::POSES), calcPosesSize(pMesh));
            pushInnerChunk(mStream);
            for (auto it : pMesh->getPoseList())
            {
                writePose(it);
            }
            popInnerChunk(mStream);
        }

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writePose(const Pose* pose)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::POSE), calcPoseSize(pose));

        // char* name (may be blank)
        writeString(pose->getName());

        // unsigned short target
        ushort val = pose->getTarget();
        writeShorts(&val, 1);

        // bool includesNormals
        bool includesNormals = !pose->getNormals().empty();
        writeBools(&includesNormals, 1);
        pushInnerChunk(mStream);
        {
            size_t vertexSize = calcPoseVertexSize(pose);
            auto nit = pose->getNormals().begin();
            for (const auto& it : pose->getVertexOffsets())
            {
                auto vertexIndex = (uint32)it.first;
                writeChunkHeader(std::to_underlying(MeshChunkID::POSE_VERTEX), vertexSize);
                // unsigned long vertexIndex
                writeInts(&vertexIndex, 1);
                // float xoffset, yoffset, zoffset
                writeFloats(it.second.ptr(), 3);
                if (includesNormals)
                {
                    // float xnormal, ynormal, znormal
                    writeFloats(nit->second.ptr(), 3);
                    nit++;
                }
            }
        }
        popInnerChunk(mStream);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeAnimations(const Mesh* pMesh)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::ANIMATIONS), calcAnimationsSize(pMesh));
        pushInnerChunk(mStream);
        for (unsigned short a = 0; a < pMesh->getNumAnimations(); ++a)
        {
            Animation* anim = pMesh->getAnimation(a);
            LogManager::getSingleton().logMessage(::std::format("Exporting animation {}", anim->getName()));
            writeAnimation(anim);
            LogManager::getSingleton().logMessage("Animation exported.");
        }
        popInnerChunk(mStream);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeAnimation(const Animation* anim)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::ANIMATION), calcAnimationSize(anim));
        // char* name
        writeString(anim->getName());
        // float length
        float len = anim->getLength();
        writeFloats(&len, 1);
        pushInnerChunk(mStream);
        if (anim->getUseBaseKeyFrame())
        {
            size_t size = MSTREAM_OVERHEAD_SIZE;
            // char* baseAnimationName (including terminator)
            size += anim->getBaseKeyFrameAnimationName().length() + 1;
            // float baseKeyFrameTime
            size += sizeof(float);
            
            writeChunkHeader(std::to_underlying(MeshChunkID::ANIMATION_BASEINFO), size);
            
            // char* baseAnimationName (blank for self)
            writeString(anim->getBaseKeyFrameAnimationName());
            
            // float baseKeyFrameTime
            auto t = (float)anim->getBaseKeyFrameTime();
            writeFloats(&t, 1);
        }

        // tracks
        for (const auto& trackIt : anim->_getVertexTrackList())
        {
            writeAnimationTrack(trackIt.second);
        }
        popInnerChunk(mStream);

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeAnimationTrack(const VertexAnimationTrack* track)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::ANIMATION_TRACK), calcAnimationTrackSize(track));
        // unsigned short type          // 1 == morph, 2 == pose
        auto animType = (uint16)track->getAnimationType();
        writeShorts(&animType, 1);
        // unsigned short target
        uint16 target = track->getHandle();
        writeShorts(&target, 1);
        pushInnerChunk(mStream);
        {
            if (track->getAnimationType() == VertexAnimationType::MORPH)
            {
                for (unsigned short i = 0; i < track->getNumKeyFrames(); ++i)
                {
                    VertexMorphKeyFrame* kf = track->getVertexMorphKeyFrame(i);
                    writeMorphKeyframe(kf, track->getAssociatedVertexData()->vertexCount);
                }
            }
            else // VertexAnimationType::POSE
            {
                for (unsigned short i = 0; i < track->getNumKeyFrames(); ++i)
                {
                    VertexPoseKeyFrame* kf = track->getVertexPoseKeyFrame(i);
                    writePoseKeyframe(kf);
                }
            }
        }
        popInnerChunk(mStream);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writeMorphKeyframe(const VertexMorphKeyFrame* kf, size_t vertexCount)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::ANIMATION_MORPH_KEYFRAME), calcMorphKeyframeSize(kf, vertexCount));
        // float time
        float timePos = kf->getTime();
        writeFloats(&timePos, 1);
        // bool includeNormals
        bool includeNormals = kf->getVertexBuffer()->getVertexSize() > (sizeof(float) * 3);
        writeBools(&includeNormals, 1);
        // float x,y,z          // repeat by number of vertices in original geometry
        HardwareBufferLockGuard vbufLock(kf->getVertexBuffer(), HardwareBuffer::LockOptions::READ_ONLY);
        writeFloats(static_cast<float*>(vbufLock.pData), vertexCount * (includeNormals ? 6 : 3));
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writePoseKeyframe(const VertexPoseKeyFrame* kf)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::ANIMATION_POSE_KEYFRAME), calcPoseKeyframeSize(kf));
        // float time
        float timePos = kf->getTime();
        writeFloats(&timePos, 1);
        pushInnerChunk(mStream);
        // pose references
        for (auto const& poseRefIt : kf->getPoseReferences())
        {
            writePoseKeyframePoseRef(poseRefIt);
        }
        popInnerChunk(mStream);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::writePoseKeyframePoseRef(
        const VertexPoseKeyFrame::PoseRef& poseRef)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::ANIMATION_POSE_REF), calcPoseKeyframePoseRefSize());
        // unsigned short poseIndex
        writeShorts(&(poseRef.poseIndex), 1);
        // float influence
        writeFloats(&(poseRef.influence), 1);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readPoses(const DataStreamPtr& stream, Mesh* pMesh)
    {
        // Find all substreams
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
                using enum MeshChunkID;
            while(!stream->eof() &&
                (streamID == POSE))
            {
                switch(streamID)
                {
                case POSE:
                    readPose(stream, pMesh);
                    break;
                default:
                    break;
                }

                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }

            }
            if (!stream->eof())
            {
                // Backpedal back to start of stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readPose(const DataStreamPtr& stream, Mesh* pMesh)
    {
        // char* name (may be blank)
        String name = readString(stream);
        // unsigned short target
        unsigned short target;
        readShorts(stream, &target, 1);

        // bool includesNormals
        bool includesNormals;
        readBools(stream, &includesNormals, 1);
        
        Pose* pose = pMesh->createPose(target, name);

        // Find all substreams
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            using enum MeshChunkID;
            while(!stream->eof() &&
                (streamID == POSE_VERTEX))
            {
                switch(streamID)
                {
                case POSE_VERTEX:
                    // create vertex offset
                    uint32 vertIndex;
                    Vector3 offset, normal;
                    // unsigned long vertexIndex
                    readInts(stream, &vertIndex, 1);
                    // float xoffset, yoffset, zoffset
                    readFloats(stream, offset.ptr(), 3);
                    
                    if (includesNormals)
                    {
                        readFloats(stream, normal.ptr(), 3);
                        pose->addVertex(vertIndex, offset, normal);                     
                    }
                    else 
                    {
                        pose->addVertex(vertIndex, offset);
                    }
                    break;
                default:
                    break;
                }

                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }

            }
            if (!stream->eof())
            {
                // Backpedal back to start of stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readAnimations(const DataStreamPtr& stream, Mesh* pMesh)
    {
        // Find all substreams
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            while(!stream->eof() &&
                (streamID == MeshChunkID::ANIMATION))
            {
                using enum MeshChunkID;
                switch(streamID)
                {
                case ANIMATION:
                    readAnimation(stream, pMesh);
                    break;
                default:
                    break;
                }

                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }

            }
            if (!stream->eof())
            {
                // Backpedal back to start of stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }


    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readAnimation(const DataStreamPtr& stream, Mesh* pMesh)
    {

        // char* name
        String name = readString(stream);
        // float length
        float len;
        readFloats(stream, &len, 1);

        Animation* anim = pMesh->createAnimation(name, len);

        // tracks
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            
            // Optional base info is possible
            if (streamID == MeshChunkID::ANIMATION_BASEINFO)
            {
                // char baseAnimationName
                String baseAnimName = readString(stream);
                // float baseKeyFrameTime
                float baseKeyTime;
                readFloats(stream, &baseKeyTime, 1);
                
                anim->setUseBaseKeyFrame(true, baseKeyTime, baseAnimName);
                
                if (!stream->eof())
                {
                    // Get next stream
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }
            }
            
            using enum MeshChunkID;
            while(!stream->eof() &&
                streamID == ANIMATION_TRACK)
            {
                switch(streamID)
                {
                case ANIMATION_TRACK:
                    readAnimationTrack(stream, anim, pMesh);
                    break;
                default:
                    break;
                };
                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }

            }
            if (!stream->eof())
            {
                // Backpedal back to start of stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readAnimationTrack(const DataStreamPtr& stream,
        Animation* anim, Mesh* pMesh)
    {
        // ushort type
        uint16 inAnimType;
        readShorts(stream, &inAnimType, 1);
        auto animType = (VertexAnimationType)inAnimType;

        // unsigned short target
        uint16 target;
        readShorts(stream, &target, 1);

        VertexAnimationTrack* track = anim->createVertexTrack(target,
            pMesh->getVertexDataByTrackHandle(target), animType);

        // keyframes
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            using enum MeshChunkID;
            while(!stream->eof() &&
                (streamID == ANIMATION_MORPH_KEYFRAME ||
                 streamID == ANIMATION_POSE_KEYFRAME))
            {
                switch(streamID)
                {
                case ANIMATION_MORPH_KEYFRAME:
                    readMorphKeyFrame(stream, pMesh, track);
                    break;
                case ANIMATION_POSE_KEYFRAME:
                    readPoseKeyFrame(stream, track);
                    break;
                default:
                    break;
                };
                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }

            }
            if (!stream->eof())
            {
                // Backpedal back to start of stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readMorphKeyFrame(const DataStreamPtr& stream, Mesh* pMesh, VertexAnimationTrack* track)
    {
        // float time
        float timePos;
        readFloats(stream, &timePos, 1);
        
        // bool includesNormals
        bool includesNormals;
        readBools(stream, &includesNormals, 1);

        VertexMorphKeyFrame* kf = track->createVertexMorphKeyFrame(timePos);

        // Create buffer, allow read and use shadow buffer
        size_t vertexCount = track->getAssociatedVertexData()->vertexCount;
        size_t vertexSize = sizeof(float) * (includesNormals ? 6 : 3);
        HardwareVertexBufferSharedPtr vbuf =
            pMesh->getHardwareBufferManager()->createVertexBuffer(
                vertexSize, vertexCount,
                HardwareBuffer::STATIC, true);
        // float x,y,z          // repeat by number of vertices in original geometry
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        readFloats(stream, static_cast<float*>(vbufLock.pData), vertexCount * (includesNormals ? 6 : 3));
        kf->setVertexBuffer(vbuf);

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readPoseKeyFrame(const DataStreamPtr& stream, VertexAnimationTrack* track)
    {
        // float time
        float timePos;
        readFloats(stream, &timePos, 1);

        // Create keyframe
        VertexPoseKeyFrame* kf = track->createVertexPoseKeyFrame(timePos);

        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));

            using enum MeshChunkID;
            while(!stream->eof() &&
                streamID ==ANIMATION_POSE_REF)
            {
                switch(streamID)
                {
                case ANIMATION_POSE_REF:
                    uint16 poseIndex;
                    float influence;
                    // unsigned short poseIndex
                    readShorts(stream, &poseIndex, 1);
                    // float influence
                    readFloats(stream, &influence, 1);

                    kf->addPoseReference(poseIndex, influence);

                    break;
                default:
                    break;
                };
                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }
            }
            if (!stream->eof())
            {
                // Backpedal back to start of stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl::readExtremes(const DataStreamPtr& stream, Mesh *pMesh)
    {
        unsigned short idx;
        readShorts(stream, &idx, 1);
        
        SubMesh *sm = pMesh->getSubMesh (idx);
        
        int n_floats = (mCurrentstreamLen - MSTREAM_OVERHEAD_SIZE -
                        sizeof (unsigned short)) / sizeof (float);
        
        assert ((n_floats % 3) == 0);
        
        auto *vert = new float[n_floats];
        readFloats(stream, vert, n_floats);
        
        for (int i = 0; i < n_floats; i += 3)
            sm->extremityPoints.push_back(Vector3{vert [i], vert [i + 1], vert [i + 2]});
        
        delete[] vert;
    }

    void MeshSerializerImpl::enableValidation()
    {
    }


    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_8::MeshSerializerImpl_v1_8()
    {
        // Version number
        mVersion = "[MeshSerializer_v1.8]";
    }
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_8::~MeshSerializerImpl_v1_8()
    = default;
    //--------------------------------------------------------------------
    auto MeshSerializerImpl_v1_8::compatibleLodStrategyName(std::string_view strategyName) -> std::string_view
    {
        if(strategyName == "distance_box" || strategyName == "distance_sphere")
            strategyName = "Distance";
        else if(strategyName == "pixel_count" || strategyName == "screen_ratio_pixel_count")
            strategyName = "PixelCount";
        return strategyName;
    }
    //--------------------------------------------------------------------
    auto MeshSerializerImpl_v1_8::isLodMixed(const Mesh* pMesh) -> bool
    {
        if(!pMesh->hasManualLodLevel()) {
            return false;
        }

        unsigned short numLods = pMesh->getNumLodLevels();
        for (unsigned short i = 1; i < numLods; ++i)
        {
            if(!pMesh->_isManualLodLevel(i)){
                return true;
            }
        }

        return false;
    }
    auto MeshSerializerImpl_v1_8::calcLodLevelSize(const Mesh* pMesh) -> size_t
    {
        if (isLodMixed(pMesh)) {
            return 0; // Supported in v1_9+
        }
        exportedLodCount = pMesh->getNumLodLevels();
        size_t size = MSTREAM_OVERHEAD_SIZE; // Header
        size += calcStringSize(compatibleLodStrategyName(pMesh->getLodStrategy()->getName())); // string strategyName;
        size += sizeof(unsigned short); // unsigned short numLevels;
        size += sizeof(bool); // bool manual; <== this is removed in v1_9

        // Loop from LOD 1 (not 0, this is full detail)
        for (ushort i = 1; i < exportedLodCount; ++i)
        {
            const MeshLodUsage& usage = pMesh->mMeshLodUsageList[i];
            if (pMesh->_isManualLodLevel(i))
            {
                size += calcLodUsageManualSize(usage);
            }
            else
            {
                size += calcLodUsageGeneratedSize(pMesh, usage, i);
            }
        }
        return size;
    }
    auto MeshSerializerImpl_v1_8::calcLodUsageManualSize(const MeshLodUsage& usage) -> size_t
    {
        // Header
        size_t size = MSTREAM_OVERHEAD_SIZE; // MeshChunkID::MESH_LOD_USAGE <== this is removed in v1_9

        // float fromDepthSquared;
        size += sizeof(float);

        // Manual part size
        size += MSTREAM_OVERHEAD_SIZE; // MeshChunkID::MESH_LOD_MANUAL
        // String manualMeshName;
        size += calcStringSize(usage.manualName);
        return size;
    }

    auto MeshSerializerImpl_v1_8::calcLodUsageGeneratedSize(const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum) -> size_t
    {
        // Usage Header
        size_t size = MSTREAM_OVERHEAD_SIZE;
        unsigned short subidx;

        // float fromDepthSquared;
        size += sizeof(float);

        // Calc generated SubMesh sections size
        for (subidx = 0; subidx < pMesh->getNumSubMeshes(); ++subidx)
        {
            SubMesh* submesh = pMesh->getSubMesh(subidx);
            size += calcLodUsageGeneratedSubmeshSize(submesh, lodNum);
        }
        return size;
    }
    auto MeshSerializerImpl_v1_8::calcLodUsageGeneratedSubmeshSize(const SubMesh* submesh, unsigned short lodNum) -> size_t
    {
        const IndexData* indexData = submesh->mLodFaceList[lodNum - 1];
        const HardwareIndexBufferSharedPtr& ibuf = indexData->indexBuffer;

        size_t size = MSTREAM_OVERHEAD_SIZE; // MeshChunkID::MESH_LOD_GENERATED
        size += sizeof(unsigned int); // unsigned int indexData->indexCount;
        size += sizeof(bool); // bool indexes32Bit
        size += !ibuf ? 0 : ibuf->getIndexSize() * indexData->indexCount; // faces
        return size;
    }

    //--------------------------------------------------------------------
    void MeshSerializerImpl_v1_8::writeLodLevel(const Mesh* pMesh)
    {
        if (isLodMixed(pMesh)) {
            LogManager::getSingleton().logMessage("MeshSerializer_v1_8 older mesh format is incompatible with mixed manual/generated Lod levels. Lod levels will not be exported.");
        } else {
            
            exportedLodCount = pMesh->getNumLodLevels();
            bool manual = pMesh->hasManualLodLevel();

            writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_LEVEL), calcLodLevelSize(pMesh));

            // string strategyName;
            writeString(compatibleLodStrategyName(pMesh->getLodStrategy()->getName()));
            // unsigned short numLevels;
            writeShorts(&exportedLodCount, 1);
            // bool manual;  (true for manual alternate meshes, false for generated)
            writeBools(&manual, 1);
            
            pushInnerChunk(mStream);
            // Loop from LOD 1 (not 0, this is full detail)
            for (ushort i = 1; i < exportedLodCount; ++i)
            {
                const MeshLodUsage& usage = pMesh->mMeshLodUsageList[i];
                assert(pMesh->_isManualLodLevel(i) == manual);
                if (manual)
                {
                    writeLodUsageManual(usage);
                }
                else
                {
                    writeLodUsageGenerated(pMesh, usage, i);
                }
            }
            popInnerChunk(mStream);
        }
    }
    //---------------------------------------------------------------------
    /*void MeshSerializerImpl_v1_8::writeLodUsageGenerated( const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum )
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_USAGE), calcLodUsageGeneratedSize(pMesh, usage, lodNum));
        writeFloats(&(usage.userValue), 1);
        pushInnerChunk(mStream);
        // Now write sections
        for (unsigned short subidx = 0; subidx < pMesh->getNumSubMeshes(); ++subidx)
        {
            SubMesh* sm = pMesh->getSubMesh(subidx);
            const IndexData* indexData = sm->mLodFaceList[lodNum-1];

            // Lock index buffer to write
            HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;

            bool idx32 = (ibuf && ibuf->getType() == HardwareIndexBuffer::IndexType::_32BIT);

            writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_GENERATED), calcLodUsageGeneratedSubmeshSize(sm, lodNum));
            unsigned int idxCount = static_cast<unsigned int>(indexData->indexCount);
            writeInts(&idxCount, 1);
            writeBools(&idx32, 1);

            if (idxCount > 0)
            {
                HardwareBufferLockGuard ibufLock(ibuf, HardwareBuffer::LockOptions::READ_ONLY);
                if (idx32)
                {
                    unsigned int* pIdx = static_cast<unsigned int*>(ibufLock.pData);
                    writeInts(pIdx + indexData->indexStart, indexData->indexCount);
                }
                else
                {
                    unsigned short* pIdx = static_cast<unsigned short*>(ibufLock.pData);
                    writeShorts(pIdx + indexData->indexStart, indexData->indexCount);
                }
            }
        }
        popInnerChunk(mStream);
    }*/
    
    void MeshSerializerImpl_v1_8::writeLodUsageGenerated(const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_USAGE), calcLodUsageGeneratedSize(pMesh, usage, lodNum));
        auto userValue = static_cast<float>(usage.userValue);
        writeFloats(&userValue, 1);
        pushInnerChunk(mStream);
        for (ushort i = 0; i < pMesh->getNumSubMeshes(); i++)
        {
            SubMesh* submesh = pMesh->getSubMesh(i);
            writeLodUsageGeneratedSubmesh(submesh, lodNum);
        }
        popInnerChunk(mStream);
    }
    void MeshSerializerImpl_v1_8::writeLodUsageGeneratedSubmesh(const SubMesh* submesh, unsigned short lodNum)
    {
        const IndexData* indexData = submesh->mLodFaceList[lodNum - 1];
        HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;
        assert(ibuf);

        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_GENERATED), calcLodUsageGeneratedSubmeshSize(submesh, lodNum));
        auto indexCount = static_cast<unsigned int>(indexData->indexCount);
        writeInts(&indexCount, 1);
        bool is32BitIndices = (ibuf->getType() == HardwareIndexBuffer::IndexType::_32BIT);
        writeBools(&is32BitIndices, 1);

        HardwareBufferLockGuard ibufLock(ibuf, HardwareBuffer::LockOptions::READ_ONLY);
        if (is32BitIndices)
        {
            auto* pIdx = static_cast<unsigned int*>(ibufLock.pData);
            writeInts(pIdx + indexData->indexStart, indexCount);
        }
        else
        {
            auto* pIdx = static_cast<unsigned short*>(ibufLock.pData);
            writeShorts(pIdx + indexData->indexStart, indexCount);
        }
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_8::writeLodUsageManual(const MeshLodUsage& usage)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_USAGE), calcLodUsageManualSize(usage));
        writeFloats(&(usage.userValue), 1);
        pushInnerChunk(mStream);
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_MANUAL), MSTREAM_OVERHEAD_SIZE + calcStringSize(usage.manualName));
        writeString(usage.manualName);
        popInnerChunk(mStream);
    }
    //---------------------------------------------------------------------

    void MeshSerializerImpl_v1_8::readMeshLodUsageGenerated( const DataStreamPtr& stream, Mesh* pMesh, unsigned short lodNum, MeshLodUsage& usage )
    {
        usage.manualName = "";
        usage.manualMesh.reset();
        pushInnerChunk(stream);
        {
            // Get one set of detail per SubMesh
            unsigned short numSubs, i;
            numSubs = pMesh->getNumSubMeshes();
            for (i = 0; i < numSubs; ++i)
            {
                auto const streamID = static_cast<MeshChunkID>(readChunk(stream));
                if (streamID != MeshChunkID::MESH_LOD_GENERATED)
                {
                    OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                        ::std::format("Missing MeshChunkID::MESH_LOD_GENERATED stream in {}", pMesh->getName()),
                        "MeshSerializerImpl::readMeshLodUsageGenerated");
                }

                SubMesh* sm = pMesh->getSubMesh(i);
                auto* indexData = new IndexData();
                sm->mLodFaceList[lodNum - 1] = indexData;
                // unsigned int numIndexes
                unsigned int numIndexes;
                readInts(stream, &numIndexes, 1);
                indexData->indexCount = static_cast<size_t>(numIndexes);

                // bool indexes32Bit
                bool idx32Bit;
                readBools(stream, &idx32Bit, 1);
                // unsigned short*/int* faceIndexes;  ((v1, v2, v3) * numFaces)
                if (idx32Bit)
                {
                    indexData->indexBuffer = pMesh->getHardwareBufferManager()->createIndexBuffer(
                        HardwareIndexBuffer::IndexType::_32BIT, indexData->indexCount,
                        pMesh->mIndexBufferUsage, pMesh->mIndexBufferShadowBuffer);
                    HardwareBufferLockGuard ibufLock(indexData->indexBuffer, HardwareBuffer::LockOptions::DISCARD);
                    readInts(stream, static_cast<unsigned int*>(ibufLock.pData), indexData->indexCount);
                }
                else
                {
                    indexData->indexBuffer = pMesh->getHardwareBufferManager()->createIndexBuffer(
                        HardwareIndexBuffer::IndexType::_16BIT, indexData->indexCount,
                        pMesh->mIndexBufferUsage, pMesh->mIndexBufferShadowBuffer);
                    HardwareBufferLockGuard ibufLock(indexData->indexBuffer, HardwareBuffer::LockOptions::DISCARD);
                    readShorts(stream, static_cast<unsigned short*>(ibufLock.pData), indexData->indexCount);
                }
            }
        }
        popInnerChunk(stream);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_8::readMeshLodUsageManual(const DataStreamPtr& stream,
        Mesh* pMesh, unsigned short lodNum, MeshLodUsage& usage)
    {
        pushInnerChunk(stream);
        // Read detail stream
        auto const streamID = static_cast<MeshChunkID>(readChunk(stream));
        if (streamID != MeshChunkID::MESH_LOD_MANUAL)
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                ::std::format("Missing MeshChunkID::MESH_LOD_MANUAL stream in {}", pMesh->getName()),
                "MeshSerializerImpl::readMeshLodUsageManual");
        }

        usage.manualName = readString(stream);
        usage.manualMesh.reset(); // will trigger load later
        popInnerChunk(stream);
    }

    void MeshSerializerImpl_v1_8::readMeshLodLevel(const DataStreamPtr& stream, Mesh* pMesh)
    {
        // Read the strategy to be used for this mesh
        String strategyName = readString(stream);
        LodStrategy *strategy = LodStrategyManager::getSingleton().getStrategy(strategyName);
        
        if (strategy)
            pMesh->setLodStrategy(strategy);

        // unsigned short numLevels;
        readShorts(stream, &(pMesh->mNumLods), 1);
        // bool manual;  (true for manual alternate meshes, false for generated)
        readBools(stream, &(pMesh->mHasManualLodLevel), 1);

        // Preallocate submesh lod face data if not manual
        if (!pMesh->hasManualLodLevel())
        {
            unsigned short numsubs = pMesh->getNumSubMeshes();
            for (unsigned short i = 0; i < numsubs; ++i)
            {
                SubMesh* sm = pMesh->getSubMesh(i);
                assert(sm->mLodFaceList.empty());
                sm->mLodFaceList.resize(pMesh->mNumLods - 1);
            }
        }

        pushInnerChunk(stream);
        // Loop from 1 rather than 0 (full detail index is not in file)
        for (unsigned short i = 1; i < pMesh->mNumLods; ++i)
        {
            auto const streamID = static_cast<MeshChunkID>(readChunk(stream));
            if (streamID != MeshChunkID::MESH_LOD_USAGE)
            {
                OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                    ::std::format("Missing MeshChunkID::MESH_LOD_USAGE stream in {}", pMesh->getName()),
                    "MeshSerializerImpl::readMeshLodInfo");
            }
            // Read depth
            MeshLodUsage usage;
            readFloats(stream, &(usage.userValue), 1);

            // Set default values
            usage.manualName = "";
            usage.manualMesh.reset();
            usage.edgeData = nullptr;

            if (pMesh->hasManualLodLevel())
            {
                readMeshLodUsageManual(stream, pMesh, i, usage);
            }
            else //(!pMesh->hasManualLodLevel())
            {
                readMeshLodUsageGenerated(stream, pMesh, i, usage);
            }
            usage.edgeData = nullptr;

            // Save usage
            pMesh->mMeshLodUsageList.push_back(usage);
        }
        popInnerChunk(stream);
    }

    void MeshSerializerImpl_v1_8::enableValidation()
    {

    }

    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_41::MeshSerializerImpl_v1_41()
    {
        // Version number
        mVersion = "[MeshSerializer_v1.41]";
    }
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_41::~MeshSerializerImpl_v1_41()
    = default;
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_41::writeMorphKeyframe(const VertexMorphKeyFrame* kf, size_t vertexCount)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::ANIMATION_MORPH_KEYFRAME), calcMorphKeyframeSize(kf, vertexCount));
        // float time
        float timePos = kf->getTime();
        writeFloats(&timePos, 1);
        // float x,y,z          // repeat by number of vertices in original geometry
        HardwareBufferLockGuard vbufLock(kf->getVertexBuffer(), HardwareBuffer::LockOptions::READ_ONLY);
        writeFloats(static_cast<float*>(vbufLock.pData), vertexCount * 3);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_41::readMorphKeyFrame(const DataStreamPtr& stream, Mesh* pMesh, VertexAnimationTrack* track)
    {
        // float time
        float timePos;
        readFloats(stream, &timePos, 1);

        VertexMorphKeyFrame* kf = track->createVertexMorphKeyFrame(timePos);

        // Create buffer, allow read and use shadow buffer
        size_t vertexCount = track->getAssociatedVertexData()->vertexCount;
        HardwareVertexBufferSharedPtr vbuf =
            pMesh->getHardwareBufferManager()->createVertexBuffer(
                VertexElement::getTypeSize(VertexElementType::FLOAT3), vertexCount,
                HardwareBuffer::STATIC, true);
        // float x,y,z          // repeat by number of vertices in original geometry
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        readFloats(stream, static_cast<float*>(vbufLock.pData), vertexCount * 3);
        kf->setVertexBuffer(vbuf);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_41::writePose(const Pose* pose)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::POSE), calcPoseSize(pose));

        // char* name (may be blank)
        writeString(pose->getName());

        // unsigned short target
        ushort val = pose->getTarget();
        writeShorts(&val, 1);
        pushInnerChunk(mStream);
        size_t vertexSize = calcPoseVertexSize();
        for (const auto& it : pose->getVertexOffsets())
        {
            auto vertexIndex = (uint32)it.first;
            writeChunkHeader(std::to_underlying(MeshChunkID::POSE_VERTEX), vertexSize);
            // unsigned long vertexIndex
            writeInts(&vertexIndex, 1);
            // float xoffset, yoffset, zoffset
            writeFloats(it.second.ptr(), 3);
        }
        popInnerChunk(mStream);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_41::readPose(const DataStreamPtr& stream, Mesh* pMesh)
    {
        // char* name (may be blank)
        String name = readString(stream);
        // unsigned short target
        unsigned short target;
        readShorts(stream, &target, 1);

        Pose* pose = pMesh->createPose(target, name);

        // Find all substreams
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            using enum MeshChunkID;
            while(!stream->eof() &&
                (streamID == POSE_VERTEX))
            {
                switch(streamID)
                {
                case POSE_VERTEX:
                    // create vertex offset
                    uint32 vertIndex;
                    Vector3 offset;
                    // unsigned long vertexIndex
                    readInts(stream, &vertIndex, 1);
                    // float xoffset, yoffset, zoffset
                    readFloats(stream, offset.ptr(), 3);

                    pose->addVertex(vertIndex, offset);
                    break;
                default:
                    break;
                }

                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }

            }
            if (!stream->eof())
            {
                // Backpedal back to start of stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl_v1_41::calcPoseSize(const Pose* pose) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // char* name (may be blank)
        size += pose->getName().length() + 1;
        // unsigned short target
        size += sizeof(uint16);

        // vertex offsets
        size += pose->getVertexOffsets().size() * calcPoseVertexSize();

        return size;

    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl_v1_41::calcPoseVertexSize() -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;
        // unsigned long vertexIndex
        size += sizeof(uint32);
        // float xoffset, yoffset, zoffset
        size += sizeof(float) * 3;

        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl_v1_41::calcMorphKeyframeSize(const VertexMorphKeyFrame* kf,
        size_t vertexCount) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;
        // float time
        size += sizeof(float);
        // float x,y,z
        size += sizeof(float) * 3 * vertexCount;

        return size;
    }

    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_4::MeshSerializerImpl_v1_4()
    {
        // Version number
        mVersion = "[MeshSerializer_v1.40]";
    }
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_4::~MeshSerializerImpl_v1_4()
    = default;
    auto MeshSerializerImpl_v1_4::calcLodLevelSize(const Mesh* pMesh) -> size_t
    {
        if (isLodMixed(pMesh) || pMesh->getLodStrategy() != DistanceLodBoxStrategy::getSingletonPtr()) {
            return 0; // Supported in v1_9+
        }
        exportedLodCount = pMesh->getNumLodLevels();
        size_t size = MSTREAM_OVERHEAD_SIZE; // Header
        //size += calcStringSize(pMesh->getLodStrategy()->getName()); // string strategyName; <== missing in v1_4
        size += sizeof(unsigned short); // unsigned short numLevels;
        size += sizeof(bool); // bool manual; <== this is removed in v1_9

        // Loop from LOD 1 (not 0, this is full detail)
        for (ushort i = 1; i < exportedLodCount; ++i)
        {
            const MeshLodUsage& usage = pMesh->mMeshLodUsageList[i];
            if (pMesh->_isManualLodLevel(i))
            {
                size += calcLodUsageManualSize(usage);
            }
            else
            {
                size += calcLodUsageGeneratedSize(pMesh, usage, i);
            }
        }
        return size;
    }

    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_4::writeLodLevel(const Mesh* pMesh)
    {
        if (isLodMixed(pMesh)) {
            LogManager::getSingleton().logMessage("MeshSerializer_v1_4 or older mesh format is incompatible with mixed manual/generated Lod levels. Lod levels will not be exported.");
        } else if (pMesh->getLodStrategy() != DistanceLodBoxStrategy::getSingletonPtr()) {
            LogManager::getSingleton().logMessage("MeshSerializer_v1_4 or older mesh format is only compatible with Distance Lod Strategy. Lod levels will not be exported.");
        } else {
            exportedLodCount = pMesh->getNumLodLevels();
            bool manual = pMesh->hasManualLodLevel();

            writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_LEVEL), calcLodLevelSize(pMesh));

            // Details
            // string strategyName;
            //writeString(pMesh->getLodStrategy()->getName()); <== missing in v1_4
            // unsigned short numLevels;
            writeShorts(&exportedLodCount, 1);
            // bool manual;  (true for manual alternate meshes, false for generated)
            writeBools(&manual, 1);

            pushInnerChunk(mStream);
            // Loop from LOD 1 (not 0, this is full detail)
            for (ushort i = 1; i < exportedLodCount; ++i)
            {
                const MeshLodUsage& usage = pMesh->mMeshLodUsageList[i];
                assert(pMesh->_isManualLodLevel(i) == manual);
                if (manual)
                {
                    writeLodUsageManual(usage);
                }
                else
                {
                    /*readFloats(stream, &(usage.value), 1);
            usage.userValue = Math::Sqrt(usage.value);*/
                    writeLodUsageGenerated(pMesh, usage, i);
                }
            }
            popInnerChunk(mStream);
        }
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_4::writeLodUsageGenerated(const Mesh* pMesh, const MeshLodUsage& usage, unsigned short lodNum)
    {
        writeChunkHeader(std::to_underlying(MeshChunkID::MESH_LOD_USAGE), calcLodUsageGeneratedSize(pMesh, usage, lodNum));
        auto value = static_cast<float>(usage.value);
        writeFloats(&value, 1); // <== In v1_4 this is value instead of userValue
        pushInnerChunk(mStream);
        for (ushort i = 0; i < pMesh->getNumSubMeshes(); i++)
        {
            SubMesh* submesh = pMesh->getSubMesh(i);
            writeLodUsageGeneratedSubmesh(submesh, lodNum);
        }
        popInnerChunk(mStream);
    }

    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_4::readMeshLodLevel(const DataStreamPtr& stream, Mesh* pMesh)
    {
        // Use the old strategy for this mesh
        LodStrategy *strategy = DistanceLodSphereStrategy::getSingletonPtr();
        pMesh->setLodStrategy(strategy);

        // unsigned short numLevels;
        readShorts(stream, &(pMesh->mNumLods), 1);
        bool manual; // true for manual alternate meshes, false for generated
        readBools(stream, &manual, 1);

        pMesh->mHasManualLodLevel = manual;

        // Preallocate submesh LOD face data if not manual
        if (!manual)
        {

            unsigned short numsubs = pMesh->getNumSubMeshes();
            for (unsigned short i = 0; i < numsubs; ++i)
            {
                SubMesh* sm = pMesh->getSubMesh(i);
                assert(sm->mLodFaceList.empty());
                sm->mLodFaceList.resize(pMesh->mNumLods-1);
            }
        }
        pushInnerChunk(stream);
        // Loop from 1 rather than 0 (full detail index is not in file)
        for (unsigned short i = 1; i < pMesh->mNumLods; ++i)
        {
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            if (streamID != MeshChunkID::MESH_LOD_USAGE)
            {
                OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                    ::std::format("Missing MeshChunkID::MESH_LOD_USAGE stream in {}", pMesh->getName()),
                    "MeshSerializerImpl::readMeshLodInfo");
            }
            // Read depth
            MeshLodUsage usage;
            readFloats(stream, &(usage.value), 1);
            usage.userValue = Math::Sqrt(usage.value);

            // Set default values
            usage.manualName = "";
            usage.manualMesh.reset();
            usage.edgeData = nullptr;

            if (manual)
            {
                readMeshLodUsageManual(stream, pMesh, i, usage);
            }
            else //(!pMesh->isLodManual)
            {
                readMeshLodUsageGenerated(stream, pMesh, i, usage);
            }
            usage.edgeData = nullptr;

            // Save usage
            pMesh->mMeshLodUsageList.push_back(usage);
        }
        popInnerChunk(stream);
    }

    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_3::MeshSerializerImpl_v1_3()
    {
        // Version number
        mVersion = "[MeshSerializer_v1.30]";
    }
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_3::~MeshSerializerImpl_v1_3()
    = default;
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_3::readEdgeListLodInfo(const DataStreamPtr& stream,
        EdgeData* edgeData)
    {
        // unsigned long numTriangles
        uint32 numTriangles;
        readInts(stream, &numTriangles, 1);
        // Allocate correct amount of memory
        edgeData->triangles.resize(numTriangles);
        edgeData->triangleFaceNormals.resize(numTriangles);
        edgeData->triangleLightFacings.resize(numTriangles);
        // unsigned long numEdgeGroups
        uint32 numEdgeGroups;
        readInts(stream, &numEdgeGroups, 1);
        // Allocate correct amount of memory
        edgeData->edgeGroups.resize(numEdgeGroups);
        // Triangle* triangleList
        uint32 tmp[3];
        for (size_t t = 0; t < numTriangles; ++t)
        {
            EdgeData::Triangle& tri = edgeData->triangles[t];
            // unsigned long indexSet
            readInts(stream, tmp, 1);
            tri.indexSet = tmp[0];
            // unsigned long vertexSet
            readInts(stream, tmp, 1);
            tri.vertexSet = tmp[0];
            // unsigned long vertIndex[3]
            readInts(stream, tmp, 3);
            tri.vertIndex[0] = tmp[0];
            tri.vertIndex[1] = tmp[1];
            tri.vertIndex[2] = tmp[2];
            // unsigned long sharedVertIndex[3]
            readInts(stream, tmp, 3);
            tri.sharedVertIndex[0] = tmp[0];
            tri.sharedVertIndex[1] = tmp[1];
            tri.sharedVertIndex[2] = tmp[2];
            // float normal[4]
            readFloats(stream, &(edgeData->triangleFaceNormals[t].x), 4);

        }

        // Assume the mesh is closed, it will update later
        edgeData->isClosed = true;

        pushInnerChunk(stream);
        for (uint32 eg = 0; eg < numEdgeGroups; ++eg)
        {
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            if (streamID != MeshChunkID::EDGE_GROUP)
            {
                OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR,
                    "Missing MeshChunkID::EDGE_GROUP stream",
                    "MeshSerializerImpl_v1_3::readEdgeListLodInfo");
            }
            EdgeData::EdgeGroup& edgeGroup = edgeData->edgeGroups[eg];

            // unsigned long vertexSet
            readInts(stream, tmp, 1);
            edgeGroup.vertexSet = tmp[0];
            // unsigned long numEdges
            uint32 numEdges;
            readInts(stream, &numEdges, 1);
            edgeGroup.edges.resize(numEdges);
            // Edge* edgeList
            for (uint32 e = 0; e < numEdges; ++e)
            {
                EdgeData::Edge& edge = edgeGroup.edges[e];
                // unsigned long  triIndex[2]
                readInts(stream, tmp, 2);
                edge.triIndex[0] = tmp[0];
                edge.triIndex[1] = tmp[1];
                // unsigned long  vertIndex[2]
                readInts(stream, tmp, 2);
                edge.vertIndex[0] = tmp[0];
                edge.vertIndex[1] = tmp[1];
                // unsigned long  sharedVertIndex[2]
                readInts(stream, tmp, 2);
                edge.sharedVertIndex[0] = tmp[0];
                edge.sharedVertIndex[1] = tmp[1];
                // bool degenerate
                readBools(stream, &(edge.degenerate), 1);

                // The mesh is closed only if no degenerate edge here
                if (edge.degenerate)
                {
                    edgeData->isClosed = false;
                }
            }
        }
        popInnerChunk(stream);
        reorganiseTriangles(edgeData);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_3::reorganiseTriangles(EdgeData* edgeData)
    {
        size_t numTriangles = edgeData->triangles.size();

        if (edgeData->edgeGroups.size() == 1)
        {
            // Special case for only one edge group in the edge list, which occurring
            // most time. In this case, all triangles belongs to that group.
            edgeData->edgeGroups.front().triStart = 0;
            edgeData->edgeGroups.front().triCount = numTriangles;
        }
        else
        {
            // Calculate number of triangles for edge groups

            for (auto & edgeGroup : edgeData->edgeGroups)
            {
                edgeGroup.triStart = 0;
                edgeGroup.triCount = 0;
            }

            bool isGrouped = true;
            EdgeData::EdgeGroup* lastEdgeGroup = nullptr;
            for (size_t t = 0; t < numTriangles; ++t)
            {
                // Gets the edge group that the triangle belongs to
                const EdgeData::Triangle& tri = edgeData->triangles[t];
                EdgeData::EdgeGroup* edgeGroup = &edgeData->edgeGroups[tri.vertexSet];

                // Does edge group changes from last edge group?
                if (isGrouped && edgeGroup != lastEdgeGroup)
                {
                    // Remember last edge group
                    lastEdgeGroup = edgeGroup;

                    // Is't first time encounter this edge group?
                    if (!edgeGroup->triCount && !edgeGroup->triStart)
                    {
                        // setup first triangle of this edge group
                        edgeGroup->triStart = t;
                    }
                    else
                    {
                        // original triangles doesn't grouping by edge group
                        isGrouped = false;
                    }
                }

                // Count number of triangles for this edge group
                if(edgeGroup)
                    ++edgeGroup->triCount;
            }

            //
            // Note that triangles has been sorted by vertex set for a long time,
            // but never stored to old version mesh file.
            //
            // Adopt this fact to avoid remap triangles here.
            //

            // Does triangles grouped by vertex set?
            if (!isGrouped)
            {
                // Ok, the triangles of this edge list isn't grouped by vertex set
                // perfectly, seems ancient mesh file.
                //
                // We need work hardly to group triangles by vertex set.
                //

                // Calculate triStart and reset triCount to zero for each edge group first
                size_t triStart = 0;
                for (auto & edgeGroup : edgeData->edgeGroups)
                {
                    edgeGroup.triStart = triStart;
                    triStart += edgeGroup.triCount;
                    edgeGroup.triCount = 0;
                }

                // The map used to mapping original triangle index to new index
                using TriangleIndexRemap = std::vector<size_t>;
                TriangleIndexRemap triangleIndexRemap(numTriangles);

                // New triangles information that should be group by vertex set.
                EdgeData::TriangleList newTriangles(numTriangles);
                EdgeData::TriangleFaceNormalList newTriangleFaceNormals(numTriangles);

                // Calculate triangle index map and organise triangles information
                for (size_t t = 0; t < numTriangles; ++t)
                {
                    // Gets the edge group that the triangle belongs to
                    const EdgeData::Triangle& tri = edgeData->triangles[t];
                    EdgeData::EdgeGroup& edgeGroup = edgeData->edgeGroups[tri.vertexSet];

                    // Calculate new index
                    size_t newIndex = edgeGroup.triStart + edgeGroup.triCount;
                    ++edgeGroup.triCount;

                    // Setup triangle index mapping entry
                    triangleIndexRemap[t] = newIndex;

                    // Copy triangle info to new placement
                    newTriangles[newIndex] = tri;
                    newTriangleFaceNormals[newIndex] = edgeData->triangleFaceNormals[t];
                }

                // Replace with new triangles information
                edgeData->triangles.swap(newTriangles);
                edgeData->triangleFaceNormals.swap(newTriangleFaceNormals);

                // Now, update old triangle indices to new index
                for (auto & edgeGroup : edgeData->edgeGroups)
                {
                    for (auto & edge : edgeGroup.edges)
                    {
                        edge.triIndex[0] = triangleIndexRemap[edge.triIndex[0]];
                        if (!edge.degenerate)
                        {
                            edge.triIndex[1] = triangleIndexRemap[edge.triIndex[1]];
                        }
                    }
                }
            }
        }
    }
    auto MeshSerializerImpl_v1_3::calcEdgeListLodSize(const EdgeData* edgeData, bool isManual) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // unsigned short lodIndex
        size += sizeof(uint16);

        // bool isManual            // If manual, no edge data here, loaded from manual mesh
        size += sizeof(bool);
        if (!isManual)
        {
            // bool isClosed
            //size += sizeof(bool); <== missing in v1_3
            // unsigned long numTriangles
            size += sizeof(uint32);
            // unsigned long numEdgeGroups
            size += sizeof(uint32);
            // Triangle* triangleList
            size_t triSize = 0;
            // unsigned long indexSet
            // unsigned long vertexSet
            // unsigned long vertIndex[3]
            // unsigned long sharedVertIndex[3]
            // float normal[4]
            triSize += sizeof(uint32)* 8
                + sizeof(float)* 4;

            size += triSize * edgeData->triangles.size();
            // Write the groups
            for (const auto & edgeGroup : edgeData->edgeGroups)
            {
                size += calcEdgeGroupSize(edgeGroup);
            }

        }

        return size;
    }
    //---------------------------------------------------------------------
    auto MeshSerializerImpl_v1_3::calcEdgeGroupSize(const EdgeData::EdgeGroup& group) -> size_t
    {
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // unsigned long vertexSet
        size += sizeof(uint32);
        // unsigned long triStart
        //size += sizeof(uint32); <== missing in v1_3
        // unsigned long triCount
        //size += sizeof(uint32); <== missing in v1_3
        // unsigned long numEdges
        size += sizeof(uint32);
        // Edge* edgeList
        size_t edgeSize = 0;
        // unsigned long  triIndex[2]
        // unsigned long  vertIndex[2]
        // unsigned long  sharedVertIndex[2]
        // bool degenerate
        edgeSize += sizeof(uint32)* 6 + sizeof(bool);
        size += edgeSize * group.edges.size();

        return size;
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_3::writeEdgeList(const Mesh* pMesh)
    {
        assert(exportedLodCount != 0);
        writeChunkHeader(std::to_underlying(MeshChunkID::EDGE_LISTS), calcEdgeListSize(pMesh));
        pushInnerChunk(mStream);
        for (ushort i = 0; i < exportedLodCount; ++i)
        {
            const EdgeData* edgeData = pMesh->getEdgeList(i);
            bool isManual = !pMesh->mMeshLodUsageList[i].manualName.empty();
            writeChunkHeader(std::to_underlying(MeshChunkID::EDGE_LIST_LOD), calcEdgeListLodSize(edgeData, isManual));
            
            // unsigned short lodIndex
            writeShorts(&i, 1);
            
            // bool isManual            // If manual, no edge data here, loaded from manual mesh
            writeBools(&isManual, 1);
            if (!isManual)
            {
                // unsigned long  numTriangles
                auto count = static_cast<uint32>(edgeData->triangles.size());
                writeInts(&count, 1);
                // unsigned long numEdgeGroups
                count = static_cast<uint32>(edgeData->edgeGroups.size());
                writeInts(&count, 1);
                // Triangle* triangleList
                // Iterate rather than writing en-masse to allow endian conversion
                for (auto fni = edgeData->triangleFaceNormals.begin();
                    const EdgeData::Triangle& tri : edgeData->triangles)
                {
                    // unsigned long indexSet;
                    uint32 tmp[3];
                    tmp[0] = static_cast<uint32>(tri.indexSet);
                    writeInts(tmp, 1);
                    // unsigned long vertexSet;
                    tmp[0] = static_cast<uint32>(tri.vertexSet);
                    writeInts(tmp, 1);
                    // unsigned long vertIndex[3];
                    tmp[0] = static_cast<uint32>(tri.vertIndex[0]);
                    tmp[1] = static_cast<uint32>(tri.vertIndex[1]);
                    tmp[2] = static_cast<uint32>(tri.vertIndex[2]);
                    writeInts(tmp, 3);
                    // unsigned long sharedVertIndex[3];
                    tmp[0] = static_cast<uint32>(tri.sharedVertIndex[0]);
                    tmp[1] = static_cast<uint32>(tri.sharedVertIndex[1]);
                    tmp[2] = static_cast<uint32>(tri.sharedVertIndex[2]);
                    writeInts(tmp, 3);
                    // float normal[4];
                    writeFloats(&(fni->x), 4);

                    ++fni;
                }
                pushInnerChunk(mStream);
                // Write the groups
                for (const auto & edgeGroup : edgeData->edgeGroups)
                {
                    writeChunkHeader(std::to_underlying(MeshChunkID::EDGE_GROUP), calcEdgeGroupSize(edgeGroup));
                    // unsigned long vertexSet
                    auto vertexSet = static_cast<uint32>(edgeGroup.vertexSet);
                    writeInts(&vertexSet, 1);
                    // unsigned long numEdges
                    count = static_cast<uint32>(edgeGroup.edges.size());
                    writeInts(&count, 1);
                    // Edge* edgeList
                    // Iterate rather than writing en-masse to allow endian conversion
                    for (const auto & edge : edgeGroup.edges)
                    {
                        uint32 tmp[2];
                        // unsigned long  triIndex[2]
                        tmp[0] = static_cast<uint32>(edge.triIndex[0]);
                        tmp[1] = static_cast<uint32>(edge.triIndex[1]);
                        writeInts(tmp, 2);
                        // unsigned long  vertIndex[2]
                        tmp[0] = static_cast<uint32>(edge.vertIndex[0]);
                        tmp[1] = static_cast<uint32>(edge.vertIndex[1]);
                        writeInts(tmp, 2);
                        // unsigned long  sharedVertIndex[2]
                        tmp[0] = static_cast<uint32>(edge.sharedVertIndex[0]);
                        tmp[1] = static_cast<uint32>(edge.sharedVertIndex[1]);
                        writeInts(tmp, 2);
                        // bool degenerate
                        writeBools(&(edge.degenerate), 1);
                    }
                    
                }
                popInnerChunk(mStream);
                
            }
            
        }
        popInnerChunk(mStream);
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_2::MeshSerializerImpl_v1_2()
    {
        // Version number
        mVersion = "[MeshSerializer_v1.20]";
    }
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_2::~MeshSerializerImpl_v1_2()
    = default;
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_2::readMesh(const DataStreamPtr& stream, Mesh* pMesh, MeshSerializerListener *listener)
    {
        MeshSerializerImpl::readMesh(stream, pMesh, listener);
        // Always automatically build edge lists for this version
        pMesh->mAutoBuildEdgeLists = true;

    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_2::readGeometry(const DataStreamPtr& stream, Mesh* pMesh,
        VertexData* dest)
    {
        unsigned short bindIdx = 0;

        dest->vertexStart = 0;

        unsigned int vertexCount = 0;
        readInts(stream, &vertexCount, 1);
        dest->vertexCount = vertexCount;

        // Vertex buffers

        readGeometryPositions(bindIdx, stream, pMesh, dest);
        ++bindIdx;
        // Find optional geometry streams
        if (!stream->eof())
        {
            pushInnerChunk(stream);
            auto streamID = static_cast<MeshChunkID>(readChunk(stream));
            unsigned short texCoordSet = 0;
            
            using enum MeshChunkID;
            while(!stream->eof() &&
                (streamID == GEOMETRY_NORMALS ||
                 streamID == GEOMETRY_COLOURS ||
                 streamID == GEOMETRY_TEXCOORDS ))
            {
                switch (streamID)
                {
                case GEOMETRY_NORMALS:
                    readGeometryNormals(bindIdx++, stream, pMesh, dest);
                    break;
                case GEOMETRY_COLOURS:
                    readGeometryColours(bindIdx++, stream, pMesh, dest);
                    break;
                case GEOMETRY_TEXCOORDS:
                    readGeometryTexCoords(bindIdx++, stream, pMesh, dest, texCoordSet++);
                    break;
                default:
                    break;
                }
                // Get next stream
                if (!stream->eof())
                {
                    streamID = static_cast<MeshChunkID>(readChunk(stream));
                }
            }
            if (!stream->eof())
            {
                // Backpedal back to start of non-submesh stream
                backpedalChunkHeader(stream);
            }
            popInnerChunk(stream);
        }
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_2::readGeometryPositions(unsigned short bindIdx,
        const DataStreamPtr& stream, Mesh* pMesh, VertexData* dest)
    {
        HardwareVertexBufferSharedPtr vbuf;
        // float* pVertices (x, y, z order x numVertices)
        dest->vertexDeclaration->addElement(bindIdx, 0, VertexElementType::FLOAT3, VertexElementSemantic::POSITION);
        vbuf = pMesh->getHardwareBufferManager()->createVertexBuffer(
            dest->vertexDeclaration->getVertexSize(bindIdx),
            dest->vertexCount,
            pMesh->mVertexBufferUsage,
            pMesh->mVertexBufferShadowBuffer);
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        readFloats(stream, static_cast<float*>(vbufLock.pData), dest->vertexCount * 3);
        dest->vertexBufferBinding->setBinding(bindIdx, vbuf);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_2::readGeometryNormals(unsigned short bindIdx,
        const DataStreamPtr& stream, Mesh* pMesh, VertexData* dest)
    {
        HardwareVertexBufferSharedPtr vbuf;
        // float* pNormals (x, y, z order x numVertices)
        dest->vertexDeclaration->addElement(bindIdx, 0, VertexElementType::FLOAT3, VertexElementSemantic::NORMAL);
        vbuf = pMesh->getHardwareBufferManager()->createVertexBuffer(
            dest->vertexDeclaration->getVertexSize(bindIdx),
            dest->vertexCount,
            pMesh->mVertexBufferUsage,
            pMesh->mVertexBufferShadowBuffer);
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        readFloats(stream, static_cast<float*>(vbufLock.pData), dest->vertexCount * 3);
        dest->vertexBufferBinding->setBinding(bindIdx, vbuf);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_2::readGeometryColours(unsigned short bindIdx,
        const DataStreamPtr& stream, Mesh* pMesh, VertexData* dest)
    {
        HardwareVertexBufferSharedPtr vbuf;
        // unsigned long* pColours (RGBA 8888 format x numVertices)
        dest->vertexDeclaration->addElement(bindIdx, 0, VertexElementType::COLOUR, VertexElementSemantic::DIFFUSE);
        vbuf = pMesh->getHardwareBufferManager()->createVertexBuffer(
            dest->vertexDeclaration->getVertexSize(bindIdx),
            dest->vertexCount,
            pMesh->mVertexBufferUsage,
            pMesh->mVertexBufferShadowBuffer);
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        readInts(stream, static_cast<RGBA*>(vbufLock.pData), dest->vertexCount);
        dest->vertexBufferBinding->setBinding(bindIdx, vbuf);
    }
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_2::readGeometryTexCoords(unsigned short bindIdx,
        const DataStreamPtr& stream, Mesh* pMesh, VertexData* dest, unsigned short texCoordSet)
    {
        HardwareVertexBufferSharedPtr vbuf;
        // unsigned short dimensions    (1 for 1D, 2 for 2D, 3 for 3D)
        unsigned short dim;
        readShorts(stream, &dim, 1);
        // float* pTexCoords  (u [v] [w] order, dimensions x numVertices)
        dest->vertexDeclaration->addElement(
            bindIdx,
            0,
            VertexElement::multiplyTypeCount(VertexElementType::FLOAT1, dim),
            VertexElementSemantic::TEXTURE_COORDINATES,
            texCoordSet);
        vbuf = pMesh->getHardwareBufferManager()->createVertexBuffer(
            dest->vertexDeclaration->getVertexSize(bindIdx),
            dest->vertexCount,
            pMesh->mVertexBufferUsage,
            pMesh->mVertexBufferShadowBuffer);
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        readFloats(stream, static_cast<float*>(vbufLock.pData), dest->vertexCount * dim);
        dest->vertexBufferBinding->setBinding(bindIdx, vbuf);
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_1::MeshSerializerImpl_v1_1()
    {
        // Version number
        mVersion = "[MeshSerializer_v1.10]";
    }
    //---------------------------------------------------------------------
    MeshSerializerImpl_v1_1::~MeshSerializerImpl_v1_1()
    = default;
    //---------------------------------------------------------------------
    void MeshSerializerImpl_v1_1::readGeometryTexCoords(unsigned short bindIdx,
        const DataStreamPtr& stream, Mesh* pMesh, VertexData* dest, unsigned short texCoordSet)
    {
        float *pFloat = nullptr;
        HardwareVertexBufferSharedPtr vbuf;
        // unsigned short dimensions    (1 for 1D, 2 for 2D, 3 for 3D)
        unsigned short dim;
        readShorts(stream, &dim, 1);
        // float* pTexCoords  (u [v] [w] order, dimensions x numVertices)
        dest->vertexDeclaration->addElement(
            bindIdx,
            0,
            VertexElement::multiplyTypeCount(VertexElementType::FLOAT1, dim),
            VertexElementSemantic::TEXTURE_COORDINATES,
            texCoordSet);
        vbuf = pMesh->getHardwareBufferManager()->createVertexBuffer(
            dest->vertexDeclaration->getVertexSize(bindIdx),
            dest->vertexCount,
            pMesh->getVertexBufferUsage(),
            pMesh->isVertexBufferShadowed());
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        pFloat = static_cast<float*>(vbufLock.pData);
        readFloats(stream, pFloat, dest->vertexCount * dim);

        // Adjust individual v values to (1 - v)
        if (dim == 2)
        {
            for (size_t i = 0; i < dest->vertexCount; ++i)
            {
                ++pFloat; // skip u
                *pFloat = 1.0f - *pFloat; // v = 1 - v
                ++pFloat;
            }

        }
        dest->vertexBufferBinding->setBinding(bindIdx, vbuf);
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------




}
