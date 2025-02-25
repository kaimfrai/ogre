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

module Ogre.Core;

import :Camera;
import :Frustum;
import :Math;
import :Matrix4;
import :MovableObject;
import :Node;
import :PixelCountLodStrategy;
import :Viewport;

import <limits>;

namespace Ogre {
    //-----------------------------------------------------------------------
    PixelCountLodStrategyBase::PixelCountLodStrategyBase(std::string_view name)
        : LodStrategy(name)
    { }
    //---------------------------------------------------------------------
    auto PixelCountLodStrategyBase::getBaseValue() const -> Real
    {
        // Use the maximum possible value as base
        return std::numeric_limits<Real>::max();
    }
    //---------------------------------------------------------------------
    auto PixelCountLodStrategyBase::transformBias(Real factor) const -> Real
    {
        // No transformation required for pixel count strategy
        return factor;
    }
    //---------------------------------------------------------------------
    auto PixelCountLodStrategyBase::getIndex(Real value, const Mesh::MeshLodUsageList& meshLodUsageList) const -> ushort
    {
        // Values are descending
        return getIndexDescending(value, meshLodUsageList);
    }
    //---------------------------------------------------------------------
    auto PixelCountLodStrategyBase::getIndex(Real value, const Material::LodValueList& materialLodValueList) const -> ushort
    {
        // Values are descending
        return getIndexDescending(value, materialLodValueList);
    }
    //---------------------------------------------------------------------
    void PixelCountLodStrategyBase::sort(Mesh::MeshLodUsageList& meshLodUsageList) const
    {
        // Sort descending
        sortDescending(meshLodUsageList);
    }
    //---------------------------------------------------------------------
    auto PixelCountLodStrategyBase::isSorted(const Mesh::LodValueList& values) const -> bool
    {
        // Check if values are sorted descending
        return isSortedDescending(values);
    }

    /************************************************************************/
    /*  AbsolutPixelCountLodStrategy                                        */
    /************************************************************************/

    //-----------------------------------------------------------------------
    template<> AbsolutePixelCountLodStrategy* Singleton<AbsolutePixelCountLodStrategy>::msSingleton = nullptr;
    auto AbsolutePixelCountLodStrategy::getSingletonPtr() noexcept -> AbsolutePixelCountLodStrategy*
    {
        return msSingleton;
    }
    auto AbsolutePixelCountLodStrategy::getSingleton() noexcept -> AbsolutePixelCountLodStrategy&
    {
        assert( msSingleton );  return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    AbsolutePixelCountLodStrategy::AbsolutePixelCountLodStrategy()
        : PixelCountLodStrategyBase("pixel_count")
    { }
    AbsolutePixelCountLodStrategy::~AbsolutePixelCountLodStrategy() = default;
    //-----------------------------------------------------------------------
    auto PixelCountLodStrategyBase::getValueImpl(const MovableObject *movableObject, const Ogre::Camera *camera) const -> Real
    {
        // Get area of unprojected circle with object bounding radius
        Real boundingArea = Math::PI * Math::Sqr(movableObject->getBoundingRadiusScaled());

        // Base computation on projection type
        switch (camera->getProjectionType())
        {
        using enum ProjectionType;
        case PERSPECTIVE:
            {
                // Get camera distance
                Real distanceSquared = movableObject->getParentNode()->getSquaredViewDepth(camera);

                // Check for 0 distance
                if (distanceSquared <= std::numeric_limits<Real>::epsilon())
                    return getBaseValue();

                // Get projection matrix (this is done to avoid computation of tan(FOV / 2))
                const Matrix4& projectionMatrix = camera->getProjectionMatrix();

                // Estimate pixel count
                // multiplied out version of $A = pi * r^2$
                // where r is projected using a gluPerspective matrix as $pr = cot(fovy / 2) * r / z$
                // and then converted to pixels as $pr * height / 2$
                return 0.25 * (boundingArea * projectionMatrix[0][0] * projectionMatrix[1][1]) /
                       distanceSquared;
            }
        case ORTHOGRAPHIC:
            {
                // Compute orthographic area
                Real orthoArea = camera->getOrthoWindowHeight() * camera->getOrthoWindowWidth();

                // Check for 0 orthographic area
                if (orthoArea <= std::numeric_limits<Real>::epsilon())
                    return getBaseValue();

                // Estimate covered area
                return boundingArea / orthoArea;
            }
        default:
            {
                // This case is not covered for obvious reasons
                throw;
            }
        }
    }
    //-----------------------------------------------------------------------
    auto AbsolutePixelCountLodStrategy::getValueImpl(const MovableObject *movableObject, const Ogre::Camera *camera) const -> Real
    {
        // Get ratio of screen size to absolutely covered pixel count
        Real absoluteValue = PixelCountLodStrategyBase::getValueImpl(movableObject, camera);

        // Get viewport area
        const Viewport *viewport = camera->getViewport();
        Real viewportArea = static_cast<Real>(viewport->getActualWidth() * viewport->getActualHeight());

        // Return absolute pixel count
        return absoluteValue * viewportArea;
    }

    /************************************************************************/
    /* ScreenRatioPixelCountLodStrategy                                     */
    /************************************************************************/

    //-----------------------------------------------------------------------
    template<> ScreenRatioPixelCountLodStrategy* Singleton<ScreenRatioPixelCountLodStrategy>::msSingleton = nullptr;
    auto ScreenRatioPixelCountLodStrategy::getSingletonPtr() noexcept -> ScreenRatioPixelCountLodStrategy*
    {
        return msSingleton;
    }
    auto ScreenRatioPixelCountLodStrategy::getSingleton() noexcept -> ScreenRatioPixelCountLodStrategy&
    {
        assert( msSingleton );  return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    ScreenRatioPixelCountLodStrategy::ScreenRatioPixelCountLodStrategy()
        : PixelCountLodStrategyBase("screen_ratio_pixel_count")
    { }
    ScreenRatioPixelCountLodStrategy::~ScreenRatioPixelCountLodStrategy() = default;
    //-----------------------------------------------------------------------

} // namespace
