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
module Ogre.Tests.Core;

import :RenderSystemCapabilitiesTests;

import Ogre.Core;

import <algorithm>;
import <fstream>;
import <map>;
import <string>;
import <utility>;
import <vector>;

// Register the test suite
//--------------------------------------------------------------------------
void RenderSystemCapabilitiesTests::SetUp()
{    
    using namespace Ogre;

    // We need to be able to create FileSystem archives to load .rendercaps
    mFileSystemArchiveFactory = new FileSystemArchiveFactory();

    mArchiveManager = new ArchiveManager();
    ArchiveManager::getSingleton().addArchiveFactory(mFileSystemArchiveFactory);

    mRenderSystemCapabilitiesManager = new RenderSystemCapabilitiesManager();

    Ogre::ConfigFile cf;
    cf.load(Ogre::FileSystemLayer(/*OGRE_VERSION_NAME*/"Tsathoggua").getConfigFilePath("resources.cfg"));
    Ogre::String testPath = ::std::format("{}/CustomCapabilities", cf.getSettings("Tests").begin()->second);

    // Actual parsing happens here. The following test methods confirm parse results only.
    mRenderSystemCapabilitiesManager->parseCapabilitiesFromArchive(testPath, "FileSystem", true);
}

//--------------------------------------------------------------------------
void RenderSystemCapabilitiesTests::TearDown()
{
    delete mRenderSystemCapabilitiesManager;
    delete mArchiveManager;
    delete mFileSystemArchiveFactory;
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,IsShaderProfileSupported)
{
    // create a new RSC
    Ogre::RenderSystemCapabilities rsc;

    // check that no shader profile is supported
    EXPECT_TRUE(!rsc.isShaderProfileSupported("vs_1"));
    EXPECT_TRUE(!rsc.isShaderProfileSupported("ps_1_1"));
    EXPECT_TRUE(!rsc.isShaderProfileSupported("fp1"));

    rsc.addShaderProfile("vs_1");
    rsc.addShaderProfile("fp1");

    // check that the added shader profiles are supported
    EXPECT_TRUE(rsc.isShaderProfileSupported("vs_1"));
    EXPECT_TRUE(rsc.isShaderProfileSupported("fp1"));


    // check that non added profile is not supported
    EXPECT_TRUE(!rsc.isShaderProfileSupported("ps_1_1"));


    // check that empty string is not supported
    EXPECT_TRUE(!rsc.isShaderProfileSupported(""));
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,HasCapability)
{
    RenderSystemCapabilities rsc;

    // check that no caps (from 2 categories) are supported
    EXPECT_TRUE(!rsc.hasCapability(Capabilities::TWO_SIDED_STENCIL));
    EXPECT_TRUE(!rsc.hasCapability(Capabilities::MIPMAP_LOD_BIAS));
    EXPECT_TRUE(!rsc.hasCapability(Capabilities::TEXTURE_COMPRESSION));
    EXPECT_TRUE(!rsc.hasCapability(Capabilities::TEXTURE_COMPRESSION_VTC));
    EXPECT_TRUE(!rsc.hasCapability(Capabilities::PBUFFER));

    // add support for few caps from each category
    rsc.setCapability(Capabilities::TEXTURE_COMPRESSION);

    // check that the newly set caps are supported
    EXPECT_TRUE(rsc.hasCapability(Capabilities::TEXTURE_COMPRESSION));

    // check that the non-set caps are NOT supported
    EXPECT_TRUE(!rsc.hasCapability(Capabilities::TWO_SIDED_STENCIL));
    EXPECT_TRUE(!rsc.hasCapability(Capabilities::MIPMAP_LOD_BIAS));
    EXPECT_TRUE(!rsc.hasCapability(Capabilities::TEXTURE_COMPRESSION_VTC));
    EXPECT_TRUE(!rsc.hasCapability(Capabilities::PBUFFER));
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,SerializeBlank)
{
    RenderSystemCapabilitiesManager* rscManager = RenderSystemCapabilitiesManager::getSingletonPtr();

    RenderSystemCapabilities* rsc = rscManager->loadParsedCapabilities("TestCaps Blank");

    // if we have a non-NULL it's good enough
    EXPECT_TRUE(rsc != nullptr);
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,SerializeEnumCapability)
{
    RenderSystemCapabilitiesManager* rscManager = RenderSystemCapabilitiesManager::getSingletonPtr();

    RenderSystemCapabilities* rsc = rscManager->loadParsedCapabilities("TestCaps enum Capabilities");

    // confirm that RSC was loaded
    EXPECT_TRUE(rsc != nullptr);

    // confirm that the contents are the same as in .rendercaps file
    EXPECT_TRUE(rsc->hasCapability(Capabilities::AUTOMIPMAP_COMPRESSED));
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,SerializeStringCapability)
{
    RenderSystemCapabilitiesManager* rscManager = RenderSystemCapabilitiesManager::getSingletonPtr();

    RenderSystemCapabilities* rsc = rscManager->loadParsedCapabilities("TestCaps set String");

    // confirm that RSC was loaded
    EXPECT_TRUE(rsc != nullptr);

    EXPECT_TRUE(rsc->isShaderProfileSupported("vs99"));
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,SerializeBoolCapability)
{
    RenderSystemCapabilitiesManager* rscManager = RenderSystemCapabilitiesManager::getSingletonPtr();

    RenderSystemCapabilities* rscTrue = rscManager->loadParsedCapabilities("TestCaps set bool (true)");
    RenderSystemCapabilities* rscFalse = rscManager->loadParsedCapabilities("TestCaps set bool (false)");

    // confirm that RSC was loaded
    EXPECT_TRUE(rscTrue != nullptr);
    EXPECT_TRUE(rscFalse != nullptr);
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,SerializeIntCapability)
{
    RenderSystemCapabilitiesManager* rscManager = RenderSystemCapabilitiesManager::getSingletonPtr();

    RenderSystemCapabilities* rsc = rscManager->loadParsedCapabilities("TestCaps set int");

    // confirm that RSC was loaded
    EXPECT_TRUE(rsc != nullptr);

    // TODO: why no get?
    EXPECT_TRUE(rsc->getNumMultiRenderTargets() == 99);
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,SerializeRealCapability)
{
    RenderSystemCapabilitiesManager* rscManager = RenderSystemCapabilitiesManager::getSingletonPtr();

    RenderSystemCapabilities* rsc = rscManager->loadParsedCapabilities("TestCaps set Real");

    // confirm that RSC was loaded
    EXPECT_TRUE(rsc != nullptr);

    EXPECT_TRUE(rsc->getMaxPointSize() == 99.5);
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,SerializeShaderCapability)
{
    RenderSystemCapabilitiesManager* rscManager = RenderSystemCapabilitiesManager::getSingletonPtr();

    RenderSystemCapabilities* rsc = rscManager->loadParsedCapabilities("TestCaps addShaderProfile");

    // confirm that RSC was loaded
    EXPECT_TRUE(rsc != nullptr);

    EXPECT_TRUE(rsc->isShaderProfileSupported("vp1"));
    EXPECT_TRUE(rsc->isShaderProfileSupported("vs_1_1"));
    EXPECT_TRUE(rsc->isShaderProfileSupported("ps_99"));
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,WriteSimpleCapabilities)
{
    using namespace Ogre;
    using namespace std;
    String name = "simple caps";
    String filename = "simpleCapsTest.rendercaps";

    // set up caps of every type
    RenderSystemCapabilitiesSerializer serializer;
    RenderSystemCapabilities caps;
    caps.setMaxPointSize(10.5);
    caps.addShaderProfile("vs999");
    caps.addShaderProfile("sp999");

    // write them to file
    serializer.writeScript(&caps, name, filename);

    // read them back
    ifstream capsfile(filename.c_str());
    char buff[255];

    capsfile.getline(buff, 255);
    EXPECT_EQ(::std::format("render_system_capabilities \"{}\"", name), String(buff));

    capsfile.getline(buff, 255);
    EXPECT_EQ(String("{"), String(buff));

    // scan every line and find the set capabilities it them
    std::vector <String> lines;
    while(capsfile.good())
    {
        capsfile.getline(buff, 255);
        lines.emplace_back(buff);
    }

    // check that the file is closed nicely
    String closeBracket = *(lines.end() - 2);
    EXPECT_EQ(String("}"), closeBracket);
    EXPECT_EQ(String(""), lines.back());

    // check that all the set caps are there
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tmax_point_size 10.5") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tshader_profile sp999") != lines.end());
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,WriteAllFalseCapabilities)
{
    using namespace Ogre;
    using namespace std;
    String name = "all false caps";
    String filename = "allFalseCapsTest.rendercaps";

    // set up caps of every type
    RenderSystemCapabilitiesSerializer serializer;
    RenderSystemCapabilities caps;

    // write them to file
    serializer.writeScript(&caps, name, filename);

    // read them back
    ifstream capsfile(filename.c_str());
    char buff[255];

    capsfile.getline(buff, 255);
    EXPECT_EQ(::std::format("render_system_capabilities \"{}\"", name), String(buff));

    capsfile.getline(buff, 255);
    EXPECT_EQ(String("{"), String(buff));

    // scan every line and find the set capabilities it them
    std::vector <String> lines;
    while(capsfile.good())
    {
        capsfile.getline(buff, 255);
        lines.emplace_back(buff);
    }

      // check that the file is closed nicely
    String closeBracket = *(lines.end() - 2);
    EXPECT_EQ(String("}"), closeBracket);
    EXPECT_EQ(String(""), lines.back());

    // confirm every caps
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tautomipmap_compressed false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tanisotropy false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\thwstencil false") != lines.end());

    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tvertex_program false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttwo_sided_stencil false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tstencil_wrap false") != lines.end());

    EXPECT_TRUE(find(lines.begin(), lines.end(), "\thwocclusion false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tuser_clip_planes false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\thwrender_to_texture false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_float false") != lines.end());

    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tnon_power_of_2_textures false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_3d false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tpoint_sprites false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tpoint_extended_parameters false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tvertex_texture_fetch false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tmipmap_lod_bias false") != lines.end());

    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression_dxt false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression_vtc false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression_pvrtc false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression_bc4_bc5 false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression_bc6h_bc7 false") != lines.end());

    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tpbuffer false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tperstageconstant false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tseparate_shader_objects false") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tvao false") != lines.end());
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,WriteAllTrueCapabilities)
{
    using namespace Ogre;
    using namespace std;
    String name = "all false caps";
    String filename = "allFalseCapsTest.rendercaps";

    // set up caps of every type
    RenderSystemCapabilitiesSerializer serializer;
    RenderSystemCapabilities caps;

    // set all caps
    caps.setCapability(Capabilities::AUTOMIPMAP_COMPRESSED);
    caps.setCapability(Capabilities::ANISOTROPY);
    caps.setCapability(Capabilities::HWSTENCIL);

    caps.setCapability(Capabilities::TWO_SIDED_STENCIL);
    caps.setCapability(Capabilities::STENCIL_WRAP);

    caps.setCapability(Capabilities::HWOCCLUSION);
    caps.setCapability(Capabilities::USER_CLIP_PLANES);
    caps.setCapability(Capabilities::HWRENDER_TO_TEXTURE);
    caps.setCapability(Capabilities::TEXTURE_FLOAT);

    caps.setCapability(Capabilities::NON_POWER_OF_2_TEXTURES);
    caps.setCapability(Capabilities::TEXTURE_3D);
    caps.setCapability(Capabilities::POINT_SPRITES);
    caps.setCapability(Capabilities::POINT_EXTENDED_PARAMETERS);
    caps.setCapability(Capabilities::VERTEX_TEXTURE_FETCH);
    caps.setCapability(Capabilities::MIPMAP_LOD_BIAS);

    caps.setCapability(Capabilities::TEXTURE_COMPRESSION);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION_DXT);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION_VTC);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION_PVRTC);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION_BC4_BC5);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION_BC6H_BC7);

    caps.setCapability(Capabilities::PBUFFER);
    caps.setCapability(Capabilities::PERSTAGECONSTANT);
    caps.setCapability(Capabilities::SEPARATE_SHADER_OBJECTS);
    caps.setCapability(Capabilities::VAO);

    // write them to file
    serializer.writeScript(&caps, name, filename);

    // read them back
    ifstream capsfile(filename.c_str());
    char buff[255];

    capsfile.getline(buff, 255);
    EXPECT_EQ(::std::format("render_system_capabilities \"{}\"", name), String(buff));

    capsfile.getline(buff, 255);
    EXPECT_EQ(String("{"), String(buff));

    // scan every line and find the set capabilities it them
    std::vector <String> lines;
    while(capsfile.good())
    {
        capsfile.getline(buff, 255);
        lines.emplace_back(buff);
    }

    // check that the file is closed nicely
    String closeBracket = *(lines.end() - 2);
    EXPECT_EQ(String("}"), closeBracket);
    EXPECT_EQ(String(""), lines.back());

    // confirm all caps
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tautomipmap_compressed true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tanisotropy true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\thwstencil true") != lines.end());

    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttwo_sided_stencil true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tstencil_wrap true") != lines.end());

    EXPECT_TRUE(find(lines.begin(), lines.end(), "\thwocclusion true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tuser_clip_planes true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\thwrender_to_texture true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_float true") != lines.end());

    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tnon_power_of_2_textures true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_3d true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tpoint_sprites true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tpoint_extended_parameters true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tvertex_texture_fetch true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tmipmap_lod_bias true") != lines.end());

    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression_dxt true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression_vtc true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression_pvrtc true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression_bc4_bc5 true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\ttexture_compression_bc6h_bc7 true") != lines.end());

    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tpbuffer true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tperstageconstant true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tseparate_shader_objects true") != lines.end());
    EXPECT_TRUE(find(lines.begin(), lines.end(), "\tvao true") != lines.end());
}
//--------------------------------------------------------------------------
TEST_F(RenderSystemCapabilitiesTests,WriteAndReadComplexCapabilities)
{
    using namespace Ogre;
    using namespace std;
    String name = "complex caps";
    String filename = "complexCapsTest.rendercaps";

    // set up caps of every type
    RenderSystemCapabilitiesSerializer serializer;
    RenderSystemCapabilities caps;

    // set all caps
    caps.setCapability(Capabilities::HWSTENCIL);
    caps.setCapability(Capabilities::TWO_SIDED_STENCIL);
    caps.setCapability(Capabilities::HWOCCLUSION);
    caps.setCapability(Capabilities::HWRENDER_TO_TEXTURE);
    caps.setCapability(Capabilities::TEXTURE_FLOAT);
    caps.setCapability(Capabilities::NON_POWER_OF_2_TEXTURES);
    caps.setCapability(Capabilities::TEXTURE_3D);
    caps.setCapability(Capabilities::POINT_EXTENDED_PARAMETERS);
    caps.setCapability(Capabilities::MIPMAP_LOD_BIAS);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION_DXT);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION_VTC);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION_PVRTC);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION_BC4_BC5);
    caps.setCapability(Capabilities::TEXTURE_COMPRESSION_BC6H_BC7);
    caps.setCapability(Capabilities::PERSTAGECONSTANT);
    caps.setCapability(Capabilities::SEPARATE_SHADER_OBJECTS);
    caps.setCapability(Capabilities::VAO);

    caps.setNumTextureUnits(22);
    caps.setNumMultiRenderTargets(23);

    caps.addShaderProfile("99foo100");

    // try out stranger names
    caps.addShaderProfile("..f(_)specialsymbolextravaganza!@#$%^&*_but_no_spaces");

    caps.setVertexProgramConstantFloatCount(1111);
    caps.setFragmentProgramConstantFloatCount(4444);

    caps.setMaxPointSize(123.75);
    caps.setNonPOW2TexturesLimited(true);

    DriverVersion driverversion;
    driverversion.major = 11;
    driverversion.minor = 13;
    driverversion.release = 17;
    driverversion.build = 0;

    caps.setDriverVersion(driverversion);
    caps.setDeviceName("Dummy Device");
    caps.setRenderSystemName("Dummy RenderSystem");

    // write them to file
    serializer.writeScript(&caps, name, filename);

    auto* fdatastream = new FileStreamDataStream(filename,
            new ifstream(filename.c_str()));

    DataStreamPtr dataStreamPtr(fdatastream);

    // parsing does not return a raw RSC, but adds it to the Manager
    serializer.parseScript(dataStreamPtr);

    RenderSystemCapabilitiesManager* rscManager = RenderSystemCapabilitiesManager::getSingletonPtr();

    RenderSystemCapabilities* rsc = rscManager->loadParsedCapabilities(name);
    // confirm that RSC was loaded
    EXPECT_TRUE(rsc != nullptr);

    // create a reference, so that were are working with two refs
    RenderSystemCapabilities& caps2 = *rsc;

    EXPECT_EQ(caps.hasCapability(Capabilities::ANISOTROPY), caps2.hasCapability(Capabilities::ANISOTROPY));
    EXPECT_EQ(caps.hasCapability(Capabilities::HWSTENCIL), caps2.hasCapability(Capabilities::HWSTENCIL));

    EXPECT_EQ(caps.hasCapability(Capabilities::TWO_SIDED_STENCIL), caps2.hasCapability(Capabilities::TWO_SIDED_STENCIL));
    EXPECT_EQ(caps.hasCapability(Capabilities::STENCIL_WRAP), caps2.hasCapability(Capabilities::STENCIL_WRAP));

    EXPECT_EQ(caps.hasCapability(Capabilities::HWOCCLUSION), caps2.hasCapability(Capabilities::HWOCCLUSION));
    EXPECT_EQ(caps.hasCapability(Capabilities::USER_CLIP_PLANES), caps2.hasCapability(Capabilities::USER_CLIP_PLANES));
    EXPECT_EQ(caps.hasCapability(Capabilities::HWRENDER_TO_TEXTURE), caps2.hasCapability(Capabilities::HWRENDER_TO_TEXTURE));
    EXPECT_EQ(caps.hasCapability(Capabilities::TEXTURE_FLOAT), caps2.hasCapability(Capabilities::TEXTURE_FLOAT));

    EXPECT_EQ(caps.hasCapability(Capabilities::NON_POWER_OF_2_TEXTURES), caps2.hasCapability(Capabilities::NON_POWER_OF_2_TEXTURES));
    EXPECT_EQ(caps.hasCapability(Capabilities::TEXTURE_3D), caps2.hasCapability(Capabilities::TEXTURE_3D));
    EXPECT_EQ(caps.hasCapability(Capabilities::POINT_SPRITES), caps2.hasCapability(Capabilities::POINT_SPRITES));
    EXPECT_EQ(caps.hasCapability(Capabilities::POINT_EXTENDED_PARAMETERS), caps2.hasCapability(Capabilities::POINT_EXTENDED_PARAMETERS));
    EXPECT_EQ(caps.hasCapability(Capabilities::VERTEX_TEXTURE_FETCH), caps2.hasCapability(Capabilities::VERTEX_TEXTURE_FETCH));
    EXPECT_EQ(caps.hasCapability(Capabilities::MIPMAP_LOD_BIAS), caps2.hasCapability(Capabilities::MIPMAP_LOD_BIAS));

    EXPECT_EQ(caps.hasCapability(Capabilities::TEXTURE_COMPRESSION), caps2.hasCapability(Capabilities::TEXTURE_COMPRESSION));
    EXPECT_EQ(caps.hasCapability(Capabilities::TEXTURE_COMPRESSION_DXT), caps2.hasCapability(Capabilities::TEXTURE_COMPRESSION_DXT));
    EXPECT_EQ(caps.hasCapability(Capabilities::TEXTURE_COMPRESSION_VTC), caps2.hasCapability(Capabilities::TEXTURE_COMPRESSION_VTC));
    EXPECT_EQ(caps.hasCapability(Capabilities::TEXTURE_COMPRESSION_PVRTC), caps2.hasCapability(Capabilities::TEXTURE_COMPRESSION_PVRTC));
    EXPECT_EQ(caps.hasCapability(Capabilities::TEXTURE_COMPRESSION_BC4_BC5), caps2.hasCapability(Capabilities::TEXTURE_COMPRESSION_BC4_BC5));
    EXPECT_EQ(caps.hasCapability(Capabilities::TEXTURE_COMPRESSION_BC6H_BC7), caps2.hasCapability(Capabilities::TEXTURE_COMPRESSION_BC6H_BC7));

    EXPECT_EQ(caps.hasCapability(Capabilities::PBUFFER), caps2.hasCapability(Capabilities::PBUFFER));
    EXPECT_EQ(caps.hasCapability(Capabilities::PERSTAGECONSTANT), caps2.hasCapability(Capabilities::PERSTAGECONSTANT));
    EXPECT_EQ(caps.hasCapability(Capabilities::SEPARATE_SHADER_OBJECTS), caps2.hasCapability(Capabilities::SEPARATE_SHADER_OBJECTS));
    EXPECT_EQ(caps.hasCapability(Capabilities::VAO), caps2.hasCapability(Capabilities::VAO));

    EXPECT_EQ(caps.getNumTextureUnits(), caps2.getNumTextureUnits());
    EXPECT_EQ(caps.getNumMultiRenderTargets(), caps2.getNumMultiRenderTargets());

    EXPECT_EQ(caps.getVertexProgramConstantFloatCount(), caps2.getVertexProgramConstantFloatCount());
    EXPECT_EQ(caps.getFragmentProgramConstantFloatCount(), caps2.getFragmentProgramConstantFloatCount());

    EXPECT_EQ(caps.getMaxPointSize(), caps2.getMaxPointSize());
    EXPECT_EQ(caps.getNonPOW2TexturesLimited(), caps2.getNonPOW2TexturesLimited());
    
    // test versions
    EXPECT_EQ(caps.getDriverVersion().major, caps2.getDriverVersion().major);
    EXPECT_EQ(caps.getDriverVersion().minor, caps2.getDriverVersion().minor);
    EXPECT_EQ(caps.getDriverVersion().release, caps2.getDriverVersion().release);
    EXPECT_EQ(0, caps2.getDriverVersion().build);

    dataStreamPtr.reset();
}
//--------------------------------------------------------------------------
