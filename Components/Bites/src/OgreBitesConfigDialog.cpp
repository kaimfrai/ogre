// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
module;

#include <cstddef>

module Ogre.Components.Bites;

import :ConfigDialog;

namespace Ogre {
    class ConfigDialog;
}  // namespace Ogre

namespace OgreBites {
    auto getNativeConfigDialog() noexcept -> Ogre::ConfigDialog* {
        return nullptr;
    }
} /* namespace OgreBites */
