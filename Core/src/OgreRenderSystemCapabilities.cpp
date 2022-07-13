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

    void  DriverVersion::fromString(std::string_view versionString)
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
        mCategoryRelevant[std::to_underlying(CapabilitiesCategory::COMMON)] = true;
        mCategoryRelevant[std::to_underlying(CapabilitiesCategory::COMMON_2)] = true;
        // each rendersystem should enable these
        mCategoryRelevant[std::to_underlying(CapabilitiesCategory::D3D9)] = false;
        mCategoryRelevant[std::to_underlying(CapabilitiesCategory::GL)] = false;
    }

    void RenderSystemCapabilities::addShaderProfile(std::string_view profile) { mSupportedShaderProfiles.emplace(profile); }

    void RenderSystemCapabilities::removeShaderProfile(std::string_view profile)
    {
        mSupportedShaderProfiles.erase(mSupportedShaderProfiles.find(profile));
    }

    auto RenderSystemCapabilities::isShaderProfileSupported(std::string_view profile) const -> bool
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
        pLog->logMessage(::std::format(" * Fixed function pipeline: {}", hasCapability(Capabilities::FIXED_FUNCTION)));;
        pLog->logMessage(::std::format(" * 32-bit index buffers: {}", hasCapability(Capabilities::_32BIT_INDEX)));;
        pLog->logMessage(::std::format(" * Hardware stencil buffer: {}", hasCapability(Capabilities::HWSTENCIL)));;
        if (hasCapability(Capabilities::HWSTENCIL))
        {
            pLog->logMessage(::std::format("   - Two sided stencil support: {}", hasCapability(Capabilities::TWO_SIDED_STENCIL)));;
            pLog->logMessage(::std::format("   - Wrap stencil values: {}", hasCapability(Capabilities::STENCIL_WRAP)));;
        }
        pLog->logMessage(" * Vertex programs: yes");
        pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mVertexProgramConstantFloatCount));
        pLog->logMessage(" * Fragment programs: yes");
        pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mFragmentProgramConstantFloatCount));
        pLog->logMessage(::std::format(" * Geometry programs: {}", hasCapability(Capabilities::GEOMETRY_PROGRAM)));;
        if (hasCapability(Capabilities::GEOMETRY_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mGeometryProgramConstantFloatCount));
        }
        pLog->logMessage(::std::format(" * Tessellation Hull programs: {}", hasCapability(Capabilities::TESSELLATION_HULL_PROGRAM)));;
        if (hasCapability(Capabilities::TESSELLATION_HULL_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mTessellationHullProgramConstantFloatCount));
        }
        pLog->logMessage(::std::format(" * Tessellation Domain programs: {}", hasCapability(Capabilities::TESSELLATION_DOMAIN_PROGRAM)));;
        if (hasCapability(Capabilities::TESSELLATION_DOMAIN_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mTessellationDomainProgramConstantFloatCount));
        }
        pLog->logMessage(::std::format(" * Compute programs: {}", hasCapability(Capabilities::COMPUTE_PROGRAM)));;
        if (hasCapability(Capabilities::COMPUTE_PROGRAM))
        {
            pLog->logMessage(::std::format("   - Number of constant 4-vectors: {}", mComputeProgramConstantFloatCount));
        }
        pLog->logMessage(
            ::std::format(" * Supported Shader Profiles: {}", StringVector(mSupportedShaderProfiles.begin(), mSupportedShaderProfiles.end())));
        pLog->logMessage(::std::format(" * Read-back compiled shader: {}", hasCapability(Capabilities::CAN_GET_COMPILED_SHADER_BUFFER)));;
        pLog->logMessage(::std::format(" * Number of vertex attributes: {}", mNumVertexAttributes));
        pLog->logMessage(" * Textures");
        pLog->logMessage(::std::format("   - Number of texture units: {}", mNumTextureUnits));
        pLog->logMessage(::std::format("   - Floating point: {}", hasCapability(Capabilities::TEXTURE_FLOAT)));;
        pLog->logMessage(
            ::std::format("   - Non-power-of-two: {}", hasCapability(Capabilities::NON_POWER_OF_2_TEXTURES), true) +
            (mNonPOW2TexturesLimited ? " (limited)" : ""));
        pLog->logMessage(::std::format("   - 1D textures: {}", hasCapability(Capabilities::TEXTURE_1D)));;
        pLog->logMessage(::std::format("   - 2D array textures: {}", hasCapability(Capabilities::TEXTURE_2D_ARRAY)));;
        pLog->logMessage(::std::format("   - 3D textures: {}", hasCapability(Capabilities::TEXTURE_3D)));;
        pLog->logMessage(::std::format("   - Anisotropic filtering: {}", hasCapability(Capabilities::ANISOTROPY)));;

        pLog->logMessage(::std::format(" * Texture Compression: {}", hasCapability(Capabilities::TEXTURE_COMPRESSION)));;
        if (hasCapability(Capabilities::TEXTURE_COMPRESSION))
        {
            pLog->logMessage(::std::format("   - DXT: {}", hasCapability(Capabilities::TEXTURE_COMPRESSION_DXT)));;
            pLog->logMessage(::std::format("   - VTC: {}", hasCapability(Capabilities::TEXTURE_COMPRESSION_VTC)));;
            pLog->logMessage(::std::format("   - PVRTC: {}", hasCapability(Capabilities::TEXTURE_COMPRESSION_PVRTC)));;
            pLog->logMessage(::std::format("   - ATC: {}", hasCapability(Capabilities::TEXTURE_COMPRESSION_ATC)));;
            pLog->logMessage(::std::format("   - ETC1: {}", hasCapability(Capabilities::TEXTURE_COMPRESSION_ETC1)));;
            pLog->logMessage(::std::format("   - ETC2: {}", hasCapability(Capabilities::TEXTURE_COMPRESSION_ETC2)));;
            pLog->logMessage(::std::format("   - BC4/BC5: {}", hasCapability(Capabilities::TEXTURE_COMPRESSION_BC4_BC5)));;
            pLog->logMessage(::std::format("   - BC6H/BC7: {}", hasCapability(Capabilities::TEXTURE_COMPRESSION_BC6H_BC7)));;
            pLog->logMessage(::std::format("   - ASTC: {}", hasCapability(Capabilities::TEXTURE_COMPRESSION_ASTC)));;
            pLog->logMessage(::std::format("   - Automatic mipmap generation: {}", hasCapability(Capabilities::AUTOMIPMAP_COMPRESSED)));;
        }

        pLog->logMessage(" * Vertex Buffers");
        pLog->logMessage(::std::format("   - Render to Vertex Buffer: {}", hasCapability(Capabilities::HWRENDER_TO_VERTEX_BUFFER)));;
        pLog->logMessage(::std::format("   - Instance Data: {}", hasCapability(Capabilities::VERTEX_BUFFER_INSTANCE_DATA)));;
        pLog->logMessage(::std::format("   - Primitive Restart: {}", hasCapability(Capabilities::PRIMITIVE_RESTART)));;
        pLog->logMessage(::std::format(" * Vertex texture fetch: {}", hasCapability(Capabilities::VERTEX_TEXTURE_FETCH)));;
        if (hasCapability(Capabilities::VERTEX_TEXTURE_FETCH))
        {
            pLog->logMessage(::std::format("   - Max vertex textures: {}", mNumVertexTextureUnits));
            pLog->logMessage(::std::format("   - Vertex textures shared: {}", mVertexTextureUnitsShared));;
        }
        pLog->logMessage(::std::format(" * Read/Write Buffers: {}", hasCapability(Capabilities::READ_WRITE_BUFFERS)));;
        pLog->logMessage(::std::format(" * Hardware Occlusion Query: ", hasCapability(Capabilities::HWOCCLUSION)));;
        pLog->logMessage(::std::format(" * User clip planes: ", hasCapability(Capabilities::USER_CLIP_PLANES)));;
        pLog->logMessage(::std::format(" * Depth clamping: ", hasCapability(Capabilities::DEPTH_CLAMP)));;
        pLog->logMessage(::std::format(" * Hardware render-to-texture: ", hasCapability(Capabilities::HWRENDER_TO_TEXTURE)));;
        pLog->logMessage(::std::format("   - Multiple Render Targets: {}", mNumMultiRenderTargets));
        pLog->logMessage(::std::format("   - With different bit depths: {}", hasCapability(Capabilities::MRT_DIFFERENT_BIT_DEPTHS)));;
        pLog->logMessage(::std::format(" * Point Sprites: {}", hasCapability(Capabilities::POINT_SPRITES)));;
        if (hasCapability(Capabilities::POINT_SPRITES))
        {
            pLog->logMessage(::std::format("   - Extended parameters: {}", hasCapability(Capabilities::POINT_EXTENDED_PARAMETERS)));;
            pLog->logMessage(::std::format("   - Max Size: {}", mMaxPointSize));
        }
        pLog->logMessage(::std::format(" * Wide Lines: ", hasCapability(Capabilities::WIDE_LINES)));;
        pLog->logMessage(::std::format(" * Hardware Gamma: ", hasCapability(Capabilities::HW_GAMMA)));;
        if (mCategoryRelevant[std::to_underlying(CapabilitiesCategory::GL)])
        {
            pLog->logMessage(::std::format(" * PBuffer support: ", hasCapability(Capabilities::PBUFFER)));;
            pLog->logMessage(::std::format(" * Vertex Array Objects: ", hasCapability(Capabilities::VAO)));;
            pLog->logMessage(::std::format(" * Separate shader objects: {}", hasCapability(Capabilities::SEPARATE_SHADER_OBJECTS)));;
            pLog->logMessage(::std::format("   - redeclare GLSL interface block: {}", hasCapability(Capabilities::GLSL_SSO_REDECLARE)));;
            pLog->logMessage(::std::format(" * Debugging/ profiling events: ", hasCapability(Capabilities::DEBUG)));;
            pLog->logMessage(::std::format(" * Map buffer storage: ", hasCapability(Capabilities::MAPBUFFER)));;
        }

        if (mCategoryRelevant[std::to_underlying(CapabilitiesCategory::D3D9)])
        {
            pLog->logMessage(::std::format(" * DirectX per stage constants: ", hasCapability(Capabilities::PERSTAGECONSTANT)));;
            pLog->logMessage(::std::format(" * W-Buffer supported: ", hasCapability(Capabilities::WBUFFER)));;
        }
    }
    //---------------------------------------------------------------------
    std::string_view RenderSystemCapabilities::msGPUVendorStrings[std::to_underlying(GPUVendor::VENDOR_COUNT)];
    //---------------------------------------------------------------------
    auto RenderSystemCapabilities::vendorFromString(std::string_view vendorString) -> GPUVendor
    {
        initVendorStrings();
        GPUVendor ret = GPUVendor::UNKNOWN;
        String cmpString{ vendorString };
        StringUtil::toLowerCase(cmpString);
        for (int i = 0; i < std::to_underlying(GPUVendor::VENDOR_COUNT); ++i)
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
    auto RenderSystemCapabilities::vendorToString(GPUVendor v) -> std::string_view
    {
        initVendorStrings();
        return msGPUVendorStrings[std::to_underlying(v)];
    }
    //---------------------------------------------------------------------
    void RenderSystemCapabilities::initVendorStrings()
    {
        if (msGPUVendorStrings[0].empty())
        {
            // Always lower case!
            msGPUVendorStrings[std::to_underlying(GPUVendor::UNKNOWN)] = "unknown";
            msGPUVendorStrings[std::to_underlying(GPUVendor::NVIDIA)] = "nvidia";
            msGPUVendorStrings[std::to_underlying(GPUVendor::AMD)] = "amd";
            msGPUVendorStrings[std::to_underlying(GPUVendor::INTEL)] = "intel";
            msGPUVendorStrings[std::to_underlying(GPUVendor::IMAGINATION_TECHNOLOGIES)] = "imagination technologies";
            msGPUVendorStrings[std::to_underlying(GPUVendor::APPLE)] = "apple";    // iOS Simulator
            msGPUVendorStrings[std::to_underlying(GPUVendor::NOKIA)] = "nokia";
            msGPUVendorStrings[std::to_underlying(GPUVendor::MS_SOFTWARE)] = "microsoft"; // Microsoft software device
            msGPUVendorStrings[std::to_underlying(GPUVendor::MS_WARP)] = "ms warp";
            msGPUVendorStrings[std::to_underlying(GPUVendor::ARM)] = "arm";
            msGPUVendorStrings[std::to_underlying(GPUVendor::QUALCOMM)] = "qualcomm";
            msGPUVendorStrings[std::to_underlying(GPUVendor::MOZILLA)] = "mozilla";
            msGPUVendorStrings[std::to_underlying(GPUVendor::WEBKIT)] = "webkit";
        }
    }

}
