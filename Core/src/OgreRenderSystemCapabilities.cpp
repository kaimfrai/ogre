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
#include <ostream>

#include "OgreLog.hpp"
#include "OgreRenderSystemCapabilities.hpp"
#include "OgreString.hpp"
#include "OgreStringConverter.hpp"
#include "OgreStringVector.hpp"

namespace Ogre {

    auto DriverVersion::toString() const -> String
    {
        StringStream str;
        str << major << "." << minor << "." << release << "." << build;
        return str.str();
    }

    void  DriverVersion::fromString(const String& versionString)
    {
        StringVector tokens = StringUtil::split(versionString, ".");
        if(!tokens.empty())
        {
            major = StringConverter::parseInt(tokens[0]);
            if (tokens.size() > 1)
                minor = StringConverter::parseInt(tokens[1]);
            if (tokens.size() > 2)
                release = StringConverter::parseInt(tokens[2]);
            if (tokens.size() > 3)
                build = StringConverter::parseInt(tokens[3]);
        }
    }

    //-----------------------------------------------------------------------
    RenderSystemCapabilities::RenderSystemCapabilities()
         
    {
        for(int & mCapabilitie : mCapabilities)
        {
            mCapabilitie = 0;
        }
        mCategoryRelevant[CAPS_CATEGORY_COMMON] = true;
        mCategoryRelevant[CAPS_CATEGORY_COMMON_2] = true;
        // each rendersystem should enable these
        mCategoryRelevant[CAPS_CATEGORY_D3D9] = false;
        mCategoryRelevant[CAPS_CATEGORY_GL] = false;
    }

    void RenderSystemCapabilities::addShaderProfile(const String& profile) { mSupportedShaderProfiles.insert(profile); }

    void RenderSystemCapabilities::removeShaderProfile(const String& profile)
    {
        mSupportedShaderProfiles.erase(profile);
    }

    auto RenderSystemCapabilities::isShaderProfileSupported(const String& profile) const -> bool
    {
        return (mSupportedShaderProfiles.end() != mSupportedShaderProfiles.find(profile));
    }

    //-----------------------------------------------------------------------
    void RenderSystemCapabilities::log(Log* pLog) const
    {
        pLog->logMessage("RenderSystem capabilities");
        pLog->logMessage("-------------------------");
        pLog->logMessage(::std::format("RenderSystem Name: {}", getRenderSystemName()));
        pLog->logMessage(::std::format("GPU Vendor: {}", vendorToString(getVendor())));
        pLog->logMessage(::std::format("Device Name: {}", getDeviceName()));
        pLog->logMessage(::std::format("Driver Version: {}", getDriverVersion().toString()));
        pLog->logMessage(::std::format(" * Fixed function pipeline: {}", StringConverter::toString(hasCapability(RSC_FIXED_FUNCTION)), true));
        pLog->logMessage(::std::format(" * 32-bit index buffers: {}", StringConverter::toString(hasCapability(RSC_32BIT_INDEX)), true));
        pLog->logMessage(::std::format(" * Hardware stencil buffer: {}", StringConverter::toString(hasCapability(RSC_HWSTENCIL)), true));
        if (hasCapability(RSC_HWSTENCIL))
        {
            pLog->logMessage(::std::format("   - Two sided stencil support: {}", StringConverter::toString(hasCapability(RSC_TWO_SIDED_STENCIL)), true));
            pLog->logMessage(::std::format("   - Wrap stencil values: {}", StringConverter::toString(hasCapability(RSC_STENCIL_WRAP)), true));
        }
        pLog->logMessage(" * Vertex programs: yes");
        pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", StringConverter::toString(mVertexProgramConstantFloatCount)));
        pLog->logMessage(" * Fragment programs: yes");
        pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", StringConverter::toString(mFragmentProgramConstantFloatCount)));
        pLog->logMessage(::std::format(" * Geometry programs: {}", StringConverter::toString(hasCapability(RSC_GEOMETRY_PROGRAM)), true));
        if (hasCapability(RSC_GEOMETRY_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", StringConverter::toString(mGeometryProgramConstantFloatCount)));
        }
        pLog->logMessage(::std::format(" * Tessellation Hull programs: {}", StringConverter::toString(hasCapability(RSC_TESSELLATION_HULL_PROGRAM)), true));
        if (hasCapability(RSC_TESSELLATION_HULL_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", StringConverter::toString(mTessellationHullProgramConstantFloatCount)));
        }
        pLog->logMessage(::std::format(" * Tessellation Domain programs: {}", StringConverter::toString(hasCapability(RSC_TESSELLATION_DOMAIN_PROGRAM)), true));
        if (hasCapability(RSC_TESSELLATION_DOMAIN_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", StringConverter::toString(mTessellationDomainProgramConstantFloatCount)));
        }
        pLog->logMessage(::std::format(" * Compute programs: {}", StringConverter::toString(hasCapability(RSC_COMPUTE_PROGRAM)), true));
        if (hasCapability(RSC_COMPUTE_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", StringConverter::toString(mComputeProgramConstantFloatCount)));
        }
        pLog->logMessage(
            ::std::format(" * Supported Shader Profiles: {}", StringConverter::toString(StringVector(mSupportedShaderProfiles.begin(), mSupportedShaderProfiles.end()))));
        pLog->logMessage(::std::format(" * Read-back compiled shader: {}", StringConverter::toString(hasCapability(RSC_CAN_GET_COMPILED_SHADER_BUFFER)), true));
        pLog->logMessage(::std::format(" * Number of vertex attributes: {}", StringConverter::toString(mNumVertexAttributes)));
        pLog->logMessage(" * Textures");
        pLog->logMessage(::std::format("   - Number of texture units: {}", StringConverter::toString(mNumTextureUnits)));
        pLog->logMessage(::std::format("   - Floating point: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_FLOAT)), true));
        pLog->logMessage(
            ::std::format("   - Non-power-of-two: {}", StringConverter::toString(hasCapability(RSC_NON_POWER_OF_2_TEXTURES)), true) +
            (mNonPOW2TexturesLimited ? " (limited)" : ""));
        pLog->logMessage(::std::format("   - 1D textures: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_1D)), true));
        pLog->logMessage(::std::format("   - 2D array textures: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_2D_ARRAY)), true));
        pLog->logMessage(::std::format("   - 3D textures: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_3D)), true));
        pLog->logMessage(::std::format("   - Anisotropic filtering: {}", StringConverter::toString(hasCapability(RSC_ANISOTROPY)), true));

        pLog->logMessage(
            " * Texture Compression: "
            + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION), true));
        if (hasCapability(RSC_TEXTURE_COMPRESSION))
        {
            pLog->logMessage(::std::format("   - DXT: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_DXT)), true));
            pLog->logMessage(::std::format("   - VTC: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_VTC)), true));
            pLog->logMessage(::std::format("   - PVRTC: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_PVRTC)), true));
            pLog->logMessage(::std::format("   - ATC: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_ATC)), true));
            pLog->logMessage(::std::format("   - ETC1: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_ETC1)), true));
            pLog->logMessage(::std::format("   - ETC2: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_ETC2)), true));
            pLog->logMessage(::std::format("   - BC4/BC5: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_BC4_BC5)), true));
            pLog->logMessage(::std::format("   - BC6H/BC7: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_BC6H_BC7)), true));
            pLog->logMessage(::std::format("   - ASTC: {}", StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_ASTC)), true));
            pLog->logMessage(::std::format("   - Automatic mipmap generation: {}", StringConverter::toString(hasCapability(RSC_AUTOMIPMAP_COMPRESSED)), true));
        }

        pLog->logMessage(" * Vertex Buffers");
        pLog->logMessage(::std::format("   - Render to Vertex Buffer: {}", StringConverter::toString(hasCapability(RSC_HWRENDER_TO_VERTEX_BUFFER)), true));
        pLog->logMessage(::std::format("   - Instance Data: {}", StringConverter::toString(hasCapability(RSC_VERTEX_BUFFER_INSTANCE_DATA)), true));
        pLog->logMessage(::std::format("   - Primitive Restart: {}", StringConverter::toString(hasCapability(RSC_PRIMITIVE_RESTART)), true));
        pLog->logMessage(::std::format(" * Vertex texture fetch: {}", StringConverter::toString(hasCapability(RSC_VERTEX_TEXTURE_FETCH)), true));
        if (hasCapability(RSC_VERTEX_TEXTURE_FETCH))
        {
            pLog->logMessage(::std::format("   - Max vertex textures: {}", StringConverter::toString(mNumVertexTextureUnits)));
            pLog->logMessage(::std::format("   - Vertex textures shared: {}", StringConverter::toString(mVertexTextureUnitsShared, true)));
        }
        pLog->logMessage(::std::format(" * Read/Write Buffers: {}", StringConverter::toString(hasCapability(RSC_READ_WRITE_BUFFERS)), true));
        pLog->logMessage(
            " * Hardware Occlusion Query: "
            + StringConverter::toString(hasCapability(RSC_HWOCCLUSION), true));
        pLog->logMessage(
            " * User clip planes: "
            + StringConverter::toString(hasCapability(RSC_USER_CLIP_PLANES), true));
        pLog->logMessage(
            " * Depth clamping: "
            + StringConverter::toString(hasCapability(RSC_DEPTH_CLAMP), true));
        pLog->logMessage(
            " * Hardware render-to-texture: "
            + StringConverter::toString(hasCapability(RSC_HWRENDER_TO_TEXTURE), true));
        pLog->logMessage(::std::format("   - Multiple Render Targets: {}", StringConverter::toString(mNumMultiRenderTargets)));
        pLog->logMessage(::std::format("   - With different bit depths: {}", StringConverter::toString(hasCapability(RSC_MRT_DIFFERENT_BIT_DEPTHS)), true));
        pLog->logMessage(::std::format(" * Point Sprites: {}", StringConverter::toString(hasCapability(RSC_POINT_SPRITES)), true));
        if (hasCapability(RSC_POINT_SPRITES))
        {
            pLog->logMessage(::std::format("   - Extended parameters: {}", StringConverter::toString(hasCapability(RSC_POINT_EXTENDED_PARAMETERS)), true));
            pLog->logMessage(::std::format("   - Max Size: {}", StringConverter::toString(mMaxPointSize)));
        }
        pLog->logMessage(
            " * Wide Lines: "
            + StringConverter::toString(hasCapability(RSC_WIDE_LINES), true));
        pLog->logMessage(
            " * Hardware Gamma: "
            + StringConverter::toString(hasCapability(RSC_HW_GAMMA), true));
        if (mCategoryRelevant[CAPS_CATEGORY_GL])
        {
            pLog->logMessage(
                " * PBuffer support: "
                + StringConverter::toString(hasCapability(RSC_PBUFFER), true));
            pLog->logMessage(
                " * Vertex Array Objects: "
                + StringConverter::toString(hasCapability(RSC_VAO), true));
            pLog->logMessage(::std::format(" * Separate shader objects: {}", StringConverter::toString(hasCapability(RSC_SEPARATE_SHADER_OBJECTS)), true));
            pLog->logMessage(::std::format("   - redeclare GLSL interface block: {}", StringConverter::toString(hasCapability(RSC_GLSL_SSO_REDECLARE)), true));
            pLog->logMessage(
                " * Debugging/ profiling events: "
                + StringConverter::toString(hasCapability(RSC_DEBUG), true));
            pLog->logMessage(
                " * Map buffer storage: "
                + StringConverter::toString(hasCapability(RSC_MAPBUFFER), true));
        }

        if (mCategoryRelevant[CAPS_CATEGORY_D3D9])
        {
            pLog->logMessage(
                " * DirectX per stage constants: "
                + StringConverter::toString(hasCapability(RSC_PERSTAGECONSTANT), true));
            pLog->logMessage(
                " * W-Buffer supported: "
                + StringConverter::toString(hasCapability(RSC_WBUFFER), true));
        }
    }
    //---------------------------------------------------------------------
    String RenderSystemCapabilities::msGPUVendorStrings[GPU_VENDOR_COUNT];
    //---------------------------------------------------------------------
    auto RenderSystemCapabilities::vendorFromString(const String& vendorString) -> GPUVendor
    {
        initVendorStrings();
        GPUVendor ret = GPU_UNKNOWN;
        String cmpString = vendorString;
        StringUtil::toLowerCase(cmpString);
        for (int i = 0; i < GPU_VENDOR_COUNT; ++i)
        {
            // case insensitive (lower case)
            if (msGPUVendorStrings[i] == cmpString)
            {
                ret = static_cast<GPUVendor>(i);
                break;
            }
        }

        return ret;
        
    }
    //---------------------------------------------------------------------
    auto RenderSystemCapabilities::vendorToString(GPUVendor v) -> const String&
    {
        initVendorStrings();
        return msGPUVendorStrings[v];
    }
    //---------------------------------------------------------------------
    void RenderSystemCapabilities::initVendorStrings()
    {
        if (msGPUVendorStrings[0].empty())
        {
            // Always lower case!
            msGPUVendorStrings[GPU_UNKNOWN] = "unknown";
            msGPUVendorStrings[GPU_NVIDIA] = "nvidia";
            msGPUVendorStrings[GPU_AMD] = "amd";
            msGPUVendorStrings[GPU_INTEL] = "intel";
            msGPUVendorStrings[GPU_IMAGINATION_TECHNOLOGIES] = "imagination technologies";
            msGPUVendorStrings[GPU_APPLE] = "apple";    // iOS Simulator
            msGPUVendorStrings[GPU_NOKIA] = "nokia";
            msGPUVendorStrings[GPU_MS_SOFTWARE] = "microsoft"; // Microsoft software device
            msGPUVendorStrings[GPU_MS_WARP] = "ms warp";
            msGPUVendorStrings[GPU_ARM] = "arm";
            msGPUVendorStrings[GPU_QUALCOMM] = "qualcomm";
            msGPUVendorStrings[GPU_MOZILLA] = "mozilla";
            msGPUVendorStrings[GPU_WEBKIT] = "webkit";
        }
    }

}
