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
    auto createInstanceImpl(const String& name, const NameValuePairList* params) -> MovableObject* override;

public:
    static const String FACTORY_TYPE_NAME;
    [[nodiscard]]
    auto getType() const -> const String& override { return FACTORY_TYPE_NAME; }
};
} // namespace Ogre
