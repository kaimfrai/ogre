module Ogre.Components.Bites:StaticPluginLoader.Obj;

import :StaticPluginLoader;

import Ogre.Core;
import Ogre.PlugIns.STBICodec;
import Ogre.RenderSystems.GL;

import <cstddef>;

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
