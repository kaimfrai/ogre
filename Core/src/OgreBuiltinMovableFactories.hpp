// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
module Ogre.Core:BuiltinMovableFactories;

import :MovableObject;
import :Rectangle2D;

namespace Ogre
{
class Rectangle2DFactory : public MovableObjectFactory
{
    auto createInstanceImpl(std::string_view name, const NameValuePairList* params) -> MovableObject* override;

public:
    static std::string_view const constexpr FACTORY_TYPE_NAME = "Rectangle2D";
    [[nodiscard]] auto getType() const noexcept -> std::string_view override { return FACTORY_TYPE_NAME; }
};
} // namespace Ogre
