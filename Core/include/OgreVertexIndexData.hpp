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

export module Ogre.Core:VertexIndexData;

export import :HardwareBuffer;
export import :HardwareVertexBuffer;
export import :MemoryAllocatorConfig;
export import :Platform;
export import :Prerequisites;
export import :SharedPtr;

export import <algorithm>;
export import <vector>;

export
namespace Ogre {
class HardwareBufferManagerBase;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */

    /// Define a list of usage flags
    using BufferUsageList = std::vector<HardwareBuffer::Usage>;

    /** collects together all the vertex-related information used to render geometry.
     *
     * The RenderOperation requires a pointer to a VertexData object, and it is also used in Mesh and
     * SubMesh to store the vertex positions, normals, texture coordinates etc. VertexData can either be
     * used alone (in order to render unindexed geometry, where the stream of vertices defines the
     * triangles), or in combination with IndexData where the triangles are defined by indexes which refer
     * to the entries in VertexData.  It’s worth noting that you don’t necessarily have to use VertexData to
     * store your applications geometry; all that is required is that you can build a VertexData structure
     * when it comes to rendering. This is pretty easy since all of VertexData’s members are pointers, so
     * you could maintain your vertex buffers and declarations in alternative structures if you like, so
     * long as you can convert them for rendering.
     */
    class VertexData : public VertexDataAlloc
    {
    public:
        /// Protected copy constructor, to prevent misuse
        VertexData(const VertexData& rhs) = delete; /* do nothing, should not use */
        /// Protected operator=, to prevent misuse
        auto operator=(const VertexData& rhs) -> VertexData& = delete; /* do not use */

    private:
        HardwareBufferManagerBase* mMgr;
    public:
        /** Constructor.
        @note 
            This constructor creates the VertexDeclaration and VertexBufferBinding
            automatically, and arranges for their deletion afterwards.
        @param mgr Optional HardwareBufferManager from which to create resources
        */
        VertexData(HardwareBufferManagerBase* mgr = nullptr);
        /** Constructor.
        @note 
        This constructor receives the VertexDeclaration and VertexBufferBinding
        from the caller, and as such does not arrange for their deletion afterwards, 
        the caller remains responsible for that.
        @param dcl The VertexDeclaration to use
        @param bind The VertexBufferBinding to use
        */
        VertexData(VertexDeclaration* dcl, VertexBufferBinding* bind);
        ~VertexData();

        /** Declaration of the the format of the vertex input.
        Note that this is created for you on construction.
        */
        VertexDeclaration* vertexDeclaration;
        /** Defines which vertex buffers are bound to which sources.
        Note that this is created for you on construction.
        */
        VertexBufferBinding* vertexBufferBinding;
        /// Whether this class should delete the declaration and binding
        bool mDeleteDclBinding;
        /// The position in the bound buffers to start reading vertex data from. This allows you to use a single buffer for many different renderables.
        size_t vertexStart;
        /// The number of vertices to process in this particular rendering group
        size_t vertexCount;


        /// Struct used to hold hardware morph / pose vertex data information
        struct HardwareAnimationData
        {
            unsigned short targetBufferIndex;
            Real parametric;
        };
        using HardwareAnimationDataList = std::vector<HardwareAnimationData>;
        /// VertexElements used for hardware morph / pose animation
        HardwareAnimationDataList hwAnimationDataList;
        /// Number of hardware animation data items used
        size_t hwAnimDataItemsUsed;
        
        /** Clones this vertex data, potentially including replicating any vertex buffers.
        @param copyData Whether to create new vertex buffers too or just reference the existing ones
        @param mgr If supplied, the buffer manager through which copies should be made
        @remarks The caller is expected to delete the returned pointer when ready
        */
        [[nodiscard]]
        auto clone(bool copyData = true, HardwareBufferManagerBase* mgr = nullptr) const -> VertexData*;

        /** Modifies the vertex data to be suitable for use for rendering shadow geometry as in @cite mcguire2003fast

            Preparing vertex data to generate a shadow volume involves firstly ensuring that the 
            vertex buffer containing the positions is a standalone vertex buffer,
            with no other components in it. This method will therefore break apart any existing
            vertex buffers if position is sharing a vertex buffer. 
            Secondly, it will double the size of this vertex buffer so that there are 2 copies of 
            the position data for the mesh. The first half is used for the original, and the second 
            half is used for the 'extruded' version. The vertex count used to render will remain 
            the same though, so as not to add any overhead to regular rendering of the object.
            Both copies of the position are required in one buffer because shadow volumes stretch 
            from the original mesh to the extruded version. 

            It's important to appreciate that this method can fundamentally change the structure of your
            vertex buffers, although in reality they will be new buffers. As it happens, if other 
            objects are using the original buffers then they will be unaffected because the reference
            counting will keep them intact. However, if you have made any assumptions about the 
            structure of the vertex data in the buffers of this object, you may have to rethink them.

            Because shadow volumes are rendered in turn, no additional
            index buffer space is allocated by this method, a shared index buffer allocated by the
            shadow rendering algorithm is used for addressing this extended vertex buffer.
        */
        void prepareForShadowVolume();

        /** Additional shadow volume vertex buffer storage. 
        @remarks
            This additional buffer is only used where we have prepared this VertexData for
            use in shadow volume construction, and where the current render system supports
            vertex programs. This buffer contains the 'w' vertex position component which will
            be used by that program to differentiate between extruded and non-extruded vertices.
            This 'w' component cannot be included in the original position buffer because
            DirectX does not allow 4-component positions in the fixed-function pipeline, and the original
            position buffer must still be usable for fixed-function rendering.
        @par    
            Note that we don't store any vertex declaration or vertex buffer binding here because this
            can be reused in the shadow algorithm.
        */
        HardwareVertexBufferSharedPtr hardwareShadowVolWBuffer;


        /** Reorganises the data in the vertex buffers according to the 
            new vertex declaration passed in. Note that new vertex buffers
            are created and written to, so if the buffers being referenced 
            by this vertex data object are also used by others, then the 
            original buffers will not be damaged by this operation.
            Once this operation has completed, the new declaration 
            passed in will overwrite the current one.
        @param newDeclaration The vertex declaration which will be used
            for the reorganised buffer state. Note that the new declaration
            must not include any elements which do not already exist in the 
            current declaration; you can drop elements by 
            excluding them from the declaration if you wish, however.
        @param bufferUsage Vector of usage flags which indicate the usage options
            for each new vertex buffer created. The indexes of the entries must correspond
            to the buffer binding values referenced in the declaration.
        @param mgr Optional pointer to the manager to use to create new declarations
            and buffers etc. If not supplied, the HardwareBufferManager singleton will be used
        */
        void reorganiseBuffers(VertexDeclaration* newDeclaration, const BufferUsageList& bufferUsage, 
            HardwareBufferManagerBase* mgr = nullptr);

        /** Reorganises the data in the vertex buffers according to the 
            new vertex declaration passed in. Note that new vertex buffers
            are created and written to, so if the buffers being referenced 
            by this vertex data object are also used by others, then the 
            original buffers will not be damaged by this operation.
            Once this operation has completed, the new declaration 
            passed in will overwrite the current one.
            This version of the method derives the buffer usages from the existing
            buffers, by using the 'most flexible' usage from the equivalent sources.
        @param newDeclaration The vertex declaration which will be used
            for the reorganised buffer state. Note that the new delcaration
            must not include any elements which do not already exist in the 
            current declaration; you can drop elements by 
            excluding them from the declaration if you wish, however.
        @param mgr Optional pointer to the manager to use to create new declarations
            and buffers etc. If not supplied, the HardwareBufferManager singleton will be used
        */
        void reorganiseBuffers(VertexDeclaration* newDeclaration, HardwareBufferManagerBase* mgr = nullptr);

        /** Remove any gaps in the vertex buffer bindings.
        @remarks
            This is useful if you've removed elements and buffers from this vertex
            data and want to remove any gaps in the vertex buffer bindings. This
            method is mainly useful when reorganising vertex data manually.
        @note
            This will cause binding index of the elements in the vertex declaration
            to be altered to new binding index.
        */
        void closeGapsInBindings();

        /** Remove all vertex buffers that never used by the vertex declaration.
        @remarks
            This is useful if you've removed elements from the vertex declaration
            and want to unreference buffers that never used any more. This method
            is mainly useful when reorganising vertex data manually.
        @note
            This also remove any gaps in the vertex buffer bindings.
        */
        void removeUnusedBuffers();

        /** Convert all packed colour values (VertexElementType::COLOUR_*) in buffers used to
            another type.
        @param srcType The source colour type to assume if the ambiguous VertexElementType::COLOUR
            is encountered.
        @param destType The destination colour type, must be VertexElementType::COLOUR_ABGR or
            VertexElementType::COLOUR_ARGB.
        */
        void convertPackedColour(VertexElementType srcType, VertexElementType destType);


        /** Allocate elements to serve a holder of morph / pose target data 
            for hardware morphing / pose blending.
        @remarks
            This method will allocate the given number of 3D texture coordinate 
            sets for use as a morph target or target pose offset (3D position).
            These elements will be saved in hwAnimationDataList.
            It will also assume that the source of these new elements will be new
            buffers which are not bound at this time, so will start the sources to 
            1 higher than the current highest binding source. The caller is
            expected to bind these new buffers when appropriate. For morph animation
            the original position buffer will be the 'from' keyframe data, whilst
            for pose animation it will be the original vertex data.
            If normals are animated, then twice the number of 3D texture coordinates are required
         @return The number of sets that were supported
        */
        auto allocateHardwareAnimationElements(ushort count, bool animateNormals) -> ushort;



    };

    /** Summary class collecting together index data source information. */
    class IndexData : public IndexDataAlloc
    {
    public:
        /// Protected copy constructor, to prevent misuse
        IndexData(const IndexData& rhs) = delete; /* do nothing, should not use */
        /// Protected operator=, to prevent misuse
        auto operator=(const IndexData& rhs) -> IndexData& = delete; /* do not use */
        IndexData();
        ~IndexData();
        /// Pointer to the HardwareIndexBuffer to use, must be specified if useIndexes = true
        HardwareIndexBufferSharedPtr indexBuffer;

        /// Index in the buffer to start from for this operation
        size_t indexStart;

        /// The number of indexes to use from the buffer
        size_t indexCount;

        /** Clones this index data, potentially including replicating the index buffer.
        @param copyData Whether to create new buffers too or just reference the existing ones
        @param mgr If supplied, the buffer manager through which copies should be made
        @remarks The caller is expected to delete the returned pointer when finished
        */
        auto clone(bool copyData = true, HardwareBufferManagerBase* mgr = nullptr) const -> IndexData*;

        /** Re-order the indexes in this index data structure to be more
            vertex cache friendly; that is to re-use the same vertices as close
            together as possible. 
        @remarks
            Can only be used for index data which consists of triangle lists.
            It would in fact be pointless to use it on triangle strips or fans
            in any case.
        */
        void optimiseVertexCacheTriList();
    
    };

    /** Vertex cache profiler.
    @remarks
        Utility class for evaluating the effectiveness of the use of the vertex
        cache by a given index buffer.
    */
    class VertexCacheProfiler : public BufferAlloc
    {
        public:
            VertexCacheProfiler(unsigned int cachesize = 16)
                : size ( cachesize ) 
            {
                cache = new uint32[size];
            }

            ~VertexCacheProfiler()
            {
                delete[] cache;
            }

            void profile(const HardwareIndexBufferSharedPtr& indexBuffer);
            void reset() { hit = 0; miss = 0; tail = 0; buffersize = 0; }
            void flush() { tail = 0; buffersize = 0; }

            auto getHits() noexcept -> unsigned int { return hit; }
            auto getMisses() noexcept -> unsigned int { return miss; }
            auto getSize() noexcept -> unsigned int { return size; }
        private:
            unsigned int size;
            uint32 *cache;

            unsigned int tail{0}, buffersize{0};
            unsigned int hit{0}, miss{0};

            auto inCache(unsigned int index) -> bool;
    };
    /** @} */
    /** @} */
}
