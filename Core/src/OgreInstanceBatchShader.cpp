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
#include <algorithm>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreException.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgreHardwareBuffer.hpp"
#include "OgreHardwareBufferManager.hpp"
#include "OgreHardwareIndexBuffer.hpp"
#include "OgreHardwareVertexBuffer.hpp"
#include "OgreInstanceBatchShader.hpp"
#include "OgreInstancedEntity.hpp"
#include "OgreLogManager.hpp"
#include "OgreMaterial.hpp"
#include "OgreMatrix4.hpp"
#include "OgrePass.hpp"
#include "OgreRenderOperation.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreSubMesh.hpp"
#include "OgreTechnique.hpp"
#include "OgreVertexIndexData.hpp"

namespace Ogre
{
class InstanceManager;

    InstanceBatchShader::InstanceBatchShader( InstanceManager *creator, MeshPtr &meshReference,
                                        const MaterialPtr &material, size_t instancesPerBatch,
                                        const Mesh::IndexMap *indexToBoneMap, std::string_view batchName ) :
                InstanceBatch( creator, meshReference, material, instancesPerBatch,
                                indexToBoneMap, batchName ),
                mNumWorldMatrices( instancesPerBatch )
    {
    }

    //-----------------------------------------------------------------------
    auto InstanceBatchShader::calculateMaxNumInstances( const SubMesh *baseSubMesh, InstanceManagerFlags flags ) const -> size_t
    {
        const size_t numBones = std::max<size_t>( 1, baseSubMesh->blendIndexToBoneIndexMap.size() );

        mMaterial->load();
        Technique *technique = mMaterial->getBestTechnique();
        if( technique )
        {
            GpuProgramParametersSharedPtr vertexParam = technique->getPass(0)->getVertexProgramParameters();
            for(auto const& [key, constDef] : vertexParam->getConstantDefinitions().map)
            {
                if(((constDef.constType == GpuConstantType::MATRIX_3X4 ||
                    constDef.constType == GpuConstantType::MATRIX_4X3 ||             //OGL GLSL bitches without this
                    constDef.constType == GpuConstantType::MATRIX_2X4 ||
                    constDef.constType == GpuConstantType::FLOAT4)                   //OGL GLSL bitches without this
                    && constDef.isFloat()) ||
                   ((constDef.constType == GpuConstantType::MATRIX_DOUBLE_3X4 ||
                    constDef.constType == GpuConstantType::MATRIX_DOUBLE_4X3 ||      //OGL GLSL bitches without this
                    constDef.constType == GpuConstantType::MATRIX_DOUBLE_2X4 ||
                    constDef.constType == GpuConstantType::DOUBLE4)                  //OGL GLSL bitches without this
                    && constDef.isDouble())
                   )
                {
                    const GpuProgramParameters::AutoConstantEntry *entry =
                                    vertexParam->_findRawAutoConstantEntryFloat( constDef.physicalIndex );
                    if( entry && (entry->paramType == GpuProgramParameters::AutoConstantType::WORLD_MATRIX_ARRAY_3x4 || entry->paramType == GpuProgramParameters::AutoConstantType::WORLD_DUALQUATERNION_ARRAY_2x4))
                    {
                        //Material is correctly done!
                        size_t arraySize = constDef.arraySize;

                        //Deal with GL "hacky" way of doing 4x3 matrices
                        if(entry->paramType == GpuProgramParameters::AutoConstantType::WORLD_MATRIX_ARRAY_3x4 && constDef.constType == GpuConstantType::FLOAT4)
                            arraySize /= 3;
                        else if(entry->paramType == GpuProgramParameters::AutoConstantType::WORLD_DUALQUATERNION_ARRAY_2x4 && constDef.constType == GpuConstantType::FLOAT4)
                            arraySize /= 2;

                        //Check the num of arrays
                        size_t retVal = arraySize / numBones;

                        if(!!(flags & InstanceManagerFlags::USE16BIT))
                        {
                            if( baseSubMesh->vertexData->vertexCount * retVal > 0xFFFF )
                                retVal = 0xFFFF / baseSubMesh->vertexData->vertexCount;
                        }

                        if((retVal < 3 && entry->paramType == GpuProgramParameters::AutoConstantType::WORLD_MATRIX_ARRAY_3x4) ||
                            (retVal < 2 && entry->paramType == GpuProgramParameters::AutoConstantType::WORLD_DUALQUATERNION_ARRAY_2x4))
                        {
                            LogManager::getSingleton().logWarning(std::format("InstanceBatchShader: Mesh '{}' using material '{}'. The amount of possible "
                                        "instances per batch is very low. Performance benefits will "
                                        "be minimal, if any. It might be even slower!", mMeshReference->getName(), mMaterial->getName()));
                        }

                        return retVal;
                    }
                }
            }

            //Reaching here means material is supported, but malformed
            OGRE_EXCEPT( ExceptionCodes::INVALIDPARAMS, 
            ::std::format("Material '{}' is malformed for this instancing technique", mMaterial->getName() ),
            "InstanceBatchShader::calculateMaxNumInstances");
        }

        //Reaching here the material is just unsupported.

        return 0;
    }
    //-----------------------------------------------------------------------
    void InstanceBatchShader::buildFrom( const SubMesh *baseSubMesh, const RenderOperation &renderOperation )
    {
        if( mMeshReference->hasSkeleton() && mMeshReference->getSkeleton() )
            mNumWorldMatrices = mInstancesPerBatch * baseSubMesh->blendIndexToBoneIndexMap.size();
        InstanceBatch::buildFrom( baseSubMesh, renderOperation );
    }
    //-----------------------------------------------------------------------
    void InstanceBatchShader::setupVertices( const SubMesh* baseSubMesh )
    {
        mRenderOperation.vertexData = new VertexData();
        mRemoveOwnVertexData = true; //Raise flag to remove our own vertex data in the end (not always needed)

        VertexData *thisVertexData = mRenderOperation.vertexData;
        VertexData *baseVertexData = baseSubMesh->vertexData.get();

        thisVertexData->vertexStart = 0;
        thisVertexData->vertexCount = baseVertexData->vertexCount * mInstancesPerBatch;

        HardwareBufferManager::getSingleton().destroyVertexDeclaration( thisVertexData->vertexDeclaration );
        thisVertexData->vertexDeclaration = baseVertexData->vertexDeclaration->clone();

        if( mMeshReference->hasSkeleton() && mMeshReference->getSkeleton() )
        {
            //Building hw skinned batches follow a different path
            setupHardwareSkinned( baseSubMesh, thisVertexData, baseVertexData );
            return;
        }

        //TODO: Can't we, instead of using another source, put the index ID in the same source?
        thisVertexData->vertexDeclaration->addElement(
                                        thisVertexData->vertexDeclaration->getMaxSource() + 1, 0,
                                        VertexElementType::UBYTE4, VertexElementSemantic::BLEND_INDICES );


        for( uint16 i=0; i<thisVertexData->vertexDeclaration->getMaxSource(); ++i )
        {
            //Create our own vertex buffer
            HardwareVertexBufferSharedPtr vertexBuffer =
                                            HardwareBufferManager::getSingleton().createVertexBuffer(
                                            thisVertexData->vertexDeclaration->getVertexSize(i),
                                            thisVertexData->vertexCount,
                                            HardwareBuffer::STATIC_WRITE_ONLY );
            thisVertexData->vertexBufferBinding->setBinding( i, vertexBuffer );

            //Grab the base submesh data
            HardwareVertexBufferSharedPtr baseVertexBuffer =
                                                    baseVertexData->vertexBufferBinding->getBuffer(i);

            HardwareBufferLockGuard thisLock(vertexBuffer, HardwareBuffer::LockOptions::DISCARD);
            HardwareBufferLockGuard baseLock(baseVertexBuffer, HardwareBuffer::LockOptions::READ_ONLY);
            char* thisBuf = static_cast<char*>(thisLock.pData);
            char* baseBuf = static_cast<char*>(baseLock.pData);

            //Copy and repeat
            for( size_t j=0; j<mInstancesPerBatch; ++j )
            {
                const size_t sizeOfBuffer = baseVertexData->vertexCount *
                                            baseVertexData->vertexDeclaration->getVertexSize(i);
                memcpy( thisBuf + j * sizeOfBuffer, baseBuf, sizeOfBuffer );
            }
        }

        {
            //Now create the vertices "index ID" to individualize each instance
            const unsigned short lastSource = thisVertexData->vertexDeclaration->getMaxSource();
            HardwareVertexBufferSharedPtr vertexBuffer =
                                            HardwareBufferManager::getSingleton().createVertexBuffer(
                                            thisVertexData->vertexDeclaration->getVertexSize( lastSource ),
                                            thisVertexData->vertexCount,
                                            HardwareBuffer::STATIC_WRITE_ONLY );
            thisVertexData->vertexBufferBinding->setBinding( lastSource, vertexBuffer );

            HardwareBufferLockGuard thisLock(vertexBuffer, HardwareBuffer::LockOptions::DISCARD);
            char* thisBuf = static_cast<char*>(thisLock.pData);
            for( uint8 j=0; j<uint8(mInstancesPerBatch); ++j )
            {
                for( size_t k=0; k<baseVertexData->vertexCount; ++k )
                {
                    *thisBuf++ = j;
                    *thisBuf++ = j;
                    *thisBuf++ = j;
                    *thisBuf++ = j;
                }
            }

        }
    }
    //-----------------------------------------------------------------------
    void InstanceBatchShader::setupIndices( const SubMesh* baseSubMesh )
    {
        mRenderOperation.indexData = new IndexData();
        mRemoveOwnIndexData = true; //Raise flag to remove our own index data in the end (not always needed)

        IndexData *thisIndexData = mRenderOperation.indexData;
        IndexData *baseIndexData = baseSubMesh->indexData.get();

        thisIndexData->indexStart = 0;
        thisIndexData->indexCount = baseIndexData->indexCount * mInstancesPerBatch;

        //TODO: Check numVertices is below max supported by GPU
        HardwareIndexBuffer::IndexType indexType = HardwareIndexBuffer::IndexType::_16BIT;
        if( mRenderOperation.vertexData->vertexCount > 65535 )
            indexType = HardwareIndexBuffer::IndexType::_32BIT;
        thisIndexData->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
            indexType, thisIndexData->indexCount, HardwareBuffer::STATIC_WRITE_ONLY );

        HardwareBufferLockGuard thisLock(thisIndexData->indexBuffer, HardwareBuffer::LockOptions::DISCARD);
        HardwareBufferLockGuard baseLock(baseIndexData->indexBuffer, HardwareBuffer::LockOptions::READ_ONLY);
        auto *thisBuf16 = static_cast<uint16*>(thisLock.pData);
        auto *thisBuf32 = static_cast<uint32*>(thisLock.pData);
        bool baseIndex16bit = baseIndexData->indexBuffer->getType() == HardwareIndexBuffer::IndexType::_16BIT;

        for( size_t i=0; i<mInstancesPerBatch; ++i )
        {
            const size_t vertexOffset = i * mRenderOperation.vertexData->vertexCount / mInstancesPerBatch;

            const auto *initBuf16 = static_cast<const uint16 *>(baseLock.pData);
            const auto *initBuf32 = static_cast<const uint32 *>(baseLock.pData);

            for( size_t j=0; j<baseIndexData->indexCount; ++j )
            {
                uint32 originalVal = baseIndex16bit ? *initBuf16++ : *initBuf32++;

                if( indexType == HardwareIndexBuffer::IndexType::_16BIT )
                    *thisBuf16++ = static_cast<uint16>(originalVal + vertexOffset);
                else
                    *thisBuf32++ = static_cast<uint32>(originalVal + vertexOffset);
            }
        }
    }
    //-----------------------------------------------------------------------
    void InstanceBatchShader::setupHardwareSkinned( const SubMesh* baseSubMesh, VertexData *thisVertexData,
                                                    VertexData *baseVertexData )
    {
        const auto numBones = uint8(baseSubMesh->blendIndexToBoneIndexMap.size());
        mNumWorldMatrices = mInstancesPerBatch * numBones;

        for( uint16 i=0; i<=thisVertexData->vertexDeclaration->getMaxSource(); ++i )
        {
            //Create our own vertex buffer
            HardwareVertexBufferSharedPtr vertexBuffer =
                                            HardwareBufferManager::getSingleton().createVertexBuffer(
                                            thisVertexData->vertexDeclaration->getVertexSize(i),
                                            thisVertexData->vertexCount,
                                            HardwareBuffer::STATIC_WRITE_ONLY );
            thisVertexData->vertexBufferBinding->setBinding( i, vertexBuffer );

            VertexDeclaration::VertexElementList veList =
                                            thisVertexData->vertexDeclaration->findElementsBySource(i);

            //Grab the base submesh data
            HardwareVertexBufferSharedPtr baseVertexBuffer =
                                                    baseVertexData->vertexBufferBinding->getBuffer(i);

            HardwareBufferLockGuard thisVertexLock(vertexBuffer, HardwareBuffer::LockOptions::DISCARD);
            HardwareBufferLockGuard baseVertexLock(baseVertexBuffer, HardwareBuffer::LockOptions::READ_ONLY);
            char* thisBuf = static_cast<char*>(thisVertexLock.pData);
            char* baseBuf = static_cast<char*>(baseVertexLock.pData);
            char *startBuf = baseBuf;

            //Copy and repeat
            for( uint8 j=0; j<uint8(mInstancesPerBatch); ++j )
            {
                //Repeat source
                baseBuf = startBuf;

                for( size_t k=0; k<baseVertexData->vertexCount; ++k )
                {
                    for (auto const& it : veList)
                    {
                        using enum VertexElementSemantic;
                        switch( it.getSemantic() )
                        {
                        case BLEND_INDICES:
                        *(thisBuf + it.getOffset() + 0) = *(baseBuf + it.getOffset() + 0) + j * numBones;
                        *(thisBuf + it.getOffset() + 1) = *(baseBuf + it.getOffset() + 1) + j * numBones;
                        *(thisBuf + it.getOffset() + 2) = *(baseBuf + it.getOffset() + 2) + j * numBones;
                        *(thisBuf + it.getOffset() + 3) = *(baseBuf + it.getOffset() + 3) + j * numBones;
                            break;
                        default:
                            memcpy( thisBuf + it.getOffset(), baseBuf + it.getOffset(), it.getSize() );
                            break;
                        }
                    }
                    thisBuf += baseVertexData->vertexDeclaration->getVertexSize(i);
                    baseBuf += baseVertexData->vertexDeclaration->getVertexSize(i);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void InstanceBatchShader::getWorldTransforms( Matrix4* xform ) const
    {
        for (auto const& itor :  mInstancedEntities)
        {
            xform += itor->getTransforms( xform );
        }
    }
    //-----------------------------------------------------------------------
    auto InstanceBatchShader::getNumWorldTransforms() const noexcept -> unsigned short
    {
        return uint16(mNumWorldMatrices);
    }
}
