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

#include <cmath>
#include <cstddef>

module Ogre.Core;

import :AxisAlignedBox;
import :HardwareBuffer;
import :HardwareBufferManager;
import :HardwareIndexBuffer;
import :HardwareVertexBuffer;
import :Math;
import :Mesh;
import :Platform;
import :PrefabFactory;
import :Prerequisites;
import :SharedPtr;
import :SubMesh;
import :Vector;
import :VertexIndexData;

import <string>;

namespace Ogre {
    //---------------------------------------------------------------------
    auto PrefabFactory::createPrefab(Mesh* mesh) -> bool
    {
        std::string_view resourceName = mesh->getName();

        if(resourceName == "Prefab_Plane")
        {
            createPlane(mesh);
            return true;
        }
        else if(resourceName == "Prefab_Cube")
        {
            createCube(mesh);
            return true;
        }
        else if(resourceName == "Prefab_Sphere")
        {
            createSphere(mesh);
            return true;
        }

        return false;
    }
    //---------------------------------------------------------------------
    void PrefabFactory::createPlane(Mesh* mesh)
    {
        //! [manual_plane_geometry]
        float vertices[32] = {
            -100, -100, 0,  // pos
            0,0,1,          // normal
            0,1,            // texcoord
            100, -100, 0,
            0,0,1,
            1,1,
            100,  100, 0,
            0,0,1,
            1,0,
            -100,  100, 0 ,
            0,0,1,
            0,0 
        };

        uint16 faces[6] = {0,1,2,
                           0,2,3 };
        //! [manual_plane_geometry]

        //! [vertex_data]
        mesh->sharedVertexData = new VertexData();
        mesh->sharedVertexData->vertexCount = 4;
        VertexDeclaration* decl = mesh->sharedVertexData->vertexDeclaration;
        VertexBufferBinding* bind = mesh->sharedVertexData->vertexBufferBinding;
        //! [vertex_data]

        //! [vertex_decl]
        size_t offset = 0;
        offset += decl->addElement(0, offset, VertexElementType::FLOAT3, VertexElementSemantic::POSITION).getSize();
        offset += decl->addElement(0, offset, VertexElementType::FLOAT3, VertexElementSemantic::NORMAL).getSize();
        offset += decl->addElement(0, offset, VertexElementType::FLOAT2, VertexElementSemantic::TEXTURE_COORDINATES, 0).getSize();
        //! [vertex_decl]

        //! [vertex_buffer]
        HardwareVertexBufferPtr vbuf =
            HardwareBufferManager::getSingleton().createVertexBuffer(offset, 4, HardwareBufferUsage::GPU_ONLY);
        vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);
        bind->setBinding(0, vbuf);

        HardwareIndexBufferPtr ibuf =
            HardwareBufferManager::getSingleton().createIndexBuffer(HardwareIndexBuffer::IndexType::_16BIT, 6, HardwareBufferUsage::GPU_ONLY);
        ibuf->writeData(0, ibuf->getSizeInBytes(), faces, true);
        //! [vertex_buffer]

        //! [sub_mesh]
        SubMesh* sub = mesh->createSubMesh();
        sub->useSharedVertices = true;
        sub->indexData->indexBuffer = ibuf;
        sub->indexData->indexCount = 6;
        sub->indexData->indexStart = 0;
        //! [sub_mesh]

        using enum AxisAlignedBox::Extent;
        mesh->_setBounds(AxisAlignedBox{Finite, Vector3{-100,-100,0}, Vector3{100,100,0}}, true);
    }
    //---------------------------------------------------------------------
    void PrefabFactory::createCube(Mesh* mesh)
    {
        SubMesh* sub = mesh->createSubMesh();

        const int NUM_VERTICES = 4 * 6; // 4 vertices per side * 6 sides
        const int NUM_ENTRIES_PER_VERTEX = 8;
        const int NUM_VERTEX_ENTRIES = NUM_VERTICES * NUM_ENTRIES_PER_VERTEX;
        const int NUM_INDICES = 3 * 2 * 6; // 3 indices per face * 2 faces per side * 6 sides

        const float CUBE_SIZE = 100.0f;
        const float CUBE_HALF_SIZE = CUBE_SIZE / 2.0f;

        // Create 4 vertices per side instead of 6 that are shared for the whole cube.
        // The reason for this is with only 6 vertices the normals will look bad
        // since each vertex can "point" in a different direction depending on the face it is included in.
        float vertices[NUM_VERTEX_ENTRIES] = {
            // front side
            -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, CUBE_HALF_SIZE,   // pos
            0,0,1,  // normal
            0,1,    // texcoord
            CUBE_HALF_SIZE, -CUBE_HALF_SIZE, CUBE_HALF_SIZE,
            0,0,1,
            1,1,
            CUBE_HALF_SIZE,  CUBE_HALF_SIZE, CUBE_HALF_SIZE,
            0,0,1,
            1,0,
            -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, CUBE_HALF_SIZE ,
            0,0,1,
            0,0,

            // back side
            CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            0,0,-1,
            0,1,
            -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            0,0,-1,
            1,1,
            -CUBE_HALF_SIZE, CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            0,0,-1,
            1,0,
            CUBE_HALF_SIZE, CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            0,0,-1,
            0,0,

            // left side
            -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            -1,0,0,
            0,1,
            -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, CUBE_HALF_SIZE,
            -1,0,0,
            1,1,
            -CUBE_HALF_SIZE, CUBE_HALF_SIZE, CUBE_HALF_SIZE,
            -1,0,0,
            1,0,
            -CUBE_HALF_SIZE, CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            -1,0,0,
            0,0, 

            // right side
            CUBE_HALF_SIZE, -CUBE_HALF_SIZE, CUBE_HALF_SIZE,
            1,0,0,
            0,1,
            CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            1,0,0,
            1,1,
            CUBE_HALF_SIZE, CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            1,0,0,
            1,0,
            CUBE_HALF_SIZE, CUBE_HALF_SIZE, CUBE_HALF_SIZE,
            1,0,0,
            0,0,

            // up side
            -CUBE_HALF_SIZE, CUBE_HALF_SIZE, CUBE_HALF_SIZE,
            0,1,0,
            0,1,
            CUBE_HALF_SIZE, CUBE_HALF_SIZE, CUBE_HALF_SIZE,
            0,1,0,
            1,1,
            CUBE_HALF_SIZE, CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            0,1,0,
            1,0,
            -CUBE_HALF_SIZE, CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            0,1,0,
            0,0,

            // down side
            -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            0,-1,0,
            0,1,
            CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE,
            0,-1,0,
            1,1,
            CUBE_HALF_SIZE, -CUBE_HALF_SIZE, CUBE_HALF_SIZE,
            0,-1,0,
            1,0,
            -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, CUBE_HALF_SIZE,
            0,-1,0,
            0,0 
        };

        mesh->sharedVertexData = new VertexData();
        mesh->sharedVertexData->vertexCount = NUM_VERTICES;
        VertexDeclaration* decl = mesh->sharedVertexData->vertexDeclaration;
        VertexBufferBinding* bind = mesh->sharedVertexData->vertexBufferBinding;

        size_t offset = 0;
        offset += decl->addElement(0, offset, VertexElementType::FLOAT3, VertexElementSemantic::POSITION).getSize();
        offset += decl->addElement(0, offset, VertexElementType::FLOAT3, VertexElementSemantic::NORMAL).getSize();
        offset += decl->addElement(0, offset, VertexElementType::FLOAT2, VertexElementSemantic::TEXTURE_COORDINATES, 0).getSize();

        HardwareVertexBufferSharedPtr vbuf = 
            HardwareBufferManager::getSingleton().createVertexBuffer(
            offset, NUM_VERTICES, HardwareBuffer::STATIC_WRITE_ONLY);
        bind->setBinding(0, vbuf);

        vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

        sub->useSharedVertices = true;
        HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
            createIndexBuffer(
            HardwareIndexBuffer::IndexType::_16BIT, 
            NUM_INDICES,
            HardwareBuffer::STATIC_WRITE_ONLY);

        unsigned short faces[NUM_INDICES] = {
            // front
            0,1,2,
            0,2,3,

            // back
            4,5,6,
            4,6,7,

            // left
            8,9,10,
            8,10,11,

            // right
            12,13,14,
            12,14,15,

            // up
            16,17,18,
            16,18,19,

            // down
            20,21,22,
            20,22,23
        };

        sub->indexData->indexBuffer = ibuf;
        sub->indexData->indexCount = NUM_INDICES;
        sub->indexData->indexStart = 0;
        ibuf->writeData(0, ibuf->getSizeInBytes(), faces, true);

        using enum AxisAlignedBox::Extent;
        mesh->_setBounds(AxisAlignedBox{Finite, Vector3{-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE},
            Vector3{CUBE_HALF_SIZE, CUBE_HALF_SIZE, CUBE_HALF_SIZE}}, true);
    }
    //---------------------------------------------------------------------
    void PrefabFactory::createSphere(Mesh* mesh)
    {
        // sphere creation code taken from the DeferredShading sample, originally from the wiki
        SubMesh *pSphereVertex = mesh->createSubMesh();

        const int NUM_SEGMENTS = 16;
        const int NUM_RINGS = 16;
        const Real SPHERE_RADIUS = 50.0;

        mesh->sharedVertexData = new VertexData();
        VertexData* vertexData = mesh->sharedVertexData;

        // define the vertex format
        VertexDeclaration* vertexDecl = vertexData->vertexDeclaration;
        size_t currOffset = 0;
        currOffset += vertexDecl->addElement(0, currOffset, VertexElementType::FLOAT3, VertexElementSemantic::POSITION).getSize();
        currOffset += vertexDecl->addElement(0, currOffset, VertexElementType::FLOAT3, VertexElementSemantic::NORMAL).getSize();
        currOffset += vertexDecl->addElement(0, currOffset, VertexElementType::FLOAT2, VertexElementSemantic::TEXTURE_COORDINATES, 0).getSize();

        // allocate the vertex buffer
        vertexData->vertexCount = (NUM_RINGS + 1) * (NUM_SEGMENTS+1);
        HardwareVertexBufferSharedPtr vBuf = HardwareBufferManager::getSingleton().createVertexBuffer(vertexDecl->getVertexSize(0), vertexData->vertexCount, HardwareBuffer::STATIC_WRITE_ONLY, false);
        VertexBufferBinding* binding = vertexData->vertexBufferBinding;
        binding->setBinding(0, vBuf);
        HardwareBufferLockGuard vBufLock(vBuf, HardwareBuffer::LockOptions::DISCARD);
        auto* pVertex = static_cast<float*>(vBufLock.pData);

        // allocate index buffer
        pSphereVertex->indexData->indexCount = 6 * NUM_RINGS * (NUM_SEGMENTS + 1);
        pSphereVertex->indexData->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(HardwareIndexBuffer::IndexType::_16BIT, pSphereVertex->indexData->indexCount, HardwareBuffer::STATIC_WRITE_ONLY, false);
        HardwareIndexBufferSharedPtr iBuf = pSphereVertex->indexData->indexBuffer;
        HardwareBufferLockGuard iBufLock(iBuf, HardwareBuffer::LockOptions::DISCARD);
        auto* pIndices = static_cast<unsigned short*>(iBufLock.pData);

        float fDeltaRingAngle = (Math::PI / NUM_RINGS);
        float fDeltaSegAngle = (2 * Math::PI / NUM_SEGMENTS);
        unsigned short wVerticeIndex = 0 ;

        // Generate the group of rings for the sphere
        for( int ring = 0; ring <= NUM_RINGS; ring++ ) {
            float r0 = SPHERE_RADIUS * sinf (ring * fDeltaRingAngle);
            float y0 = SPHERE_RADIUS * cosf (ring * fDeltaRingAngle);

            // Generate the group of segments for the current ring
            for(int seg = 0; seg <= NUM_SEGMENTS; seg++) {
                float x0 = r0 * sinf(seg * fDeltaSegAngle);
                float z0 = r0 * cosf(seg * fDeltaSegAngle);

                // Add one vertex to the strip which makes up the sphere
                *pVertex++ = x0;
                *pVertex++ = y0;
                *pVertex++ = z0;

                Vector3 vNormal = Vector3{x0, y0, z0}.normalisedCopy();
                *pVertex++ = vNormal.x;
                *pVertex++ = vNormal.y;
                *pVertex++ = vNormal.z;

                *pVertex++ = (float) seg / (float) NUM_SEGMENTS;
                *pVertex++ = (float) ring / (float) NUM_RINGS;

                if (ring != NUM_RINGS) {
                    // each vertex (except the last) has six indicies pointing to it
                    *pIndices++ = wVerticeIndex + NUM_SEGMENTS + 1;
                    *pIndices++ = wVerticeIndex;               
                    *pIndices++ = wVerticeIndex + NUM_SEGMENTS;
                    *pIndices++ = wVerticeIndex + NUM_SEGMENTS + 1;
                    *pIndices++ = wVerticeIndex + 1;
                    *pIndices++ = wVerticeIndex;
                    wVerticeIndex ++;
                }
            }; // end for seg
        } // end for ring

        // Generate face list
        pSphereVertex->useSharedVertices = true;

        // the original code was missing this line:
        using enum AxisAlignedBox::Extent;
        mesh->_setBounds( AxisAlignedBox{ Finite, Vector3{-SPHERE_RADIUS, -SPHERE_RADIUS, -SPHERE_RADIUS},
            Vector3{SPHERE_RADIUS, SPHERE_RADIUS, SPHERE_RADIUS} }, false );

        mesh->_setBoundingSphereRadius(SPHERE_RADIUS);
    }
}
