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
import :LodStrategy;

import <algorithm>;
import <functional>;
import <span>;
import <utility>;
import <vector>;

namespace Ogre {
    //-----------------------------------------------------------------------
    LodStrategy::LodStrategy(std::string_view name)
        : mName(name)
    { }
    //-----------------------------------------------------------------------
    LodStrategy::~LodStrategy()
    = default;
    //-----------------------------------------------------------------------
    auto LodStrategy::transformUserValue(Real userValue) const -> Real
    {
        // No transformation by default
        return userValue;
    }
    //-----------------------------------------------------------------------
    auto LodStrategy::getValue(const MovableObject *movableObject, const Camera *camera) const -> Real
    {
        // Just return implementation with LOD camera
        return getValueImpl(movableObject, camera->getLodCamera());
    }
    //-----------------------------------------------------------------------
    void LodStrategy::assertSorted(const Mesh::LodValueList &values) const
    {
        assert(isSorted(values) && "The LOD values must be sorted");
    }
    //---------------------------------------------------------------------
    auto LodStrategy::isSortedAscending(const Mesh::LodValueList& values) -> bool
    {
        auto it = values.begin();
        Real prev = (*it);
        for (Real cur : std::span{it + 1, values.end()})
        {
            if (cur < prev)
                return false;
            prev = cur;
        }

        return true;
    }
    //---------------------------------------------------------------------
    auto LodStrategy::isSortedDescending(const Mesh::LodValueList& values) -> bool
    {
        auto it = values.begin();
        Real prev = (*it);
        for (Real cur : std::span{it + 1, values.end()})
        {
            if (cur > prev)
                return false;
            prev = cur;
        }

        return true;
    }
    //---------------------------------------------------------------------
    struct LodUsageSortLess
    {
        auto operator() (const MeshLodUsage& mesh1, const MeshLodUsage& mesh2) -> bool
        {
            // Sort ascending
            return mesh1.value < mesh2.value;
        }
    };
    void LodStrategy::sortAscending(Mesh::MeshLodUsageList& meshLodUsageList)
    {
        // Perform standard sort
        std::ranges::sort(meshLodUsageList, LodUsageSortLess());
    }
    //---------------------------------------------------------------------
    struct LodUsageSortGreater
    {
        auto operator() (const MeshLodUsage& mesh1, const MeshLodUsage& mesh2) -> bool
        {
            // Sort descending
            return mesh1.value > mesh2.value;
        }
    };
    void LodStrategy::sortDescending(Mesh::MeshLodUsageList& meshLodUsageList)
    {
        // Perform standard sort
        std::ranges::sort(meshLodUsageList, LodUsageSortGreater());
    }
    //---------------------------------------------------------------------
    auto LodStrategy::getIndexAscending(Real value, const Mesh::MeshLodUsageList& meshLodUsageList) -> ushort
    {
        ushort index = 0;
        for (const auto & i : meshLodUsageList)
        {
            if (i.value > value)
            {
                return index ? index - 1 : 0;
            }
            ++index;
        }

        // If we fall all the way through, use the highest value
        return static_cast<ushort>(meshLodUsageList.size() - 1);
    }
    //---------------------------------------------------------------------
    auto LodStrategy::getIndexDescending(Real value, const Mesh::MeshLodUsageList& meshLodUsageList) -> ushort
    {
        ushort index = 0;
        for (const auto & i : meshLodUsageList)
        {
            if (i.value < value)
            {
                return index ? index - 1 : 0;
            }
            ++index;
        }

        // If we fall all the way through, use the highest value
        return static_cast<ushort>(meshLodUsageList.size() - 1);
    }
    //---------------------------------------------------------------------
    auto LodStrategy::getIndexAscending(Real value, const Material::LodValueList& materialLodValueList) -> ushort
    {
        unsigned short index = 0;
        for (float i : materialLodValueList)
        {
            if (i > value)
            {
                return index ? index - 1 : 0;
            }
            ++index;
        }

        // If we fall all the way through, use the highest value
        return static_cast<ushort>(materialLodValueList.size() - 1);
    }
    //---------------------------------------------------------------------
    auto LodStrategy::getIndexDescending(Real value, const Material::LodValueList& materialLodValueList) -> ushort
    {
        unsigned short index = 0;
        for (float i : materialLodValueList)
        {
            if (i < value)
            {
                return index ? index - 1 : 0;
            }
            ++index;
        }

        // If we fall all the way through, use the highest value
        return static_cast<ushort>(materialLodValueList.size() - 1);
    }

} // namespace
