#include "OgreStaticPluginLoader.h"

#include <cstddef>

#include "OgreComponents.h"
#include "OgreRoot.h"
#include "OgreMemoryAllocatorConfig.h"
#include "OgreGLPlugin.h"
#include "OgreSTBICodec.h"

void OgreBites::StaticPluginLoader::load()
{
    using namespace Ogre;
    Plugin* plugin = NULL;

    plugin = OGRE_NEW GLPlugin();
    mPlugins.push_back(plugin);

    plugin = OGRE_NEW STBIPlugin();
    mPlugins.push_back(plugin);

    Root& root  = Root::getSingleton();
    for (size_t i = 0; i < mPlugins.size(); ++i) {
        root.installPlugin(mPlugins[i]);
    }
}

void OgreBites::StaticPluginLoader::unload()
{
    // don't unload plugins, since Root will have done that. Destroy here.
    for (size_t i = 0; i < mPlugins.size(); ++i) {
        OGRE_DELETE mPlugins[i];
    }
    mPlugins.clear();
}
