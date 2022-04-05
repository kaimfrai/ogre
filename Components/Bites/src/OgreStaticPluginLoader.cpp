#include "OgreStaticPluginLoader.hpp"

#include <cstddef>

#include "OgreGLPlugin.hpp"
#include "OgrePlugin.hpp"
#include "OgreRoot.hpp"
#include "OgreSTBICodec.hpp"

void OgreBites::StaticPluginLoader::load()
{
    using namespace Ogre;
    Plugin* plugin = NULL;

    plugin = new GLPlugin();
    mPlugins.push_back(plugin);

    plugin = new STBIPlugin();
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
        delete mPlugins[i];
    }
    mPlugins.clear();
}
