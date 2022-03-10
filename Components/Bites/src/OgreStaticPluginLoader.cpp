#include "OgreStaticPluginLoader.h"

#include <cstddef>

#include "OgreComponents.h"
#include "OgreRoot.h"
#include "OgreMemoryAllocatorConfig.h"

namespace Ogre {
    class Plugin;
}  // namespace Ogre

#define OGRE_STATIC_GL

#ifdef OGRE_BUILD_PLUGIN_BSP
#define OGRE_STATIC_BSPSceneManager
#endif
#ifdef OGRE_BUILD_PLUGIN_PFX
#define OGRE_STATIC_ParticleFX
#endif
#ifdef OGRE_BUILD_PLUGIN_CG
#define OGRE_STATIC_CgProgramManager
#endif

#ifdef OGRE_USE_PCZ
    #ifdef OGRE_BUILD_PLUGIN_PCZ
    #define OGRE_STATIC_PCZSceneManager
    #define OGRE_STATIC_OctreeZone
    #endif
#else
    #ifdef OGRE_BUILD_PLUGIN_OCTREE
    #define OGRE_STATIC_OctreeSceneManager
    #endif
#endif

#ifdef OGRE_BITES_STATIC_PLUGINS
// Static plugin headers
#ifdef OGRE_STATIC_CgProgramManager
#  include "OgreCgPlugin.h"
#endif
#ifdef OGRE_BUILD_PLUGIN_GLSLANG
#  include "OgreGLSLangProgramManager.h"
#endif
#ifdef OGRE_BUILD_PLUGIN_ASSIMP
#  include "OgreAssimpLoader.h"
#endif
#ifdef OGRE_STATIC_OctreeSceneManager
#  include "OgreOctreePlugin.h"
#endif
#ifdef OGRE_STATIC_ParticleFX
#  include "OgreParticleFXPlugin.h"
#endif
#ifdef OGRE_STATIC_BSPSceneManager
#  include "OgreBspSceneManagerPlugin.h"
#endif

#include "OgreGLPlugin.h"

#ifdef OGRE_STATIC_PCZSceneManager
#  include "OgrePCZPlugin.h"
#endif
#ifdef OGRE_STATIC_OctreeZone
#  include "OgreOctreeZonePlugin.h"
#endif
#ifdef OGRE_BUILD_PLUGIN_STBI
#   include "OgreSTBICodec.h"
#endif
#ifdef OGRE_BUILD_PLUGIN_DOT_SCENE
#   include "OgreDotSceneLoader.h"
#endif
#if defined(OGRE_BUILD_PLUGIN_FREEIMAGE) && !defined(OGRE_BUILD_PLUGIN_STBI)
#   include "OgreFreeImageCodec.h"
#endif
#endif

void OgreBites::StaticPluginLoader::load()
{
    using namespace Ogre;
#ifdef OGRE_BITES_STATIC_PLUGINS
    Plugin* plugin = NULL;

    plugin = OGRE_NEW GLPlugin();
    mPlugins.push_back(plugin);

#ifdef OGRE_STATIC_CgProgramManager
    plugin = OGRE_NEW CgPlugin();
    mPlugins.push_back(plugin);
#endif
#ifdef OGRE_STATIC_OctreeSceneManager
    plugin = OGRE_NEW OctreePlugin();
    mPlugins.push_back(plugin);
#endif
#ifdef OGRE_STATIC_ParticleFX
    plugin = OGRE_NEW ParticleFXPlugin();
    mPlugins.push_back(plugin);
#endif
#ifdef OGRE_STATIC_BSPSceneManager
    plugin = OGRE_NEW BspSceneManagerPlugin();
    mPlugins.push_back(plugin);
#endif
#ifdef OGRE_STATIC_PCZSceneManager
    plugin = OGRE_NEW PCZPlugin();
    mPlugins.push_back(plugin);
#endif
#ifdef OGRE_STATIC_OctreeZone
    plugin = OGRE_NEW OctreeZonePlugin();
    mPlugins.push_back(plugin);
#endif
#ifdef OGRE_BUILD_PLUGIN_STBI
    plugin = OGRE_NEW STBIPlugin();
    mPlugins.push_back(plugin);
#endif
#ifdef OGRE_BUILD_PLUGIN_DOT_SCENE
    plugin = OGRE_NEW DotScenePlugin();
    mPlugins.push_back(plugin);
#endif
#if defined(OGRE_BUILD_PLUGIN_FREEIMAGE) && !defined(OGRE_BUILD_PLUGIN_STBI)
    plugin = OGRE_NEW FreeImagePlugin();
    mPlugins.push_back(plugin);
#endif
#ifdef OGRE_BUILD_PLUGIN_ASSIMP
    plugin = OGRE_NEW AssimpPlugin();
    mPlugins.push_back(plugin);
#endif
#ifdef OGRE_BUILD_PLUGIN_GLSLANG
    plugin = OGRE_NEW GLSLangPlugin();
    mPlugins.push_back(plugin);
#endif
#endif

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
