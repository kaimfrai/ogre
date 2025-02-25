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

#include <cassert>
#include <cstdio>
#include <cstring>

module Ogre.Core;

import :BlendMode;
import :ColourValue;
import :Common;
import :Config;
import :Exception;
import :GpuProgram;
import :GpuProgramManager;
import :GpuProgramParams;
import :Light;
import :Log;
import :LogManager;
import :Material;
import :MaterialManager;
import :MaterialSerializer;
import :Math;
import :Matrix4;
import :Pass;
import :PixelFormat;
import :Platform;
import :Prerequisites;
import :RenderSystemCapabilities;
import :ResourceGroupManager;
import :SharedPtr;
import :StringConverter;
import :StringInterface;
import :Technique;
import :Texture;
import :TextureManager;
import :TextureUnitState;

import <algorithm>;
import <format>;
import <map>;
import <string>;
import <utility>;
import <vector>;

namespace Ogre
{
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    MaterialSerializer::MaterialSerializer()
    {
        mDefaults = false;
        mBuffer.clear();
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::exportMaterial(const MaterialPtr& pMat, std::string_view fileName, bool exportDefaults,
        const bool includeProgDef, std::string_view programFilename, std::string_view materialName)
    {
        clearQueue();
        mDefaults = exportDefaults;
        writeMaterial(pMat, materialName);
        exportQueued(fileName, includeProgDef, programFilename);
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::exportQueued(std::string_view fileName, const bool includeProgDef, std::string_view programFilename)
    {
        // write out gpu program definitions to the buffer
        writeGpuPrograms();

        if (mBuffer.empty())
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Queue is empty !", "MaterialSerializer::exportQueued");

        LogManager::getSingleton().logMessage(::std::format("MaterialSerializer : writing material(s) to material script : {}", fileName), LogMessageLevel::Normal);
        FILE *fp;
        fp = fopen(fileName.data(), "w");
        if (!fp)
            OGRE_EXCEPT(ExceptionCodes::CANNOT_WRITE_TO_FILE, "Cannot create material file.",
            "MaterialSerializer::export");

        // output gpu program definitions to material script file if includeProgDef is true
        if (includeProgDef && !mGpuProgramBuffer.empty())
        {
            fputs(mGpuProgramBuffer.data(), fp);
        }

        // output main buffer holding material script
        fputs(mBuffer.c_str(), fp);
        fclose(fp);

        // write program script if program filename and program definitions
        // were not included in material script
        if (!includeProgDef && !mGpuProgramBuffer.empty() && !programFilename.empty())
        {
            FILE *locFp;
            locFp = fopen(programFilename.data(), "w");
            if (!locFp)
                OGRE_EXCEPT(ExceptionCodes::CANNOT_WRITE_TO_FILE, "Cannot create program material file.",
                "MaterialSerializer::export");
            fputs(mGpuProgramBuffer.c_str(), locFp);
            fclose(locFp);
        }

        LogManager::getSingleton().logMessage("MaterialSerializer : done.", LogMessageLevel::Normal);
        clearQueue();
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::queueForExport(const MaterialPtr& pMat,
        bool clearQueued, bool exportDefaults, std::string_view materialName)
    {
        if (clearQueued)
            clearQueue();

        mDefaults = exportDefaults;
        writeMaterial(pMat, materialName);
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::clearQueue()
    {
        mBuffer.clear();
        mGpuProgramBuffer.clear();
        mGpuProgramDefinitionContainer.clear();
    }
    //-----------------------------------------------------------------------
    auto MaterialSerializer::getQueuedAsString() const -> std::string_view
    {
        return mBuffer;
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeMaterial(const MaterialPtr& pMat, std::string_view materialName)
    {
        String outMaterialName;

        if (materialName.length() > 0)
        {
            outMaterialName = materialName;
        }
        else
        {
            outMaterialName = pMat->getName();
        }

        LogManager::getSingleton().logMessage(::std::format("MaterialSerializer : writing material {} to queue.", outMaterialName ), LogMessageLevel::Normal);

        bool skipWriting = false;

        // Fire pre-write event.
        fireMaterialEvent(SerializeEvent::PRE_WRITE, skipWriting, pMat.get());
        if (skipWriting)        
            return;     

        // Material name
        writeAttribute(0, "material");
        writeValue(quoteWord(outMaterialName));
        
        beginSection(0);
        {
            // Fire write begin event.
            fireMaterialEvent(SerializeEvent::WRITE_BEGIN, skipWriting, pMat.get());

            // Write LOD information
            auto valueIt = pMat->getUserLodValues().begin();
            // Skip zero value
            if (!pMat->getUserLodValues().empty())
                valueIt++;
            String attributeVal;
            while (valueIt != pMat->getUserLodValues().end())
            {
                attributeVal.append(StringConverter::toString(*valueIt++));
                if (valueIt != pMat->getUserLodValues().end())
                    attributeVal.append(" ");
            }
            if (!attributeVal.empty())
            {
                writeAttribute(1, "lod_values");
                writeValue(attributeVal);
            }


            // Shadow receive
            if (mDefaults ||
                pMat->getReceiveShadows() != true)
            {
                writeAttribute(1, "receive_shadows");
                writeValue(pMat->getReceiveShadows() ? "on" : "off");
            }

            // When rendering shadows, treat transparent things as opaque?
            if (mDefaults ||
                pMat->getTransparencyCastsShadows() == true)
            {
                writeAttribute(1, "transparency_casts_shadows");
                writeValue(pMat->getTransparencyCastsShadows() ? "on" : "off");
            }

            // Iterate over techniques
            for(auto t : pMat->getTechniques())
            {
                // skip RTSS generated techniques
                if(!mDefaults && t->getSchemeName() == "ShaderGeneratorDefaultScheme")
                    continue;
                writeTechnique(t);
                mBuffer += "\n";
            }

            // Fire write end event.
            fireMaterialEvent(SerializeEvent::WRITE_END, skipWriting, pMat.get());
        }
        endSection(0);
        mBuffer += "\n";

        // Fire post section write event.
        fireMaterialEvent(SerializeEvent::POST_WRITE, skipWriting, pMat.get());
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeTechnique(const Technique* pTech)
    {
        bool skipWriting = false;

        // Fire pre-write event.
        fireTechniqueEvent(SerializeEvent::PRE_WRITE, skipWriting, pTech);
        if (skipWriting)        
            return; 
        
        // Technique header
        writeAttribute(1, "technique");
        // only output technique name if it exists.
        if (!pTech->getName().empty())
            writeValue(quoteWord(pTech->getName()));

        beginSection(1);
        {
            // Fire write begin event.
            fireTechniqueEvent(SerializeEvent::WRITE_BEGIN, skipWriting, pTech);

            // LOD index
            if (mDefaults ||
                pTech->getLodIndex() != 0)
            {
                writeAttribute(2, "lod_index");
                writeValue(StringConverter::toString(pTech->getLodIndex()));
            }

            // Scheme name
            if (mDefaults ||
                pTech->getSchemeName() != MaterialManager::DEFAULT_SCHEME_NAME)
            {
                writeAttribute(2, "scheme");
                writeValue(quoteWord(pTech->getSchemeName()));
            }

            // ShadowCasterMaterial name
            if (pTech->getShadowCasterMaterial())
            {
                writeAttribute(2, "shadow_caster_material");
                writeValue(quoteWord(pTech->getShadowCasterMaterial()->getName()));
            }
            // ShadowReceiverMaterial name
            if (pTech->getShadowReceiverMaterial())
            {
                writeAttribute(2, "shadow_receiver_material");
                writeValue(quoteWord(pTech->getShadowReceiverMaterial()->getName()));
            }
            // GPU vendor rules
            for (auto rule : pTech->getGPUVendorRules())
            {
                writeAttribute(2, "gpu_vendor_rule");
                if (rule.includeOrExclude == Technique::INCLUDE)
                    writeValue("include");
                else
                    writeValue("exclude");
                writeValue(quoteWord(RenderSystemCapabilities::vendorToString(rule.vendor)));
            }
            // GPU device rules
            for (const auto & rule : pTech->getGPUDeviceNameRules())
            {
                writeAttribute(2, "gpu_device_rule");
                if (rule.includeOrExclude == Technique::INCLUDE)
                    writeValue("include");
                else
                    writeValue("exclude");
                writeValue(quoteWord(rule.devicePattern));
                writeValue(StringConverter::toString(rule.caseSensitive));
            }
            // Iterate over passes
            for (auto i : pTech->getPasses())
            {
                writePass(i);
                mBuffer += "\n";
            }

            // Fire write end event.
            fireTechniqueEvent(SerializeEvent::WRITE_END, skipWriting, pTech);
        }
        endSection(1);

        // Fire post section write event.
        fireTechniqueEvent(SerializeEvent::POST_WRITE, skipWriting, pTech);

    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writePass(const Pass* pPass)
    {
        bool skipWriting = false;

        // Fire pre-write event.
        firePassEvent(SerializeEvent::PRE_WRITE, skipWriting, pPass);
        if (skipWriting)        
            return;
        
        writeAttribute(2, "pass");
        // only output pass name if its not the default name
        if (pPass->getName() != StringConverter::toString(pPass->getIndex()))
            writeValue(quoteWord(pPass->getName()));

        beginSection(2);
        {
            // Fire write begin event.
            firePassEvent(SerializeEvent::WRITE_BEGIN, skipWriting, pPass);

            //lighting
            if (mDefaults ||
                pPass->getLightingEnabled() != true)
            {
                writeAttribute(3, "lighting");
                writeValue(pPass->getLightingEnabled() ? "on" : "off");
            }
            // max_lights
            if (mDefaults ||
                pPass->getMaxSimultaneousLights() != OGRE_MAX_SIMULTANEOUS_LIGHTS)
            {
                writeAttribute(3, "max_lights");
                writeValue(StringConverter::toString(pPass->getMaxSimultaneousLights()));
            }
            // start_light
            if (mDefaults ||
                pPass->getStartLight() != 0)
            {
                writeAttribute(3, "start_light");
                writeValue(StringConverter::toString(pPass->getStartLight()));
            }
            // iteration
            if (mDefaults ||
                pPass->getIteratePerLight() || (pPass->getPassIterationCount() > 1))
            {
                writeAttribute(3, "iteration");
                // pass iteration count
                if (pPass->getPassIterationCount() > 1 || pPass->getLightCountPerIteration() > 1)
                {
                    writeValue(StringConverter::toString(pPass->getPassIterationCount()));
                    if (pPass->getIteratePerLight())
                    {
                        if (pPass->getLightCountPerIteration() > 1)
                        {
                            writeValue("per_n_lights");
                            writeValue(StringConverter::toString(
                                pPass->getLightCountPerIteration()));
                        }
                        else
                        {
                            writeValue("per_light");
                        }
                    }
                }
                else
                {
                    writeValue(pPass->getIteratePerLight() ? "once_per_light" : "once");
                }

                if (pPass->getIteratePerLight() && pPass->getRunOnlyForOneLightType())
                {
                    using enum Light::LightTypes;
                    switch (pPass->getOnlyLightType())
                    {
                    case DIRECTIONAL:
                        writeValue("directional");
                        break;
                    case POINT:
                        writeValue("point");
                        break;
                    case SPOTLIGHT:
                        writeValue("spot");
                        break;
                    };
                }
            }

            if(mDefaults || pPass->getLightMask() != QueryTypeMask{0xFFFFFFFF})
            {
                writeAttribute(3, "light_mask");
                writeValue(StringConverter::toString(pPass->getLightMask()));
            }

            if (pPass->getLightingEnabled())
            {
                // Ambient
                if (mDefaults ||
                    pPass->getAmbient().r != 1 ||
                    pPass->getAmbient().g != 1 ||
                    pPass->getAmbient().b != 1 ||
                    pPass->getAmbient().a != 1 ||
                    (!!(pPass->getVertexColourTracking() & TrackVertexColourEnum::AMBIENT)))
                {
                    writeAttribute(3, "ambient");
                    if (!!(pPass->getVertexColourTracking() & TrackVertexColourEnum::AMBIENT))
                        writeValue("vertexcolour");
                    else
                        writeColourValue(pPass->getAmbient(), true);
                }

                // Diffuse
                if (mDefaults ||
                    pPass->getDiffuse().r != 1 ||
                    pPass->getDiffuse().g != 1 ||
                    pPass->getDiffuse().b != 1 ||
                    pPass->getDiffuse().a != 1 ||
                    (!!(pPass->getVertexColourTracking() & TrackVertexColourEnum::DIFFUSE)))
                {
                    writeAttribute(3, "diffuse");
                    if (!!(pPass->getVertexColourTracking() & TrackVertexColourEnum::DIFFUSE))
                        writeValue("vertexcolour");
                    else
                        writeColourValue(pPass->getDiffuse(), true);
                }

                // Specular
                if (mDefaults ||
                    pPass->getSpecular().r != 0 ||
                    pPass->getSpecular().g != 0 ||
                    pPass->getSpecular().b != 0 ||
                    pPass->getSpecular().a != 1 ||
                    pPass->getShininess() != 0 ||
                    (!!(pPass->getVertexColourTracking() & TrackVertexColourEnum::SPECULAR)))
                {
                    writeAttribute(3, "specular");
                    if (!!(pPass->getVertexColourTracking() & TrackVertexColourEnum::SPECULAR))
                    {
                        writeValue("vertexcolour");
                    }
                    else
                    {
                        writeColourValue(pPass->getSpecular(), true);
                    }
                    writeValue(StringConverter::toString(pPass->getShininess()));

                }

                // Emissive
                if (mDefaults ||
                    pPass->getSelfIllumination().r != 0 ||
                    pPass->getSelfIllumination().g != 0 ||
                    pPass->getSelfIllumination().b != 0 ||
                    pPass->getSelfIllumination().a != 1 ||
                    (!!(pPass->getVertexColourTracking() & TrackVertexColourEnum::EMISSIVE)))
                {
                    writeAttribute(3, "emissive");
                    if (!!(pPass->getVertexColourTracking() & TrackVertexColourEnum::EMISSIVE))
                        writeValue("vertexcolour");
                    else
                        writeColourValue(pPass->getSelfIllumination(), true);
                }
            }

            // Point size
            if (mDefaults ||
                pPass->getPointSize() != 1.0)
            {
                writeAttribute(3, "point_size");
                writeValue(StringConverter::toString(pPass->getPointSize()));
            }

            // Point sprites
            if (mDefaults ||
                pPass->getPointSpritesEnabled())
            {
                writeAttribute(3, "point_sprites");
                writeValue(pPass->getPointSpritesEnabled() ? "on" : "off");
            }

            // Point attenuation
            if (mDefaults ||
                pPass->isPointAttenuationEnabled())
            {
                writeAttribute(3, "point_size_attenuation");
                writeValue(pPass->isPointAttenuationEnabled() ? "on" : "off");
                if (pPass->isPointAttenuationEnabled() &&
                    (pPass->getPointAttenuationConstant() != 0.0 ||
                     pPass->getPointAttenuationLinear() != 1.0 ||
                     pPass->getPointAttenuationQuadratic() != 0.0))
                {
                    writeValue(StringConverter::toString(pPass->getPointAttenuationConstant()));
                    writeValue(StringConverter::toString(pPass->getPointAttenuationLinear()));
                    writeValue(StringConverter::toString(pPass->getPointAttenuationQuadratic()));
                }
            }

            // Point min size
            if (mDefaults ||
                pPass->getPointMinSize() != 0.0)
            {
                writeAttribute(3, "point_size_min");
                writeValue(StringConverter::toString(pPass->getPointMinSize()));
            }

            // Point max size
            if (mDefaults ||
                pPass->getPointMaxSize() != 0.0)
            {
                writeAttribute(3, "point_size_max");
                writeValue(StringConverter::toString(pPass->getPointMaxSize()));
            }

            // scene blend factor
            if (mDefaults ||
                pPass->getSourceBlendFactor() != SceneBlendFactor::ONE ||
                pPass->getDestBlendFactor() != SceneBlendFactor::ZERO ||
                pPass->getSourceBlendFactorAlpha() != SceneBlendFactor::ONE ||
                pPass->getDestBlendFactorAlpha() != SceneBlendFactor::ZERO)
            {
                writeAttribute(3, "separate_scene_blend");
                writeSceneBlendFactor(pPass->getSourceBlendFactor(), pPass->getDestBlendFactor(),
                    pPass->getSourceBlendFactorAlpha(), pPass->getDestBlendFactorAlpha());
            }


            //depth check
            if (mDefaults ||
                pPass->getDepthCheckEnabled() != true)
            {
                writeAttribute(3, "depth_check");
                writeValue(pPass->getDepthCheckEnabled() ? "on" : "off");
            }
            // alpha_rejection
            if (mDefaults ||
                pPass->getAlphaRejectFunction() != CompareFunction::ALWAYS_PASS ||
                pPass->getAlphaRejectValue() != 0)
            {
                writeAttribute(3, "alpha_rejection");
                writeCompareFunction(pPass->getAlphaRejectFunction());
                writeValue(StringConverter::toString(pPass->getAlphaRejectValue()));
            }
            // alpha_to_coverage
            if (mDefaults ||
                pPass->isAlphaToCoverageEnabled())
            {
                writeAttribute(3, "alpha_to_coverage");
                writeValue(pPass->isAlphaToCoverageEnabled() ? "on" : "off");
            }
            // transparent_sorting
            if (mDefaults ||
                pPass->getTransparentSortingForced() == true ||
                pPass->getTransparentSortingEnabled() != true)
            {
                writeAttribute(3, "transparent_sorting");
                writeValue(pPass->getTransparentSortingForced() ? "force" :
                    (pPass->getTransparentSortingEnabled() ? "on" : "off"));
            }


            //depth write
            if (mDefaults ||
                pPass->getDepthWriteEnabled() != true)
            {
                writeAttribute(3, "depth_write");
                writeValue(pPass->getDepthWriteEnabled() ? "on" : "off");
            }

            //depth function
            if (mDefaults ||
                pPass->getDepthFunction() != CompareFunction::LESS_EQUAL)
            {
                writeAttribute(3, "depth_func");
                writeCompareFunction(pPass->getDepthFunction());
            }

            //depth bias
            if (mDefaults ||
                pPass->getDepthBiasConstant() != 0 ||
                pPass->getDepthBiasSlopeScale() != 0)
            {
                writeAttribute(3, "depth_bias");
                writeValue(StringConverter::toString(pPass->getDepthBiasConstant()));
                writeValue(StringConverter::toString(pPass->getDepthBiasSlopeScale()));
            }
            //iteration depth bias
            if (mDefaults ||
                pPass->getIterationDepthBias() != 0)
            {
                writeAttribute(3, "iteration_depth_bias");
                writeValue(StringConverter::toString(pPass->getIterationDepthBias()));
            }

            //light scissor
            if (mDefaults ||
                pPass->getLightScissoringEnabled() != false)
            {
                writeAttribute(3, "light_scissor");
                writeValue(pPass->getLightScissoringEnabled() ? "on" : "off");
            }

            //light clip planes
            if (mDefaults ||
                pPass->getLightClipPlanesEnabled() != false)
            {
                writeAttribute(3, "light_clip_planes");
                writeValue(pPass->getLightClipPlanesEnabled() ? "on" : "off");
            }

            // illumination stage
            if (pPass->getIlluminationStage() != IlluminationStage::UNKNOWN)
            {
                using enum IlluminationStage;
                writeAttribute(3, "illumination_stage");
                switch(pPass->getIlluminationStage())
                {
                case AMBIENT:
                    writeValue("ambient");
                    break;
                case PER_LIGHT:
                    writeValue("per_light");
                    break;
                case DECAL:
                    writeValue("decal");
                    break;
                case UNKNOWN:
                    break;
                };
            }

            // hardware culling mode
            if (mDefaults ||
                pPass->getCullingMode() != CullingMode::CLOCKWISE)
            {
                using enum CullingMode;
                CullingMode hcm = pPass->getCullingMode();
                writeAttribute(3, "cull_hardware");
                switch (hcm)
                {
                case NONE :
                    writeValue("none");
                    break;
                case CLOCKWISE :
                    writeValue("clockwise");
                    break;
                case ANTICLOCKWISE :
                    writeValue("anticlockwise");
                    break;
                }
            }

            // software culling mode
            if (mDefaults ||
                pPass->getManualCullingMode() != ManualCullingMode::BACK)
            {
                using enum ManualCullingMode;
                ManualCullingMode scm = pPass->getManualCullingMode();
                writeAttribute(3, "cull_software");
                switch (scm)
                {
                case NONE :
                    writeValue("none");
                    break;
                case BACK :
                    writeValue("back");
                    break;
                case FRONT :
                    writeValue("front");
                    break;
                }
            }

            //shading
            if (mDefaults ||
                pPass->getShadingMode() != ShadeOptions::GOURAUD)
            {
                writeAttribute(3, "shading");
                using enum ShadeOptions;
                switch (pPass->getShadingMode())
                {
                case FLAT:
                    writeValue("flat");
                    break;
                case GOURAUD:
                    writeValue("gouraud");
                    break;
                case PHONG:
                    writeValue("phong");
                    break;
                }
            }


            if (mDefaults ||
                pPass->getPolygonMode() != PolygonMode::SOLID)
            {
                writeAttribute(3, "polygon_mode");
                using enum PolygonMode;
                switch (pPass->getPolygonMode())
                {
                case POINTS:
                    writeValue("points");
                    break;
                case WIREFRAME:
                    writeValue("wireframe");
                    break;
                case SOLID:
                    writeValue("solid");
                    break;
                }
            }

            // polygon mode overrideable
            if (mDefaults ||
                !pPass->getPolygonModeOverrideable())
            {
                writeAttribute(3, "polygon_mode_overrideable");
                writeValue(pPass->getPolygonModeOverrideable() ? "on" : "off");
            }

            // normalise normals
            if (mDefaults ||
                pPass->getNormaliseNormals() != false)
            {
                writeAttribute(3, "normalise_normals");
                writeValue(pPass->getNormaliseNormals() ? "on" : "off");
            }

            //fog override
            if (mDefaults ||
                pPass->getFogOverride() != false)
            {
                writeAttribute(3, "fog_override");
                writeValue(pPass->getFogOverride() ? "true" : "false");
                if (pPass->getFogOverride())
                {
                    using enum FogMode;
                    switch (pPass->getFogMode())
                    {
                    case NONE:
                        writeValue("none");
                        break;
                    case LINEAR:
                        writeValue("linear");
                        break;
                    case EXP2:
                        writeValue("exp2");
                        break;
                    case EXP:
                        writeValue("exp");
                        break;
                    }

                    if (pPass->getFogMode() != FogMode::NONE)
                    {
                        writeColourValue(pPass->getFogColour());
                        writeValue(StringConverter::toString(pPass->getFogDensity()));
                        writeValue(StringConverter::toString(pPass->getFogStart()));
                        writeValue(StringConverter::toString(pPass->getFogEnd()));
                    }
                }
            }

            //  GPU Vertex and Fragment program references and parameters
            if (pPass->hasVertexProgram())
            {
                writeVertexProgramRef(pPass);
            }

            if (pPass->hasFragmentProgram())
            {
                writeFragmentProgramRef(pPass);
            }

            if(pPass->hasTessellationHullProgram())
            {
                writeTesselationHullProgramRef(pPass);
            }

            if(pPass->hasTessellationDomainProgram())
            {
                writeTesselationDomainProgramRef(pPass);
            }
            
            if (pPass->hasGeometryProgram())
            {
                writeGeometryProgramRef(pPass);
            }

            // Nested texture layers
            for (auto it : pPass->getTextureUnitStates())
            {
                writeTextureUnit(it);
            }

            // Fire write end event.
            firePassEvent(SerializeEvent::WRITE_END, skipWriting, pPass);
        }
        endSection(2);
        
        // Fire post section write event.
        firePassEvent(SerializeEvent::POST_WRITE, skipWriting, pPass);
        
        LogManager::getSingleton().logMessage("MaterialSerializer : done.", LogMessageLevel::Normal);
    }
    //-----------------------------------------------------------------------
    auto MaterialSerializer::convertFiltering(FilterOptions fo) -> String
    {
        using enum FilterOptions;
        switch (fo)
        {
        case NONE:
            return "none";
        case POINT:
            return "point";
        case LINEAR:
            return "linear";
        case ANISOTROPIC:
            return "anisotropic";
        }

        return "point";
    }
    //-----------------------------------------------------------------------
    static auto convertTexAddressMode(TextureAddressingMode tam) -> String
    {
        using enum TextureAddressingMode;
        switch (tam)
        {
        case BORDER:
            return "border";
        case CLAMP:
            return "clamp";
        case MIRROR:
            return "mirror";
        case WRAP:
        case UNKNOWN:
            return "wrap";
        }

        return "wrap";
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeTextureUnit(const TextureUnitState *pTex)
    {
        bool skipWriting = false;

        // Fire pre-write event.
        fireTextureUnitStateEvent(SerializeEvent::PRE_WRITE, skipWriting, pTex);
        if (skipWriting)        
            return;
    
        LogManager::getSingleton().logMessage("MaterialSerializer : parsing texture layer.", LogMessageLevel::Normal);
        mBuffer += "\n";
        writeAttribute(3, "texture_unit");
        // only write out name if its not equal to the default name
        if (pTex->getName() != StringConverter::toString(pTex->getParent()->getTextureUnitStateIndex(pTex)))
            writeValue(quoteWord(pTex->getName()));

        beginSection(3);
        {
            // Fire write begin event.
            fireTextureUnitStateEvent(SerializeEvent::WRITE_BEGIN, skipWriting, pTex);

            // texture_alias
            if (!pTex->getTextureNameAlias().empty() && pTex->getTextureNameAlias() != pTex->getName())
            {
                writeAttribute(4, "texture_alias");
                writeValue(quoteWord(pTex->getTextureNameAlias()));
            }

            //texture name
            if (pTex->getNumFrames() == 1 && !pTex->getTextureName().empty())
            {
                writeAttribute(4, "texture");
                writeValue(quoteWord(pTex->getTextureName()));

                using enum TextureType;
                switch (pTex->getTextureType())
                {
                case _1D:
                    writeValue("1d");
                    break;
                case _2D:
                    // nothing, this is the default
                    break;
                case _2D_ARRAY:
                    writeValue("2darray");
                    break;
                case _3D:
                    writeValue("3d");
                    break;
                case CUBE_MAP:
                    writeValue("cubic");
                    break;
                default:
                    break;
                };

                if (pTex->getNumMipmaps() != TextureManager::getSingleton().getDefaultNumMipmaps())
                {
                    writeValue(StringConverter::toString(pTex->getNumMipmaps()));
                }

                if (pTex->getDesiredFormat() != PixelFormat::UNKNOWN)
                {
                    writeValue(PixelUtil::getFormatName(pTex->getDesiredFormat()));
                }
            }

            //anim. texture
            if (pTex->getNumFrames() > 1)
            {
                writeAttribute(4, "anim_texture");
                for (unsigned int n = 0; n < pTex->getNumFrames(); n++)
                    writeValue(quoteWord(pTex->getFrameTextureName(n)));
                writeValue(StringConverter::toString(pTex->getAnimationDuration()));
            }

            //anisotropy level
            if (mDefaults ||
                pTex->getTextureAnisotropy() != 1)
            {
                writeAttribute(4, "max_anisotropy");
                writeValue(StringConverter::toString(pTex->getTextureAnisotropy()));
            }

            //texture coordinate set
            if (mDefaults ||
                pTex->getTextureCoordSet() != 0)
            {
                writeAttribute(4, "tex_coord_set");
                writeValue(StringConverter::toString(pTex->getTextureCoordSet()));
            }

            //addressing mode
            const Sampler::UVWAddressingMode& uvw =
                pTex->getTextureAddressingMode();
            if (mDefaults ||
                uvw.u != TextureAddressingMode::WRAP ||
                uvw.v != TextureAddressingMode::WRAP ||
                uvw.w != TextureAddressingMode::WRAP )
            {
                writeAttribute(4, "tex_address_mode");
                if (uvw.u == uvw.v && uvw.u == uvw.w)
                {
                    writeValue(convertTexAddressMode(uvw.u));
                }
                else
                {
                    writeValue(convertTexAddressMode(uvw.u));
                    writeValue(convertTexAddressMode(uvw.v));
                    if (uvw.w != TextureAddressingMode::WRAP)
                    {
                        writeValue(convertTexAddressMode(uvw.w));
                    }
                }
            }

            //border colour
            const ColourValue& borderColour =
                pTex->getTextureBorderColour();
            if (mDefaults ||
                borderColour != ColourValue::Black)
            {
                writeAttribute(4, "tex_border_colour");
                writeColourValue(borderColour, true);
            }

            //filtering
            if (TextureManager::getSingletonPtr() && (mDefaults || !pTex->isDefaultFiltering()))
            {
                writeAttribute(4, "filtering");
                writeValue(
                    ::std::format("{} {} {}",
                    convertFiltering(pTex->getTextureFiltering(FilterType::Min))
                    , convertFiltering(pTex->getTextureFiltering(FilterType::Mag))
                    , convertFiltering(pTex->getTextureFiltering(FilterType::Mip))));
            }

            // Mip biasing
            if (mDefaults ||
                pTex->getTextureMipmapBias() != 0.0f)
            {
                writeAttribute(4, "mipmap_bias");
                writeValue(
                    StringConverter::toString(pTex->getTextureMipmapBias()));
            }

            // colour_op_ex
            if (mDefaults ||
                pTex->getColourBlendMode().operation != LayerBlendOperationEx::MODULATE ||
                pTex->getColourBlendMode().source1 != LayerBlendSource::TEXTURE ||
                pTex->getColourBlendMode().source2 != LayerBlendSource::CURRENT)
            {
                writeAttribute(4, "colour_op_ex");
                writeLayerBlendOperationEx(pTex->getColourBlendMode().operation);
                writeLayerBlendSource(pTex->getColourBlendMode().source1);
                writeLayerBlendSource(pTex->getColourBlendMode().source2);
                if (pTex->getColourBlendMode().operation == LayerBlendOperationEx::BLEND_MANUAL)
                    writeValue(StringConverter::toString(pTex->getColourBlendMode().factor));
                if (pTex->getColourBlendMode().source1 == LayerBlendSource::MANUAL)
                    writeColourValue(pTex->getColourBlendMode().colourArg1, false);
                if (pTex->getColourBlendMode().source2 == LayerBlendSource::MANUAL)
                    writeColourValue(pTex->getColourBlendMode().colourArg2, false);

                //colour_op_multipass_fallback
                writeAttribute(4, "colour_op_multipass_fallback");
                writeSceneBlendFactor(pTex->getColourBlendFallbackSrc());
                writeSceneBlendFactor(pTex->getColourBlendFallbackDest());
            }

            // alpha_op_ex
            if (mDefaults ||
                pTex->getAlphaBlendMode().operation != LayerBlendOperationEx::MODULATE ||
                pTex->getAlphaBlendMode().source1 != LayerBlendSource::TEXTURE ||
                pTex->getAlphaBlendMode().source2 != LayerBlendSource::CURRENT)
            {
                writeAttribute(4, "alpha_op_ex");
                writeLayerBlendOperationEx(pTex->getAlphaBlendMode().operation);
                writeLayerBlendSource(pTex->getAlphaBlendMode().source1);
                writeLayerBlendSource(pTex->getAlphaBlendMode().source2);
                if (pTex->getAlphaBlendMode().operation == LayerBlendOperationEx::BLEND_MANUAL)
                    writeValue(StringConverter::toString(pTex->getAlphaBlendMode().factor));
                else if (pTex->getAlphaBlendMode().source1 == LayerBlendSource::MANUAL)
                    writeValue(StringConverter::toString(pTex->getAlphaBlendMode().alphaArg1));
                else if (pTex->getAlphaBlendMode().source2 == LayerBlendSource::MANUAL)
                    writeValue(StringConverter::toString(pTex->getAlphaBlendMode().alphaArg2));
            }

            bool individualTransformElems = false;
            // rotate
            if (mDefaults ||
                pTex->getTextureRotate() != Radian{0})
            {
                writeAttribute(4, "rotate");
                writeValue(StringConverter::toString(pTex->getTextureRotate().valueDegrees()));
                individualTransformElems = true;
            }

            // scroll
            if (mDefaults ||
                pTex->getTextureUScroll() != 0 ||
                pTex->getTextureVScroll() != 0 )
            {
                writeAttribute(4, "scroll");
                writeValue(StringConverter::toString(pTex->getTextureUScroll()));
                writeValue(StringConverter::toString(pTex->getTextureVScroll()));
                individualTransformElems = true;
            }
            // scale
            if (mDefaults ||
                pTex->getTextureUScale() != 1.0 ||
                pTex->getTextureVScale() != 1.0 )
            {
                writeAttribute(4, "scale");
                writeValue(StringConverter::toString(pTex->getTextureUScale()));
                writeValue(StringConverter::toString(pTex->getTextureVScale()));
                individualTransformElems = true;
            }

            // free transform
            if (!individualTransformElems &&
                (mDefaults ||
                pTex->getTextureTransform() != Matrix4::IDENTITY))
            {
                writeAttribute(4, "transform");
                const Matrix4& xform = pTex->getTextureTransform();
                for (int row = 0; row < 4; ++row)
                {
                    for (int col = 0; col < 4; ++col)
                    {
                        writeValue(StringConverter::toString(xform[row][col]));
                    }
                }
            }

            // Used to store the u and v speeds of scroll animation effects
            float scrollAnimU = 0;
            float scrollAnimV = 0;

            EffectMap effMap = pTex->getEffects();
            if (!effMap.empty())
            {
                for (auto const& [key, ef] : effMap)
                {
                    using enum TextureUnitState::TextureEffectType;
                    switch (ef.type)
                    {
                    case ENVIRONMENT_MAP :
                        writeEnvironmentMapEffect(ef, pTex);
                        break;
                    case ROTATE :
                        writeRotationEffect(ef, pTex);
                        break;
                    case UVSCROLL :
                        scrollAnimU = scrollAnimV = ef.arg1;
                        break;
                    case USCROLL :
                        scrollAnimU = ef.arg1;
                        break;
                    case VSCROLL :
                        scrollAnimV = ef.arg1;
                        break;
                    case TRANSFORM :
                        writeTransformEffect(ef, pTex);
                        break;
                    default:
                        break;
                    }
                }
            }

            // u and v scroll animation speeds merged, if present serialize scroll_anim
            if(scrollAnimU || scrollAnimV) {
                TextureUnitState::TextureEffect texEffect;
                texEffect.arg1 = scrollAnimU;
                texEffect.arg2 = scrollAnimV;
                writeScrollEffect(texEffect, pTex);
            }

            using enum TextureUnitState::ContentType;
            // Content type
            if (mDefaults ||
                pTex->getContentType() != NAMED)
            {
                writeAttribute(4, "content_type");
                switch(pTex->getContentType())
                {
                case NAMED:
                    writeValue("named");
                    break;
                case SHADOW:
                    writeValue("shadow");
                    break;
                case COMPOSITOR:
                    writeValue("compositor");
                    writeValue(quoteWord(pTex->getReferencedCompositorName()));
                    writeValue(quoteWord(pTex->getReferencedTextureName()));
                    writeValue(StringConverter::toString(pTex->getReferencedMRTIndex()));
                    break;
                };
            }

            // Fire write end event.
            fireTextureUnitStateEvent(SerializeEvent::WRITE_END, skipWriting, pTex);
        }
        endSection(3);

        // Fire post section write event.
        fireTextureUnitStateEvent(SerializeEvent::POST_WRITE, skipWriting, pTex);

    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeEnvironmentMapEffect(const TextureUnitState::TextureEffect& effect, const TextureUnitState *pTex)
    {
        writeAttribute(4, "env_map");
        using enum TextureUnitState::EnvMapType;
        switch (static_cast<TextureUnitState::EnvMapType>(effect.subtype))
        {
        case PLANAR:
            writeValue("planar");
            break;
        case CURVED:
            writeValue("spherical");
            break;
        case NORMAL:
            writeValue("cubic_normal");
            break;
        case REFLECTION:
            writeValue("cubic_reflection");
            break;
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeRotationEffect(const TextureUnitState::TextureEffect& effect, const TextureUnitState *pTex)
    {
        if (effect.arg1)
        {
            writeAttribute(4, "rotate_anim");
            writeValue(StringConverter::toString(effect.arg1));
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeTransformEffect(const TextureUnitState::TextureEffect& effect, const TextureUnitState *pTex)
    {
        writeAttribute(4, "wave_xform");

        switch (static_cast<TextureUnitState::TextureTransformType>(effect.subtype))
        {
        using enum TextureUnitState::TextureTransformType;
        case ROTATE:
            writeValue("rotate");
            break;
        case SCALE_U:
            writeValue("scale_x");
            break;
        case SCALE_V:
            writeValue("scale_y");
            break;
        case TRANSLATE_U:
            writeValue("scroll_x");
            break;
        case TRANSLATE_V:
            writeValue("scroll_y");
            break;
        }

        switch (effect.waveType)
        {
            using enum WaveformType;
        case INVERSE_SAWTOOTH:
            writeValue("inverse_sawtooth");
            break;
        case SAWTOOTH:
            writeValue("sawtooth");
            break;
        case SINE:
            writeValue("sine");
            break;
        case SQUARE:
            writeValue("square");
            break;
        case TRIANGLE:
            writeValue("triangle");
            break;
        case PWM:
            writeValue("pwm");
            break;
        }

        writeValue(StringConverter::toString(effect.base));
        writeValue(StringConverter::toString(effect.frequency));
        writeValue(StringConverter::toString(effect.phase));
        writeValue(StringConverter::toString(effect.amplitude));
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeScrollEffect(
        const TextureUnitState::TextureEffect& effect, const TextureUnitState *pTex)
    {
        if (effect.arg1 || effect.arg2)
        {
            writeAttribute(4, "scroll_anim");
            writeValue(StringConverter::toString(effect.arg1));
            writeValue(StringConverter::toString(effect.arg2));
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeSceneBlendFactor(const SceneBlendFactor sbf)
    {
        switch (sbf)
        {
            using enum SceneBlendFactor;
        case DEST_ALPHA:
            writeValue("dest_alpha");
            break;
        case DEST_COLOUR:
            writeValue("dest_colour");
            break;
        case ONE:
            writeValue("one");
            break;
        case ONE_MINUS_DEST_ALPHA:
            writeValue("one_minus_dest_alpha");
            break;
        case ONE_MINUS_DEST_COLOUR:
            writeValue("one_minus_dest_colour");
            break;
        case ONE_MINUS_SOURCE_ALPHA:
            writeValue("one_minus_src_alpha");
            break;
        case ONE_MINUS_SOURCE_COLOUR:
            writeValue("one_minus_src_colour");
            break;
        case SOURCE_ALPHA:
            writeValue("src_alpha");
            break;
        case SOURCE_COLOUR:
            writeValue("src_colour");
            break;
        case ZERO:
            writeValue("zero");
            break;
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeSceneBlendFactor(const SceneBlendFactor sbf_src, const SceneBlendFactor sbf_dst)
    {
        if (sbf_src == SceneBlendFactor::ONE && sbf_dst == SceneBlendFactor::ONE )
            writeValue("add");
        else if (sbf_src == SceneBlendFactor::DEST_COLOUR && sbf_dst == SceneBlendFactor::ZERO)
            writeValue("modulate");
        else if (sbf_src == SceneBlendFactor::SOURCE_COLOUR && sbf_dst == SceneBlendFactor::ONE_MINUS_SOURCE_COLOUR)
            writeValue("colour_blend");
        else if (sbf_src == SceneBlendFactor::SOURCE_ALPHA && sbf_dst == SceneBlendFactor::ONE_MINUS_SOURCE_ALPHA)
            writeValue("alpha_blend");
        else
        {
            writeSceneBlendFactor(sbf_src);
            writeSceneBlendFactor(sbf_dst);
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeSceneBlendFactor(
        const SceneBlendFactor c_src, const SceneBlendFactor c_dest, 
        const SceneBlendFactor a_src, const SceneBlendFactor a_dest)
    {
        writeSceneBlendFactor(c_src, c_dest);
        writeSceneBlendFactor(a_src, a_dest);
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeCompareFunction(const CompareFunction cf)
    {
        using enum CompareFunction;
        switch (cf)
        {
        case ALWAYS_FAIL:
            writeValue("always_fail");
            break;
        case ALWAYS_PASS:
            writeValue("always_pass");
            break;
        case EQUAL:
            writeValue("equal");
            break;
        case GREATER:
            writeValue("greater");
            break;
        case GREATER_EQUAL:
            writeValue("greater_equal");
            break;
        case LESS:
            writeValue("less");
            break;
        case LESS_EQUAL:
            writeValue("less_equal");
            break;
        case NOT_EQUAL:
            writeValue("not_equal");
            break;
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeColourValue(const ColourValue &colour, bool writeAlpha)
    {
        writeValue(StringConverter::toString(colour.r));
        writeValue(StringConverter::toString(colour.g));
        writeValue(StringConverter::toString(colour.b));
        if (writeAlpha)
            writeValue(StringConverter::toString(colour.a));
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeLayerBlendOperationEx(const LayerBlendOperationEx op)
    {
        using enum LayerBlendOperationEx;
        switch (op)
        {
        case ADD:
            writeValue("add");
            break;
        case ADD_SIGNED:
            writeValue("add_signed");
            break;
        case ADD_SMOOTH:
            writeValue("add_smooth");
            break;
        case BLEND_CURRENT_ALPHA:
            writeValue("blend_current_alpha");
            break;
        case BLEND_DIFFUSE_COLOUR:
            writeValue("blend_diffuse_colour");
            break;
        case BLEND_DIFFUSE_ALPHA:
            writeValue("blend_diffuse_alpha");
            break;
        case BLEND_MANUAL:
            writeValue("blend_manual");
            break;
        case BLEND_TEXTURE_ALPHA:
            writeValue("blend_texture_alpha");
            break;
        case MODULATE:
            writeValue("modulate");
            break;
        case MODULATE_X2:
            writeValue("modulate_x2");
            break;
        case MODULATE_X4:
            writeValue("modulate_x4");
            break;
        case SOURCE1:
            writeValue("source1");
            break;
        case SOURCE2:
            writeValue("source2");
            break;
        case SUBTRACT:
            writeValue("subtract");
            break;
        case DOTPRODUCT:
            writeValue("dotproduct");
            break;
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeLayerBlendSource(const LayerBlendSource lbs)
    {
        using enum LayerBlendSource;
        switch (lbs)
        {
        case CURRENT:
            writeValue("src_current");
            break;
        case DIFFUSE:
            writeValue("src_diffuse");
            break;
        case MANUAL:
            writeValue("src_manual");
            break;
        case SPECULAR:
            writeValue("src_specular");
            break;
        case TEXTURE:
            writeValue("src_texture");
            break;
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeVertexProgramRef(const Pass* pPass)
    {
        writeGpuProgramRef("vertex_program_ref",
            pPass->getVertexProgram(), pPass->getVertexProgramParameters());
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeTesselationHullProgramRef(const Pass* pPass)
    {
        writeGpuProgramRef("tesselation_hull_program_ref",
            pPass->getTessellationHullProgram(), pPass->getTessellationHullProgramParameters());
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeTesselationDomainProgramRef(const Pass* pPass)
    {
        writeGpuProgramRef("tesselation_domain_program_ref",
            pPass->getTessellationDomainProgram(), pPass->getTessellationDomainProgramParameters());
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeGeometryProgramRef(const Pass* pPass)
    {
        writeGpuProgramRef("geometry_program_ref",
            pPass->getGeometryProgram(), pPass->getGeometryProgramParameters());
    }
    //--
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeFragmentProgramRef(const Pass* pPass)
    {
        writeGpuProgramRef("fragment_program_ref",
            pPass->getFragmentProgram(), pPass->getFragmentProgramParameters());
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeGpuProgramRef(std::string_view attrib,
                                                const GpuProgramPtr& program, const GpuProgramParametersSharedPtr& params)
    {       
        bool skipWriting = false;

        // Fire pre-write event.
        fireGpuProgramRefEvent(SerializeEvent::PRE_WRITE, skipWriting, attrib, program, params, nullptr);
        if (skipWriting)        
            return;

        mBuffer += "\n";
        writeAttribute(3, attrib);
        writeValue(quoteWord(program->getName()));
        beginSection(3);
        {
            // write out parameters
            GpuProgramParameters* defaultParams = nullptr;
            // does the GPU program have default parameters?
            if (program->hasDefaultParameters())
                defaultParams = program->getDefaultParameters().get();

            // Fire write begin event.
            fireGpuProgramRefEvent(SerializeEvent::WRITE_BEGIN, skipWriting, attrib, program, params, defaultParams);

            writeGPUProgramParameters(params, defaultParams);

            // Fire write end event.
            fireGpuProgramRefEvent(SerializeEvent::WRITE_END, skipWriting, attrib, program, params, defaultParams);
        }
        endSection(3);

        // add to GpuProgram container
        mGpuProgramDefinitionContainer.insert(program->getName());

        // Fire post section write event.
        fireGpuProgramRefEvent(SerializeEvent::POST_WRITE, skipWriting, attrib, program, params, nullptr);
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeGPUProgramParameters(
        const GpuProgramParametersSharedPtr& params,
        GpuProgramParameters* defaultParams, unsigned short level,
        const bool useMainBuffer)
    {
        // iterate through the constant definitions
        if (params->hasNamedParameters())
        {
            writeNamedGpuProgramParameters(params, defaultParams, level, useMainBuffer);
        }
        else
        {
            writeLowLevelGpuProgramParameters(params, defaultParams, level, useMainBuffer);
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeNamedGpuProgramParameters(
        const GpuProgramParametersSharedPtr& params,
        GpuProgramParameters* defaultParams, unsigned short level,
        const bool useMainBuffer)
    {
        for(auto const& [paramName, def] : params->getConstantDefinitions().map)
        {
            // get the constant definition
            // get any auto-link
            const GpuProgramParameters::AutoConstantEntry* autoEntry = 
                params->findAutoConstantEntry(paramName);
            const GpuProgramParameters::AutoConstantEntry* defaultAutoEntry = nullptr;
            if (defaultParams)
            {
                defaultAutoEntry = 
                    defaultParams->findAutoConstantEntry(paramName);
            }

            writeGpuProgramParameter("param_named", 
                                     paramName, autoEntry, defaultAutoEntry, 
                                     def.isFloat(), def.isDouble(), (def.isInt() || def.isSampler()), def.isUnsignedInt(),
                                     def.physicalIndex, def.elementSize * def.arraySize,
                                     params, defaultParams, level, useMainBuffer);
        }

    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeLowLevelGpuProgramParameters(
        const GpuProgramParametersSharedPtr& params,
        GpuProgramParameters* defaultParams, unsigned short level,
        const bool useMainBuffer)
    {
        // Iterate over the logical->physical mappings
        // This will represent the values which have been set

        // float params
        GpuLogicalBufferStructPtr floatLogical = params->getLogicalBufferStruct();
        if( floatLogical )
        {
            for(auto const& [logicalIndex, logicalUse] : floatLogical->map)
            {
                const GpuProgramParameters::AutoConstantEntry* autoEntry = 
                    params->findFloatAutoConstantEntry(logicalIndex);
                const GpuProgramParameters::AutoConstantEntry* defaultAutoEntry = nullptr;
                if (defaultParams)
                {
                    defaultAutoEntry = defaultParams->findFloatAutoConstantEntry(logicalIndex);
                }

                writeGpuProgramParameter("param_indexed", 
                                         StringConverter::toString(logicalIndex), autoEntry, 
                                         defaultAutoEntry, true, false, false, false,
                                         logicalUse.physicalIndex, logicalUse.currentSize,
                                         params, defaultParams, level, useMainBuffer);
            }
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeGpuProgramParameter(
        std::string_view commandName, std::string_view identifier, 
        const GpuProgramParameters::AutoConstantEntry* autoEntry, 
        const GpuProgramParameters::AutoConstantEntry* defaultAutoEntry, 
        bool isFloat, bool isDouble, bool isInt, bool isUnsignedInt,
        size_t physicalIndex, size_t physicalSize,
        const GpuProgramParametersSharedPtr& params, GpuProgramParameters* defaultParams,
        const ushort level, const bool useMainBuffer)
    {
        // Skip any params with array qualifiers
        // These are only for convenience of setters, the full array will be
        // written using the base, non-array identifier
        if (identifier.find('[') != String::npos)
        {
            return;
        }

        // get any auto-link
        // don't duplicate constants that are defined as a default parameter
        bool different = false;
        if (defaultParams)
        {
            // if default is auto but we're not or vice versa
            if ((autoEntry == nullptr) != (defaultAutoEntry == nullptr))
            {
                different = true;
            }
            else if (autoEntry)
            {
                // both must be auto
                // compare the auto values
                different = (autoEntry->paramType != defaultAutoEntry->paramType
                             || autoEntry->data != defaultAutoEntry->data);
            }
            else
            {
                // compare the non-auto (raw buffer) values
                // param buffers are always initialised with all zeros
                // so unset == unset
                if (isFloat)
                {
                    different = memcmp(
                        params->getFloatPointer(physicalIndex), 
                        defaultParams->getFloatPointer(physicalIndex),
                        sizeof(float) * physicalSize) != 0;
                }
                else if (isDouble)
                {
                    different = memcmp(
                        params->getDoublePointer(physicalIndex),
                        defaultParams->getDoublePointer(physicalIndex),
                        sizeof(double) * physicalSize) != 0;
                }
                else if (isInt)
                {
                    different = memcmp(
                        params->getIntPointer(physicalIndex), 
                        defaultParams->getIntPointer(physicalIndex),
                        sizeof(int) * physicalSize) != 0;
                }
                else if (isUnsignedInt)
                {
                    different = memcmp(
                        params->getUnsignedIntPointer(physicalIndex),
                        defaultParams->getUnsignedIntPointer(physicalIndex),
                        sizeof(uint) * physicalSize) != 0;
                }
                //else if (isBool)
                //{
                //    // different = memcmp(
                //    //     params->getBoolPointer(physicalIndex), 
                //    //     defaultParams->getBoolPointer(physicalIndex),
                //    //     sizeof(bool) * physicalSize) != 0;
                //    different = memcmp(
                //        params->getUnsignedIntPointer(physicalIndex), 
                //        defaultParams->getUnsignedIntPointer(physicalIndex),
                //        sizeof(uint) * physicalSize) != 0;
                //}
            }
        }

        if (!defaultParams || different)
        {
            std::string label{ commandName };

            // is it auto
            if (autoEntry)
                label += "_auto";

            writeAttribute(level, label, useMainBuffer);
            // output param index / name
            writeValue(quoteWord(identifier), useMainBuffer);

            // if auto output auto type name and data if needed
            if (autoEntry)
            {
                const GpuProgramParameters::AutoConstantDefinition* autoConstDef =
                    GpuProgramParameters::getAutoConstantDefinition(autoEntry->paramType);

                assert(autoConstDef && "Bad auto constant Definition Table");
                // output auto constant name
                writeValue(quoteWord(autoConstDef->name), useMainBuffer);
                // output data if it uses it
                using enum GpuProgramParameters::ACDataType;
                switch(autoConstDef->dataType)
                {
                case REAL:
                    writeValue(StringConverter::toString(autoEntry->fData), useMainBuffer);
                    break;

                case INT:
                    writeValue(StringConverter::toString(autoEntry->data), useMainBuffer);
                    break;

                default:
                    break;
                }
            }
            else // not auto so output all the values used
            {
                String countLabel;

                // only write a number if > 1
                if (physicalSize > 1)
                    countLabel = StringConverter::toString(physicalSize);

                if (isFloat)
                {
                    // Get pointer to start of values
                    const float* pFloat = params->getFloatPointer(physicalIndex);

                    writeValue(::std::format("float{}", countLabel), useMainBuffer);
                    // iterate through real constants
                    for (size_t f = 0; f < physicalSize; ++f)
                    {
                        writeValue(StringConverter::toString(*pFloat++), useMainBuffer);
                    }
                }
                else if (isDouble)
                {
                    // Get pointer to start of values
                    const double* pDouble = params->getDoublePointer(physicalIndex);

                    writeValue(::std::format("double{}", countLabel), useMainBuffer);
                    // iterate through double constants
                    for (size_t f = 0; f < physicalSize; ++f)
                    {
                        writeValue(StringConverter::toString(*pDouble++), useMainBuffer);
                    }
                }
                else if (isInt)
                {
                    // Get pointer to start of values
                    const int* pInt = params->getIntPointer(physicalIndex);

                    writeValue(::std::format("int{}", countLabel), useMainBuffer);
                    // iterate through int constants
                    for (size_t f = 0; f < physicalSize; ++f)
                    {
                        writeValue(StringConverter::toString(*pInt++), useMainBuffer);
                    }
                }
                else if (isUnsignedInt) 
                {
                    // Get pointer to start of values
                    const uint* pUInt = params->getUnsignedIntPointer(physicalIndex);

                    writeValue(::std::format("uint{}", countLabel), useMainBuffer);
                    // iterate through uint constants
                    for (size_t f = 0; f < physicalSize; ++f)
                    {
                        writeValue(StringConverter::toString(*pUInt++), useMainBuffer);
                    }
                }
                //else if (isBool)
                //{
                //    // Get pointer to start of values
                //    // const bool* pBool = params->getBoolPointer(physicalIndex);
                //    const uint* pBool = params->getUnsignedIntPointer(physicalIndex);

                //    writeValue(::std::format("bool{}", countLabel), useMainBuffer);
                //    // iterate through bool constants
                //    for (size_t f = 0; f < physicalSize; ++f)
                //    {
                //        writeValue(StringConverter::toString(*pBool++), useMainBuffer);
                //    }
                //}
            }
        }
    }
    //-----------------------------------------------------------------------
    void MaterialSerializer::writeGpuPrograms()
    {
        // iterate through gpu program names in container
        for (auto const& currentDef : mGpuProgramDefinitionContainer)
        {
            // get gpu program from gpu program manager
            GpuProgramPtr program = GpuProgramManager::getSingleton().getByName(
                currentDef, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            // write gpu program definition type to buffer
            // check program type for vertex program
            // write program type
            mGpuProgramBuffer += "\n";
            writeAttribute(0, program->getParameter("type"), false);

            // write program name
            writeValue( quoteWord(program->getName()), false);
            // write program language
            auto const language = program->getLanguage();
            writeValue( language, false );
            // write opening braces
            beginSection(0, false);
            {
                // write program source + filename
                writeAttribute(1, "source", false);
                writeValue(quoteWord(program->getSourceFile()), false);
                // write special parameters based on language
                for (const auto& name : program->getParameters())
                {
                    if (name != "type" &&
                        name != "assemble_code" &&
                        name != "micro_code" &&
                        name != "external_micro_code")
                    {
                        String paramstr = program->getParameter(name);
                        if ((name == "includes_skeletal_animation") && (paramstr == "false"))
                            paramstr.clear();
                        if ((name == "includes_morph_animation") && (paramstr == "false"))
                            paramstr.clear();
                        if ((name == "includes_pose_animation") && (paramstr == "0"))
                            paramstr.clear();
                        if ((name == "uses_vertex_texture_fetch") && (paramstr == "false"))
                            paramstr.clear();

                        if ((language != "asm") && (name == "syntax"))
                            paramstr.clear();

                        if (!paramstr.empty())
                        {
                            writeAttribute(1, name, false);
                            writeValue(paramstr, false);
                        }
                    }
                }

                // write default parameters
                if (program->hasDefaultParameters())
                {
                    mGpuProgramBuffer += "\n";
                    GpuProgramParametersSharedPtr gpuDefaultParams = program->getDefaultParameters();
                    writeAttribute(1, "default_params", false);
                    beginSection(1, false);
                    writeGPUProgramParameters(gpuDefaultParams, nullptr, 2, false);
                    endSection(1, false);
                }
            }
            // write closing braces
            endSection(0, false);
        }

        mGpuProgramBuffer += "\n";
    }

    //---------------------------------------------------------------------
    void MaterialSerializer::addListener(Listener* listener)
    {
        mListeners.push_back(listener);
    }

    //---------------------------------------------------------------------
    void MaterialSerializer::removeListener(Listener* listener)
    {
        auto i = std::ranges::find(mListeners, listener);
        if (i != mListeners.end())
            mListeners.erase(i);
    }

    //---------------------------------------------------------------------
    void MaterialSerializer::fireMaterialEvent(SerializeEvent event, bool& skip, const Material* mat)
    {
        for (auto const& it : mListeners)
        {
            it->materialEventRaised(this, event, skip, mat);
            if (skip)
                break;
        }       
    }

    //---------------------------------------------------------------------
    void MaterialSerializer::fireTechniqueEvent(SerializeEvent event, bool& skip, const Technique* tech)
    {
        for (auto const& it : mListeners)
        {
            it->techniqueEventRaised(this, event, skip, tech);
            if (skip)
                break;
        }
    }

    //---------------------------------------------------------------------
    void MaterialSerializer::firePassEvent(SerializeEvent event, bool& skip, const Pass* pass)
    {
        for (auto const& it : mListeners)
        {
            it->passEventRaised(this, event, skip, pass);
            if (skip)
                break;
        }
    }

    //---------------------------------------------------------------------
    void MaterialSerializer::fireGpuProgramRefEvent(SerializeEvent event, bool& skip,
        std::string_view attrib, 
        const GpuProgramPtr& program, 
        const GpuProgramParametersSharedPtr& params,
        GpuProgramParameters* defaultParams)
    {
        for (auto const& it : mListeners)
        {
            it->gpuProgramRefEventRaised(this, event, skip, attrib, program, params, defaultParams);
            if (skip)
                break;
        }
    }   

    //---------------------------------------------------------------------
    void MaterialSerializer::fireTextureUnitStateEvent(SerializeEvent event, bool& skip,
        const TextureUnitState* textureUnit)
    {
        for (auto const& it : mListeners)
        {
            it->textureUnitStateEventRaised(this, event, skip, textureUnit);
            if (skip)
                break;
        }
    }   
}
