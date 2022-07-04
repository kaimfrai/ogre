// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT

#include "RootWithoutRenderSystemFixture.hpp"

#include <map>
#include <utility>

#include "OgreConfigFile.hpp"
#include "OgreDefaultHardwareBufferManager.hpp"
#include "OgreFileSystemLayer.hpp"
#include "OgreHardwareBufferManager.hpp"
#include "OgreMaterialManager.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreRoot.hpp"

using namespace Ogre;

void RootWithoutRenderSystemFixture::SetUp()
{
    mFSLayer = new FileSystemLayer(/*OGRE_VERSION_NAME*/"Tsathoggua");
    mRoot = new Root("");
    mHBM = new DefaultHardwareBufferManager;

    MaterialManager::getSingleton().initialise();

    // Load resource paths from config file
    ConfigFile cf;
    String resourcesPath = mFSLayer->getConfigFilePath("resources.cfg");

    cf.load(resourcesPath);
    // Go through all sections & settings in the file
    for (auto const&[secName, settings] : cf.getSettingsBySection()) {
        for (auto const&[typeName, archName] : settings)
        {
            ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
        }
    }
}

void RootWithoutRenderSystemFixture::TearDown()
{
    delete mRoot;
    delete mHBM;
    delete mFSLayer;
}
