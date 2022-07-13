/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "UseCustomCapabilitiesTests.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreCompositorManager.hpp"
#include "OgreConfigOptionMap.hpp"
#include "OgreGpuProgramManager.hpp"
#include "OgreMaterialManager.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderSystemCapabilities.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreRoot.hpp"
#include "OgreStringVector.hpp"

// Register the test suite

//--------------------------------------------------------------------------
void UseCustomCapabilitiesTests::SetUp()
{    
    using namespace Ogre;

    if(Ogre::HighLevelGpuProgramManager::getSingletonPtr())
        delete Ogre::HighLevelGpuProgramManager::getSingletonPtr();
    if(Ogre::GpuProgramManager::getSingletonPtr())
        delete Ogre::GpuProgramManager::getSingletonPtr();
    if(Ogre::CompositorManager::getSingletonPtr())
        delete Ogre::CompositorManager::getSingletonPtr();
    if(Ogre::MaterialManager::getSingletonPtr())
        delete Ogre::MaterialManager::getSingletonPtr();
    if(Ogre::ResourceGroupManager::getSingletonPtr())
        delete Ogre::ResourceGroupManager::getSingletonPtr();
}
//--------------------------------------------------------------------------
void UseCustomCapabilitiesTests::TearDown()
{
}
//--------------------------------------------------------------------------
static void checkCaps(const Ogre::RenderSystemCapabilities* caps)
{
    using namespace Ogre;

    EXPECT_EQ(caps->hasCapability(Capabilities::ANISOTROPY), true);
    EXPECT_EQ(caps->hasCapability(Capabilities::HWSTENCIL), true);

    EXPECT_EQ(caps->hasCapability(Capabilities::TWO_SIDED_STENCIL), true);
    EXPECT_EQ(caps->hasCapability(Capabilities::STENCIL_WRAP), true);

    EXPECT_EQ(caps->hasCapability(Capabilities::HWOCCLUSION), true);
    EXPECT_EQ(caps->hasCapability(Capabilities::USER_CLIP_PLANES), true);
    EXPECT_EQ(caps->hasCapability(Capabilities::HWRENDER_TO_TEXTURE), true);
    EXPECT_EQ(caps->hasCapability(Capabilities::TEXTURE_FLOAT), true);

    EXPECT_EQ(caps->hasCapability(Capabilities::NON_POWER_OF_2_TEXTURES), false);
    EXPECT_EQ(caps->hasCapability(Capabilities::TEXTURE_3D), true);
    EXPECT_EQ(caps->hasCapability(Capabilities::POINT_SPRITES), true);
    EXPECT_EQ(caps->hasCapability(Capabilities::POINT_EXTENDED_PARAMETERS), true);
    EXPECT_EQ(caps->hasCapability(Capabilities::VERTEX_TEXTURE_FETCH), false);
    EXPECT_EQ(caps->hasCapability(Capabilities::MIPMAP_LOD_BIAS), true);

    EXPECT_EQ(caps->hasCapability(Capabilities::TEXTURE_COMPRESSION), true);
    EXPECT_EQ(caps->hasCapability(Capabilities::TEXTURE_COMPRESSION_DXT), true);
    EXPECT_EQ(caps->hasCapability(Capabilities::TEXTURE_COMPRESSION_VTC), false);
    EXPECT_EQ(caps->hasCapability(Capabilities::TEXTURE_COMPRESSION_PVRTC), false);
    EXPECT_EQ(caps->hasCapability(Capabilities::TEXTURE_COMPRESSION_BC4_BC5), false);
    EXPECT_EQ(caps->hasCapability(Capabilities::TEXTURE_COMPRESSION_BC6H_BC7), false);

    EXPECT_EQ(caps->hasCapability(Capabilities::PBUFFER), false);
    EXPECT_EQ(caps->hasCapability(Capabilities::PERSTAGECONSTANT), false);
    EXPECT_EQ(caps->hasCapability(Capabilities::VAO), false);
    EXPECT_EQ(caps->hasCapability(Capabilities::SEPARATE_SHADER_OBJECTS), false);

    EXPECT_TRUE(caps->isShaderProfileSupported("arbfp1"));
    EXPECT_TRUE(caps->isShaderProfileSupported("arbvp1"));
    EXPECT_TRUE(caps->isShaderProfileSupported("glsl"));
    EXPECT_TRUE(caps->isShaderProfileSupported("ps_1_1"));
    EXPECT_TRUE(caps->isShaderProfileSupported("ps_1_2"));
    EXPECT_TRUE(caps->isShaderProfileSupported("ps_1_3"));
    EXPECT_TRUE(caps->isShaderProfileSupported("ps_1_4"));

    EXPECT_EQ(caps->getMaxPointSize(), (Real)1024);
    EXPECT_EQ(caps->getNonPOW2TexturesLimited(), false);
    EXPECT_EQ(caps->getNumTextureUnits(), (Ogre::ushort)16);
    EXPECT_EQ(caps->getNumMultiRenderTargets(), (Ogre::ushort)4);

    EXPECT_EQ(caps->getVertexProgramConstantFloatCount(), (Ogre::ushort)256);
    EXPECT_EQ(caps->getFragmentProgramConstantFloatCount(), (Ogre::ushort)64);

    EXPECT_EQ(caps->getNumVertexTextureUnits(), (Ogre::ushort)0);
    EXPECT_TRUE(caps->isShaderProfileSupported("arbvp1"));
    EXPECT_TRUE(caps->isShaderProfileSupported("arbfp1"));
}
//--------------------------------------------------------------------------
static void setUpGLRenderSystemOptions(Ogre::RenderSystem* rs)
{
    using namespace Ogre;
    ConfigOptionMap options = rs->getConfigOptions();
    // set default options
    // this should work on every semi-normal system
    rs->setConfigOption(String("Colour Depth"), String("32"));
    rs->setConfigOption(String("FSAA"), String("0"));
    rs->setConfigOption(String("Full Screen"), String("No"));
    rs->setConfigOption(String("VSync"), String("No"));
    rs->setConfigOption(String("Video Mode"), String("800 x 600"));

    // use the best RTT
    ConfigOption optionRTT = options["RTT Preferred Mode"];

    if(find(optionRTT.possibleValues.begin(), optionRTT.possibleValues.end(), "FBO") != optionRTT.possibleValues.end())
    {
        rs->setConfigOption(String("RTT Preferred Mode"), String("FBO"));
    }
    else if(find(optionRTT.possibleValues.begin(), optionRTT.possibleValues.end(), "PBuffer") != optionRTT.possibleValues.end())
    {
        rs->setConfigOption(String("RTT Preferred Mode"), String("PBuffer"));
    }
    else
        rs->setConfigOption(String("RTT Preferred Mode"), String("Copy"));
}
//--------------------------------------------------------------------------
TEST_F(UseCustomCapabilitiesTests,CustomCapabilitiesGL)
{
    using namespace Ogre;

    Root root{BLANKSTRING};
    mStaticPluginLoader.load();

    RenderSystem* rs = root.getRenderSystemByName("OpenGL Rendering Subsystem");
    if(!rs)
    {
        // This test is irrelevant because GL RenderSystem is not available
    }
    else
    {
        try {
            setUpGLRenderSystemOptions(rs);
            root.setRenderSystem(rs);

            root.initialise(true, "OGRE testCustomCapabilitiesGL Window",
                "../../Tests/Media/CustomCapabilities/customCapabilitiesTest.cfg");

            const RenderSystemCapabilities* caps = rs->getCapabilities();

            checkCaps(caps);
        }
        // clean up root, in case of error, and let cppunit to handle the exception
        catch(...)
        {
        }
    }
    mStaticPluginLoader.unload();
}
//--------------------------------------------------------------------------
static void setUpD3D9RenderSystemOptions(Ogre::RenderSystem* rs)
{
    using namespace Ogre;
    ConfigOptionMap options = rs->getConfigOptions();
    // set default options
    // this should work on every semi-normal system
    rs->setConfigOption(String("Anti aliasing"), String("None"));
    rs->setConfigOption(String("Full Screen"), String("No"));
    rs->setConfigOption(String("VSync"), String("No"));
    rs->setConfigOption(String("Video Mode"), String("800 x 600 @ 32-bit colour"));

    // pick first available device
    ConfigOption optionDevice = options["Rendering Device"];

    rs->setConfigOption(optionDevice.name, optionDevice.currentValue);
}
//--------------------------------------------------------------------------
TEST_F(UseCustomCapabilitiesTests,CustomCapabilitiesD3D9)
{
    Root* root = new Root(BLANKSTRING);        
    mStaticPluginLoader.load();

    RenderSystem* rs = root->getRenderSystemByName("Direct3D9 Rendering Subsystem");
    if(!rs)
    {
        // This test is irrelevant because D3D9 RenderSystem is not available
    }
    else
    {   
        try {
            setUpD3D9RenderSystemOptions(rs);
            root->setRenderSystem(rs);
            root->initialise(true, "OGRE testCustomCapabilitiesD3D9 Window",
                "../../Tests/Media/CustomCapabilities/customCapabilitiesTest.cfg");

            const RenderSystemCapabilities* caps = rs->getCapabilities();

            checkCaps(caps);
        }
        // clean up root, in case of error, and let cppunit to handle the exception
        catch(...)
        {
        }
    }

    delete root;
}
//--------------------------------------------------------------------------


