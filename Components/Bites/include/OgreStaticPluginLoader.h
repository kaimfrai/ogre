// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT

#ifndef OGRE_COMPONENTS_BITES_STATICPLUGINLOADER_H
#define OGRE_COMPONENTS_BITES_STATICPLUGINLOADER_H

#include "OgrePlugin.h"

#include <memory>
#include <vector>

namespace OgreBites
{
    /** \addtogroup Optional
    *  @{
    */
    /** \addtogroup Bites
    *  @{
    */
    /** Utility class for loading the plugins statically.

        When loading plugins statically, you are limited to loading plugins
        that are known about at compile time. This class will load all built
        plugins.
    */
    class StaticPluginLoader {
        std::vector<::std::unique_ptr<Ogre::Plugin>> mPlugins;

    public:
        /** Load all the enabled plugins */
        void load();

        void unload();

        ~StaticPluginLoader();
    };
    /** @} */
    /** @} */
}

#endif
