#include "OgreStaticPluginLoader.hpp"

#include <cstddef>

#include "OgreGLPlugin.hpp"
#include "OgrePlugin.hpp"
#include "OgreRoot.hpp"
#include "OgreSTBICodec.hpp"

void OgreBites::StaticPluginLoader::load()
{
    using namespace Ogre;
    Plugin* plugin = nullptr;

    plugin = new GLPlugin();
    mPlugins.push_back(plugin);

    plugin = new STBIPlugin();
    mPlugins.push_back(plugin);

    Root& root  = Root::getSingleton();
    for (auto & mPlugin : mPlugins) {
        root.installPlugin(mPlugin);
    }
}

void OgreBites::StaticPluginLoader::unload()
{
    // don't unload plugins, since Root will have done that. Destroy here.
    for (auto & mPlugin : mPlugins) {
        delete mPlugin;
    }
    mPlugins.clear();
}
