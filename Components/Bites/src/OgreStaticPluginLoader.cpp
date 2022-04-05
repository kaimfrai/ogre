#include "OgreStaticPluginLoader.hpp"

#include "OgreGLPlugin.hpp"
#include "OgrePlatform.hpp"
#include "OgrePlugin.hpp"
#include "OgreRoot.hpp"
#include "OgreSTBICodec.hpp"

void OgreBites::StaticPluginLoader::load()
{
    using namespace Ogre;

    mPlugins.emplace_back(new GLPlugin());

    mPlugins.emplace_back(new STBIPlugin());

    Root& root  = Root::getSingleton();
    for (auto const& plugin : mPlugins) {
        root.installPlugin(plugin.get());
    }
}

void OgreBites::StaticPluginLoader::unload()
{
    auto* root  = Ogre::Root::getSingletonPtr();
    if  (root)
        for (auto const& plugin : mPlugins) {
            root->uninstallPlugin(plugin.get());
        }
    mPlugins.clear();
}


OgreBites::StaticPluginLoader::~StaticPluginLoader()
{
    unload();
}
