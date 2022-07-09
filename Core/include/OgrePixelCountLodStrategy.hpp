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
#ifndef OGRE_CORE_PIXELCOUNTLODSTRATEGY_H
#define OGRE_CORE_PIXELCOUNTLODSTRATEGY_H

#include "OgreLodStrategy.hpp"
#include "OgreMaterial.hpp"
#include "OgreMesh.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSingleton.hpp"

namespace Ogre {

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup LOD
    *  @{
    */

    class AbsolutePixelCountLodStrategy;
class Camera;
class MovableObject;

    /// Backward compatible name for Distance_Box strategy.
    using PixelCountLodStrategy = AbsolutePixelCountLodStrategy;

    /** Abstract base class for level of detail strategy based on pixel count approximations from bounding sphere projection. */
    class PixelCountLodStrategyBase : public LodStrategy
    {
    protected:
        auto getValueImpl(const MovableObject *movableObject, const Camera *camera) const -> Real override;

    public:
        /** Default constructor. */
        PixelCountLodStrategyBase(const String& name);

        /// @copydoc LodStrategy::getBaseValue
        [[nodiscard]] auto getBaseValue() const -> Real override;

        /// @copydoc LodStrategy::transformBias
        [[nodiscard]] auto transformBias(Real factor) const -> Real override;

        /// @copydoc LodStrategy::getIndex
        [[nodiscard]] auto getIndex(Real value, const Mesh::MeshLodUsageList& meshLodUsageList) const -> ushort override;

        /// @copydoc LodStrategy::getIndex
        [[nodiscard]] auto getIndex(Real value, const Material::LodValueList& materialLodValueList) const -> ushort override;

        /// @copydoc LodStrategy::sort
        void sort(Mesh::MeshLodUsageList& meshLodUsageList) const override;

        /// @copydoc LodStrategy::isSorted
        [[nodiscard]] auto isSorted(const Mesh::LodValueList& values) const -> bool override;
    };

    class AbsolutePixelCountLodStrategy : public PixelCountLodStrategyBase, public Singleton<AbsolutePixelCountLodStrategy>
    {
    public:
        /** Default constructor. */
        AbsolutePixelCountLodStrategy();
        ~AbsolutePixelCountLodStrategy() override;

        auto getValueImpl(const MovableObject *movableObject, const Camera *camera) const -> Real override;

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> AbsolutePixelCountLodStrategy&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> AbsolutePixelCountLodStrategy*;
    };

    class ScreenRatioPixelCountLodStrategy : public PixelCountLodStrategyBase, public Singleton<ScreenRatioPixelCountLodStrategy>
    {
    public:
        /** Default constructor. */
        ScreenRatioPixelCountLodStrategy();
        ~ScreenRatioPixelCountLodStrategy() override;

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> ScreenRatioPixelCountLodStrategy&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> ScreenRatioPixelCountLodStrategy*;
    };
    /** @} */
    /** @} */

} // namespace

#endif
