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

#include <algorithm>
#include <cassert>
#include <functional>
#include <span>
#include <utility>
#include <vector>

#include "OgreCamera.hpp"
#include "OgreLodStrategy.hpp"

namespace Ogre {
class MovableObject;

    //-----------------------------------------------------------------------
    LodStrategy::LodStrategy(String  name)
        : mName(std::move(name))
    { }
    //-----------------------------------------------------------------------
    LodStrategy::~LodStrategy()
    = default;
    //-----------------------------------------------------------------------
    Real LodStrategy::transformUserValue(Real userValue) const
    {
        // No transformation by default
        return userValue;
    }
    //-----------------------------------------------------------------------
    Real LodStrategy::getValue(const MovableObject *movableObject, const Camera *camera) const
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
    bool LodStrategy::isSortedAscending(const Mesh::LodValueList& values)
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
    bool LodStrategy::isSortedDescending(const Mesh::LodValueList& values)
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
        bool operator() (const MeshLodUsage& mesh1, const MeshLodUsage& mesh2)
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
        bool operator() (const MeshLodUsage& mesh1, const MeshLodUsage& mesh2)
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
    ushort LodStrategy::getIndexAscending(Real value, const Mesh::MeshLodUsageList& meshLodUsageList)
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
    ushort LodStrategy::getIndexDescending(Real value, const Mesh::MeshLodUsageList& meshLodUsageList)
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
    ushort LodStrategy::getIndexAscending(Real value, const Material::LodValueList& materialLodValueList)
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
    ushort LodStrategy::getIndexDescending(Real value, const Material::LodValueList& materialLodValueList)
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
