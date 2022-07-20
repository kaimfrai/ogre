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
export module Ogre.Core:RenderOperation;

export import :Prerequisites;
export import :VertexIndexData;

export
namespace Ogre {


    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup RenderSystem
     *  @{
     */
    /** 'New' rendering operation using vertex buffers. */
    class RenderOperation {
    public:
        /// The rendering operation type to perform
        enum class OperationType : uint8 {
            /// A list of points, 1 vertex per point
            POINT_LIST = 1,
            /// A list of lines, 2 vertices per line
            LINE_LIST = 2,
            /// A strip of connected lines, 1 vertex per line plus 1 start vertex
            LINE_STRIP = 3,
            /// A list of triangles, 3 vertices per triangle
            TRIANGLE_LIST = 4,
            /// A strip of triangles, 3 vertices for the first triangle, and 1 per triangle after that
            TRIANGLE_STRIP = 5,
            /// A fan of triangles, 3 vertices for the first triangle, and 1 per triangle after that
            TRIANGLE_FAN = 6,
            /// Patch control point operations, used with tessellation stages
            PATCH_1_CONTROL_POINT    = 7,
            PATCH_2_CONTROL_POINT    = 8,
            PATCH_3_CONTROL_POINT    = 9,
            PATCH_4_CONTROL_POINT    = 10,
            PATCH_5_CONTROL_POINT    = 11,
            PATCH_6_CONTROL_POINT    = 12,
            PATCH_7_CONTROL_POINT    = 13,
            PATCH_8_CONTROL_POINT    = 14,
            PATCH_9_CONTROL_POINT    = 15,
            PATCH_10_CONTROL_POINT   = 16,
            PATCH_11_CONTROL_POINT   = 17,
            PATCH_12_CONTROL_POINT   = 18,
            PATCH_13_CONTROL_POINT   = 19,
            PATCH_14_CONTROL_POINT   = 20,
            PATCH_15_CONTROL_POINT   = 21,
            PATCH_16_CONTROL_POINT   = 22,
            PATCH_17_CONTROL_POINT   = 23,
            PATCH_18_CONTROL_POINT   = 24,
            PATCH_19_CONTROL_POINT   = 25,
            PATCH_20_CONTROL_POINT   = 26,
            PATCH_21_CONTROL_POINT   = 27,
            PATCH_22_CONTROL_POINT   = 28,
            PATCH_23_CONTROL_POINT   = 29,
            PATCH_24_CONTROL_POINT   = 30,
            PATCH_25_CONTROL_POINT   = 31,
            PATCH_26_CONTROL_POINT   = 32,
            PATCH_27_CONTROL_POINT   = 33,
            PATCH_28_CONTROL_POINT   = 34,
            PATCH_29_CONTROL_POINT   = 35,
            PATCH_30_CONTROL_POINT   = 36,
            PATCH_31_CONTROL_POINT   = 37,
            PATCH_32_CONTROL_POINT   = 38,
            // max valid base  = (1 << 6) - 1
            /// Mark that the index buffer contains adjacency information
            DETAIL_ADJACENCY_BIT     = 1 << 6,
            /// like POINT_LIST but with adjacency information for the geometry shader
            LINE_LIST_ADJ            = LINE_LIST | DETAIL_ADJACENCY_BIT,
            /// like LINE_STRIP but with adjacency information for the geometry shader
            LINE_STRIP_ADJ           = LINE_STRIP | DETAIL_ADJACENCY_BIT,
            /// like TRIANGLE_LIST but with adjacency information for the geometry shader
            TRIANGLE_LIST_ADJ        = TRIANGLE_LIST | DETAIL_ADJACENCY_BIT,
            /// like TRIANGLE_STRIP but with adjacency information for the geometry shader
            TRIANGLE_STRIP_ADJ       = TRIANGLE_STRIP | DETAIL_ADJACENCY_BIT,
        };

        friend auto constexpr operator bitor (OperationType left, OperationType right) -> OperationType
        {
            return static_cast<OperationType>
            (   std::to_underlying(left)
            bitor
                std::to_underlying(right)
            );
        }

        friend auto constexpr operator |= (OperationType& left, OperationType right) -> OperationType&
        {
            return left = left bitor right;
        }

        /// Vertex source data
        VertexData *vertexData{nullptr};

        /// Index data - only valid if useIndexes is true
        IndexData *indexData{nullptr};
        /// Debug pointer back to renderable which created this
        const Renderable* srcRenderable{nullptr};

        /// The number of instances for the render operation - this option is supported
        /// in only a part of the render systems.
        uint32 numberOfInstances{1};

        /// The type of operation to perform
        OperationType operationType{OperationType::TRIANGLE_LIST};

        /** Specifies whether to use indexes to determine the vertices to use as input. If false, the vertices are
            simply read in sequence to define the primitives. If true, indexes are used instead to identify vertices
            anywhere in the buffer, and allowing vertices to be used more than once.
            If true, then the indexBuffer, indexStart and numIndexes properties must be valid. */
        bool useIndexes{true};

        /** A flag to indicate that it is possible for this operation to use a global
            vertex instance buffer if available.*/
        bool useGlobalInstancingVertexBufferIsAvailable{true};

        RenderOperation()
            
              
        = default;
    };
    /** @} */
    /** @} */
}
