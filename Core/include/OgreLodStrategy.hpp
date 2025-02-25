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
export module Ogre.Core:LodStrategy;

export import :Material;
export import :MemoryAllocatorConfig;
export import :Mesh;
export import :Prerequisites;

export
namespace Ogre {
class Camera;
class MovableObject;

    /** \addtogroup Core
    *  @{
    */
    /** \defgroup LOD Level of Detail
    *  @{
    */
    /** Strategy for determining level of detail.
    @remarks
        Generally, to create a new LOD strategy, all of the following will
        need to be implemented: getValueImpl, getBaseValue, transformBias,
        getIndex, sort, and isSorted.
        In addition, transformUserValue may be overridden.
    */
    class LodStrategy : public LodAlloc
    {
    private:
        /** Name of this strategy. */
        String mName;

        /** Compute the LOD value for a given movable object relative to a given camera. */
        virtual auto getValueImpl(const MovableObject *movableObject, const Camera *camera) const -> Real = 0;

    public:
        /** Constructor accepting name. */
        LodStrategy(std::string_view name);

        /** Virtual destructor. */
        virtual ~LodStrategy();

        /** Get the value of the first (highest) level of detail. */
        [[nodiscard]] virtual auto getBaseValue() const -> Real = 0;

        /** Transform LOD bias so it only needs to be multiplied by the LOD value. */
        [[nodiscard]] virtual auto transformBias(Real factor) const -> Real = 0;

        /** Transform user supplied value to internal value.
        @remarks
            By default, performs no transformation.
        @remarks
            Do not throw exceptions for invalid values here, as the LOD strategy
            may be changed such that the values become valid.
        */
        [[nodiscard]] virtual auto transformUserValue(Real userValue) const -> Real;

        /** Compute the LOD value for a given movable object relative to a given camera. */
        auto getValue(const MovableObject *movableObject, const Camera *camera) const -> Real;

        /** Get the index of the LOD usage which applies to a given value. */
        [[nodiscard]] virtual auto getIndex(Real value, const Mesh::MeshLodUsageList& meshLodUsageList) const -> ushort = 0;

        /** Get the index of the LOD usage which applies to a given value. */
        [[nodiscard]] virtual auto getIndex(Real value, const Material::LodValueList& materialLodValueList) const -> ushort = 0;

        /** Sort mesh LOD usage list from greatest to least detail */
        virtual void sort(Mesh::MeshLodUsageList& meshLodUsageList) const = 0;

        /** Determine if the LOD values are sorted from greatest detail to least detail. */
        [[nodiscard]] virtual auto isSorted(const Mesh::LodValueList& values) const -> bool = 0;

        /** Assert that the LOD values are sorted from greatest detail to least detail. */
        void assertSorted(const Mesh::LodValueList& values) const;

        /** Get the name of this strategy. */
        [[nodiscard]] auto getName() const noexcept -> std::string_view { return mName; }

    protected:
        /** Implementation of isSorted suitable for ascending values. */
        static auto isSortedAscending(const Mesh::LodValueList& values) -> bool;
        /** Implementation of isSorted suitable for descending values. */
        static auto isSortedDescending(const Mesh::LodValueList& values) -> bool;

        /** Implementation of sort suitable for ascending values. */
        static void sortAscending(Mesh::MeshLodUsageList& meshLodUsageList);
        /** Implementation of sort suitable for descending values. */
        static void sortDescending(Mesh::MeshLodUsageList& meshLodUsageList);

        /** Implementation of getIndex suitable for ascending values. */
        static auto getIndexAscending(Real value, const Mesh::MeshLodUsageList& meshLodUsageList) -> ushort;
        /** Implementation of getIndex suitable for descending values. */
        static auto getIndexDescending(Real value, const Mesh::MeshLodUsageList& meshLodUsageList) -> ushort;

        /** Implementation of getIndex suitable for ascending values. */
        static auto getIndexAscending(Real value, const Material::LodValueList& materialLodValueList) -> ushort;
        /** Implementation of getIndex suitable for descending values. */
        static auto getIndexDescending(Real value, const Material::LodValueList& materialLodValueList) -> ushort;

    };
    /** @} */
    /** @} */

} // namespace
