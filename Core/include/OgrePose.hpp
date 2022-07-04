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
#ifndef OGRE_CORE_POSE_H
#define OGRE_CORE_POSE_H

#include <cstddef>
#include <map>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreVector.hpp"

namespace Ogre {

    template <typename T> class MapIterator;
    template <typename T> class ConstMapIterator;
class VertexData;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Animation
    *  @{
    */
    /** A pose is a linked set of vertex offsets applying to one set of vertex
        data. 
    @remarks
        The target index referred to by the pose has a meaning set by the user
        of this class; but for example when used by Mesh it refers to either the
        Mesh shared geometry (0) or a SubMesh dedicated geometry (1+).
        Pose instances can be referred to by keyframes in VertexAnimationTrack in
        order to animate based on blending poses together.
    */
    class Pose : public AnimationAlloc
    {
    public:
        /** Constructor
            @param target The target vertexdata index (0 for shared, 1+ for 
                dedicated at the submesh index + 1)
            @param name Optional name
        */
        Pose(ushort target, String  name = BLANKSTRING);
        /// Return the name of the pose (may be blank)
        auto getName() const noexcept -> const String& { return mName; }
        /// Return the target geometry index of the pose
        auto getTarget() const noexcept -> ushort { return mTarget; }
        /// A collection of vertex offsets based on the vertex index
        using VertexOffsetMap = std::map<size_t, Vector3>;
        /// An iterator over the vertex offsets
        using VertexOffsetIterator = MapIterator<VertexOffsetMap>;
        /// An iterator over the vertex offsets
        using ConstVertexOffsetIterator = ConstMapIterator<VertexOffsetMap>;
        /// A collection of normals based on the vertex index
        using NormalsMap = std::map<size_t, Vector3>;
        /// An iterator over the vertex offsets
        using NormalsIterator = MapIterator<NormalsMap>;
        /// An iterator over the vertex offsets
        using ConstNormalsIterator = ConstMapIterator<NormalsMap>;
        /// Return whether the pose vertices include normals
        auto getIncludesNormals() const noexcept -> bool { return !mNormalsMap.empty(); }

        /** Adds an offset to a vertex for this pose. 
        @param index The vertex index
        @param offset The position offset for this pose
        */
        void addVertex(size_t index, const Vector3& offset);

        /** Adds an offset to a vertex and a new normal for this pose. 
        @param index The vertex index
        @param offset The position offset for this pose
        @param normal The new vertex normal
        */
        void addVertex(size_t index, const Vector3& offset, const Vector3& normal);

        /** Remove a vertex offset. */
        void removeVertex(size_t index);

        /** Clear all vertices. */
        void clearVertices();

        /** Gets a const reference to the vertex offsets. */
        auto getVertexOffsets() const noexcept -> const VertexOffsetMap& { return mVertexOffsetMap; }

        /** Gets a const reference to the vertex normals */
        auto getNormals() const noexcept -> const NormalsMap& { return mNormalsMap; }

        /** writable access to the vertex offsets for offline processing
         *
         * @attention does not invalidate the vertexbuffer
         */
        auto _getVertexOffsets() noexcept -> VertexOffsetMap& { return mVertexOffsetMap; }

        /** writable access to the vertex normals for offline processing
         *
         * @attention does not invalidate the vertexbuffer
         */
        auto _getNormals() noexcept -> NormalsMap& { return mNormalsMap; }

        /** Get a hardware vertex buffer version of the vertex offsets. */
        auto _getHardwareVertexBuffer(const VertexData* origData) const -> const HardwareVertexBufferSharedPtr&;

        /** Clone this pose and create another one configured exactly the same
            way (only really useful for cloning holders of this class).
        */
        [[nodiscard]]
        auto clone() const -> Pose*;
    private:
        /// Target geometry index
        ushort mTarget;
        /// Optional name
        String mName;
        /// Primary storage, sparse vertex use
        VertexOffsetMap mVertexOffsetMap;
        /// Primary storage, sparse vertex use
        NormalsMap mNormalsMap;
        /// Derived hardware buffer, covers all vertices
        mutable HardwareVertexBufferSharedPtr mBuffer;
    };
    using PoseList = std::vector<Pose *>;

    /** @} */
    /** @} */

}

#endif
