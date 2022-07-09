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
#include <format>
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

    void  DriverVersion::fromString(StringView versionString)
    {
        auto const tokens = StringUtil::split(versionString, ".");
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

    void RenderSystemCapabilities::addShaderProfile(StringView profile) { mSupportedShaderProfiles.emplace(profile); }

    void RenderSystemCapabilities::removeShaderProfile(StringView profile)
    {
        mSupportedShaderProfiles.erase(mSupportedShaderProfiles.find(profile));
    }

    auto RenderSystemCapabilities::isShaderProfileSupported(StringView profile) const -> bool
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
        pLog->logMessage(::std::format(" * Fixed function pipeline: {}", hasCapability(RSC_FIXED_FUNCTION)));;
        pLog->logMessage(::std::format(" * 32-bit index buffers: {}", hasCapability(RSC_32BIT_INDEX)));;
        pLog->logMessage(::std::format(" * Hardware stencil buffer: {}", hasCapability(RSC_HWSTENCIL)));;
        if (hasCapability(RSC_HWSTENCIL))
        {
            pLog->logMessage(::std::format("   - Two sided stencil support: {}", hasCapability(RSC_TWO_SIDED_STENCIL)));;
            pLog->logMessage(::std::format("   - Wrap stencil values: {}", hasCapability(RSC_STENCIL_WRAP)));;
        }
        pLog->logMessage(" * Vertex programs: yes");
        pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mVertexProgramConstantFloatCount));
        pLog->logMessage(" * Fragment programs: yes");
        pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mFragmentProgramConstantFloatCount));
        pLog->logMessage(::std::format(" * Geometry programs: {}", hasCapability(RSC_GEOMETRY_PROGRAM)));;
        if (hasCapability(RSC_GEOMETRY_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mGeometryProgramConstantFloatCount));
        }
        pLog->logMessage(::std::format(" * Tessellation Hull programs: {}", hasCapability(RSC_TESSELLATION_HULL_PROGRAM)));;
        if (hasCapability(RSC_TESSELLATION_HULL_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mTessellationHullProgramConstantFloatCount));
        }
        pLog->logMessage(::std::format(" * Tessellation Domain programs: {}", hasCapability(RSC_TESSELLATION_DOMAIN_PROGRAM)));;
        if (hasCapability(RSC_TESSELLATION_DOMAIN_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mTessellationDomainProgramConstantFloatCount));
        }
        pLog->logMessage(::std::format(" * Compute programs: {}", hasCapability(RSC_COMPUTE_PROGRAM)));;
        if (hasCapability(RSC_COMPUTE_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mComputeProgramConstantFloatCount));
        }
        pLog->logMessage(
            ::std::format(" * Supported Shader Profiles: {}", StringVector(mSupportedShaderProfiles.begin(), mSupportedShaderProfiles.end())));
        pLog->logMessage(::std::format(" * Read-back compiled shader: {}", hasCapability(RSC_CAN_GET_COMPILED_SHADER_BUFFER)));;
        pLog->logMessage(::std::format(" * Number of vertex attributes: {}", mNumVertexAttributes));
        pLog->logMessage(" * Textures");
        pLog->logMessage(::std::format("   - Number of texture units: {}", mNumTextureUnits));
        pLog->logMessage(::std::format("   - Floating point: {}", hasCapability(RSC_TEXTURE_FLOAT)));;
        pLog->logMessage(
            ::std::format("   - Non-power-of-two: {}", hasCapability(RSC_NON_POWER_OF_2_TEXTURES), true) +
            (mNonPOW2TexturesLimited ? " (limited)" : ""));
        pLog->logMessage(::std::format("   - 1D textures: {}", hasCapability(RSC_TEXTURE_1D)));;
        pLog->logMessage(::std::format("   - 2D array textures: {}", hasCapability(RSC_TEXTURE_2D_ARRAY)));;
        pLog->logMessage(::std::format("   - 3D textures: {}", hasCapability(RSC_TEXTURE_3D)));;
        pLog->logMessage(::std::format("   - Anisotropic filtering: {}", hasCapability(RSC_ANISOTROPY)));;

        pLog->logMessage(::std::format(" * Texture Compression: {}", hasCapability(RSC_TEXTURE_COMPRESSION)));;
        if (hasCapability(RSC_TEXTURE_COMPRESSION))
        {
            pLog->logMessage(::std::format("   - DXT: {}", hasCapability(RSC_TEXTURE_COMPRESSION_DXT)));;
            pLog->logMessage(::std::format("   - VTC: {}", hasCapability(RSC_TEXTURE_COMPRESSION_VTC)));;
            pLog->logMessage(::std::format("   - PVRTC: {}", hasCapability(RSC_TEXTURE_COMPRESSION_PVRTC)));;
            pLog->logMessage(::std::format("   - ATC: {}", hasCapability(RSC_TEXTURE_COMPRESSION_ATC)));;
            pLog->logMessage(::std::format("   - ETC1: {}", hasCapability(RSC_TEXTURE_COMPRESSION_ETC1)));;
            pLog->logMessage(::std::format("   - ETC2: {}", hasCapability(RSC_TEXTURE_COMPRESSION_ETC2)));;
            pLog->logMessage(::std::format("   - BC4/BC5: {}", hasCapability(RSC_TEXTURE_COMPRESSION_BC4_BC5)));;
            pLog->logMessage(::std::format("   - BC6H/BC7: {}", hasCapability(RSC_TEXTURE_COMPRESSION_BC6H_BC7)));;
            pLog->logMessage(::std::format("   - ASTC: {}", hasCapability(RSC_TEXTURE_COMPRESSION_ASTC)));;
            pLog->logMessage(::std::format("   - Automatic mipmap generation: {}", hasCapability(RSC_AUTOMIPMAP_COMPRESSED)));;
        }

        pLog->logMessage(" * Vertex Buffers");
        pLog->logMessage(::std::format("   - Render to Vertex Buffer: {}", hasCapability(RSC_HWRENDER_TO_VERTEX_BUFFER)));;
        pLog->logMessage(::std::format("   - Instance Data: {}", hasCapability(RSC_VERTEX_BUFFER_INSTANCE_DATA)));;
        pLog->logMessage(::std::format("   - Primitive Restart: {}", hasCapability(RSC_PRIMITIVE_RESTART)));;
        pLog->logMessage(::std::format(" * Vertex texture fetch: {}", hasCapability(RSC_VERTEX_TEXTURE_FETCH)));;
        if (hasCapability(RSC_VERTEX_TEXTURE_FETCH))
        {
            pLog->logMessage(::std::format("   - Max vertex textures: {}", mNumVertexTextureUnits));
            pLog->logMessage(::std::format("   - Vertex textures shared: {}", mVertexTextureUnitsShared));;
        }
        pLog->logMessage(::std::format(" * Read/Write Buffers: {}", hasCapability(RSC_READ_WRITE_BUFFERS)));;
        pLog->logMessage(::std::format(" * Hardware Occlusion Query: ", hasCapability(RSC_HWOCCLUSION)));;
        pLog->logMessage(::std::format(" * User clip planes: ", hasCapability(RSC_USER_CLIP_PLANES)));;
        pLog->logMessage(::std::format(" * Depth clamping: ", hasCapability(RSC_DEPTH_CLAMP)));;
        pLog->logMessage(::std::format(" * Hardware render-to-texture: ", hasCapability(RSC_HWRENDER_TO_TEXTURE)));;
        pLog->logMessage(::std::format("   - Multiple Render Targets: {}", mNumMultiRenderTargets));
        pLog->logMessage(::std::format("   - With different bit depths: {}", hasCapability(RSC_MRT_DIFFERENT_BIT_DEPTHS)));;
        pLog->logMessage(::std::format(" * Point Sprites: {}", hasCapability(RSC_POINT_SPRITES)));;
        if (hasCapability(RSC_POINT_SPRITES))
        {
            pLog->logMessage(::std::format("   - Extended parameters: {}", hasCapability(RSC_POINT_EXTENDED_PARAMETERS)));;
            pLog->logMessage(::std::format("   - Max Size: {}", mMaxPointSize));
        }
        pLog->logMessage(::std::format(" * Wide Lines: ", hasCapability(RSC_WIDE_LINES)));;
        pLog->logMessage(::std::format(" * Hardware Gamma: ", hasCapability(RSC_HW_GAMMA)));;
        if (mCategoryRelevant[CAPS_CATEGORY_GL])
        {
            pLog->logMessage(::std::format(" * PBuffer support: ", hasCapability(RSC_PBUFFER)));;
            pLog->logMessage(::std::format(" * Vertex Array Objects: ", hasCapability(RSC_VAO)));;
            pLog->logMessage(::std::format(" * Separate shader objects: {}", hasCapability(RSC_SEPARATE_SHADER_OBJECTS)));;
            pLog->logMessage(::std::format("   - redeclare GLSL interface block: {}", hasCapability(RSC_GLSL_SSO_REDECLARE)));;
            pLog->logMessage(::std::format(" * Debugging/ profiling events: ", hasCapability(RSC_DEBUG)));;
            pLog->logMessage(::std::format(" * Map buffer storage: ", hasCapability(RSC_MAPBUFFER)));;
        }

        if (mCategoryRelevant[CAPS_CATEGORY_D3D9])
        {
            pLog->logMessage(::std::format(" * DirectX per stage constants: ", hasCapability(RSC_PERSTAGECONSTANT)));;
            pLog->logMessage(::std::format(" * W-Buffer supported: ", hasCapability(RSC_WBUFFER)));;
        }
    }
    //---------------------------------------------------------------------
    StringView RenderSystemCapabilities::msGPUVendorStrings[GPU_VENDOR_COUNT];
    //---------------------------------------------------------------------
    auto RenderSystemCapabilities::vendorFromString(StringView vendorString) -> GPUVendor
    {
        initVendorStrings();
        GPUVendor ret = GPU_UNKNOWN;
        String cmpString{ vendorString };
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
    auto RenderSystemCapabilities::vendorToString(GPUVendor v) -> StringView
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
