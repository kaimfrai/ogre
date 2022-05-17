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
    typedef AbsolutePixelCountLodStrategy PixelCountLodStrategy;

    /** Abstract base class for level of detail strategy based on pixel count approximations from bounding sphere projection. */
    class PixelCountLodStrategyBase : public LodStrategy
    {
    protected:
        auto getValueImpl(const MovableObject *movableObject, const Camera *camera) const -> Real override;

    public:
        /** Default constructor. */
        PixelCountLodStrategyBase(const String& name);

        /// @copydoc LodStrategy::getBaseValue
        [[nodiscard]] virtual auto getBaseValue() const -> Real;

        /// @copydoc LodStrategy::transformBias
        [[nodiscard]] virtual auto transformBias(Real factor) const -> Real;

        /// @copydoc LodStrategy::getIndex
        [[nodiscard]] virtual auto getIndex(Real value, const Mesh::MeshLodUsageList& meshLodUsageList) const -> ushort;

        /// @copydoc LodStrategy::getIndex
        [[nodiscard]] virtual auto getIndex(Real value, const Material::LodValueList& materialLodValueList) const -> ushort;

        /// @copydoc LodStrategy::sort
        virtual void sort(Mesh::MeshLodUsageList& meshLodUsageList) const;

        /// @copydoc LodStrategy::isSorted
        [[nodiscard]] virtual auto isSorted(const Mesh::LodValueList& values) const -> bool;
    };

    class AbsolutePixelCountLodStrategy : public PixelCountLodStrategyBase, public Singleton<AbsolutePixelCountLodStrategy>
    {
    public:
        /** Default constructor. */
        AbsolutePixelCountLodStrategy();
        ~AbsolutePixelCountLodStrategy();

        auto getValueImpl(const MovableObject *movableObject, const Camera *camera) const -> Real override;

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() -> AbsolutePixelCountLodStrategy&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() -> AbsolutePixelCountLodStrategy*;
    };

    class ScreenRatioPixelCountLodStrategy : public PixelCountLodStrategyBase, public Singleton<ScreenRatioPixelCountLodStrategy>
    {
    public:
        /** Default constructor. */
        ScreenRatioPixelCountLodStrategy();
        ~ScreenRatioPixelCountLodStrategy();

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() -> ScreenRatioPixelCountLodStrategy&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() -> ScreenRatioPixelCountLodStrategy*;
    };
    /** @} */
    /** @} */

} // namespace

#endif
