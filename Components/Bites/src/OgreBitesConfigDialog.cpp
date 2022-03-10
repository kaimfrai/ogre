// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT

#include "OgreBitesConfigDialog.h"

#include "OgreConfigDialogImp.h"
#include "OgrePlatform.h"

namespace Ogre {
    class ConfigDialog;
}  // namespace Ogre

namespace OgreBites {
    Ogre::ConfigDialog* getNativeConfigDialog() {
        return NULL;
    }
} /* namespace OgreBites */
