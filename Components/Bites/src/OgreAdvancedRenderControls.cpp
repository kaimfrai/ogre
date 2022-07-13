// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT

#include "OgreAdvancedRenderControls.hpp"

#include <string>
#include <vector>

#include "OgreCamera.hpp"
#include "OgreCommon.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreMaterialManager.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreProfiler.hpp"
#include "OgreQuaternion.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderSystemCapabilities.hpp"
#include "OgreRenderTarget.hpp"
#include "OgreRoot.hpp"
#include "OgreShaderGenerator.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderRenderState.hpp"
#include "OgreShaderSubRenderState.hpp"
#include "OgreStringConverter.hpp"
#include "OgreStringVector.hpp"
#include "OgreTextureManager.hpp"
#include "OgreTrays.hpp"
#include "OgreVector.hpp"
#include "OgreViewport.hpp"

namespace Ogre {
struct FrameEvent;
}  // namespace Ogre

namespace OgreBites {
AdvancedRenderControls::AdvancedRenderControls(TrayManager* trayMgr, Ogre::Camera* cam)
    : mCamera(cam), mTrayMgr(trayMgr) {
    mRoot = Ogre::Root::getSingletonPtr();

    // create a params panel for displaying sample details
    Ogre::StringVector items;
    items.push_back("cam.pX");
    items.push_back("cam.pY");
    items.push_back("cam.pZ");
    items.push_back("");
    items.push_back("cam.oW");
    items.push_back("cam.oX");
    items.push_back("cam.oY");
    items.push_back("cam.oZ");
    items.push_back("");
    items.push_back("Filtering");
    items.push_back("Poly Mode");

    mShaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
    items.push_back("RT Shaders");
    items.push_back("Lighting Model");
    items.push_back("Compact Policy");
    items.push_back("Generated VS");
    items.push_back("Generated FS");

    mDetailsPanel = mTrayMgr->createParamsPanel(TrayLocation::NONE, "DetailsPanel", 200, items);
    mDetailsPanel->hide();

    mDetailsPanel->setParamValue(9, "Bilinear");
    mDetailsPanel->setParamValue(10, "Solid");

    mDetailsPanel->setParamValue(11, "Off");
    if (!mRoot->getRenderSystem()->getCapabilities()->hasCapability(Ogre::Capabilities::FIXED_FUNCTION)) {
        mDetailsPanel->setParamValue(11, "On");
    }

    mDetailsPanel->setParamValue(12, "Pixel");
    mDetailsPanel->setParamValue(13, "Low");
    mDetailsPanel->setParamValue(14, "0");
    mDetailsPanel->setParamValue(15, "0");
}

AdvancedRenderControls::~AdvancedRenderControls() {
    mTrayMgr->destroyWidget(mDetailsPanel);
}

auto AdvancedRenderControls::keyPressed(const KeyDownEvent& evt) noexcept -> bool {
    if (mTrayMgr->isDialogVisible())
        return true; // don't process any more keys if dialog is up

    int key = evt.keysym.sym;

    if (key == 'f') // toggle visibility of advanced frame stats
    {
        mTrayMgr->toggleAdvancedFrameStats();
    } else if (key == 'g') // toggle visibility of even rarer debugging details
    {
        if (mDetailsPanel->getTrayLocation() == TrayLocation::NONE) {
            mTrayMgr->moveWidgetToTray(mDetailsPanel, TrayLocation::TOPRIGHT, 0);
            mDetailsPanel->show();
        } else {
            mTrayMgr->removeWidgetFromTray(mDetailsPanel);
            mDetailsPanel->hide();
        }
    } else if (key == 't') // cycle texture filtering mode
    {
        Ogre::String newVal;
        Ogre::TextureFilterOptions tfo;
        unsigned int aniso;

        Ogre::FilterOptions mip = Ogre::MaterialManager::getSingleton().getDefaultTextureFiltering(Ogre::FilterType::Mip);

        switch (Ogre::MaterialManager::getSingleton().getDefaultTextureFiltering(Ogre::FilterType::Mag)) {
        case Ogre::FilterOptions::LINEAR:
            if (mip == Ogre::FilterOptions::POINT) {
                newVal = "Trilinear";
                tfo = Ogre::TextureFilterOptions::TRILINEAR;
                aniso = 1;
            } else {
                newVal = "Anisotropic";
                tfo = Ogre::TextureFilterOptions::ANISOTROPIC;
                aniso = 8;
            }
            break;
        case Ogre::FilterOptions::ANISOTROPIC:
            newVal = "None";
            tfo = Ogre::TextureFilterOptions::NONE;
            aniso = 1;
            break;
        default:
            newVal = "Bilinear";
            tfo = Ogre::TextureFilterOptions::BILINEAR;
            aniso = 1;
            break;
        }

        Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);
        Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(aniso);
        mDetailsPanel->setParamValue(9, newVal);
    } else if (key == 'r') // cycle polygon rendering mode
    {
        Ogre::String newVal;
        Ogre::PolygonMode pm;

        switch (mCamera->getPolygonMode()) {
        case Ogre::PolygonMode::SOLID:
            newVal = "Wireframe";
            pm = Ogre::PolygonMode::WIREFRAME;
            break;
        case Ogre::PolygonMode::WIREFRAME:
            newVal = "Points";
            pm = Ogre::PolygonMode::POINTS;
            break;
        default:
            newVal = "Solid";
            pm = Ogre::PolygonMode::SOLID;
            break;
        }

        mCamera->setPolygonMode(pm);
        mDetailsPanel->setParamValue(10, newVal);
    } else if (key == SDLK_F5) // refresh all textures
    {
        Ogre::TextureManager::getSingleton().reloadAll();
    }
    else if (key == SDLK_F6)   // take a screenshot
    {
        mCamera->getViewport()->getTarget()->writeContentsToTimestampedFile("screenshot", ".png");
    }
    // Toggle visibility of profiler window
    else if (key == 'p')
    {
        if (auto prof = Ogre::Profiler::getSingletonPtr())
            prof->setEnabled(!prof->getEnabled());
    }
    // Toggle schemes.
    else if (key == SDLK_F2) {
        if (mRoot->getRenderSystem()->getCapabilities()->hasCapability(Ogre::Capabilities::FIXED_FUNCTION)) {
            Ogre::Viewport* mainVP = mCamera->getViewport();
            std::string_view curMaterialScheme = mainVP->getMaterialScheme();

            if (curMaterialScheme == Ogre::MaterialManager::DEFAULT_SCHEME_NAME) {
                mainVP->setMaterialScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
                mDetailsPanel->setParamValue(11, "On");
            } else if (curMaterialScheme == Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME) {
                mainVP->setMaterialScheme(Ogre::MaterialManager::DEFAULT_SCHEME_NAME);
                mDetailsPanel->setParamValue(11, "Off");
            }
        }
    }
    // Toggles per pixel per light model.
    else if (key == SDLK_F3) {
        static bool useFFPLighting = true;

        //![rtss_per_pixel]
        // Grab the scheme render state.
        Ogre::RTShader::RenderState* schemRenderState =
            mShaderGenerator->getRenderState(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

        // Add per pixel lighting sub render state to the global scheme render state.
        // It will override the default FFP lighting sub render state.
        if (useFFPLighting) {
            auto perPixelLightModel = mShaderGenerator->createSubRenderState("FFP_Lighting");

            schemRenderState->addTemplateSubRenderState(perPixelLightModel);
        }
        //![rtss_per_pixel]

        // Search the per pixel sub render state and remove it.
        else {
            for (auto srs : schemRenderState->getSubRenderStates()) {
                // This is the per pixel sub render state -> remove it.
                if (srs->getType() == "FFP_Lighting") {
                    schemRenderState->removeSubRenderState(srs);
                    break;
                }
            }
        }

        // Invalidate the scheme in order to re-generate all shaders based technique related to this
        // scheme.
        mShaderGenerator->invalidateScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

        // Update UI.
        if (!useFFPLighting)
            mDetailsPanel->setParamValue(12, "Pixel");
        else
            mDetailsPanel->setParamValue(12, "Vertex");
        useFFPLighting = !useFFPLighting;
    }
    // Switch vertex shader outputs compaction policy.
    else if (key == SDLK_F4) {
        switch (mShaderGenerator->getVertexShaderOutputsCompactPolicy()) {
        case Ogre::RTShader::VSOutputCompactPolicy::LOW:
            mShaderGenerator->setVertexShaderOutputsCompactPolicy(Ogre::RTShader::VSOutputCompactPolicy::MEDIUM);
            mDetailsPanel->setParamValue(13, "Medium");
            break;

        case Ogre::RTShader::VSOutputCompactPolicy::MEDIUM:
            mShaderGenerator->setVertexShaderOutputsCompactPolicy(Ogre::RTShader::VSOutputCompactPolicy::HIGH);
            mDetailsPanel->setParamValue(13, "High");
            break;

        case Ogre::RTShader::VSOutputCompactPolicy::HIGH:
            mShaderGenerator->setVertexShaderOutputsCompactPolicy(Ogre::RTShader::VSOutputCompactPolicy::LOW);
            mDetailsPanel->setParamValue(13, "Low");
            break;
        }

        // Invalidate the scheme in order to re-generate all shaders based technique related to this
        // scheme.
        mShaderGenerator->invalidateScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
    }

    return InputListener::keyPressed(evt);
}

void AdvancedRenderControls::frameRendered(const Ogre::FrameEvent& evt) {
    using namespace Ogre;
    if (!mTrayMgr->isDialogVisible() && mDetailsPanel->isVisible())
    {
        // if details panel is visible, then update its contents
        mDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(mCamera->getDerivedPosition().x));
        mDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(mCamera->getDerivedPosition().y));
        mDetailsPanel->setParamValue(2, Ogre::StringConverter::toString(mCamera->getDerivedPosition().z));
        mDetailsPanel->setParamValue(4, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().w));
        mDetailsPanel->setParamValue(5, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().x));
        mDetailsPanel->setParamValue(6, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().y));
        mDetailsPanel->setParamValue(7, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().z));

        mDetailsPanel->setParamValue(14, StringConverter::toString(mShaderGenerator->getShaderCount(GpuProgramType::VERTEX_PROGRAM)));
        mDetailsPanel->setParamValue(15, StringConverter::toString(mShaderGenerator->getShaderCount(GpuProgramType::FRAGMENT_PROGRAM)));
    }
}

} /* namespace OgreBites */
