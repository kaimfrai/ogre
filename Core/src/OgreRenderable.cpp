// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.

#include <cstddef>
#include <map>
#include <utility>

#include "OgreException.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderable.hpp"

namespace Ogre
{
void Renderable::setCustomParameter(size_t index, const Vector4& value) { mCustomParameters[index] = value; }

void Renderable::removeCustomParameter(size_t index) { mCustomParameters.erase(index); }

auto Renderable::hasCustomParameter(size_t index) const -> bool
{
    return mCustomParameters.find(index) != mCustomParameters.end();
}

auto Renderable::getCustomParameter(size_t index) const -> const Vector4&
{
    auto i = mCustomParameters.find(index);
    if (i != mCustomParameters.end())
    {
        return i->second;
    }
    else
    {
        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Parameter at the given index was not found.");
    }
}

void Renderable::_updateCustomGpuParameter(const GpuProgramParameters::AutoConstantEntry& constantEntry,
                                           GpuProgramParameters* params) const
{
    auto i = mCustomParameters.find(constantEntry.data);
    if (i != mCustomParameters.end())
    {
        params->_writeRawConstant(constantEntry.physicalIndex, i->second, constantEntry.elementCount);
    }
}
} // namespace Ogre
