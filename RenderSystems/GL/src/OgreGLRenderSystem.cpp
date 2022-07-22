/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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
module;

#include "glad/glad.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

module Ogre.RenderSystems.GL;

import :CopyingRenderTexture;
import :FBOMultiRenderTarget;
import :FBORenderTexture;
import :GLSL.SLProgramFactory;
import :GpuNvparseProgram;
import :GpuProgram;
import :GpuProgramManager;
import :HardwareBuffer;
import :HardwareBufferManager;
import :HardwareOcclusionQuery;
import :HardwarePixelBuffer;
import :PBRenderTexture;
import :PixelFormat;
import :Prerequisites;
import :RenderSystem;
import :StateCacheManager;
import :Texture;
import :TextureManager;
import :atifs.ATI_FS_GLGpuProgram;

import Ogre.Core;
import Ogre.RenderSystems.GLSupport;

import <algorithm>;
import <istream>;
import <list>;
import <map>;
import <memory>;
import <set>;
import <string>;
import <utility>;

namespace Ogre {
class HardwareOcclusionQuery;
class ResourceManager;
}  // namespace Ogre

// Convenience macro from ARB_vertex_buffer_object spec
#define VBO_BUFFER_OFFSET(i) ((char *)(i))
namespace Ogre {

    static GLNativeSupport constinit*  glsupport;
    static auto get_proc(const char* proc) -> void* {
        return glsupport->getProcAddress(proc);
    }

    using Matrix4f = TransformBase<4, float>;

    // Callback function used when registering GLGpuPrograms
    static auto createGLArbGpuProgram(ResourceManager* creator,
                                      std::string_view name, ResourceHandle handle,
                                      std::string_view group, bool isManual, ManualResourceLoader* loader,
                                      GpuProgramType gptype, std::string_view syntaxCode) -> GpuProgram*
    {
        auto* ret = new GLArbGpuProgram(
            creator, name, handle, group, isManual, loader);
        ret->setType(gptype);
        ret->setSyntaxCode(syntaxCode);
        return ret;
    }

    static auto createGLGpuNvparseProgram(ResourceManager* creator,
                                          std::string_view name, ResourceHandle handle,
                                          std::string_view group, bool isManual, ManualResourceLoader* loader,
                                          GpuProgramType gptype, std::string_view syntaxCode) -> GpuProgram*
    {
        auto* ret = new GLGpuNvparseProgram(
            creator, name, handle, group, isManual, loader);
        ret->setType(gptype);
        ret->setSyntaxCode(syntaxCode);
        return ret;
    }

    static auto createGL_ATI_FS_GpuProgram(ResourceManager* creator,
                                           std::string_view name, ResourceHandle handle,
                                           std::string_view group, bool isManual, ManualResourceLoader* loader,
                                           GpuProgramType gptype, std::string_view syntaxCode) -> GpuProgram*
    {

        auto* ret = new ATI_FS_GLGpuProgram(
            creator, name, handle, group, isManual, loader);
        ret->setType(gptype);
        ret->setSyntaxCode(syntaxCode);
        return ret;
    }

    static auto getCombinedMinMipFilter(FilterOptions min, FilterOptions mip) -> GLint
    {
        using enum FilterOptions;
        switch(min)
        {
        case ANISOTROPIC:
        case LINEAR:
            switch (mip)
            {
            case ANISOTROPIC:
            case LINEAR:
                // linear min, linear mip
                return GL_LINEAR_MIPMAP_LINEAR;
            case POINT:
                // linear min, point mip
                return GL_LINEAR_MIPMAP_NEAREST;
            case NONE:
                // linear min, no mip
                return GL_LINEAR;
            }
            break;
        case POINT:
        case NONE:
            switch (mip)
            {
            case ANISOTROPIC:
            case LINEAR:
                // nearest min, linear mip
                return GL_NEAREST_MIPMAP_LINEAR;
            case POINT:
                // nearest min, point mip
                return GL_NEAREST_MIPMAP_NEAREST;
            case NONE:
                // nearest min, no mip
                return GL_NEAREST;
            }
            break;
        }

        // should never get here
        return 0;
    }

    GLRenderSystem::GLRenderSystem()
    
        
    {
        size_t i;

        LogManager::getSingleton().logMessage(::std::format("{} created.", getName()));

        mRenderAttribsBound.reserve(100);
        mRenderInstanceAttribsBound.reserve(100);

        // Get our GLSupport
        mGLSupport.reset(getGLSupport(GLNativeSupport::ContextProfile::COMPATIBILITY));
        glsupport = mGLSupport.get();

        mWorldMatrix = Matrix4::IDENTITY;
        mViewMatrix = Matrix4::IDENTITY;

        initConfigOptions();

        for (i = 0; i < OGRE_MAX_TEXTURE_LAYERS; i++)
        {
            // Dummy value
            mTextureCoordIndex[i] = 99;
            mTextureTypes[i] = 0;
        }

        mActiveRenderTarget = nullptr;
        mCurrentContext = nullptr;
        mMainContext = nullptr;

        mGLInitialised = false;
        mEnableFixedPipeline = true;

        mCurrentLights = 0;
        mMinFilter = FilterOptions::LINEAR;
        mMipFilter = FilterOptions::POINT;
        mCurrentVertexProgram = nullptr;
        mCurrentGeometryProgram = nullptr;
        mCurrentFragmentProgram = nullptr;
        mRTTManager = nullptr;
    }

    GLRenderSystem::~GLRenderSystem()
    {
        shutdown();
    }

    auto GLRenderSystem::getFixedFunctionParams(TrackVertexColourType tracking,
                                                                          FogMode fog) -> const GpuProgramParametersPtr&
    {
        _setSurfaceTracking(tracking);
        _setFog(fog);

        return mFixedFunctionParams;
    }

    void GLRenderSystem::applyFixedFunctionParams(const GpuProgramParametersPtr& params, GpuParamVariability mask)
    {
        bool updateLightPos = false;

        // Autoconstant index is not a physical index
        for (const auto& ac : params->getAutoConstants())
        {
            // Only update needed slots
            if (!!(ac.variability & mask))
            {
                const float* ptr = params->getFloatPointer(ac.physicalIndex);
                using enum GpuProgramParameters::AutoConstantType;
                switch(ac.paramType)
                {
                case WORLD_MATRIX:
                    setWorldMatrix(Matrix4::FromPtr(ptr));
                    break;
                case VIEW_MATRIX:
                    // force light update
                    updateLightPos = true;
                    mask |= GpuParamVariability::LIGHTS;
                    setViewMatrix(Matrix4::FromPtr(ptr));
                    break;
                case PROJECTION_MATRIX:
                    setProjectionMatrix(Matrix4::FromPtr(ptr));
                    break;
                case SURFACE_AMBIENT_COLOUR:
                    mStateCacheManager->setMaterialAmbient(ptr[0], ptr[1], ptr[2], ptr[3]);
                    break;
                case SURFACE_DIFFUSE_COLOUR:
                    mStateCacheManager->setMaterialDiffuse(ptr[0], ptr[1], ptr[2], ptr[3]);
                    break;
                case SURFACE_SPECULAR_COLOUR:
                    mStateCacheManager->setMaterialSpecular(ptr[0], ptr[1], ptr[2], ptr[3]);
                    break;
                case SURFACE_EMISSIVE_COLOUR:
                    mStateCacheManager->setMaterialEmissive(ptr[0], ptr[1], ptr[2], ptr[3]);
                    break;
                case SURFACE_SHININESS:
                    mStateCacheManager->setMaterialShininess(ptr[0]);
                    break;
                case POINT_PARAMS:
                    mStateCacheManager->setPointSize(ptr[0]);
                    mStateCacheManager->setPointParameters(ptr + 1);
                    break;
                case FOG_PARAMS:
                    glFogf(GL_FOG_DENSITY, ptr[0]);
                    glFogf(GL_FOG_START, ptr[1]);
                    glFogf(GL_FOG_END, ptr[2]);
                    break;
                case FOG_COLOUR:
                    glFogfv(GL_FOG_COLOR, ptr);
                    break;
                case AMBIENT_LIGHT_COLOUR:
                    mStateCacheManager->setLightAmbient(ptr[0], ptr[1], ptr[2]);
                    break;
                case LIGHT_DIFFUSE_COLOUR:
                    glLightfv(GL_LIGHT0 + ac.data, GL_DIFFUSE, ptr);
                    break;
                case LIGHT_SPECULAR_COLOUR:
                    glLightfv(GL_LIGHT0 + ac.data, GL_SPECULAR, ptr);
                    break;
                case LIGHT_ATTENUATION:
                    glLightf(GL_LIGHT0 + ac.data, GL_CONSTANT_ATTENUATION, ptr[1]);
                    glLightf(GL_LIGHT0 + ac.data, GL_LINEAR_ATTENUATION, ptr[2]);
                    glLightf(GL_LIGHT0 + ac.data, GL_QUADRATIC_ATTENUATION, ptr[3]);
                    break;
                case SPOTLIGHT_PARAMS:
                {
                    float cutoff = ptr[3] ? Math::RadiansToDegrees(std::acos(ptr[1])) : 180;
                    glLightf(GL_LIGHT0 + ac.data, GL_SPOT_CUTOFF, cutoff);
                    glLightf(GL_LIGHT0 + ac.data, GL_SPOT_EXPONENT, ptr[2]);
                    break;
                }
                case LIGHT_POSITION:
                case LIGHT_DIRECTION:
                    // handled below
                    updateLightPos = true;
                    break;
                default:
                    OgreAssert(false, "unknown autoconstant");
                    break;
                }
            }
        }

        if(!updateLightPos) return;

        // GL lights use eye coordinates, which we only know now

        // Save previous modelview
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixf(Matrix4f(mViewMatrix.transpose())[0]);

        for (const auto& ac : params->getAutoConstants())
        {
            // Only update needed slots
            if (!!((GpuParamVariability::GLOBAL | GpuParamVariability::LIGHTS) & mask))
            {
                const float* ptr = params->getFloatPointer(ac.physicalIndex);
                using enum GpuProgramParameters::AutoConstantType;
                switch(ac.paramType)
                {
                case LIGHT_POSITION:
                    glLightfv(GL_LIGHT0 + ac.data, GL_POSITION, ptr);
                    break;
                case LIGHT_DIRECTION:
                    glLightfv(GL_LIGHT0 + ac.data, GL_SPOT_DIRECTION, ptr);
                    break;
                default:
                    break;
                }
            }
        }
        glPopMatrix();
    }

    auto GLRenderSystem::getName() const noexcept -> std::string_view
    {
        static std::string_view const strName("OpenGL Rendering Subsystem");
        return strName;
    }

    void GLRenderSystem::_initialise()
    {
        RenderSystem::_initialise();

        mGLSupport->start();

        // Create the texture manager
        mTextureManager = new GLTextureManager(this);
    }

    void GLRenderSystem::initConfigOptions()
    {
        GLRenderSystemCommon::initConfigOptions();

        ConfigOption optRTTMode;
        optRTTMode.name = "RTT Preferred Mode";
        optRTTMode.possibleValues = {"FBO", "PBuffer", "Copy"};
        optRTTMode.currentValue = optRTTMode.possibleValues[0];
        optRTTMode.immutable = true;
        mOptions["RTT Preferred Mode"] = optRTTMode;

        ConfigOption opt;
        opt.name = "Fixed Pipeline Enabled";
        opt.possibleValues = {"Yes", "No"};
        opt.currentValue = opt.possibleValues[0];
        opt.immutable = false;

        mOptions["Fixed Pipeline Enabled"] = opt;
    }

    auto GLRenderSystem::createRenderSystemCapabilities() const -> RenderSystemCapabilities*
    {
        auto* rsc = new RenderSystemCapabilities();

        rsc->setCategoryRelevant(CapabilitiesCategory::GL, true);
        rsc->setDriverVersion(mDriverVersion);
        const char* deviceName = (const char*)glGetString(GL_RENDERER);
        rsc->setDeviceName(deviceName);
        rsc->setRenderSystemName(getName());
        rsc->setVendor(mVendor);

        if (mEnableFixedPipeline)
        {
            // Supports fixed-function
            rsc->setCapability(Capabilities::FIXED_FUNCTION);
        }

        rsc->setCapability(Capabilities::AUTOMIPMAP_COMPRESSED);

        // Check for Multitexturing support and set number of texture units
        GLint units;
        glGetIntegerv( GL_MAX_TEXTURE_UNITS, &units );

        if (GLAD_GL_ARB_fragment_program)
        {
            // Also check GL_MAX_TEXTURE_IMAGE_UNITS_ARB since NV at least
            // only increased this on the FX/6x00 series
            GLint arbUnits;
            glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &arbUnits );
            if (arbUnits > units)
                units = arbUnits;
        }
        rsc->setNumTextureUnits(std::min(OGRE_MAX_TEXTURE_LAYERS, units));

        // Check for Anisotropy support
        if(GLAD_GL_EXT_texture_filter_anisotropic)
        {
            GLfloat maxAnisotropy = 0;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
            rsc->setMaxSupportedAnisotropy(maxAnisotropy);
            rsc->setCapability(Capabilities::ANISOTROPY);
        }

        // Point sprites
        if (GLAD_GL_VERSION_2_0 || GLAD_GL_ARB_point_sprite)
        {
            rsc->setCapability(Capabilities::POINT_SPRITES);
        }

        if(GLAD_GL_ARB_point_parameters)
        {
            glPointParameterf = glPointParameterfARB;
            glPointParameterfv = glPointParameterfvARB;
        }
        else if(GLAD_GL_EXT_point_parameters)
        {
            glPointParameterf = glPointParameterfEXT;
            glPointParameterfv = glPointParameterfvEXT;
        }

        rsc->setCapability(Capabilities::POINT_EXTENDED_PARAMETERS);


        // Check for hardware stencil support and set bit depth
        GLint stencil;
        glGetIntegerv(GL_STENCIL_BITS,&stencil);

        if(stencil)
        {
            rsc->setCapability(Capabilities::HWSTENCIL);
        }

        rsc->setCapability(Capabilities::HW_GAMMA);

        rsc->setCapability(Capabilities::MAPBUFFER);
        rsc->setCapability(Capabilities::_32BIT_INDEX);

        if(GLAD_GL_ARB_vertex_program)
        {
            rsc->setCapability(Capabilities::VERTEX_PROGRAM);

            // Vertex Program Properties
            GLint floatConstantCount;
            glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &floatConstantCount);
            rsc->setVertexProgramConstantFloatCount(floatConstantCount);

            GLint attrs;
            glGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &attrs);
            rsc->setNumVertexAttributes(attrs);

            rsc->addShaderProfile("arbvp1");
            if (GLAD_GL_NV_vertex_program2_option)
            {
                rsc->addShaderProfile("vp30");
            }

            if (GLAD_GL_NV_vertex_program3)
            {
                rsc->addShaderProfile("vp40");
            }

            if (GLAD_GL_NV_gpu_program4)
            {
                rsc->addShaderProfile("gp4vp");
                rsc->addShaderProfile("gpu_vp");
            }
        }

        if (GLAD_GL_NV_register_combiners2 &&
            GLAD_GL_NV_texture_shader)
        {
            rsc->addShaderProfile("fp20");
        }

        // NFZ - check for ATI fragment shader support
        if (GLAD_GL_ATI_fragment_shader)
        {
            // only 8 Vector4 constant floats supported
            rsc->setFragmentProgramConstantFloatCount(8);

            rsc->addShaderProfile("ps_1_4");
            rsc->addShaderProfile("ps_1_3");
            rsc->addShaderProfile("ps_1_2");
            rsc->addShaderProfile("ps_1_1");
        }

        if (GLAD_GL_ARB_fragment_program)
        {
            // Fragment Program Properties
            GLint floatConstantCount;
            glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &floatConstantCount);
            rsc->setFragmentProgramConstantFloatCount(floatConstantCount);

            rsc->addShaderProfile("arbfp1");
            if (GLAD_GL_NV_fragment_program_option)
            {
                rsc->addShaderProfile("fp30");
            }

            if (GLAD_GL_NV_fragment_program2)
            {
                rsc->addShaderProfile("fp40");
            }

            if (GLAD_GL_NV_gpu_program4)
            {
                rsc->addShaderProfile("gp4fp");
                rsc->addShaderProfile("gpu_fp");
            }
        }

        // NFZ - Check if GLSL is supported
        if ( GLAD_GL_VERSION_2_0 ||
             (GLAD_GL_ARB_shading_language_100 &&
              GLAD_GL_ARB_shader_objects &&
              GLAD_GL_ARB_fragment_shader &&
              GLAD_GL_ARB_vertex_shader) )
        {
            rsc->addShaderProfile("glsl");
            if(getNativeShadingLanguageVersion() >= 120)
                rsc->addShaderProfile("glsl120");
            if(getNativeShadingLanguageVersion() >= 110)
                rsc->addShaderProfile("glsl110");
            if(getNativeShadingLanguageVersion() >= 100)
                rsc->addShaderProfile("glsl100");
        }

        // Check if geometry shaders are supported
        if (GLAD_GL_VERSION_2_0 &&
            GLAD_GL_EXT_geometry_shader4)
        {
            rsc->setCapability(Capabilities::GEOMETRY_PROGRAM);
            GLint floatConstantCount = 0;
            glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT, &floatConstantCount);
            rsc->setGeometryProgramConstantFloatCount(floatConstantCount/4);

            GLint maxOutputVertices;
            glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT,&maxOutputVertices);
            rsc->setGeometryProgramNumOutputVertices(maxOutputVertices);
        }

        if(GLAD_GL_NV_gpu_program4)
        {
            rsc->setCapability(Capabilities::GEOMETRY_PROGRAM);
            rsc->addShaderProfile("nvgp4");

            //Also add the CG profiles
            rsc->addShaderProfile("gpu_gp");
            rsc->addShaderProfile("gp4gp");
        }

        if (checkExtension("GL_ARB_get_program_binary"))
        {
            // states 3.0 here: http://developer.download.nvidia.com/opengl/specs/GL_ARB_get_program_binary.txt
            // but not here: http://www.opengl.org/sdk/docs/man4/xhtml/glGetProgramBinary.xml
            // and here states 4.1: http://www.geeks3d.com/20100727/opengl-4-1-allows-the-use-of-binary-shaders/
            GLint formats;
            glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);

            if(formats > 0)
                rsc->setCapability(Capabilities::CAN_GET_COMPILED_SHADER_BUFFER);
        }

        if (hasMinGLVersion(3, 3) || GLAD_GL_ARB_instanced_arrays)
        {
            // states 3.3 here: http://www.opengl.org/sdk/docs/man3/xhtml/glVertexAttribDivisor.xml
            rsc->setCapability(Capabilities::VERTEX_BUFFER_INSTANCE_DATA);
        }

        //Check if render to vertex buffer (transform feedback in OpenGL)
        if (GLAD_GL_VERSION_2_0 &&
            GLAD_GL_NV_transform_feedback)
        {
            rsc->setCapability(Capabilities::HWRENDER_TO_VERTEX_BUFFER);
        }

        // Check for texture compression
        rsc->setCapability(Capabilities::TEXTURE_COMPRESSION);

        // Check for dxt compression
        if(GLAD_GL_EXT_texture_compression_s3tc)
        {
                rsc->setCapability(Capabilities::TEXTURE_COMPRESSION_DXT);
        }
        // Check for vtc compression
        if(GLAD_GL_NV_texture_compression_vtc)
        {
            rsc->setCapability(Capabilities::TEXTURE_COMPRESSION_VTC);
        }

        // As are user clipping planes
        rsc->setCapability(Capabilities::USER_CLIP_PLANES);

        // 2-sided stencil?
        if (GLAD_GL_VERSION_2_0 || GLAD_GL_EXT_stencil_two_side)
        {
            rsc->setCapability(Capabilities::TWO_SIDED_STENCIL);
        }
        rsc->setCapability(Capabilities::STENCIL_WRAP);
        rsc->setCapability(Capabilities::HWOCCLUSION);

        // Check for non-power-of-2 texture support
        if(GLAD_GL_ARB_texture_non_power_of_two)
        {
            rsc->setCapability(Capabilities::NON_POWER_OF_2_TEXTURES);
        }

        // Check for Float textures
        if(GLAD_GL_ATI_texture_float || GLAD_GL_ARB_texture_float)
        {
            rsc->setCapability(Capabilities::TEXTURE_FLOAT);
        }

        // 3D textures should be supported by GL 1.2, which is our minimum version
        rsc->setCapability(Capabilities::TEXTURE_1D);
        rsc->setCapability(Capabilities::TEXTURE_3D);

        if(hasMinGLVersion(3, 0) || GLAD_GL_EXT_texture_array)
            rsc->setCapability(Capabilities::TEXTURE_2D_ARRAY);

        // Check for framebuffer object extension
        if(GLAD_GL_EXT_framebuffer_object)
        {
            // Probe number of draw buffers
            // Only makes sense with FBO support, so probe here
            if(GLAD_GL_VERSION_2_0 ||
               GLAD_GL_ARB_draw_buffers ||
               GLAD_GL_ATI_draw_buffers)
            {
                GLint buffers;
                glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &buffers);
                rsc->setNumMultiRenderTargets(std::min<int>(buffers, (GLint)OGRE_MAX_MULTIPLE_RENDER_TARGETS));
                rsc->setCapability(Capabilities::MRT_DIFFERENT_BIT_DEPTHS);

            }
            rsc->setCapability(Capabilities::HWRENDER_TO_TEXTURE);
        }

        // Check GLSupport for PBuffer support
        if(GLAD_GL_ARB_pixel_buffer_object || GLAD_GL_EXT_pixel_buffer_object)
        {
            // Use PBuffers
            rsc->setCapability(Capabilities::HWRENDER_TO_TEXTURE);
            rsc->setCapability(Capabilities::PBUFFER);
        }

        // Point size
        float ps;
        glGetFloatv(GL_POINT_SIZE_MAX, &ps);
        rsc->setMaxPointSize(ps);

        // Vertex texture fetching
        if (checkExtension("GL_ARB_vertex_shader"))
        {
            GLint vUnits;
            glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &vUnits);
            rsc->setNumVertexTextureUnits(static_cast<ushort>(vUnits));
            if (vUnits > 0)
            {
                rsc->setCapability(Capabilities::VERTEX_TEXTURE_FETCH);
            }
        }

        rsc->setCapability(Capabilities::MIPMAP_LOD_BIAS);

        // Alpha to coverage?
        if (checkExtension("GL_ARB_multisample"))
        {
            // Alpha to coverage always 'supported' when MSAA is available
            // although card may ignore it if it doesn't specifically support A2C
            rsc->setCapability(Capabilities::ALPHA_TO_COVERAGE);
        }

        GLfloat lineWidth[2] = {1, 1};
        glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidth);
        if(lineWidth[1] != 1 && lineWidth[1] != lineWidth[0])
            rsc->setCapability(Capabilities::WIDE_LINES);

        return rsc;
    }

    void GLRenderSystem::initialiseFromRenderSystemCapabilities(RenderSystemCapabilities* caps, RenderTarget* primary)
    {
        if(caps->getRenderSystemName() != getName())
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "Trying to initialize GLRenderSystem from RenderSystemCapabilities that do not support OpenGL",
                        "GLRenderSystem::initialiseFromRenderSystemCapabilities");
        }

        // set texture the number of texture units
        mFixedFunctionTextureUnits = caps->getNumTextureUnits();

        //In GL there can be less fixed function texture units than general
        //texture units. Get the minimum of the two.
        if (caps->hasCapability(Capabilities::FRAGMENT_PROGRAM))
        {
            GLint maxTexCoords = 0;
            glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &maxTexCoords);
            if (mFixedFunctionTextureUnits > maxTexCoords)
            {
                mFixedFunctionTextureUnits = maxTexCoords;
            }
        }

        if(!GLAD_GL_ARB_vertex_buffer_object)
        {
            // Assign ARB functions same to GL 1.5 version since
            // interface identical
            glBindBufferARB = glBindBuffer;
            glBufferDataARB = glBufferData;
            glBufferSubDataARB = glBufferSubData;
            glDeleteBuffersARB = glDeleteBuffers;
            glGenBuffersARB = glGenBuffers;
            glGetBufferParameterivARB = glGetBufferParameteriv;
            glGetBufferPointervARB = glGetBufferPointerv;
            glGetBufferSubDataARB = glGetBufferSubData;
            glIsBufferARB = glIsBuffer;
            glMapBufferARB = glMapBuffer;
            glUnmapBufferARB = glUnmapBuffer;
        }

        mHardwareBufferManager = new GLHardwareBufferManager;

        // XXX Need to check for nv2 support and make a program manager for it
        // XXX Probably nv1 as well for older cards
        // GPU Program Manager setup
        mGpuProgramManager = new GLGpuProgramManager();

        if(caps->hasCapability(Capabilities::VERTEX_PROGRAM))
        {
            if(caps->isShaderProfileSupported("arbvp1"))
            {
                mGpuProgramManager->registerProgramFactory("arbvp1", createGLArbGpuProgram);
            }

            if(caps->isShaderProfileSupported("vp30"))
            {
                mGpuProgramManager->registerProgramFactory("vp30", createGLArbGpuProgram);
            }

            if(caps->isShaderProfileSupported("vp40"))
            {
                mGpuProgramManager->registerProgramFactory("vp40", createGLArbGpuProgram);
            }

            if(caps->isShaderProfileSupported("gp4vp"))
            {
                mGpuProgramManager->registerProgramFactory("gp4vp", createGLArbGpuProgram);
            }

            if(caps->isShaderProfileSupported("gpu_vp"))
            {
                mGpuProgramManager->registerProgramFactory("gpu_vp", createGLArbGpuProgram);
            }
        }

        if(caps->hasCapability(Capabilities::GEOMETRY_PROGRAM))
        {
            //TODO : Should these be createGLArbGpuProgram or createGLGpuNVparseProgram?
            if(caps->isShaderProfileSupported("nvgp4"))
            {
                mGpuProgramManager->registerProgramFactory("nvgp4", createGLArbGpuProgram);
            }
            if(caps->isShaderProfileSupported("gp4gp"))
            {
                mGpuProgramManager->registerProgramFactory("gp4gp", createGLArbGpuProgram);
            }
            if(caps->isShaderProfileSupported("gpu_gp"))
            {
                mGpuProgramManager->registerProgramFactory("gpu_gp", createGLArbGpuProgram);
            }
        }

        if(caps->hasCapability(Capabilities::FRAGMENT_PROGRAM))
        {

            if(caps->isShaderProfileSupported("fp20"))
            {
                mGpuProgramManager->registerProgramFactory("fp20", createGLGpuNvparseProgram);
            }

            if(caps->isShaderProfileSupported("ps_1_4"))
            {
                mGpuProgramManager->registerProgramFactory("ps_1_4", createGL_ATI_FS_GpuProgram);
            }

            if(caps->isShaderProfileSupported("ps_1_3"))
            {
                mGpuProgramManager->registerProgramFactory("ps_1_3", createGL_ATI_FS_GpuProgram);
            }

            if(caps->isShaderProfileSupported("ps_1_2"))
            {
                mGpuProgramManager->registerProgramFactory("ps_1_2", createGL_ATI_FS_GpuProgram);
            }

            if(caps->isShaderProfileSupported("ps_1_1"))
            {
                mGpuProgramManager->registerProgramFactory("ps_1_1", createGL_ATI_FS_GpuProgram);
            }

            if(caps->isShaderProfileSupported("arbfp1"))
            {
                mGpuProgramManager->registerProgramFactory("arbfp1", createGLArbGpuProgram);
            }

            if(caps->isShaderProfileSupported("fp40"))
            {
                mGpuProgramManager->registerProgramFactory("fp40", createGLArbGpuProgram);
            }

            if(caps->isShaderProfileSupported("fp30"))
            {
                mGpuProgramManager->registerProgramFactory("fp30", createGLArbGpuProgram);
            }

            if(caps->isShaderProfileSupported("gp4fp"))
            {
                mGpuProgramManager->registerProgramFactory("gp4fp", createGLArbGpuProgram);
            }

            if(caps->isShaderProfileSupported("gpu_fp"))
            {
                mGpuProgramManager->registerProgramFactory("gpu_fp", createGLArbGpuProgram);
            }

        }

        if(caps->isShaderProfileSupported("glsl"))
        {
            // NFZ - check for GLSL vertex and fragment shader support successful
            mGLSLProgramFactory = new GLSL::GLSLProgramFactory();
            HighLevelGpuProgramManager::getSingleton().addFactory(mGLSLProgramFactory);
            LogManager::getSingleton().logMessage("GLSL support detected");
        }

        if(caps->hasCapability(Capabilities::HWOCCLUSION) && !GLAD_GL_ARB_occlusion_query)
        {
            // Assign ARB functions same to GL 1.5 version since
            // interface identical
            glBeginQueryARB = glBeginQuery;
            glDeleteQueriesARB = glDeleteQueries;
            glEndQueryARB = glEndQuery;
            glGenQueriesARB = glGenQueries;
            glGetQueryObjectivARB = glGetQueryObjectiv;
            glGetQueryObjectuivARB = glGetQueryObjectuiv;
            glGetQueryivARB = glGetQueryiv;
            glIsQueryARB = glIsQuery;
        }


        /// Do this after extension function pointers are initialised as the extension
        /// is used to probe further capabilities.
        auto cfi = getConfigOptions().find("RTT Preferred Mode");
        // RTT Mode: 0 use whatever available, 1 use PBuffers, 2 force use copying
        int rttMode = 0;
        if (cfi != getConfigOptions().end())
        {
            if (cfi->second.currentValue == "PBuffer")
            {
                rttMode = 1;
            }
            else if (cfi->second.currentValue == "Copy")
            {
                rttMode = 2;
            }
        }




        // Check for framebuffer object extension
        if(caps->hasCapability(Capabilities::HWRENDER_TO_TEXTURE) && rttMode < 1)
        {
            // Before GL version 2.0, we need to get one of the extensions
            if(GLAD_GL_ARB_draw_buffers)
                glDrawBuffers = glDrawBuffersARB;
            else if(GLAD_GL_ATI_draw_buffers)
                glDrawBuffers = glDrawBuffersATI;

            // Create FBO manager
            LogManager::getSingleton().logMessage("GL: Using GL_EXT_framebuffer_object for rendering to textures (best)");
            mRTTManager = new GLFBOManager(false);
            //TODO: Check if we're using OpenGL 3.0 and add Capabilities::RTT_DEPTHBUFFER_RESOLUTION_LESSEQUAL flag
        }
        else
        {
            // Check GLSupport for PBuffer support
            if(caps->hasCapability(Capabilities::PBUFFER) && rttMode < 2)
            {
                if(caps->hasCapability(Capabilities::HWRENDER_TO_TEXTURE))
                {
                    // Use PBuffers
                    mRTTManager = new GLPBRTTManager(mGLSupport.get(), primary);
                    LogManager::getSingleton().logWarning("GL: Using PBuffers for rendering to textures");

                    //TODO: Depth buffer sharing in pbuffer is left unsupported
                }
            }
            else
            {
                // No pbuffer support either -- fallback to simplest copying from framebuffer
                mRTTManager = new GLCopyingRTTManager();
                LogManager::getSingleton().logWarning("GL: Using framebuffer copy for rendering to textures (worst)");
                LogManager::getSingleton().logWarning("GL: RenderTexture size is restricted to size of framebuffer. If you are on Linux, consider using GLX instead of SDL.");

                //Copy method uses the main depth buffer but no other depth buffer
                caps->setCapability(Capabilities::RTT_MAIN_DEPTHBUFFER_ATTACHABLE);
                caps->setCapability(Capabilities::RTT_DEPTHBUFFER_RESOLUTION_LESSEQUAL);
            }

            // Downgrade number of simultaneous targets
            caps->setNumMultiRenderTargets(1);
        }

        mGLInitialised = true;
    }

    void GLRenderSystem::shutdown()
    {
        RenderSystem::shutdown();

        // Deleting the GLSL program factory
        if (mGLSLProgramFactory)
        {
            // Remove from manager safely
            if (HighLevelGpuProgramManager::getSingletonPtr())
                HighLevelGpuProgramManager::getSingleton().removeFactory(mGLSLProgramFactory);
            delete mGLSLProgramFactory;
            mGLSLProgramFactory = nullptr;
        }

        // Delete extra threads contexts
        for (auto pCurContext : mBackgroundContextList)
        {
            pCurContext->releaseContext();
            delete pCurContext;
        }
        mBackgroundContextList.clear();

        // Deleting the GPU program manager and hardware buffer manager.  Has to be done before the mGLSupport->stop().
        delete mGpuProgramManager;
        mGpuProgramManager = nullptr;

        delete mHardwareBufferManager;
        mHardwareBufferManager = nullptr;

        delete mRTTManager;
        mRTTManager = nullptr;

        mGLSupport->stop();

        delete mTextureManager;
        mTextureManager = nullptr;

        // There will be a new initial window and so forth, thus any call to test
        //  some params will access an invalid pointer, so it is best to reset
        //  the whole state.
        mGLInitialised = false;
    }

    void GLRenderSystem::setShadingType(ShadeOptions so)
    {
        // XXX Don't do this when using shader
        using enum ShadeOptions;
        switch(so)
        {
        case FLAT:
            mStateCacheManager->setShadeModel(GL_FLAT);
            break;
        default:
            mStateCacheManager->setShadeModel(GL_SMOOTH);
            break;
        }
    }
    //---------------------------------------------------------------------
    auto GLRenderSystem::_createRenderWindow(std::string_view name,
                                                      unsigned int width, unsigned int height, bool fullScreen,
                                                      const NameValuePairList *miscParams) -> RenderWindow*
    {
        RenderSystem::_createRenderWindow(name, width, height, fullScreen, miscParams);

        // Create the window
        RenderWindow* win = mGLSupport->newWindow(name, width, height,
                                                  fullScreen, miscParams);

        attachRenderTarget( *win );

        if (!mGLInitialised)
        {
            // set up glew and GLSupport
            initialiseContext(win);

            const char* shadingLangVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
            auto const tokens = StringUtil::split(shadingLangVersion, ". ");
            mNativeShadingLanguageVersion = (StringConverter::parseUnsignedInt(tokens[0]) * 100) + StringConverter::parseUnsignedInt(tokens[1]);

            auto it = mOptions.find("Fixed Pipeline Enabled");
            if (it != mOptions.end())
            {
                mEnableFixedPipeline = StringConverter::parseBool(it->second.currentValue);
            }

            // Initialise GL after the first window has been created
            // TODO: fire this from emulation options, and don't duplicate Real and Current capabilities
            mRealCapabilities.reset(createRenderSystemCapabilities());
            initFixedFunctionParams(); // create params

            // use real capabilities if custom capabilities are not available
            if(!mUseCustomCapabilities)
                mCurrentCapabilities = mRealCapabilities.get();

            fireEvent("RenderSystemCapabilitiesCreated");

            initialiseFromRenderSystemCapabilities(mCurrentCapabilities, win);

            // Initialise the main context
            _oneTimeContextInitialization();
            if(mCurrentContext)
                mCurrentContext->setInitialized();
        }

        if( win->getDepthBufferPool() != DepthBuffer::PoolId::NO_DEPTH )
        {
            //Unlike D3D9, OGL doesn't allow sharing the main depth buffer, so keep them separate.
            //Only Copy does, but Copy means only one depth buffer...
            GLContext *windowContext = dynamic_cast<GLRenderTarget*>(win)->getContext();;

            auto depthBuffer =
                new GLDepthBufferCommon(DepthBuffer::PoolId::DEFAULT, this, windowContext, nullptr, nullptr, win, true);

            mDepthBufferPool[depthBuffer->getPoolId()].push_back( depthBuffer );

            win->attachDepthBuffer( depthBuffer );
        }

        return win;
    }
    //---------------------------------------------------------------------
    auto GLRenderSystem::_createDepthBufferFor( RenderTarget *renderTarget ) -> DepthBuffer*
    {
        if( auto fbo = dynamic_cast<GLRenderTarget*>(renderTarget)->getFBO() )
        {
            //Find best depth & stencil format suited for the RT's format
            GLuint depthFormat, stencilFormat;
            mRTTManager->getBestDepthStencil(fbo->getFormat(), &depthFormat, &stencilFormat);

            auto *depthBuffer = new GLRenderBuffer( depthFormat, fbo->getWidth(),
                                                              fbo->getHeight(), fbo->getFSAA() );

            GLRenderBuffer *stencilBuffer = nullptr;
            if ( depthFormat == GL_DEPTH24_STENCIL8_EXT)
            {
                // If we have a packed format, the stencilBuffer is the same as the depthBuffer
                stencilBuffer = depthBuffer;
            }
            else if(stencilFormat)
            {
                stencilBuffer = new GLRenderBuffer( stencilFormat, fbo->getWidth(),
                                                    fbo->getHeight(), fbo->getFSAA() );
            }

            return new GLDepthBufferCommon(DepthBuffer::PoolId{}, this, mCurrentContext, depthBuffer, stencilBuffer,
                                           renderTarget, false);
        }

        return nullptr;
    }

    void GLRenderSystem::initialiseContext(RenderWindow* primary)
    {
        // Set main and current context
        mMainContext = dynamic_cast<GLRenderTarget*>(primary)->getContext();
        mCurrentContext = mMainContext;

        // Set primary context as active
        if(mCurrentContext)
            mCurrentContext->setCurrent();

        gladLoadGLLoader(get_proc);

        if (!GLAD_GL_VERSION_1_5) {
            OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR,
                        "OpenGL 1.5 is not supported",
                        "GLRenderSystem::initialiseContext");
        }

        // Get extension function pointers
        initialiseExtensions();

        mStateCacheManager = mCurrentContext->createOrRetrieveStateCacheManager<GLStateCacheManager>();

        LogManager::getSingleton().logMessage("***************************");
        LogManager::getSingleton().logMessage("*** GL Renderer Started ***");
        LogManager::getSingleton().logMessage("***************************");
    }



    //-----------------------------------------------------------------------
    auto GLRenderSystem::createMultiRenderTarget(std::string_view name) -> MultiRenderTarget *
    {
        auto fboMgr = dynamic_cast<GLFBOManager*>(mRTTManager);
        if (!fboMgr)
            OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, "MultiRenderTarget is not supported");

        MultiRenderTarget *retval = new GLFBOMultiRenderTarget(fboMgr, name);
        attachRenderTarget( *retval );
        return retval;
    }

    //-----------------------------------------------------------------------
    void GLRenderSystem::destroyRenderWindow(std::string_view name)
    {
        // Find it to remove from list.
        ::std::unique_ptr<RenderTarget> pWin{detachRenderTarget(name) };
        OgreAssert(pWin, "unknown RenderWindow name");

        GLContext *windowContext = dynamic_cast<GLRenderTarget*>(pWin.get())->getContext();
    
        //1 Window <-> 1 Context, should be always true
        assert( windowContext );

        //Find the depth buffer from this window and remove it.
        for (auto& [key, value] : mDepthBufferPool)
        {
            auto itor = value.begin();
            auto end  = value.end();

            while( itor != end )
            {
                //A DepthBuffer with no depth & stencil pointers is a dummy one,
                //look for the one that matches the same GL context
                auto depthBuffer = static_cast<GLDepthBufferCommon*>(*itor);
                GLContext *glContext = depthBuffer->getGLContext();

                if( glContext == windowContext &&
                    (depthBuffer->getDepthBuffer() || depthBuffer->getStencilBuffer()) )
                {
                    delete *itor;
                    value.erase( itor );
                    return;
                }
                ++itor;
            }
        }
    }

    //---------------------------------------------------------------------
    void GLRenderSystem::_useLights(unsigned short limit)
    {
        if(limit == mCurrentLights)
            return;

        unsigned short num = 0;
        for (;num < limit; ++num)
        {
            setGLLight(num, true);
        }
        // Disable extra lights
        for (; num < mCurrentLights; ++num)
        {
            setGLLight(num, false);
        }
        mCurrentLights = limit;
    }

    void GLRenderSystem::setGLLight(size_t index, bool lt)
    {
        setFFPLightParams(index, lt);

        GLenum gl_index = GL_LIGHT0 + index;

        if (!lt)
        {
            // Disable in the scene
            mStateCacheManager->setEnabled(gl_index, false);
        }
        else
        {
            GLfloat f4vals[4] = {0, 0, 0, 1};
            // Disable ambient light for movables;
            glLightfv(gl_index, GL_AMBIENT, f4vals);

            // Enable in the scene
            mStateCacheManager->setEnabled(gl_index, true);
        }
    }

    //-----------------------------------------------------------------------------
    void GLRenderSystem::makeGLMatrix(GLfloat gl_matrix[16], const Matrix4& m)
    {
        size_t x = 0;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                gl_matrix[x] = m[j][i];
                x++;
            }
        }
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::setWorldMatrix( const Matrix4 &m )
    {
        mWorldMatrix = m;
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(Matrix4f((mViewMatrix * mWorldMatrix).transpose())[0]);
    }

    //-----------------------------------------------------------------------------
    void GLRenderSystem::setViewMatrix( const Matrix4 &m )
    {
        mViewMatrix = m;
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(Matrix4f((mViewMatrix * mWorldMatrix).transpose())[0]);

        // also mark clip planes dirty
        if (!mClipPlanes.empty())
            mClipPlanesDirty = true;
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::setProjectionMatrix(const Matrix4 &m)
    {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(Matrix4f(m.transpose())[0]);
        glMatrixMode(GL_MODELVIEW);

        // also mark clip planes dirty
        if (!mClipPlanes.empty())
            mClipPlanesDirty = true;
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setSurfaceTracking(TrackVertexColourType tracking)
    {

        // Track vertex colour
        if(tracking != TrackVertexColourEnum::NONE)
        {
            GLenum gt = GL_DIFFUSE;
            // There are actually 15 different combinations for tracking, of which
            // GL only supports the most used 5. This means that we have to do some
            // magic to find the best match. NOTE:
            //  GL_AMBIENT_AND_DIFFUSE != GL_AMBIENT | GL__DIFFUSE
            if(!!(tracking & TrackVertexColourEnum::AMBIENT))
            {
                if(!!(tracking & TrackVertexColourEnum::DIFFUSE))
                {
                    gt = GL_AMBIENT_AND_DIFFUSE;
                }
                else
                {
                    gt = GL_AMBIENT;
                }
            }
            else if(!!(tracking & TrackVertexColourEnum::DIFFUSE))
            {
                gt = GL_DIFFUSE;
            }
            else if(!!(tracking & TrackVertexColourEnum::SPECULAR))
            {
                gt = GL_SPECULAR;
            }
            else if(!!(tracking & TrackVertexColourEnum::EMISSIVE))
            {
                gt = GL_EMISSION;
            }
            glColorMaterial(GL_FRONT_AND_BACK, gt);

            mStateCacheManager->setEnabled(GL_COLOR_MATERIAL, true);
        }
        else
        {
            mStateCacheManager->setEnabled(GL_COLOR_MATERIAL, false);
        }
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setPointParameters(bool attenuationEnabled, Real minSize, Real maxSize)
    {
        if(attenuationEnabled)
        {
            // Point size is still calculated in pixels even when attenuation is
            // enabled, which is pretty awkward, since you typically want a viewport
            // independent size if you're looking for attenuation.
            // So, scale the point size up by viewport size (this is equivalent to
            // what D3D does as standard)
            minSize = minSize * mActiveViewport->getActualHeight();
            if (maxSize == 0.0f)
                maxSize = mCurrentCapabilities->getMaxPointSize(); // pixels
            else
                maxSize = maxSize * mActiveViewport->getActualHeight();

            if (mCurrentCapabilities->hasCapability(Capabilities::VERTEX_PROGRAM))
                mStateCacheManager->setEnabled(GL_VERTEX_PROGRAM_POINT_SIZE, true);
        }
        else
        {
            if (maxSize == 0.0f)
                maxSize = mCurrentCapabilities->getMaxPointSize();
            if (mCurrentCapabilities->hasCapability(Capabilities::VERTEX_PROGRAM))
                mStateCacheManager->setEnabled(GL_VERTEX_PROGRAM_POINT_SIZE, false);
        }

        mStateCacheManager->setPointParameters(nullptr, minSize, maxSize);
    }

    void GLRenderSystem::_setLineWidth(float width)
    {
        glLineWidth(width);
    }

    //---------------------------------------------------------------------
    void GLRenderSystem::_setPointSpritesEnabled(bool enabled)
    {
        if (!getCapabilities()->hasCapability(Capabilities::POINT_SPRITES))
            return;

        mStateCacheManager->setEnabled(GL_POINT_SPRITE, enabled);

        // Set sprite texture coord generation
        // Don't offer this as an option since D3D links it to sprite enabled
        for (ushort i = 0; i < mFixedFunctionTextureUnits; ++i)
        {
            mStateCacheManager->activateGLTextureUnit(i);
            glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE,
                      enabled ? GL_TRUE : GL_FALSE);
        }
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setTexture(size_t stage, bool enabled, const TexturePtr &texPtr)
    {
        GLenum lastTextureType = mTextureTypes[stage];

        if (!mStateCacheManager->activateGLTextureUnit(stage))
            return;

        if (enabled)
        {
            GLTexturePtr tex = static_pointer_cast<GLTexture>(texPtr);

            // note used
            tex->touch();
            mTextureTypes[stage] = tex->getGLTextureTarget();

            if(lastTextureType != mTextureTypes[stage] && lastTextureType != 0)
            {
                if (stage < mFixedFunctionTextureUnits)
                {
                    if(lastTextureType != GL_TEXTURE_2D_ARRAY_EXT)
                        glDisable( lastTextureType );
                }
            }

            if (stage < mFixedFunctionTextureUnits)
            {
                if(mTextureTypes[stage] != GL_TEXTURE_2D_ARRAY_EXT)
                    glEnable( mTextureTypes[stage] );
            }

            mStateCacheManager->bindGLTexture( mTextureTypes[stage], tex->getGLID() );
        }
        else
        {
            if (stage < mFixedFunctionTextureUnits)
            {
                if (lastTextureType != 0)
                {
                    if(mTextureTypes[stage] != GL_TEXTURE_2D_ARRAY_EXT)
                        glDisable( mTextureTypes[stage] );
                }
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            }
            // bind zero texture
            mStateCacheManager->bindGLTexture(GL_TEXTURE_2D, 0);
        }
    }

    void GLRenderSystem::_setSampler(size_t unit, Sampler& sampler)
    {
        if (!mStateCacheManager->activateGLTextureUnit(unit))
            return;

        GLenum target = mTextureTypes[unit];

        const Sampler::UVWAddressingMode& uvw = sampler.getAddressingMode();
        mStateCacheManager->setTexParameteri(target, GL_TEXTURE_WRAP_S, getTextureAddressingMode(uvw.u));
        mStateCacheManager->setTexParameteri(target, GL_TEXTURE_WRAP_T, getTextureAddressingMode(uvw.v));
        mStateCacheManager->setTexParameteri(target, GL_TEXTURE_WRAP_R, getTextureAddressingMode(uvw.w));

        if (uvw.u == TextureAddressingMode::BORDER || uvw.v == TextureAddressingMode::BORDER || uvw.w == TextureAddressingMode::BORDER)
            glTexParameterfv( target, GL_TEXTURE_BORDER_COLOR, sampler.getBorderColour().ptr());

        if (mCurrentCapabilities->hasCapability(Capabilities::MIPMAP_LOD_BIAS))
        {
            glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, sampler.getMipmapBias());
        }

        if (mCurrentCapabilities->hasCapability(Capabilities::ANISOTROPY))
            mStateCacheManager->setTexParameteri(
                target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                std::min<uint>(mCurrentCapabilities->getMaxSupportedAnisotropy(), sampler.getAnisotropy()));

        if(GLAD_GL_VERSION_2_0)
        {
            mStateCacheManager->setTexParameteri(target, GL_TEXTURE_COMPARE_MODE,
                                                 sampler.getCompareEnabled() ? GL_COMPARE_REF_DEPTH_TO_TEXTURE_EXT
                                                                             : GL_NONE);
            if (sampler.getCompareEnabled())
                mStateCacheManager->setTexParameteri(target, GL_TEXTURE_COMPARE_FUNC,
                                                     convertCompareFunction(sampler.getCompareFunction()));
        }

        // Combine with existing mip filter
        mStateCacheManager->setTexParameteri(
            target, GL_TEXTURE_MIN_FILTER,
            getCombinedMinMipFilter(sampler.getFiltering(FilterType::Min), sampler.getFiltering(FilterType::Mip)));

        using enum FilterOptions;
        switch (sampler.getFiltering(FilterType::Mag))
        {
        case ANISOTROPIC: // GL treats linear and aniso the same
        case LINEAR:
            mStateCacheManager->setTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        case POINT:
        case NONE:
            mStateCacheManager->setTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        }
    }

    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setTextureCoordSet(size_t stage, size_t index)
    {
        mTextureCoordIndex[stage] = index;
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setTextureCoordCalculation(size_t stage, TexCoordCalcMethod m,
                                                     const Frustum* frustum)
    {
        if (stage >= mFixedFunctionTextureUnits)
        {
            // Can't do this
            return;
        }

        GLfloat M[16];
        Matrix4 projectionBias;

        // Default to no extra auto texture matrix
        mUseAutoTextureMatrix = false;

        GLfloat eyePlaneS[] = {1.0, 0.0, 0.0, 0.0};
        GLfloat eyePlaneT[] = {0.0, 1.0, 0.0, 0.0};
        GLfloat eyePlaneR[] = {0.0, 0.0, 1.0, 0.0};
        GLfloat eyePlaneQ[] = {0.0, 0.0, 0.0, 1.0};

        if (!mStateCacheManager->activateGLTextureUnit(stage))
            return;

        using enum TexCoordCalcMethod;
        switch( m )
        {
        case NONE:
            mStateCacheManager->disableTextureCoordGen( GL_TEXTURE_GEN_S );
            mStateCacheManager->disableTextureCoordGen( GL_TEXTURE_GEN_T );
            mStateCacheManager->disableTextureCoordGen( GL_TEXTURE_GEN_R );
            mStateCacheManager->disableTextureCoordGen( GL_TEXTURE_GEN_Q );
            break;

        case ENVIRONMENT_MAP:
            glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
            glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );

            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_S );
            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_T );
            mStateCacheManager->disableTextureCoordGen( GL_TEXTURE_GEN_R );
            mStateCacheManager->disableTextureCoordGen( GL_TEXTURE_GEN_Q );

            // Need to use a texture matrix to flip the spheremap
            mUseAutoTextureMatrix = true;
            memset(mAutoTextureMatrix, 0, sizeof(GLfloat)*16);
            mAutoTextureMatrix[0] = mAutoTextureMatrix[10] = mAutoTextureMatrix[15] = 1.0f;
            mAutoTextureMatrix[5] = -1.0f;

            break;

        case ENVIRONMENT_MAP_PLANAR:

            glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
            glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
            glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );

            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_S );
            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_T );
            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_R );
            mStateCacheManager->disableTextureCoordGen( GL_TEXTURE_GEN_Q );

            break;
        case ENVIRONMENT_MAP_REFLECTION:

            glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
            glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
            glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );

            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_S );
            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_T );
            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_R );
            mStateCacheManager->disableTextureCoordGen( GL_TEXTURE_GEN_Q );

            // We need an extra texture matrix here
            // This sets the texture matrix to be the inverse of the view matrix
            mUseAutoTextureMatrix = true;
            makeGLMatrix( M, mViewMatrix);

            // Transpose 3x3 in order to invert matrix (rotation)
            // Note that we need to invert the Z _before_ the rotation
            // No idea why we have to invert the Z at all, but reflection is wrong without it
            mAutoTextureMatrix[0] = M[0]; mAutoTextureMatrix[1] = M[4]; mAutoTextureMatrix[2] = -M[8];
            mAutoTextureMatrix[4] = M[1]; mAutoTextureMatrix[5] = M[5]; mAutoTextureMatrix[6] = -M[9];
            mAutoTextureMatrix[8] = M[2]; mAutoTextureMatrix[9] = M[6]; mAutoTextureMatrix[10] = -M[10];
            mAutoTextureMatrix[3] = mAutoTextureMatrix[7] = mAutoTextureMatrix[11] = 0.0f;
            mAutoTextureMatrix[12] = mAutoTextureMatrix[13] = mAutoTextureMatrix[14] = 0.0f;
            mAutoTextureMatrix[15] = 1.0f;

            break;
        case ENVIRONMENT_MAP_NORMAL:
            glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP );
            glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP );
            glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP );

            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_S );
            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_T );
            mStateCacheManager->enableTextureCoordGen( GL_TEXTURE_GEN_R );
            mStateCacheManager->disableTextureCoordGen( GL_TEXTURE_GEN_Q );
            break;
        case PROJECTIVE_TEXTURE:
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            glTexGenfv(GL_S, GL_EYE_PLANE, eyePlaneS);
            glTexGenfv(GL_T, GL_EYE_PLANE, eyePlaneT);
            glTexGenfv(GL_R, GL_EYE_PLANE, eyePlaneR);
            glTexGenfv(GL_Q, GL_EYE_PLANE, eyePlaneQ);
            mStateCacheManager->enableTextureCoordGen(GL_TEXTURE_GEN_S);
            mStateCacheManager->enableTextureCoordGen(GL_TEXTURE_GEN_T);
            mStateCacheManager->enableTextureCoordGen(GL_TEXTURE_GEN_R);
            mStateCacheManager->enableTextureCoordGen(GL_TEXTURE_GEN_Q);

            mUseAutoTextureMatrix = true;

            // Set scale and translation matrix for projective textures
            projectionBias = Matrix4::CLIPSPACE2DTOIMAGESPACE;

            projectionBias = projectionBias * frustum->getProjectionMatrix();
            if(mTexProjRelative)
            {
                Matrix4 viewMatrix;
                frustum->calcViewMatrixRelative(mTexProjRelativeOrigin, viewMatrix);
                projectionBias = projectionBias * viewMatrix;
            }
            else
            {
                projectionBias = projectionBias * frustum->getViewMatrix();
            }
            projectionBias = projectionBias * mWorldMatrix;

            makeGLMatrix(mAutoTextureMatrix, projectionBias);
            break;
        default:
            break;
        }
    }
    //-----------------------------------------------------------------------------
    auto GLRenderSystem::getTextureAddressingMode(
        TextureAddressingMode tam) const -> GLint
    {
        using enum TextureAddressingMode;
        switch(tam)
        {
        default:
        case WRAP:
            return GL_REPEAT;
        case MIRROR:
            return GL_MIRRORED_REPEAT;
        case CLAMP:
            return GL_CLAMP_TO_EDGE;
        case BORDER:
            return GL_CLAMP_TO_BORDER;
        }

    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setTextureAddressingMode(size_t stage, const Sampler::UVWAddressingMode& uvw)
    {
        if (!mStateCacheManager->activateGLTextureUnit(stage))
            return;
        mStateCacheManager->setTexParameteri( mTextureTypes[stage], GL_TEXTURE_WRAP_S,
                         getTextureAddressingMode(uvw.u));
        mStateCacheManager->setTexParameteri( mTextureTypes[stage], GL_TEXTURE_WRAP_T,
                         getTextureAddressingMode(uvw.v));
        mStateCacheManager->setTexParameteri( mTextureTypes[stage], GL_TEXTURE_WRAP_R,
                         getTextureAddressingMode(uvw.w));
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setTextureMatrix(size_t stage, const Matrix4& xform)
    {
        if (stage >= mFixedFunctionTextureUnits)
        {
            // Can't do this
            return;
        }

        if (!mStateCacheManager->activateGLTextureUnit(stage))
            return;
        glMatrixMode(GL_TEXTURE);

        // Load this matrix in
        glLoadMatrixf(Matrix4f(xform.transpose())[0]);

        if (mUseAutoTextureMatrix)
        {
            // Concat auto matrix
            glMultMatrixf(mAutoTextureMatrix);
        }

        glMatrixMode(GL_MODELVIEW);
    }
    //-----------------------------------------------------------------------------
    auto GLRenderSystem::getBlendMode(SceneBlendFactor ogreBlend) const -> GLint
    {
        using enum SceneBlendFactor;
        switch(ogreBlend)
        {
        case ONE:
            return GL_ONE;
        case ZERO:
            return GL_ZERO;
        case DEST_COLOUR:
            return GL_DST_COLOR;
        case SOURCE_COLOUR:
            return GL_SRC_COLOR;
        case ONE_MINUS_DEST_COLOUR:
            return GL_ONE_MINUS_DST_COLOR;
        case ONE_MINUS_SOURCE_COLOUR:
            return GL_ONE_MINUS_SRC_COLOR;
        case DEST_ALPHA:
            return GL_DST_ALPHA;
        case SOURCE_ALPHA:
            return GL_SRC_ALPHA;
        case ONE_MINUS_DEST_ALPHA:
            return GL_ONE_MINUS_DST_ALPHA;
        case ONE_MINUS_SOURCE_ALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;
        };
        // to keep compiler happy
        return GL_ONE;
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setAlphaRejectSettings(CompareFunction func, unsigned char value, bool alphaToCoverage)
    {
        bool enable = func != CompareFunction::ALWAYS_PASS;

        mStateCacheManager->setEnabled(GL_ALPHA_TEST, enable);

        if(enable)
        {
            glAlphaFunc(convertCompareFunction(func), value / 255.0f);
        }

        if (getCapabilities()->hasCapability(Capabilities::ALPHA_TO_COVERAGE))
        {
            mStateCacheManager->setEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE, alphaToCoverage && enable);
        }

    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setViewport(Viewport *vp)
    {
        // Check if viewport is different
        if (!vp)
        {
            mActiveViewport = nullptr;
            _setRenderTarget(nullptr);
        }
        else if (vp != mActiveViewport || vp->_isUpdated())
        {
            RenderTarget* target;
            target = vp->getTarget();
            _setRenderTarget(target);
            mActiveViewport = vp;

            // Calculate the "lower-left" corner of the viewport
            Rect vpRect = vp->getActualDimensions();
            if (!target->requiresTextureFlipping())
            {
                // Convert "upper-left" corner to "lower-left"
                std::swap(vpRect.top, vpRect.bottom);
                vpRect.top = target->getHeight() - vpRect.top;
                vpRect.bottom = target->getHeight() - vpRect.bottom;
            }
            mStateCacheManager->setViewport(vpRect);

            vp->_clearUpdatedFlag();
        }
    }

    //-----------------------------------------------------------------------------
    void GLRenderSystem::_endFrame()
    {
        // unbind GPU programs at end of frame
        // this is mostly to avoid holding bound programs that might get deleted
        // outside via the resource manager
        unbindGpuProgram(GpuProgramType::VERTEX_PROGRAM);
        unbindGpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
    }

    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setCullingMode(CullingMode mode)
    {
        mCullingMode = mode;
        // NB: Because two-sided stencil API dependence of the front face, we must
        // use the same 'winding' for the front face everywhere. As the OGRE default
        // culling mode is clockwise, we also treat anticlockwise winding as front
        // face for consistently. On the assumption that, we can't change the front
        // face by glFrontFace anywhere.

        GLenum cullMode;
        bool flip = flipFrontFace();

        using enum CullingMode;
        switch( mode )
        {
        case NONE:
            mStateCacheManager->setEnabled( GL_CULL_FACE, false );
            return;
        case CLOCKWISE:
            cullMode = flip ? GL_FRONT : GL_BACK;
            break;
        case ANTICLOCKWISE:
            cullMode = flip ? GL_BACK : GL_FRONT;
            break;
        }

        mStateCacheManager->setEnabled( GL_CULL_FACE, true );
        mStateCacheManager->setCullFace( cullMode );
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setDepthBufferParams(bool depthTest, bool depthWrite, CompareFunction depthFunction)
    {
        _setDepthBufferCheckEnabled(depthTest);
        _setDepthBufferWriteEnabled(depthWrite);
        _setDepthBufferFunction(depthFunction);
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setDepthBufferCheckEnabled(bool enabled)
    {
        if (enabled)
        {
            mStateCacheManager->setClearDepth(1.0f);
        }
        mStateCacheManager->setEnabled(GL_DEPTH_TEST, enabled);
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setDepthBufferWriteEnabled(bool enabled)
    {
        GLboolean flag = enabled ? GL_TRUE : GL_FALSE;
        mStateCacheManager->setDepthMask( flag );
        // Store for reference in _beginFrame
        mDepthWrite = enabled;
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setDepthBufferFunction(CompareFunction func)
    {
        mStateCacheManager->setDepthFunc(convertCompareFunction(func));
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setDepthBias(float constantBias, float slopeScaleBias)
    {
        bool enable = constantBias != 0 || slopeScaleBias != 0;
        mStateCacheManager->setEnabled(GL_POLYGON_OFFSET_FILL, enable);
        mStateCacheManager->setEnabled(GL_POLYGON_OFFSET_POINT, enable);
        mStateCacheManager->setEnabled(GL_POLYGON_OFFSET_LINE, enable);

        if (enable)
        {
            glPolygonOffset(-slopeScaleBias, -constantBias);
        }
    }
    //-----------------------------------------------------------------------------
    static auto getBlendOp(SceneBlendOperation op) -> GLenum
    {
        using enum SceneBlendOperation;
        switch (op)
        {
        case ADD:
            return GL_FUNC_ADD;
        case SUBTRACT:
            return GL_FUNC_SUBTRACT;
        case REVERSE_SUBTRACT:
            return GL_FUNC_REVERSE_SUBTRACT;
        case MIN:
            return GL_MIN;
        case MAX:
            return GL_MAX;
        }
        return GL_FUNC_ADD;
    }
    void GLRenderSystem::setColourBlendState(const ColourBlendState& state)
    {
        // record this
        mCurrentBlend = state;

        if (state.blendingEnabled())
        {
            mStateCacheManager->setEnabled(GL_BLEND, true);
            mStateCacheManager->setBlendFunc(
                getBlendMode(state.sourceFactor), getBlendMode(state.destFactor),
                getBlendMode(state.sourceFactorAlpha), getBlendMode(state.destFactorAlpha));
        }
        else
        {
            mStateCacheManager->setEnabled(GL_BLEND, false);
        }

        mStateCacheManager->setBlendEquation(getBlendOp(state.operation), getBlendOp(state.alphaOperation));
        mStateCacheManager->setColourMask(state.writeR, state.writeG, state.writeB, state.writeA);
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::setLightingEnabled(bool enabled)
    {
        mStateCacheManager->setEnabled(GL_LIGHTING, enabled);
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setFog(FogMode mode)
    {

        GLint fogMode;
        using enum FogMode;
        switch (mode)
        {
        case EXP:
            fogMode = GL_EXP;
            break;
        case EXP2:
            fogMode = GL_EXP2;
            break;
        case LINEAR:
            fogMode = GL_LINEAR;
            break;
        default:
            // Give up on it
            mStateCacheManager->setEnabled(GL_FOG, false);
            mFixedFunctionParams->clearAutoConstant(18);
            mFixedFunctionParams->clearAutoConstant(19);
            return;
        }

        mFixedFunctionParams->setAutoConstant(18, GpuProgramParameters::AutoConstantType::FOG_PARAMS);
        mFixedFunctionParams->setAutoConstant(19, GpuProgramParameters::AutoConstantType::FOG_COLOUR);
        mStateCacheManager->setEnabled(GL_FOG, true);
        glFogi(GL_FOG_MODE, fogMode);
    }

    void GLRenderSystem::_setPolygonMode(PolygonMode level)
    {
        GLenum glmode;
        using enum PolygonMode;
        switch(level)
        {
        case POINTS:
            glmode = GL_POINT;
            break;
        case WIREFRAME:
            glmode = GL_LINE;
            break;
        default:
        case SOLID:
            glmode = GL_FILL;
            break;
        }
        mStateCacheManager->setPolygonMode(glmode);
    }
    //---------------------------------------------------------------------
    void GLRenderSystem::setStencilState(const StencilState& state)
    {
        mStateCacheManager->setEnabled(GL_STENCIL_TEST, state.enabled);

        if(!state.enabled)
            return;

        bool flip;
        mStencilWriteMask = state.writeMask;

        auto compareOp = convertCompareFunction(state.compareOp);

        if (state.twoSidedOperation)
        {
            if (!mCurrentCapabilities->hasCapability(Capabilities::TWO_SIDED_STENCIL))
                OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "2-sided stencils are not supported");

            // NB: We should always treat CCW as front face for consistent with default
            // culling mode. Therefore, we must take care with two-sided stencil settings.
            flip = flipFrontFace();
            if(GLAD_GL_VERSION_2_0) // New GL2 commands
            {
                // Back
                glStencilMaskSeparate(GL_BACK, state.writeMask);
                glStencilFuncSeparate(GL_BACK, compareOp, state.referenceValue, state.compareMask);
                glStencilOpSeparate(GL_BACK, 
                    convertStencilOp(state.stencilFailOp, !flip),
                    convertStencilOp(state.depthFailOp, !flip),
                    convertStencilOp(state.depthStencilPassOp, !flip));
                // Front
                glStencilMaskSeparate(GL_FRONT, state.writeMask);
                glStencilFuncSeparate(GL_FRONT, compareOp, state.referenceValue, state.compareMask);
                glStencilOpSeparate(GL_FRONT, 
                    convertStencilOp(state.stencilFailOp, flip),
                    convertStencilOp(state.depthFailOp, flip),
                    convertStencilOp(state.depthStencilPassOp, flip));
            }
            else // EXT_stencil_two_side
            {
                mStateCacheManager->setEnabled(GL_STENCIL_TEST_TWO_SIDE_EXT, true);
                // Back
                glActiveStencilFaceEXT(GL_BACK);
                mStateCacheManager->setStencilMask(state.writeMask);
                glStencilFunc(compareOp, state.referenceValue, state.compareMask);
                glStencilOp(
                    convertStencilOp(state.stencilFailOp, !flip),
                    convertStencilOp(state.depthFailOp, !flip),
                    convertStencilOp(state.depthStencilPassOp, !flip));
                // Front
                glActiveStencilFaceEXT(GL_FRONT);
                mStateCacheManager->setStencilMask(state.writeMask);
                glStencilFunc(compareOp, state.referenceValue, state.compareMask);
                glStencilOp(
                    convertStencilOp(state.stencilFailOp, flip),
                    convertStencilOp(state.depthFailOp, flip),
                    convertStencilOp(state.depthStencilPassOp, flip));
            }
        }
        else
        {
            if(!GLAD_GL_VERSION_2_0)
                mStateCacheManager->setEnabled(GL_STENCIL_TEST_TWO_SIDE_EXT, false);

            flip = false;
            mStateCacheManager->setStencilMask(state.writeMask);
            glStencilFunc(compareOp, state.referenceValue, state.compareMask);
            glStencilOp(
                convertStencilOp(state.stencilFailOp, flip),
                convertStencilOp(state.depthFailOp, flip),
                convertStencilOp(state.depthStencilPassOp, flip));
        }
    }
    //---------------------------------------------------------------------
    auto GLRenderSystem::convertCompareFunction(CompareFunction func) const -> GLint
    {
        using enum CompareFunction;
        switch(func)
        {
        case ALWAYS_FAIL:
            return GL_NEVER;
        case ALWAYS_PASS:
            return GL_ALWAYS;
        case LESS:
            return GL_LESS;
        case LESS_EQUAL:
            return GL_LEQUAL;
        case EQUAL:
            return GL_EQUAL;
        case NOT_EQUAL:
            return GL_NOTEQUAL;
        case GREATER_EQUAL:
            return GL_GEQUAL;
        case GREATER:
            return GL_GREATER;
        };
        // to keep compiler happy
        return GL_ALWAYS;
    }
    //---------------------------------------------------------------------
    auto GLRenderSystem::convertStencilOp(StencilOperation op, bool invert) const -> GLint
    {
        using enum StencilOperation;
        switch(op)
        {
        case KEEP:
            return GL_KEEP;
        case ZERO:
            return GL_ZERO;
        case REPLACE:
            return GL_REPLACE;
        case INCREMENT:
            return invert ? GL_DECR : GL_INCR;
        case DECREMENT:
            return invert ? GL_INCR : GL_DECR;
        case INCREMENT_WRAP:
            return invert ? GL_DECR_WRAP_EXT : GL_INCR_WRAP_EXT;
        case DECREMENT_WRAP:
            return invert ? GL_INCR_WRAP_EXT : GL_DECR_WRAP_EXT;
        case INVERT:
            return GL_INVERT;
        };
        // to keep compiler happy
        return static_cast<GLint>(KEEP);
    }
    //---------------------------------------------------------------------
    void GLRenderSystem::_setTextureUnitFiltering(size_t unit,
                                                  FilterType ftype, FilterOptions fo)
    {
        if (!mStateCacheManager->activateGLTextureUnit(unit))
            return;
        using enum FilterType;
        switch(ftype)
        {
        case Min:
            mMinFilter = fo;
            // Combine with existing mip filter
            mStateCacheManager->setTexParameteri(
                mTextureTypes[unit],
                GL_TEXTURE_MIN_FILTER,
                getCombinedMinMipFilter(mMinFilter, mMipFilter));
            break;
        case Mag:
            switch (fo)
            {
                using enum FilterOptions;
            case ANISOTROPIC: // GL treats linear and aniso the same
            case LINEAR:
                mStateCacheManager->setTexParameteri(
                    mTextureTypes[unit],
                    GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);
                break;
            case POINT:
            case NONE:
                mStateCacheManager->setTexParameteri(
                    mTextureTypes[unit],
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
                break;
            }
            break;
        case Mip:
            mMipFilter = fo;
            // Combine with existing min filter
            mStateCacheManager->setTexParameteri(
                mTextureTypes[unit],
                GL_TEXTURE_MIN_FILTER,
                getCombinedMinMipFilter(mMinFilter, mMipFilter));
            break;
        }
    }
    //-----------------------------------------------------------------------------
    void GLRenderSystem::_setTextureBlendMode(size_t stage, const LayerBlendModeEx& bm)
    {
        if (stage >= mFixedFunctionTextureUnits)
        {
            // Can't do this
            return;
        }

        GLenum src1op, src2op, cmd;
        GLfloat cv1[4], cv2[4];

        if (bm.blendType == LayerBlendType::COLOUR)
        {
            cv1[0] = bm.colourArg1.r;
            cv1[1] = bm.colourArg1.g;
            cv1[2] = bm.colourArg1.b;
            cv1[3] = bm.colourArg1.a;
            mManualBlendColours[stage][0] = bm.colourArg1;


            cv2[0] = bm.colourArg2.r;
            cv2[1] = bm.colourArg2.g;
            cv2[2] = bm.colourArg2.b;
            cv2[3] = bm.colourArg2.a;
            mManualBlendColours[stage][1] = bm.colourArg2;
        }

        if (bm.blendType == LayerBlendType::ALPHA)
        {
            cv1[0] = mManualBlendColours[stage][0].r;
            cv1[1] = mManualBlendColours[stage][0].g;
            cv1[2] = mManualBlendColours[stage][0].b;
            cv1[3] = bm.alphaArg1;

            cv2[0] = mManualBlendColours[stage][1].r;
            cv2[1] = mManualBlendColours[stage][1].g;
            cv2[2] = mManualBlendColours[stage][1].b;
            cv2[3] = bm.alphaArg2;
        }

        switch (bm.source1)
        {
        using enum LayerBlendSource;
        case CURRENT:
            src1op = GL_PREVIOUS;
            break;
        case TEXTURE:
            src1op = GL_TEXTURE;
            break;
        case MANUAL:
            src1op = GL_CONSTANT;
            break;
        case DIFFUSE:
            src1op = GL_PRIMARY_COLOR;
            break;
            // XXX
        case SPECULAR:
            src1op = GL_PRIMARY_COLOR;
            break;
        default:
            src1op = 0;
        }

        switch (bm.source2)
        {
        using enum LayerBlendSource;
        case CURRENT:
            src2op = GL_PREVIOUS;
            break;
        case TEXTURE:
            src2op = GL_TEXTURE;
            break;
        case MANUAL:
            src2op = GL_CONSTANT;
            break;
        case DIFFUSE:
            src2op = GL_PRIMARY_COLOR;
            break;
            // XXX
        case SPECULAR:
            src2op = GL_PRIMARY_COLOR;
            break;
        default:
            src2op = 0;
        }

        switch (bm.operation)
        {
            using enum LayerBlendOperationEx;
        case SOURCE1:
            cmd = GL_REPLACE;
            break;
        case SOURCE2:
            cmd = GL_REPLACE;
            break;
        case MODULATE:
            cmd = GL_MODULATE;
            break;
        case MODULATE_X2:
            cmd = GL_MODULATE;
            break;
        case MODULATE_X4:
            cmd = GL_MODULATE;
            break;
        case ADD:
            cmd = GL_ADD;
            break;
        case ADD_SIGNED:
            cmd = GL_ADD_SIGNED;
            break;
        case ADD_SMOOTH:
            cmd = GL_INTERPOLATE;
            break;
        case SUBTRACT:
            cmd = GL_SUBTRACT;
            break;
        case BLEND_DIFFUSE_COLOUR:
            cmd = GL_INTERPOLATE;
            break;
        case BLEND_DIFFUSE_ALPHA:
            cmd = GL_INTERPOLATE;
            break;
        case BLEND_TEXTURE_ALPHA:
            cmd = GL_INTERPOLATE;
            break;
        case BLEND_CURRENT_ALPHA:
            cmd = GL_INTERPOLATE;
            break;
        case BLEND_MANUAL:
            cmd = GL_INTERPOLATE;
            break;
        case DOTPRODUCT:
            cmd = GL_DOT3_RGB;
            break;
        default:
            cmd = 0;
        }

        if (!mStateCacheManager->activateGLTextureUnit(stage))
            return;
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

        if (bm.blendType == LayerBlendType::COLOUR)
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, cmd);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, src1op);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, src2op);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_CONSTANT);
        }
        else
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, cmd);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, src1op);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, src2op);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_CONSTANT);
        }

        float blendValue[4] = {0, 0, 0, static_cast<float>(bm.factor)};
        using enum LayerBlendOperationEx;
        switch (bm.operation)
        {
        case BLEND_DIFFUSE_COLOUR:
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PRIMARY_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_PRIMARY_COLOR);
            break;
        case BLEND_DIFFUSE_ALPHA:
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PRIMARY_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_PRIMARY_COLOR);
            break;
        case BLEND_TEXTURE_ALPHA:
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_TEXTURE);
            break;
        case BLEND_CURRENT_ALPHA:
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PREVIOUS);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_PREVIOUS);
            break;
        case BLEND_MANUAL:
            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, blendValue);
            break;
        default:
            break;
        };

        switch (bm.operation)
        {
        case MODULATE_X2:
            glTexEnvi(GL_TEXTURE_ENV, bm.blendType == LayerBlendType::COLOUR ?
                      GL_RGB_SCALE : GL_ALPHA_SCALE, 2);
            break;
        case MODULATE_X4:
            glTexEnvi(GL_TEXTURE_ENV, bm.blendType == LayerBlendType::COLOUR ?
                      GL_RGB_SCALE : GL_ALPHA_SCALE, 4);
            break;
        default:
            glTexEnvi(GL_TEXTURE_ENV, bm.blendType == LayerBlendType::COLOUR ?
                      GL_RGB_SCALE : GL_ALPHA_SCALE, 1);
            break;
        }

        if (bm.blendType == LayerBlendType::COLOUR){
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
            if (bm.operation == BLEND_DIFFUSE_COLOUR){
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_COLOR);
            } else {
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
            }
        }

        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GL_SRC_ALPHA);
        if(bm.source1 == LayerBlendSource::MANUAL)
            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, cv1);
        if (bm.source2 == LayerBlendSource::MANUAL)
            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, cv2);
    }
    //---------------------------------------------------------------------
    void GLRenderSystem::_render(const RenderOperation& op)
    {
        // Call super class
        RenderSystem::_render(op);

        mMaxBuiltInTextureAttribIndex = 0;

        HardwareVertexBufferSharedPtr globalInstanceVertexBuffer = getGlobalInstanceVertexBuffer();
        VertexDeclaration* globalVertexDeclaration = getGlobalInstanceVertexBufferVertexDeclaration();
        size_t numberOfInstances = op.numberOfInstances;

        if (op.useGlobalInstancingVertexBufferIsAvailable)
        {
            numberOfInstances *= getGlobalNumberOfInstances();
        }

        const VertexDeclaration::VertexElementList& decl =
            op.vertexData->vertexDeclaration->getElements();
        for (const auto & elem : decl)
        {
            size_t source = elem.getSource();

            if (!op.vertexData->vertexBufferBinding->isBufferBound(source))
                continue; // skip unbound elements

            HardwareVertexBufferSharedPtr vertexBuffer =
                op.vertexData->vertexBufferBinding->getBuffer(source);

            bindVertexElementToGpu(elem, vertexBuffer, op.vertexData->vertexStart);
        }

        if( globalInstanceVertexBuffer && globalVertexDeclaration != nullptr )
        {
            for (const auto & elem : globalVertexDeclaration->getElements())
            {
                bindVertexElementToGpu(elem, globalInstanceVertexBuffer, 0);
            }
        }

        bool multitexturing = (getCapabilities()->getNumTextureUnits() > 1);
        if (multitexturing)
            glClientActiveTextureARB(GL_TEXTURE0);

        // Find the correct type to render
        GLint primType;
        auto operationType = op.operationType;
        using enum RenderOperation::OperationType;
        // Use adjacency if there is a geometry program and it requested adjacency info
        if(mGeometryProgramBound && mCurrentGeometryProgram && dynamic_cast<GpuProgram*>(mCurrentGeometryProgram)->isAdjacencyInfoRequired())
            operationType |= DETAIL_ADJACENCY_BIT;
        switch (operationType)
        {
        case POINT_LIST:
            primType = GL_POINTS;
            break;
        case LINE_LIST:
            primType = GL_LINES;
            break;
        case LINE_LIST_ADJ:
            primType = GL_LINES_ADJACENCY_EXT;
            break;
        case LINE_STRIP:
            primType = GL_LINE_STRIP;
            break;
        case LINE_STRIP_ADJ:
            primType = GL_LINE_STRIP_ADJACENCY_EXT;
            break;
        default:
        case TRIANGLE_LIST:
            primType = GL_TRIANGLES;
            break;
        case TRIANGLE_LIST_ADJ:
            primType = GL_TRIANGLES_ADJACENCY_EXT;
            break;
        case TRIANGLE_STRIP:
            primType = GL_TRIANGLE_STRIP;
            break;
        case TRIANGLE_STRIP_ADJ:
            primType = GL_TRIANGLE_STRIP_ADJACENCY_EXT;
            break;
        case TRIANGLE_FAN:
            primType = GL_TRIANGLE_FAN;
            break;
        }

        if (op.useIndexes)
        {
            void* pBufferData = nullptr;
            mStateCacheManager->bindGLBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB,
                                op.indexData->indexBuffer->_getImpl<GLHardwareBuffer>()->getGLBufferId());

            pBufferData = VBO_BUFFER_OFFSET(
                op.indexData->indexStart * op.indexData->indexBuffer->getIndexSize());

            GLenum indexType = (op.indexData->indexBuffer->getType() == HardwareIndexBuffer::IndexType::_16BIT) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

            do
            {
                if(numberOfInstances > 1)
                {
                    glDrawElementsInstancedARB(primType, op.indexData->indexCount, indexType, pBufferData, numberOfInstances);
                }
                else
                {
                    glDrawElements(primType, op.indexData->indexCount, indexType, pBufferData);
                }
            } while (updatePassIterationRenderState());

        }
        else
        {
            do
            {
                if(numberOfInstances > 1)
                {
                    glDrawArraysInstancedARB(primType, 0, op.vertexData->vertexCount, numberOfInstances);
                }
                else
                {
                    glDrawArrays(primType, 0, op.vertexData->vertexCount);
                }
            } while (updatePassIterationRenderState());
        }

        glDisableClientState( GL_VERTEX_ARRAY );
        // only valid up to GL_MAX_TEXTURE_UNITS, which is recorded in mFixedFunctionTextureUnits
        if (multitexturing)
        {
            unsigned short mNumEnabledTextures = std::max(std::min((unsigned short)mDisabledTexUnitsFrom, mFixedFunctionTextureUnits), (unsigned short)(mMaxBuiltInTextureAttribIndex + 1));		
            for (unsigned short i = 0; i < mNumEnabledTextures; i++)
            {
                // No need to disable for texture units that weren't used
                glClientActiveTextureARB(GL_TEXTURE0 + i);
                glDisableClientState( GL_TEXTURE_COORD_ARRAY );
            }
            glClientActiveTextureARB(GL_TEXTURE0);
        }
        else
        {
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );
        }
        glDisableClientState( GL_NORMAL_ARRAY );
        glDisableClientState( GL_COLOR_ARRAY );
        if (GLAD_GL_EXT_secondary_color)
        {
            glDisableClientState( GL_SECONDARY_COLOR_ARRAY );
        }
        // unbind any custom attributes
        for (unsigned int & ai : mRenderAttribsBound)
        {
            glDisableVertexAttribArrayARB(ai);
        }

        // unbind any instance attributes
        for (unsigned int & ai : mRenderInstanceAttribsBound)
        {
            glVertexAttribDivisorARB(ai, 0);
        }

        mRenderAttribsBound.clear();
        mRenderInstanceAttribsBound.clear();

    }
    //---------------------------------------------------------------------
    void GLRenderSystem::setNormaliseNormals(bool normalise)
    {
        mStateCacheManager->setEnabled(GL_NORMALIZE, normalise);
    }
    //---------------------------------------------------------------------
    void GLRenderSystem::bindGpuProgram(GpuProgram* prg)
    {
        if (!prg)
        {
            OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR,
                        "Null program bound.",
                        "GLRenderSystem::bindGpuProgram");
        }

        auto* glprg = dynamic_cast<GLGpuProgramBase*>(prg);

        // Unbind previous gpu program first.
        //
        // Note:
        //  1. Even if both previous and current are the same object, we can't
        //     bypass re-bind completely since the object itself maybe modified.
        //     But we can bypass unbind based on the assumption that object
        //     internally GL program type shouldn't be changed after it has
        //     been created. The behavior of bind to a GL program type twice
        //     should be same as unbind and rebind that GL program type, even
        //     for difference objects.
        //  2. We also assumed that the program's type (vertex or fragment) should
        //     not be changed during it's in using. If not, the following switch
        //     statement will confuse GL state completely, and we can't fix it
        //     here. To fix this case, we must coding the program implementation
        //     itself, if type is changing (during load/unload, etc), and it's inuse,
        //     unbind and notify render system to correct for its state.
        //
        using enum GpuProgramType;
        switch (prg->getType())
        {
        case VERTEX_PROGRAM:
            if (mCurrentVertexProgram != glprg)
            {
                if (mCurrentVertexProgram)
                    mCurrentVertexProgram->unbindProgram();
                mCurrentVertexProgram = glprg;
            }
            break;

        case FRAGMENT_PROGRAM:
            if (mCurrentFragmentProgram != glprg)
            {
                if (mCurrentFragmentProgram)
                    mCurrentFragmentProgram->unbindProgram();
                mCurrentFragmentProgram = glprg;
            }
            break;
        case GEOMETRY_PROGRAM:
            if (mCurrentGeometryProgram != glprg)
            {
                if (mCurrentGeometryProgram)
                    mCurrentGeometryProgram->unbindProgram();
                mCurrentGeometryProgram = glprg;
            }
            break;
        case COMPUTE_PROGRAM:
        case DOMAIN_PROGRAM:
        case HULL_PROGRAM:
            break;
        default:
            break;
        }

        // Bind the program
        glprg->bindProgram();

        RenderSystem::bindGpuProgram(prg);
    }
    //---------------------------------------------------------------------
    void GLRenderSystem::unbindGpuProgram(GpuProgramType gptype)
    {

        if (gptype == GpuProgramType::VERTEX_PROGRAM && mCurrentVertexProgram)
        {
            mActiveVertexGpuProgramParameters.reset();
            mCurrentVertexProgram->unbindProgram();
            mCurrentVertexProgram = nullptr;
        }
        else if (gptype == GpuProgramType::GEOMETRY_PROGRAM && mCurrentGeometryProgram)
        {
            mActiveGeometryGpuProgramParameters.reset();
            mCurrentGeometryProgram->unbindProgram();
            mCurrentGeometryProgram = nullptr;
        }
        else if (gptype == GpuProgramType::FRAGMENT_PROGRAM && mCurrentFragmentProgram)
        {
            mActiveFragmentGpuProgramParameters.reset();
            mCurrentFragmentProgram->unbindProgram();
            mCurrentFragmentProgram = nullptr;
        }
        RenderSystem::unbindGpuProgram(gptype);

    }
    //---------------------------------------------------------------------
    void GLRenderSystem::bindGpuProgramParameters(GpuProgramType gptype, const GpuProgramParametersPtr& params, GpuParamVariability mask)
    {
        if (!!(mask & GpuParamVariability::GLOBAL))
        {
            // We could maybe use GL_EXT_bindable_uniform here to produce Dx10-style
            // shared constant buffers, but GPU support seems fairly weak?
            // for now, just copy
            params->_copySharedParams();
        }

        using enum GpuProgramType;
        switch (gptype)
        {
        case VERTEX_PROGRAM:
            mActiveVertexGpuProgramParameters = params;
            mCurrentVertexProgram->bindProgramParameters(params, mask);
            break;
        case GEOMETRY_PROGRAM:
            mActiveGeometryGpuProgramParameters = params;
            mCurrentGeometryProgram->bindProgramParameters(params, mask);
            break;
        case FRAGMENT_PROGRAM:
            mActiveFragmentGpuProgramParameters = params;
            mCurrentFragmentProgram->bindProgramParameters(params, mask);
            break;
        case COMPUTE_PROGRAM:
        case DOMAIN_PROGRAM:
        case HULL_PROGRAM:
            break;
        default:
            break;
        }
    }
    //---------------------------------------------------------------------
    void GLRenderSystem::setClipPlanesImpl(const PlaneList& clipPlanes)
    {
        // A note on GL user clipping:
        // When an ARB vertex program is enabled in GL, user clipping is completely
        // disabled. There is no way around this, it's just turned off.
        // When using GLSL, user clipping can work but you have to include a
        // glClipVertex command in your vertex shader.
        // Thus the planes set here may not actually be respected.


        size_t i = 0;
        size_t numClipPlanes;
        GLdouble clipPlane[4];

        // Save previous modelview
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        // just load view matrix (identity world)
        GLfloat mat[16];
        makeGLMatrix(mat, mViewMatrix);
        glLoadMatrixf(mat);

        numClipPlanes = clipPlanes.size();
        for (i = 0; i < numClipPlanes; ++i)
        {
            auto clipPlaneId = static_cast<GLenum>(GL_CLIP_PLANE0 + i);
            const Plane& plane = clipPlanes[i];

            if (i >= 6/*GL_MAX_CLIP_PLANES*/)
            {
                OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, "Unable to set clip plane",
                            "GLRenderSystem::setClipPlanes");
            }

            clipPlane[0] = plane.normal.x;
            clipPlane[1] = plane.normal.y;
            clipPlane[2] = plane.normal.z;
            clipPlane[3] = plane.d;

            glClipPlane(clipPlaneId, clipPlane);
            mStateCacheManager->setEnabled(clipPlaneId, true);
        }

        // disable remaining clip planes
        for ( ; i < 6/*GL_MAX_CLIP_PLANES*/; ++i)
        {
            mStateCacheManager->setEnabled(static_cast<GLenum>(GL_CLIP_PLANE0 + i), false);
        }

        // restore matrices
        glPopMatrix();
    }
    //---------------------------------------------------------------------
    void GLRenderSystem::setScissorTest(bool enabled, const Rect& rect)
    {
        mStateCacheManager->setEnabled(GL_SCISSOR_TEST, enabled);

        if (!enabled)
            return;

        // If request texture flipping, use "upper-left", otherwise use "lower-left"
        bool flipping = mActiveRenderTarget->requiresTextureFlipping();
        //  GL measures from the bottom, not the top
        long targetHeight = mActiveRenderTarget->getHeight();
        long top = flipping ? rect.top : targetHeight - rect.bottom;
        // NB GL uses width / height rather than right / bottom
        glScissor(rect.left, top, rect.width(), rect.height());
    }
    //---------------------------------------------------------------------
    void GLRenderSystem::clearFrameBuffer(FrameBufferType buffers,
                                          const ColourValue& colour, float depth, unsigned short stencil)
    {
        bool colourMask =
            !(mCurrentBlend.writeR && mCurrentBlend.writeG && mCurrentBlend.writeB && mCurrentBlend.writeA);

        if(mCurrentContext)
			mCurrentContext->setCurrent();

        GLbitfield flags = 0;
        if (!!(buffers & FrameBufferType::COLOUR))
        {
            flags |= GL_COLOR_BUFFER_BIT;
            // Enable buffer for writing if it isn't
            if (colourMask)
            {
                mStateCacheManager->setColourMask(true, true, true, true);
            }
            mStateCacheManager->setClearColour(colour.r, colour.g, colour.b, colour.a);
        }
        if (!!(buffers & FrameBufferType::DEPTH))
        {
            flags |= GL_DEPTH_BUFFER_BIT;
            // Enable buffer for writing if it isn't
            if (!mDepthWrite)
            {
                mStateCacheManager->setDepthMask( GL_TRUE );
            }
            mStateCacheManager->setClearDepth(depth);
        }
        if (!!(buffers & FrameBufferType::STENCIL))
        {
            flags |= GL_STENCIL_BUFFER_BIT;
            // Enable buffer for writing if it isn't
            mStateCacheManager->setStencilMask(0xFFFFFFFF);

            glClearStencil(stencil);
        }

        Rect vpRect = mActiveViewport->getActualDimensions();
        bool needScissorBox =
            vpRect != Rect{0, 0, static_cast<int>(mActiveRenderTarget->getWidth()), static_cast<int>(mActiveRenderTarget->getHeight())};
        if (needScissorBox)
        {
            // Should be enable scissor test due the clear region is
            // relied on scissor box bounds.
            setScissorTest(true, vpRect);
        }

        // Clear buffers
        glClear(flags);

        // Restore scissor test
        if (needScissorBox)
        {
            setScissorTest(false);
        }

        // Reset buffer write state
        if (!mDepthWrite && !!(buffers & FrameBufferType::DEPTH))
        {
            mStateCacheManager->setDepthMask( GL_FALSE );
        }
        if ((!!colourMask && !!(buffers & FrameBufferType::COLOUR)))
        {
            mStateCacheManager->setColourMask(mCurrentBlend.writeR, mCurrentBlend.writeG,
                                              mCurrentBlend.writeB, mCurrentBlend.writeA);
        }
        if (!!(buffers & FrameBufferType::STENCIL))
        {
            mStateCacheManager->setStencilMask(mStencilWriteMask);
        }
    }
    //---------------------------------------------------------------------
    auto GLRenderSystem::createHardwareOcclusionQuery() -> HardwareOcclusionQuery*
    {
        auto* ret = new GLHardwareOcclusionQuery();
        mHwOcclusionQueries.push_back(ret);
        return ret;
    }
    //---------------------------------------------------------------------
    void GLRenderSystem::_oneTimeContextInitialization()
    {
        // Set nicer lighting model -- d3d9 has this by default
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
        mStateCacheManager->setEnabled(GL_COLOR_SUM, true);
        mStateCacheManager->setEnabled(GL_DITHER, false);

        // Check for FSAA
        // Enable the extension if it was enabled by the GLSupport
        if (checkExtension("GL_ARB_multisample"))
        {
            int fsaa_active = false;
            glGetIntegerv(GL_SAMPLE_BUFFERS_ARB,(GLint*)&fsaa_active);
            if(fsaa_active)
            {
                mStateCacheManager->setEnabled(GL_MULTISAMPLE_ARB, true);
                LogManager::getSingleton().logMessage("Using FSAA from GL_ARB_multisample extension.");
            }            
        }

		if (checkExtension("GL_ARB_seamless_cube_map"))
		{
            // Enable seamless cube maps
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		}
    }

    //---------------------------------------------------------------------
    void GLRenderSystem::_switchContext(GLContext *context)
    {
        // Unbind GPU programs and rebind to new context later, because
        // scene manager treat render system as ONE 'context' ONLY, and it
        // cached the GPU programs using state.
        if (mCurrentVertexProgram)
            mCurrentVertexProgram->unbindProgram();
        if (mCurrentGeometryProgram)
            mCurrentGeometryProgram->unbindProgram();
        if (mCurrentFragmentProgram)
            mCurrentFragmentProgram->unbindProgram();

        // Disable lights
        for (unsigned short i = 0; i < mCurrentLights; ++i)
        {
            setGLLight(i, false);
        }
        mCurrentLights = 0;

        // Disable textures
        _disableTextureUnitsFrom(0);

        // It's ready for switching
        if (mCurrentContext!=context)
        {
            mCurrentContext->endCurrent();
            mCurrentContext = context;
        }
        mCurrentContext->setCurrent();

        mStateCacheManager = mCurrentContext->createOrRetrieveStateCacheManager<GLStateCacheManager>();

        // Check if the context has already done one-time initialisation
        if(!mCurrentContext->getInitialized())
        {
            _oneTimeContextInitialization();
            mCurrentContext->setInitialized();
        }

        // Rebind GPU programs to new context
        if (mCurrentVertexProgram)
            mCurrentVertexProgram->bindProgram();
        if (mCurrentGeometryProgram)
            mCurrentGeometryProgram->bindProgram();
        if (mCurrentFragmentProgram)
            mCurrentFragmentProgram->bindProgram();

        // Must reset depth/colour write mask to according with user desired, otherwise,
        // clearFrameBuffer would be wrong because the value we are recorded may be
        // difference with the really state stored in GL context.
        mStateCacheManager->setDepthMask(mDepthWrite);
        mStateCacheManager->setColourMask(mCurrentBlend.writeR, mCurrentBlend.writeG,
                                          mCurrentBlend.writeB, mCurrentBlend.writeA);
        mStateCacheManager->setStencilMask(mStencilWriteMask);

    }
    //---------------------------------------------------------------------
    void GLRenderSystem::_setRenderTarget(RenderTarget *target)
    {
        // Unbind frame buffer object
        if(mActiveRenderTarget)
            mRTTManager->unbind(mActiveRenderTarget);

        mActiveRenderTarget = target;
        if (target)
        {
            // Switch context if different from current one
            GLContext *newContext = dynamic_cast<GLRenderTarget*>(target)->getContext();
            if(newContext && mCurrentContext != newContext)
            {
                _switchContext(newContext);
            }

            //Check the FBO's depth buffer status
            auto depthBuffer = static_cast<GLDepthBufferCommon*>(target->getDepthBuffer());

            if( target->getDepthBufferPool() != DepthBuffer::PoolId::NO_DEPTH &&
                (!depthBuffer || depthBuffer->getGLContext() != mCurrentContext ) )
            {
                //Depth is automatically managed and there is no depth buffer attached to this RT
                //or the Current context doesn't match the one this Depth buffer was created with
                setDepthBufferFor( target );
            }

            // Bind frame buffer object
            mRTTManager->bind(target);

            if (GLAD_GL_EXT_framebuffer_sRGB)
            {
                // Enable / disable sRGB states
                mStateCacheManager->setEnabled(GL_FRAMEBUFFER_SRGB_EXT, target->isHardwareGammaEnabled());
                // Note: could test GL_FRAMEBUFFER_SRGB_CAPABLE_EXT here before
                // enabling, but GL spec says incapable surfaces ignore the setting
                // anyway. We test the capability to enable isHardwareGammaEnabled.
            }
        }
    }
    //---------------------------------------------------------------------
    void GLRenderSystem::_unregisterContext(GLContext *context)
    {
        if(mCurrentContext == context) {
            // Change the context to something else so that a valid context
            // remains active. When this is the main context being unregistered,
            // we set the main context to 0.
            if(mCurrentContext != mMainContext) {
                _switchContext(mMainContext);
            } else {
                /// No contexts remain
                mCurrentContext->endCurrent();
                mCurrentContext = nullptr;
                mMainContext = nullptr;
                mStateCacheManager = nullptr;
            }
        }
    }

    //---------------------------------------------------------------------
    void GLRenderSystem::beginProfileEvent( std::string_view eventName )
    {
        markProfileEvent(::std::format("Begin Event: {}", eventName));
    }

    //---------------------------------------------------------------------
    void GLRenderSystem::endProfileEvent( )
    {
        markProfileEvent("End Event");
    }

    //---------------------------------------------------------------------
    void GLRenderSystem::markProfileEvent( std::string_view eventName )
    {
        if( eventName.empty() )
            return;

        if(GLAD_GL_GREMEDY_string_marker)
            glStringMarkerGREMEDY(eventName.length(), eventName.data());
    }

    //---------------------------------------------------------------------
    void GLRenderSystem::bindVertexElementToGpu(const VertexElement& elem,
                                                const HardwareVertexBufferSharedPtr& vertexBuffer,
                                                const size_t vertexStart)
    {
        void* pBufferData = nullptr;
        const GLHardwareBuffer* hwGlBuffer = vertexBuffer->_getImpl<GLHardwareBuffer>();

        mStateCacheManager->bindGLBuffer(GL_ARRAY_BUFFER_ARB, 
                        hwGlBuffer->getGLBufferId());
        pBufferData = VBO_BUFFER_OFFSET(elem.getOffset());

        if (vertexStart)
        {
            pBufferData = static_cast<char*>(pBufferData) + vertexStart * vertexBuffer->getVertexSize();
        }

        VertexElementSemantic sem = elem.getSemantic();
        bool multitexturing = (getCapabilities()->getNumTextureUnits() > 1);

        bool isCustomAttrib = false;
        if (mCurrentVertexProgram)
        {
            isCustomAttrib = !mEnableFixedPipeline || mCurrentVertexProgram->isAttributeValid(sem, elem.getIndex());

            if (vertexBuffer->isInstanceData())
            {
                GLint attrib = GLSLProgramCommon::getFixedAttributeIndex(sem, elem.getIndex());
                glVertexAttribDivisorARB(attrib, vertexBuffer->getInstanceDataStepRate() );
                mRenderInstanceAttribsBound.push_back(attrib);
            }
        }


        // Custom attribute support
        // tangents, binormals, blendweights etc always via this route
        // builtins may be done this way too
        if (isCustomAttrib)
        {
            GLint attrib = GLSLProgramCommon::getFixedAttributeIndex(sem, elem.getIndex());
            unsigned short typeCount = VertexElement::getTypeCount(elem.getType());
            GLboolean normalised = GL_FALSE;
            using enum VertexElementType;
            switch(elem.getType())
            {
            case UBYTE4_NORM:
            case SHORT2_NORM:
            case USHORT2_NORM:
            case SHORT4_NORM:
            case USHORT4_NORM:
                normalised = GL_TRUE;
                break;
            default:
                break;
            };

            glVertexAttribPointerARB(
                attrib,
                typeCount,
                GLHardwareBufferManager::getGLType(elem.getType()),
                normalised,
                static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                pBufferData);
            glEnableVertexAttribArrayARB(attrib);

            mRenderAttribsBound.push_back(attrib);
        }
        else
        {
            // fixed-function & builtin attribute support
            using enum VertexElementSemantic;
            switch(sem)
            {
            case POSITION:
                glVertexPointer(VertexElement::getTypeCount(
                    elem.getType()),
                                GLHardwareBufferManager::getGLType(elem.getType()),
                                static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                                pBufferData);
                glEnableClientState( GL_VERTEX_ARRAY );
                break;
            case NORMAL:
                glNormalPointer(
                    GLHardwareBufferManager::getGLType(elem.getType()),
                    static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                    pBufferData);
                glEnableClientState( GL_NORMAL_ARRAY );
                break;
            case DIFFUSE:
                glColorPointer(4,
                               GLHardwareBufferManager::getGLType(elem.getType()),
                               static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                               pBufferData);
                glEnableClientState( GL_COLOR_ARRAY );
                break;
            case SPECULAR:
                if (GLAD_GL_EXT_secondary_color)
                {
                    glSecondaryColorPointerEXT(4,
                                               GLHardwareBufferManager::getGLType(elem.getType()),
                                               static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                                               pBufferData);
                    glEnableClientState( GL_SECONDARY_COLOR_ARRAY );
                }
                break;
            case TEXTURE_COORDINATES:

                if (mCurrentVertexProgram)
                {
                    // Programmable pipeline - direct UV assignment
                    glClientActiveTextureARB(GL_TEXTURE0 + elem.getIndex());
                    glTexCoordPointer(
                        VertexElement::getTypeCount(elem.getType()),
                        GLHardwareBufferManager::getGLType(elem.getType()),
                        static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                        pBufferData);
                    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
                    if (elem.getIndex() > mMaxBuiltInTextureAttribIndex)
                        mMaxBuiltInTextureAttribIndex = elem.getIndex();
                }
                else
                {
                    // fixed function matching to units based on tex_coord_set
                    for (unsigned int i = 0; i < mDisabledTexUnitsFrom; i++)
                    {
                        // Only set this texture unit's texcoord pointer if it
                        // is supposed to be using this element's index
                        if (mTextureCoordIndex[i] == elem.getIndex() && i < mFixedFunctionTextureUnits)
                        {
                            if (multitexturing)
                                glClientActiveTextureARB(GL_TEXTURE0 + i);
                            glTexCoordPointer(
                                VertexElement::getTypeCount(elem.getType()),
                                GLHardwareBufferManager::getGLType(elem.getType()),
                                static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                                pBufferData);
                            glEnableClientState( GL_TEXTURE_COORD_ARRAY );
                        }
                    }
                }
                break;
            default:
                break;
            };
        } // isCustomAttrib
    }
	
    void GLRenderSystem::_copyContentsToMemory(Viewport* vp, const Box& src, const PixelBox &dst, RenderWindow::FrameBuffer buffer)
    {
        GLenum format = GLPixelUtil::getGLOriginFormat(dst.format);
        GLenum type = GLPixelUtil::getGLOriginDataType(dst.format);

        if ((format == GL_NONE) || (type == 0))
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Unsupported format.", "GLRenderSystem::copyContentsToMemory" );
        }

        // Switch context if different from current one
        _setViewport(vp);

        if(dst.getWidth() != dst.rowPitch)
            glPixelStorei(GL_PACK_ROW_LENGTH, dst.rowPitch);
        // Must change the packing to ensure no overruns!
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

        glReadBuffer((buffer == RenderWindow::FrameBuffer::FRONT)? GL_FRONT : GL_BACK);

        uint32_t height = vp->getTarget()->getHeight();

        glReadPixels((GLint)src.left, (GLint)(height - src.bottom),
                     (GLsizei)dst.getWidth(), (GLsizei)dst.getHeight(),
                     format, type, dst.getTopLeftFrontPixelPtr());

        // restore default alignment
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glPixelStorei(GL_PACK_ROW_LENGTH, 0);

        PixelUtil::bulkPixelVerticalFlip(dst);
    }
	//---------------------------------------------------------------------
    void GLRenderSystem::initialiseExtensions()
    {
        // Set version string
        const GLubyte* pcVer = glGetString(GL_VERSION);
        assert(pcVer && "Problems getting GL version string using glGetString");

        String tmpStr = (const char*)pcVer;
        mDriverVersion.fromString(tmpStr.substr(0, tmpStr.find(' ')));
        LogManager::getSingleton().logMessage(::std::format("GL_VERSION = {}", mDriverVersion.toString()));

        // Get vendor
        const GLubyte* pcVendor = glGetString(GL_VENDOR);
        tmpStr = (const char*)pcVendor;
        LogManager::getSingleton().logMessage(::std::format("GL_VENDOR = {}", tmpStr));
        mVendor = RenderSystemCapabilities::vendorFromString(tmpStr.substr(0, tmpStr.find(' ')));

        // Get renderer
        const GLubyte* pcRenderer = glGetString(GL_RENDERER);
        tmpStr = (const char*)pcRenderer;
        LogManager::getSingleton().logMessage(::std::format("GL_RENDERER = {}", tmpStr));

        // Set extension list
        StringStream ext;
        String str;

        const GLubyte* pcExt = glGetString(GL_EXTENSIONS);
        assert(pcExt && "Problems getting GL extension string using glGetString");
        LogManager::getSingleton().logMessage(::std::format("GL_EXTENSIONS = {}", String((const char*)pcExt)));

        ext << pcExt;

        while(ext >> str)
        {
            mExtensionList.insert(str);
        }
    }
}
