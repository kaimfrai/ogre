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

import :Exception;
import :HardwareBuffer;
import :HardwareBufferManager;
import :HardwareVertexBuffer;
import :Pose;
import :VertexIndexData;

import <utility>;

namespace Ogre {
    //---------------------------------------------------------------------
    Pose::Pose(ushort target, std::string_view name)
        : mTarget(target), mName(name)
    {
    }
    //---------------------------------------------------------------------
    void Pose::addVertex(size_t index, const Vector3& offset)
    {
        OgreAssert(mNormalsMap.empty(),
                   "Inconsistent calls to addVertex, must include normals always or never");

        if(offset.squaredLength() < 1e-6f)
        {
            return;
        }

        mVertexOffsetMap[index] = offset;
        mBuffer.reset();
    }
    //---------------------------------------------------------------------
    void Pose::addVertex(size_t index, const Vector3& offset, const Vector3& normal)
    {
        OgreAssert(mVertexOffsetMap.empty() || !mNormalsMap.empty(),
                   "Inconsistent calls to addVertex, must include normals always or never");

        if(offset.squaredLength() < 1e-6f && normal.squaredLength() < 1e-6f)
        {
            return;
        }

        mVertexOffsetMap[index] = offset;
        mNormalsMap[index] = normal;
        mBuffer.reset();
    }
    //---------------------------------------------------------------------
    void Pose::removeVertex(size_t index)
    {
        auto i = mVertexOffsetMap.find(index);
        if (i != mVertexOffsetMap.end())
        {
            mVertexOffsetMap.erase(i);
            mBuffer.reset();
        }
        auto j = mNormalsMap.find(index);
        if (j != mNormalsMap.end())
        {
            mNormalsMap.erase(j);
        }
    }
    //---------------------------------------------------------------------
    void Pose::clearVertices()
    {
        mVertexOffsetMap.clear();
        mNormalsMap.clear();
        mBuffer.reset();
    }

    //---------------------------------------------------------------------
    auto Pose::_getHardwareVertexBuffer(const VertexData* origData) const -> const HardwareVertexBufferSharedPtr&
    {
        size_t numVertices = origData->vertexCount;
        
        if (!mBuffer)
        {
            // Create buffer
            size_t vertexSize = VertexElement::getTypeSize(VertexElementType::FLOAT3);
            bool normals = getIncludesNormals();
            if (normals)
                vertexSize += VertexElement::getTypeSize(VertexElementType::FLOAT3);
                
            mBuffer = HardwareBufferManager::getSingleton().createVertexBuffer(
                vertexSize, numVertices, HardwareBuffer::STATIC_WRITE_ONLY);

            HardwareBufferLockGuard bufLock(mBuffer, HardwareBuffer::LockOptions::DISCARD);
            auto* pFloat = static_cast<float*>(bufLock.pData);
            // initialise - these will be the values used where no pose vertex is included
            memset(pFloat, 0, mBuffer->getSizeInBytes()); 
            if (normals)
            {
                // zeroes are fine for positions (deltas), but for normals we need the original
                // mesh normals, since delta normals don't work (re-normalisation would
                // always result in a blended normal even with full pose applied)
                const VertexElement* origNormElem = 
                    origData->vertexDeclaration->findElementBySemantic(VertexElementSemantic::NORMAL, 0);
                assert(origNormElem);
                
                const HardwareVertexBufferSharedPtr& origBuffer = 
                    origData->vertexBufferBinding->getBuffer(origNormElem->getSource());
                HardwareBufferLockGuard origBufLock(origBuffer, HardwareBuffer::LockOptions::READ_ONLY);
                float* pDst = pFloat + 3;
                float* pSrc;
                origNormElem->baseVertexPointerToElement(origBufLock.pData, &pSrc);
                for (size_t v = 0; v < numVertices; ++v)
                {
                    memcpy(pDst, pSrc, sizeof(float)*3);
                    
                    pDst += 6;
                    pSrc = (float*)(((char*)pSrc) + origBuffer->getVertexSize());
                }
            }
            // Set each vertex
            size_t numFloatsPerVertex = normals ? 6: 3;
            
            for (auto nIt = mNormalsMap.begin();
                    auto const& [vKey, vertex] : mVertexOffsetMap)
            {
                auto const& [nKey, normal] = *nIt;
                // Remember, vertex maps are *sparse* so may have missing entries
                // This is why we skip
                float* pDst = pFloat + (numFloatsPerVertex * vKey);
                *pDst++ = vertex.x;
                *pDst++ = vertex.y;
                *pDst++ = vertex.z;
                if (normals)
                {
                    *pDst++ = normal.x;
                    *pDst++ = normal.y;
                    *pDst++ = normal.z;
                    ++nIt;
                }
                
            }
        }
        return mBuffer;
    }
    //---------------------------------------------------------------------
    auto Pose::clone() const -> Pose*
    {
        Pose* newPose = new Pose(mTarget, mName);
        newPose->mVertexOffsetMap = mVertexOffsetMap;
        newPose->mNormalsMap = mNormalsMap;
        // Allow buffer to recreate itself, contents may change anyway
        return newPose;
    }

}
