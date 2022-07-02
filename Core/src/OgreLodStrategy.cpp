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
        for (++it; it != values.end(); ++it)
        {
            Real cur = (*it);
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
        for (++it; it != values.end(); ++it)
        {
            Real cur = (*it);
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
        std::sort(meshLodUsageList.begin(), meshLodUsageList.end(), LodUsageSortLess());
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
        std::sort(meshLodUsageList.begin(), meshLodUsageList.end(), LodUsageSortGreater());
    }
    //---------------------------------------------------------------------
    auto LodStrategy::getIndexAscending(Real value, const Mesh::MeshLodUsageList& meshLodUsageList) -> ushort
    {
        Mesh::MeshLodUsageList::const_iterator i, iend;
        iend = meshLodUsageList.end();
        ushort index = 0;
        for (i = meshLodUsageList.begin(); i != iend; ++i, ++index)
        {
            if (i->value > value)
            {
                return index ? index - 1 : 0;
            }
        }

        // If we fall all the way through, use the highest value
        return static_cast<ushort>(meshLodUsageList.size() - 1);
    }
    //---------------------------------------------------------------------
    auto LodStrategy::getIndexDescending(Real value, const Mesh::MeshLodUsageList& meshLodUsageList) -> ushort
    {
        Mesh::MeshLodUsageList::const_iterator i, iend;
        iend = meshLodUsageList.end();
        ushort index = 0;
        for (i = meshLodUsageList.begin(); i != iend; ++i, ++index)
        {
            if (i->value < value)
            {
                return index ? index - 1 : 0;
            }
        }

        // If we fall all the way through, use the highest value
        return static_cast<ushort>(meshLodUsageList.size() - 1);
    }
    //---------------------------------------------------------------------
    auto LodStrategy::getIndexAscending(Real value, const Material::LodValueList& materialLodValueList) -> ushort
    {
        Material::LodValueList::const_iterator i, iend;
        iend = materialLodValueList.end();
        unsigned short index = 0;
        for (i = materialLodValueList.begin(); i != iend; ++i, ++index)
        {
            if (*i > value)
            {
                return index ? index - 1 : 0;
            }
        }

        // If we fall all the way through, use the highest value
        return static_cast<ushort>(materialLodValueList.size() - 1);
    }
    //---------------------------------------------------------------------
    auto LodStrategy::getIndexDescending(Real value, const Material::LodValueList& materialLodValueList) -> ushort
    {
        Material::LodValueList::const_iterator i, iend;
        iend = materialLodValueList.end();
        unsigned short index = 0;
        for (i = materialLodValueList.begin(); i != iend; ++i, ++index)
        {
            if (*i < value)
            {
                return index ? index - 1 : 0;
            }
        }

        // If we fall all the way through, use the highest value
        return static_cast<ushort>(materialLodValueList.size() - 1);
    }

} // namespace
