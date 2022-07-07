// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT

#ifndef __BUILTIN_MOVABLE_FACTORIES_H_
#define __BUILTIN_MOVABLE_FACTORIES_H_

#include <OgreMovableObject.hpp>
#include <OgreRectangle2D.hpp>

namespace Ogre
{
class Rectangle2DFactory : public MovableObjectFactory
{
    auto createInstanceImpl(std::string_view name, const NameValuePairList* params) -> MovableObject* override;

public:
    static const String FACTORY_TYPE_NAME;
    [[nodiscard]] auto getType() const noexcept -> std::string_view override { return FACTORY_TYPE_NAME; }
};
} // namespace Ogre

#endif
