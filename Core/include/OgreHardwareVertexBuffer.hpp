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

export module Ogre.Core:HardwareVertexBuffer;

// Precompiler options
export import :ColourValue;
export import :HardwareBuffer;
export import :MemoryAllocatorConfig;
export import :Platform;
export import :Prerequisites;
export import :SharedPtr;

export import <list>;
export import <map>;
export import <utility>;

export
namespace Ogre {
    class HardwareBufferManagerBase;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */
    /** Specialisation of HardwareBuffer for a vertex buffer. */
    class HardwareVertexBuffer : public HardwareBuffer
    {
            bool mIsInstanceData;
            HardwareBufferManagerBase* mMgr;
            size_t mNumVertices;
            size_t mVertexSize;
            size_t mInstanceDataStepRate;           
            /// Checks if vertex instance data is supported by the render system
            virtual auto checkIfVertexInstanceDataIsSupported() -> bool;

        public:
            /// Should be called by HardwareBufferManager
            HardwareVertexBuffer(HardwareBufferManagerBase* mgr, size_t vertexSize, size_t numVertices,
                                 Usage usage, bool useSystemMemory, bool useShadowBuffer);
            HardwareVertexBuffer(HardwareBufferManagerBase* mgr, size_t vertexSize, size_t numVertices,
                                 HardwareBuffer* delegate);
            ~HardwareVertexBuffer() override;
            /// Return the manager of this buffer, if any
            [[nodiscard]] auto getManager() const noexcept -> HardwareBufferManagerBase* { return mMgr; }
            /// Gets the size in bytes of a single vertex in this buffer
            [[nodiscard]] auto getVertexSize() const noexcept -> size_t { return mVertexSize; }
            /// Get the number of vertices in this buffer
            [[nodiscard]] auto getNumVertices() const noexcept -> size_t { return mNumVertices; }
            /// Get if this vertex buffer is an "instance data" buffer (per instance)
            [[nodiscard]] auto isInstanceData() const noexcept -> bool { return mIsInstanceData; }
            /// Set if this vertex buffer is an "instance data" buffer (per instance)
            void setIsInstanceData(const bool val);
            /// Get the number of instances to draw using the same per-instance data before advancing in the buffer by one element.
            [[nodiscard]] auto getInstanceDataStepRate() const -> size_t;
            /// Set the number of instances to draw using the same per-instance data before advancing in the buffer by one element.
            void setInstanceDataStepRate(const size_t val);


            // NB subclasses should override lock, unlock, readData, writeData

    };

    /// Vertex element semantics, used to identify the meaning of vertex buffer contents
    enum class VertexElementSemantic {
        /// Position, typically VertexElementType::FLOAT3
        POSITION = 1,
        /// Blending weights
        BLEND_WEIGHTS = 2,
        /// Blending indices
        BLEND_INDICES = 3,
        /// Normal, typically VertexElementType::FLOAT3
        NORMAL = 4,
        /// Colour, typically VertexElementType::UBYTE4
        COLOUR = 5,
        /// Secondary colour. Generally free for custom data. Means specular with OpenGL FFP.
        COLOUR2 = 6,
        /// Texture coordinates, typically VertexElementType::FLOAT2
        TEXTURE_COORDINATES = 7,
        /// Binormal (Y axis if normal is Z)
        BINORMAL = 8,
        /// Tangent (X axis if normal is Z)
        TANGENT = 9,
        /// The  number of VertexElementSemantic elements (note - the first value POSITION is 1)
        COUNT = 9,
        /// @deprecated use COLOUR
        DIFFUSE = COLOUR,
        /// @deprecated use COLOUR2
        SPECULAR = COLOUR2
    };

    /**
     * Vertex element type, used to identify the base types of the vertex contents
     *
     * @note VertexElementType::SHORT1, VertexElementType::SHORT3, VertexElementType::USHORT1 and VertexElementType::USHORT3 should never be used
     * because they aren't supported on any known hardware - they are unaligned as their size
     * is not a multiple of 4 bytes. Therefore drivers usually must add padding on upload.
     */
    enum class VertexElementType
    {
        FLOAT1 = 0,
        FLOAT2 = 1,
        FLOAT3 = 2,
        FLOAT4 = 3,

        SHORT1 = 5,  ///< @deprecated (see #VertexElementType note)
        SHORT2 = 6,
        SHORT3 = 7,  ///< @deprecated (see #VertexElementType note)
        SHORT4 = 8,
        UBYTE4 = 9,
        _DETAIL_SWAP_RB = 10,

        // the following are not universally supported on all hardware:
        DOUBLE1 = 12,
        DOUBLE2 = 13,
        DOUBLE3 = 14,
        DOUBLE4 = 15,
        USHORT1 = 16,  ///< @deprecated (see #VertexElementType note)
        USHORT2 = 17,
        USHORT3 = 18,  ///< @deprecated (see #VertexElementType note)
        USHORT4 = 19,
        INT1 = 20,
        INT2 = 21,
        INT3 = 22,
        INT4 = 23,
        UINT1 = 24,
        UINT2 = 25,
        UINT3 = 26,
        UINT4 = 27,
        BYTE4 = 28,  /// signed bytes
        BYTE4_NORM = 29,   /// signed bytes (normalized to -1..1)
        UBYTE4_NORM = 30,  /// unsigned bytes (normalized to 0..1)
        SHORT2_NORM = 31,  /// signed shorts (normalized to -1..1)
        SHORT4_NORM = 32,
        USHORT2_NORM = 33, /// unsigned shorts (normalized to 0..1)
        USHORT4_NORM = 34,
        COLOUR = UBYTE4_NORM,  ///< @deprecated use UBYTE4_NORM
        COLOUR_ARGB = UBYTE4_NORM,  ///< @deprecated use UBYTE4_NORM
        COLOUR_ABGR = UBYTE4_NORM,  ///< @deprecated use VertexElementType::UBYTE4_NORM
    };

    /** This class declares the usage of a single vertex buffer as a component
        of a complete VertexDeclaration.
        @remarks
        Several vertex buffers can be used to supply the input geometry for a
        rendering operation, and in each case a vertex buffer can be used in
        different ways for different operations; the buffer itself does not
        define the semantics (position, normal etc), the VertexElement
        class does.
    */
    class VertexElement : public VertexDataAlloc
    {
    private:
        /// The offset in the buffer that this element starts at
        size_t mOffset;
        /// The source vertex buffer, as bound to an index using VertexBufferBinding
        unsigned short mSource;
        /// Index of the item, only applicable for some elements like texture coords
        unsigned short mIndex;
        /// The type of element
        VertexElementType mType;
        /// The meaning of the element
        VertexElementSemantic mSemantic;
    public:
        /// Constructor, should not be called directly, only needed because of list
        VertexElement() = default;
        /// Constructor, should not be called directly, call VertexDeclaration::addElement
        VertexElement(unsigned short source, size_t offset, VertexElementType theType,
            VertexElementSemantic semantic, unsigned short index = 0);
        /// Gets the vertex buffer index from where this element draws it's values
        [[nodiscard]] auto getSource() const noexcept -> unsigned short { return mSource; }
        /// Gets the offset into the buffer where this element starts
        [[nodiscard]] auto getOffset() const noexcept -> size_t { return mOffset; }
        /// Gets the data format of this element
        [[nodiscard]] auto getType() const noexcept -> VertexElementType { return mType; }
        /// Gets the meaning of this element
        [[nodiscard]] auto getSemantic() const noexcept -> VertexElementSemantic { return mSemantic; }
        /// Gets the index of this element, only applicable for repeating elements
        [[nodiscard]] auto getIndex() const noexcept -> unsigned short { return mIndex; }
        /// Gets the size of this element in bytes
        [[nodiscard]] auto getSize() const -> size_t;
        /// Utility method for helping to calculate offsets
        static auto getTypeSize(VertexElementType etype) -> size_t;
        /// Utility method which returns the count of values in a given type (result for colors may be counter-intuitive)
        static auto getTypeCount(VertexElementType etype) -> unsigned short;
        /** Simple converter function which will return a type large enough to hold 'count' values
            of the same type as the values in 'baseType'.  The 'baseType' parameter should have the
            smallest count available.  The return type may have the count rounded up to the next multiple
            of 4 bytes.  Byte types will always return a 4-count type, while short types will return either
            a 2-count or 4-count type.
        */
        static auto multiplyTypeCount(VertexElementType baseType, unsigned short count) -> VertexElementType;
        /** Simple converter function which will turn a type into it's single-value (or lowest multiple-value)
            equivalent - makes switches on type easier.  May give counter-intuitive results with bytes or shorts.
        */
        static auto getBaseType(VertexElementType multiType) -> VertexElementType;

        /** Utility method for converting colour from
            one packed 32-bit colour type to another.
        @param srcType The source type
        @param dstType The destination type
        @param ptr Read / write value to change
        */
        static void convertColourValue(VertexElementType srcType,
            VertexElementType dstType, uint32* ptr);

        /** Utility method for converting colour to
            a packed 32-bit colour type.
        @param src source colour
        @param dst The destination type
        */
        static auto convertColourValue(const ColourValue& src, VertexElementType dst) -> uint32
        {
            return src.getAsABGR();
        }

        /** Utility method to get the most appropriate packed colour vertex element format. */
        static auto getBestColourVertexElementType() -> VertexElementType;

        [[nodiscard]] inline auto operator== (const VertexElement& rhs) const noexcept -> bool = default;

        /** Adjusts a pointer to the base of a vertex to point at this element.

            Pointers are passed as a parameter because we can't
            rely on covariant return types.
        @param pBase Pointer to the start of a vertex in this buffer.
        @param pElem Pointer to a pointer which will be set to the start of this element.
        */
        template<typename T>
        void baseVertexPointerToElement(void* pBase, T** pElem) const
        {
            // The only way we can do this is to cast to char* in order to use byte offset
            // then cast back to T*.
            *pElem = reinterpret_cast<T*>(static_cast<char*>(pBase) + mOffset);
        }
    };
    /** This class declares the format of a set of vertex inputs, which
        can be issued to the rendering API through a RenderOperation.

    The ordering is important on Direct3D9 with Direct3D 7 grade cards.
    Calling closeGapsInSource() will format this VertexDeclaration accordingly.

    Whilst GL and more modern graphics cards in D3D will allow you to defy these rules,
    sticking to them will reduce state changes and improve performance on modern APIs as well.

    Like the other classes in this functional area, these declarations should be created and
    destroyed using the HardwareBufferManager.
    */
    class VertexDeclaration : public VertexDataAlloc
    {
    public:
        /// Defines the list of vertex elements that makes up this declaration
        using VertexElementList = std::list<VertexElement>;
        /// Sort routine for vertex elements
        static auto vertexElementLess(const VertexElement& e1, const VertexElement& e2) -> bool;
    protected:
        VertexElementList mElementList;

        /** Notify derived class that it is time to invalidate cached state, such as VAO or ID3D11InputLayout */
        virtual void notifyChanged() {}
    public:
        /// Standard constructor, not you should use HardwareBufferManager::createVertexDeclaration
        VertexDeclaration();
        virtual ~VertexDeclaration();

        /** Get the number of elements in the declaration. */
        [[nodiscard]] auto getElementCount() const -> size_t { return mElementList.size(); }
        /** Gets read-only access to the list of vertex elements. */
        [[nodiscard]] auto getElements() const noexcept -> const VertexElementList&;
        /** Get a single element. */
        [[nodiscard]] auto getElement(unsigned short index) const -> const VertexElement*;

        /** Sorts the elements in this list to be compatible with D3D7 graphics cards

           the order is as follows: position, blending weights, normals, diffuse colours, specular colours,
           texture coordinates
        */
        void sort();

        /** Remove any gaps in the source buffer list used by this declaration.

            This is useful if you've modified a declaration and want to remove
            any gaps in the list of buffers being used. Note, however, that if this
            declaration is already being used with a VertexBufferBinding, you will
            need to alter that too. This method is mainly useful when reorganising
            buffers based on an altered declaration.

            Whilst in theory you have completely full reign over the format of you vertices, in reality
            there are some restrictions. D3D7 grade hardware imposes a fixed ordering on the elements which are
            pulled from each buffer:

            -   VertexElements should be added in the following order, and the order of the elements within any shared
            buffer should be as follows:
                1.  Positions
                2.  Blending weights
                3.  Normals
                4.  Diffuse colours
                5.  Specular colours
                6.  Texture coordinates (starting at 0, listed in order, with no gaps)
            -   You must not have unused gaps in your buffers which are not referenced by any VertexElement
            -   You must not cause the buffer & offset settings of 2 VertexElements to overlap

            OpenGL and D3D9 compatible hardware are not required to follow these strict limitations, so you might
            find, for example that if you broke these rules your application would run under OpenGL and under DirectX on
            recent cards, but it is not guaranteed to run on older hardware under DirectX unless you stick to the above
            rules.
        @note
            This will also call sort()
        */
        void closeGapsInSource();

        /** Generates a new VertexDeclaration for optimal usage based on the current
            vertex declaration, which can be used with VertexData::reorganiseBuffers later
            if you wish, or simply used as a template.
        @remarks
            Different buffer organisations and buffer usages will be returned
            depending on the parameters passed to this method.
        @param skeletalAnimation Whether this vertex data is going to be
            skeletally animated
        @param vertexAnimation Whether this vertex data is going to be vertex animated
        @param vertexAnimationNormals Whether vertex data animation is going to include normals animation
        */
        [[nodiscard]] auto getAutoOrganisedDeclaration(bool skeletalAnimation,
            bool vertexAnimation, bool vertexAnimationNormals) const -> VertexDeclaration*;

        /** Gets the index of the highest source value referenced by this declaration. */
        [[nodiscard]] auto getMaxSource() const noexcept -> unsigned short;



        /** Adds a new VertexElement to this declaration.
        @remarks
            This method adds a single element (positions, normals etc) to the end of the
            vertex declaration. <b>Please read the information in VertexDeclaration about
        the importance of ordering and structure for compatibility with older D3D drivers</b>.
        @param source The binding index of HardwareVertexBuffer which will provide the source for this element.
            See VertexBufferBinding for full information.
        @param offset The offset in bytes where this element is located in the buffer
        @param theType The data format of the element (3 floats, a colour etc)
        @param semantic The meaning of the data (position, normal, diffuse colour etc)
        @param index Optional index for multi-input elements like texture coordinates
        @return A reference to the VertexElement added.
        */
        auto addElement(unsigned short source, size_t offset, VertexElementType theType,
            VertexElementSemantic semantic, unsigned short index = 0) -> const VertexElement&;
        /** Inserts a new VertexElement at a given position in this declaration.
        @remarks
        This method adds a single element (positions, normals etc) at a given position in this
        vertex declaration. <b>Please read the information in VertexDeclaration about
        the importance of ordering and structure for compatibility with older D3D drivers</b>.
        @param atPosition Position where the new element is inserted
        @param source The binding index of HardwareVertexBuffer which will provide the source for this element.
        See VertexBufferBinding for full information.
        @param offset The offset in bytes where this element is located in the buffer
        @param theType The data format of the element (3 floats, a colour etc)
        @param semantic The meaning of the data (position, normal, diffuse colour etc)
        @param index Optional index for multi-input elements like texture coordinates
        @return A reference to the VertexElement added.
        */
        auto insertElement(unsigned short atPosition,
            unsigned short source, size_t offset, VertexElementType theType,
            VertexElementSemantic semantic, unsigned short index = 0) -> const VertexElement&;

        /** Remove the element at the given index from this declaration. */
        void removeElement(unsigned short elem_index);

        /** Remove the element with the given semantic and usage index.
        @remarks
            In this case 'index' means the usage index for repeating elements such
            as texture coordinates. For other elements this will always be 0 and does
            not refer to the index in the vector.
        */
        void removeElement(VertexElementSemantic semantic, unsigned short index = 0);

        /** Remove all elements. */
        void removeAllElements();

        /** Modify an element in-place, params as addElement.
       @remarks
       <b>Please read the information in VertexDeclaration about
        the importance of ordering and structure for compatibility with older D3D drivers</b>.
     */
        void modifyElement(unsigned short elem_index, unsigned short source, size_t offset, VertexElementType theType,
            VertexElementSemantic semantic, unsigned short index = 0);

        /** Finds a VertexElement with the given semantic, and index if there is more than
            one element with the same semantic.
        @remarks
            If the element is not found, this method returns null.
        */
        [[nodiscard]] auto findElementBySemantic(VertexElementSemantic sem, unsigned short index = 0) const -> const VertexElement*;
        /** Based on the current elements, gets the size of the vertex for a given buffer source.
        @param source The buffer binding index for which to get the vertex size.
        */

        /** Gets a list of elements which use a given source.
        @remarks
            Note that the list of elements is returned by value therefore is separate from
            the declaration as soon as this method returns.
        */
        [[nodiscard]] auto findElementsBySource(unsigned short source) const -> VertexElementList;

        /** Gets the vertex size defined by this declaration for a given source. */
        [[nodiscard]] auto getVertexSize(unsigned short source) const -> size_t;
        
        /** Return the index of the next free texture coordinate set which may be added
            to this declaration.
        */
        [[nodiscard]] auto getNextFreeTextureCoordinate() const noexcept -> unsigned short;

        /** Clones this declaration. 
        @param mgr Optional HardwareBufferManager to use for creating the clone
            (if null, use the current default).
        */
        [[nodiscard]]
        auto clone(HardwareBufferManagerBase* mgr = nullptr) const -> VertexDeclaration*;

        [[nodiscard]] auto operator== (const VertexDeclaration& rhs) const noexcept -> bool = default;
    };

    /** Records the state of all the vertex buffer bindings required to provide a vertex declaration
        with the input data it needs for the vertex elements.
    @remarks
        Why do we have this binding list rather than just have VertexElement referring to the
        vertex buffers direct? Well, in the underlying APIs, binding the vertex buffers to an
        index (or 'stream') is the way that vertex data is linked, so this structure better
        reflects the realities of that. In addition, by separating the vertex declaration from
        the list of vertex buffer bindings, it becomes possible to reuse bindings between declarations
        and vice versa, giving opportunities to reduce the state changes required to perform rendering.
    @par
        Like the other classes in this functional area, these binding maps should be created and
        destroyed using the HardwareBufferManager.
    */
    class VertexBufferBinding : public VertexDataAlloc
    {
    public:
        /// Defines the vertex buffer bindings used as source for vertex declarations
        using VertexBufferBindingMap = std::map<unsigned short, HardwareVertexBufferSharedPtr>;
    private:
        VertexBufferBindingMap mBindingMap;
        mutable unsigned short mHighIndex{0};
    public:
        /// Constructor, should not be called direct, use HardwareBufferManager::createVertexBufferBinding
        VertexBufferBinding();
        ~VertexBufferBinding();
        /** Set a binding, associating a vertex buffer with a given index.
        @remarks
            If the index is already associated with a vertex buffer,
            the association will be replaced. This may cause the old buffer
            to be destroyed if nothing else is referring to it.
            You should assign bindings from 0 and not leave gaps, although you can
            bind them in any order.
        */
        void setBinding(unsigned short index, const HardwareVertexBufferSharedPtr& buffer);
        /** Removes an existing binding. */
        void unsetBinding(unsigned short index);

        /** Removes all the bindings. */
        void unsetAllBindings();

        /// Gets a read-only version of the buffer bindings
        auto getBindings() const noexcept -> const VertexBufferBindingMap&;

        /// Gets the buffer bound to the given source index
        auto getBuffer(unsigned short index) const -> const HardwareVertexBufferSharedPtr&;
        /// Gets whether a buffer is bound to the given source index
        auto isBufferBound(unsigned short index) const -> bool;

        auto getBufferCount() const -> size_t { return mBindingMap.size(); }

        /** Gets the highest index which has already been set, plus 1.
        @remarks
            This is to assist in binding the vertex buffers such that there are
            not gaps in the list.
        */
        auto getNextIndex() const noexcept -> unsigned short { return mHighIndex++; }

        /** Gets the last bound index.
        */
        auto getLastBoundIndex() const noexcept -> unsigned short;

        using BindingIndexMap = std::map<ushort, ushort>;

        /** Check whether any gaps in the bindings.
        */
        auto hasGaps() const -> bool;

        /** Remove any gaps in the bindings.
        @remarks
            This is useful if you've removed vertex buffer from this vertex buffer
            bindings and want to remove any gaps in the bindings. Note, however,
            that if this bindings is already being used with a VertexDeclaration,
            you will need to alter that too. This method is mainly useful when
            reorganising buffers manually.
        @param
            bindingIndexMap To be retrieve the binding index map that used to
            translation old index to new index; will be cleared by this method
            before fill-in.
        */
        void closeGaps(BindingIndexMap& bindingIndexMap);

        /// Returns true if this binding has an element that contains instance data
        auto hasInstanceData() const -> bool
        {
            for (const auto& b : mBindingMap)
                if (b.second->isInstanceData())
                    return true;
            return false;
        }
    };
    /** @} */
    /** @} */



}
