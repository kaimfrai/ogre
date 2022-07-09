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
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ostream>
#include <span>
#include <string>
#include <vector>

#include "OgreBlendMode.hpp"
#include "OgreColourValue.hpp"
#include "OgreCommon.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreMaterial.hpp"
#include "OgreMaterialManager.hpp"
#include "OgrePass.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderSystemCapabilities.hpp"
#include "OgreRoot.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreString.hpp"
#include "OgreTechnique.hpp"
#include "OgreTexture.hpp"
#include "OgreTextureUnitState.hpp"


namespace Ogre {
    //-----------------------------------------------------------------------------
    Technique::Technique(Material* parent)
        : mParent(parent), mIlluminationPassesCompilationPhase(IPS_NOT_COMPILED), mIsSupported(false), mLodIndex(0), mSchemeIndex(0)
    {
        // See above, defaults to unsupported until examined
    }
    //-----------------------------------------------------------------------------
    Technique::Technique(Material* parent, const Technique& oth)
        : mParent(parent), mLodIndex(0), mSchemeIndex(0)
    {
        // Copy using operator=
        *this = oth;
    }
    //-----------------------------------------------------------------------------
    Technique::~Technique()
    {
        removeAllPasses();
        clearIlluminationPasses();
    }
    //-----------------------------------------------------------------------------
    auto Technique::isSupported() const noexcept -> bool
    {
        return mIsSupported;
    }
    //-----------------------------------------------------------------------------
    auto Technique::calculateSize() const -> size_t
    {
        size_t memSize = 0;

        // Tally up passes
        for (auto mPasse : mPasses)
        {
            memSize += mPasse->calculateSize();
        }
        return memSize;
    }
    //-----------------------------------------------------------------------------
    auto Technique::_compile(bool autoManageTextureUnits) -> String
    {
        StringStream errors;

        if(!Root::getSingleton().getRenderSystem()) {
            errors << "NULL RenderSystem";
        } else {
            mIsSupported = checkGPURules(errors) && checkHardwareSupport(autoManageTextureUnits, errors);
        }

        // Compile for categorised illumination on demand
        clearIlluminationPasses();
        mIlluminationPassesCompilationPhase = IPS_NOT_COMPILED;

        return errors.str();

    }
    //---------------------------------------------------------------------
    auto Technique::checkHardwareSupport(bool autoManageTextureUnits, StringStream& compileErrors) -> bool
    {
        // Go through each pass, checking requirements
        unsigned short passNum = 0;
        const RenderSystemCapabilities* caps =
            Root::getSingleton().getRenderSystem()->getCapabilities();
        unsigned short numTexUnits = caps->getNumTextureUnits();
        for (auto i = mPasses.begin(); i != mPasses.end(); ++i)
        {
            Pass* currPass = *i;
            // Adjust pass index
            currPass->_notifyIndex(passNum);

            // Check texture unit requirements
            size_t numTexUnitsRequested = currPass->getNumTextureUnitStates();
            // Don't trust getNumTextureUnits for programmable
            if(!currPass->hasFragmentProgram())
            {
                if (numTexUnitsRequested > numTexUnits)
                {
                    if (!autoManageTextureUnits)
                    {
                        // The user disabled auto pass split
                        compileErrors << "Pass " << passNum <<
                            ": Too many texture units for the current hardware and no splitting allowed."
                            << std::endl;
                        return false;
                    }
                    else if (currPass->hasVertexProgram())
                    {
                        // Can't do this one, and can't split a programmable pass
                        compileErrors << "Pass " << passNum <<
                            ": Too many texture units for the current hardware and "
                            "cannot split programmable passes."
                            << std::endl;
                        return false;
                    }
                }

                // Check a few fixed-function options in texture layers
                size_t texUnit = 0;
                for(const TextureUnitState* tex : currPass->getTextureUnitStates())
                {
                    String err;
                    if ((tex->getTextureType() == TEX_TYPE_3D) && !caps->hasCapability(RSC_TEXTURE_3D))
                    {
                        err = "Volume textures";
                    }

                    if ((tex->getTextureType() == TEX_TYPE_2D_ARRAY) &&
                        !caps->hasCapability(RSC_TEXTURE_2D_ARRAY))
                    {
                        err = "Array textures";
                    }

                    if (!err.empty())
                    {
                        // Fail
                        compileErrors << "Pass " << passNum << " Tex " << texUnit << ": " << err
                                      << " not supported by current environment.";
                        return false;
                    }
                    ++texUnit;
                }

                // We're ok on operations, now we need to check # texture units

                    // Keep splitting this pass so long as units requested > gpu units
                while (numTexUnitsRequested > numTexUnits)
                {
                    // chop this pass into many passes
                    currPass = currPass->_split(numTexUnits);
                    numTexUnitsRequested = currPass->getNumTextureUnitStates();
                    // Advance pass number
                    ++passNum;
                    // Reset iterator
                    i = mPasses.begin() + passNum;
                    // Move the new pass to the right place (will have been created
                    // at the end, may be other passes in between)
                    assert(mPasses.back() == currPass);
                    std::ranges::move_backward(std::span{i, mPasses.end() - 1}, mPasses.end());
                    *i = currPass;
                    // Adjust pass index
                    currPass->_notifyIndex(passNum);
                }
            }

            

            //Check compilation errors for all program types.
            for (int t = 0; t < 6; t++)
            {
                auto programType = GpuProgramType(t);
                if (currPass->hasGpuProgram(programType))
                {
                    GpuProgramPtr program = currPass->getGpuProgram(programType);
                    if (!program->isSupported())
                    {
                        compileErrors << ::std::format("Pass {}: {} program {} cannot be used -", passNum, GpuProgram::getProgramTypeName(programType), program->getName()
                                           );
                        if (program->hasCompileError() && program->getSource().empty())
                            compileErrors << "resource not found.";
                        else if (program->hasCompileError())
                            compileErrors << "compile error.";
                        else
                            compileErrors << "not supported.";

                        compileErrors << std::endl;
                        return false;
                    }
                }
            }

            ++passNum;
        }
        // If we got this far, we're ok
        return true;
    }
    //---------------------------------------------------------------------
    auto Technique::checkGPURules(StringStream& errors) -> bool
    {
        const RenderSystemCapabilities* caps =
            Root::getSingleton().getRenderSystem()->getCapabilities();

        StringStream includeRules;
        bool includeRulesPresent = false;
        bool includeRuleMatched = false;

        // Check vendors first
        for (auto & mGPUVendorRule : mGPUVendorRules)
        {
            if (mGPUVendorRule.includeOrExclude == INCLUDE)
            {
                includeRulesPresent = true;
                includeRules << caps->vendorToString(mGPUVendorRule.vendor) << " ";
                if (mGPUVendorRule.vendor == caps->getVendor())
                    includeRuleMatched = true;
            }
            else // EXCLUDE
            {
                if (mGPUVendorRule.vendor == caps->getVendor())
                {
                    errors << "Excluded GPU vendor: " << caps->vendorToString(mGPUVendorRule.vendor)
                        << std::endl;
                    return false;
                }
                    
            }
        }

        if (includeRulesPresent && !includeRuleMatched)
        {
            errors << "Failed to match GPU vendor: " << includeRules.str( )
                << std::endl;
            return false;
        }

        // now check device names
        includeRules.str("");
        includeRulesPresent = false;
        includeRuleMatched = false;

        for (auto & mGPUDeviceNameRule : mGPUDeviceNameRules)
        {
            if (mGPUDeviceNameRule.includeOrExclude == INCLUDE)
            {
                includeRulesPresent = true;
                includeRules << mGPUDeviceNameRule.devicePattern << " ";
                if (StringUtil::match(caps->getDeviceName(), mGPUDeviceNameRule.devicePattern, mGPUDeviceNameRule.caseSensitive))
                    includeRuleMatched = true;
            }
            else // EXCLUDE
            {
                if (StringUtil::match(caps->getDeviceName(), mGPUDeviceNameRule.devicePattern, mGPUDeviceNameRule.caseSensitive))
                {
                    errors << "Excluded GPU device: " << mGPUDeviceNameRule.devicePattern
                        << std::endl;
                    return false;
                }

            }
        }

        if (includeRulesPresent && !includeRuleMatched)
        {
            errors << "Failed to match GPU device: " << includeRules.str( )
                << std::endl;
            return false;
        }

        // passed
        return true;
    }
    //-----------------------------------------------------------------------------
    auto Technique::createPass() -> Pass*
    {
        Pass* newPass = new Pass(this, static_cast<unsigned short>(mPasses.size()));
        mPasses.push_back(newPass);
        return newPass;
    }
    //-----------------------------------------------------------------------------
    auto Technique::getPass(StringView name) const -> Pass*
    {
        Pass* foundPass = nullptr;

        // iterate through techniques to find a match
        for (auto const& i : mPasses)
        {
            if (i->getName() == name )
            {
                foundPass = i;
                break;
            }
        }

        return foundPass;
    }
    //-----------------------------------------------------------------------------
    void Technique::removePass(unsigned short index)
    {
        assert(index < mPasses.size() && "Index out of bounds");
        auto i = mPasses.begin() + index;
        (*i)->queueForDeletion();
        i = mPasses.erase(i);
        // Adjust passes index
        for (; i != mPasses.end(); ++i, ++index)
        {
            (*i)->_notifyIndex(index);
        }
    }
    //-----------------------------------------------------------------------------
    void Technique::removeAllPasses()
    {
        for (auto & mPasse : mPasses)
        {
            mPasse->queueForDeletion();
        }
        mPasses.clear();
    }

    //-----------------------------------------------------------------------------
    auto Technique::movePass(const unsigned short sourceIndex, const unsigned short destinationIndex) -> bool
    {
        bool moveSuccessful = false;

        // don't move the pass if source == destination
        if (sourceIndex == destinationIndex) return true;

        if( (sourceIndex < mPasses.size()) && (destinationIndex < mPasses.size()))
        {
            auto i = mPasses.begin() + sourceIndex;
            //Passes::iterator DestinationIterator = mPasses.begin() + destinationIndex;

            Pass* pass = (*i);
            mPasses.erase(i);

            i = mPasses.begin() + destinationIndex;

            mPasses.insert(i, pass);

            // Adjust passes index
            unsigned short beginIndex, endIndex;
            if (destinationIndex > sourceIndex)
            {
                beginIndex = sourceIndex;
                endIndex = destinationIndex;
            }
            else
            {
                beginIndex = destinationIndex;
                endIndex = sourceIndex;
            }
            for (unsigned short index = beginIndex; index <= endIndex; ++index)
            {
                mPasses[index]->_notifyIndex(index);
            }
            moveSuccessful = true;
        }

        return moveSuccessful;
    }

    //-----------------------------------------------------------------------------
    auto Technique::operator=(const Technique& rhs) -> Technique&
    {
        mName = rhs.mName;
        this->mIsSupported = rhs.mIsSupported;
        this->mLodIndex = rhs.mLodIndex;
        this->mSchemeIndex = rhs.mSchemeIndex;
        this->mShadowCasterMaterial = rhs.mShadowCasterMaterial;
        this->mShadowCasterMaterialName = rhs.mShadowCasterMaterialName;
        this->mShadowReceiverMaterial = rhs.mShadowReceiverMaterial;
        this->mShadowReceiverMaterialName = rhs.mShadowReceiverMaterialName;
        this->mGPUVendorRules = rhs.mGPUVendorRules;
        this->mGPUDeviceNameRules = rhs.mGPUDeviceNameRules;

        // copy passes
        removeAllPasses();
        for (auto mPasse : rhs.mPasses)
        {
            Pass* p = new Pass(this, mPasse->getIndex(), *mPasse);
            mPasses.push_back(p);
        }
        // Compile for categorised illumination on demand
        clearIlluminationPasses();
        mIlluminationPassesCompilationPhase = IPS_NOT_COMPILED;
        return *this;
    }
    //-----------------------------------------------------------------------------
    auto Technique::isTransparent() const noexcept -> bool
    {
        if (mPasses.empty())
        {
            return false;
        }
        else
        {
            // Base decision on the transparency of the first pass
            return mPasses[0]->isTransparent();
        }
    }
    //-----------------------------------------------------------------------------
    auto Technique::isTransparentSortingEnabled() const noexcept -> bool
    {
        if (mPasses.empty())
        {
            return true;
        }
        else
        {
            // Base decision on the transparency of the first pass
            return mPasses[0]->getTransparentSortingEnabled();
        }
    }
    //-----------------------------------------------------------------------------
    auto Technique::isTransparentSortingForced() const noexcept -> bool
    {
        if (mPasses.empty())
        {
            return false;
        }
        else
        {
            // Base decision on the first pass
            return mPasses[0]->getTransparentSortingForced();
        }
    }
    //-----------------------------------------------------------------------------
    auto Technique::isDepthWriteEnabled() const noexcept -> bool
    {
        if (mPasses.empty())
        {
            return false;
        }
        else
        {
            // Base decision on the depth settings of the first pass
            return mPasses[0]->getDepthWriteEnabled();
        }
    }
    //-----------------------------------------------------------------------------
    auto Technique::isDepthCheckEnabled() const noexcept -> bool
    {
        if (mPasses.empty())
        {
            return false;
        }
        else
        {
            // Base decision on the depth settings of the first pass
            return mPasses[0]->getDepthCheckEnabled();
        }
    }
    //-----------------------------------------------------------------------------
    auto Technique::hasColourWriteDisabled() const -> bool
    {
        if (mPasses.empty())
        {
            return true;
        }
        else
        {
            // Base decision on the colour write settings of the first pass
            return !mPasses[0]->getColourWriteEnabled();
        }
    }
    //-----------------------------------------------------------------------------
    void Technique::_prepare()
    {
        assert (mIsSupported && "This technique is not supported");
        // Load each pass
        for (auto & mPasse : mPasses)
        {
            mPasse->_prepare();
        }

        for (auto & mIlluminationPasse : mIlluminationPasses)
        {
            if(mIlluminationPasse->pass != mIlluminationPasse->originalPass)
                mIlluminationPasse->pass->_prepare();
        }
    }
    //-----------------------------------------------------------------------------
    void Technique::_unprepare()
    {
        // Unload each pass
        for (auto & mPasse : mPasses)
        {
            mPasse->_unprepare();
        }
    }
    //-----------------------------------------------------------------------------
    void Technique::_load()
    {
        assert (mIsSupported && "This technique is not supported");
        // Load each pass
        for (auto & mPasse : mPasses)
        {
            mPasse->_load();
        }

        for (auto & mIlluminationPasse : mIlluminationPasses)
        {
            if(mIlluminationPasse->pass != mIlluminationPasse->originalPass)
                mIlluminationPasse->pass->_load();
        }

        if (mShadowCasterMaterial)
        {
            mShadowCasterMaterial->load();
        }
        else if (!mShadowCasterMaterialName.empty())
        {
            // in case we could not get material as it wasn't yet parsed/existent at that time.
            mShadowCasterMaterial = MaterialManager::getSingleton().getByName(mShadowCasterMaterialName);
            if (mShadowCasterMaterial)
                mShadowCasterMaterial->load();
        }
        if (mShadowReceiverMaterial)
        {
            mShadowReceiverMaterial->load();
        }
        else if (!mShadowReceiverMaterialName.empty())
        {
            // in case we could not get material as it wasn't yet parsed/existent at that time.
            mShadowReceiverMaterial = MaterialManager::getSingleton().getByName(mShadowReceiverMaterialName);
            if (mShadowReceiverMaterial)
                mShadowReceiverMaterial->load();
        }
    }
    //-----------------------------------------------------------------------------
    void Technique::_unload()
    {
        // Unload each pass
        for (auto & mPasse : mPasses)
        {
            mPasse->_unload();
        }   
    }
    //-----------------------------------------------------------------------------
    auto Technique::isLoaded() const noexcept -> bool
    {
        // Only supported technique will be loaded
        return mParent->isLoaded() && mIsSupported;
    }
    //-----------------------------------------------------------------------
    #define ALL_PASSES(fncall) for(auto p : mPasses) p->fncall
    void Technique::setPointSize(Real ps) { ALL_PASSES(setPointSize(ps)); }
    //-----------------------------------------------------------------------
    void Technique::setAmbient(float red, float green, float blue) { setAmbient(ColourValue(red, green, blue)); }
    //-----------------------------------------------------------------------
    void Technique::setAmbient(const ColourValue& ambient) { ALL_PASSES(setAmbient(ambient)); }
    //-----------------------------------------------------------------------
    void Technique::setDiffuse(float red, float green, float blue, float alpha)
    {
        ALL_PASSES(setDiffuse(red, green, blue, alpha));
    }
    //-----------------------------------------------------------------------
    void Technique::setDiffuse(const ColourValue& diffuse) { setDiffuse(diffuse.r, diffuse.g, diffuse.b, diffuse.a); }
    //-----------------------------------------------------------------------
    void Technique::setSpecular(float red, float green, float blue, float alpha)
    {
        ALL_PASSES(setSpecular(red, green, blue, alpha));
    }
    //-----------------------------------------------------------------------
    void Technique::setSpecular(const ColourValue& specular)
    {
        setSpecular(specular.r, specular.g, specular.b, specular.a);
    }
    //-----------------------------------------------------------------------
    void Technique::setShininess(Real val) { ALL_PASSES(setShininess(val)); }
    //-----------------------------------------------------------------------
    void Technique::setSelfIllumination(float red, float green, float blue)
    {
        setSelfIllumination(ColourValue(red, green, blue));
    }
    //-----------------------------------------------------------------------
    void Technique::setSelfIllumination(const ColourValue& selfIllum) { ALL_PASSES(setSelfIllumination(selfIllum)); }
    //-----------------------------------------------------------------------
    void Technique::setDepthCheckEnabled(bool enabled) { ALL_PASSES(setDepthCheckEnabled(enabled)); }
    //-----------------------------------------------------------------------
    void Technique::setDepthWriteEnabled(bool enabled) { ALL_PASSES(setDepthWriteEnabled(enabled)); }
    //-----------------------------------------------------------------------
    void Technique::setDepthFunction(CompareFunction func) { ALL_PASSES(setDepthFunction(func)); }
    //-----------------------------------------------------------------------
    void Technique::setColourWriteEnabled(bool enabled) { ALL_PASSES(setColourWriteEnabled(enabled)); }
    //-----------------------------------------------------------------------
    void Technique::setColourWriteEnabled(bool red, bool green, bool blue, bool alpha)
    {
        ALL_PASSES(setColourWriteEnabled(red, green, blue, alpha));
    }
    //-----------------------------------------------------------------------
    void Technique::setCullingMode(CullingMode mode) { ALL_PASSES(setCullingMode(mode)); }
    //-----------------------------------------------------------------------
    void Technique::setManualCullingMode(ManualCullingMode mode) { ALL_PASSES(setManualCullingMode(mode)); }
    //-----------------------------------------------------------------------
    void Technique::setLightingEnabled(bool enabled) { ALL_PASSES(setLightingEnabled(enabled)); }
    //-----------------------------------------------------------------------
    void Technique::setShadingMode(ShadeOptions mode) { ALL_PASSES(setShadingMode(mode)); }
    //-----------------------------------------------------------------------
    void Technique::setFog(bool overrideScene, FogMode mode, const ColourValue& colour, Real expDensity,
                           Real linearStart, Real linearEnd)
    {
        ALL_PASSES(setFog(overrideScene, mode, colour, expDensity, linearStart, linearEnd));
    }
    //-----------------------------------------------------------------------
    void Technique::setDepthBias(float constantBias, float slopeScaleBias)
    {
        ALL_PASSES(setDepthBias(constantBias, slopeScaleBias));
    }
    //-----------------------------------------------------------------------
    void Technique::setTextureFiltering(TextureFilterOptions filterType)
    {
        ALL_PASSES(setTextureFiltering(filterType));
    }
    // --------------------------------------------------------------------
    void Technique::setTextureAnisotropy(unsigned int maxAniso) { ALL_PASSES(setTextureAnisotropy(maxAniso)); }
    // --------------------------------------------------------------------
    void Technique::setSceneBlending(const SceneBlendType sbt) { ALL_PASSES(setSceneBlending(sbt)); }
    // --------------------------------------------------------------------
    void Technique::setSeparateSceneBlending(const SceneBlendType sbt, const SceneBlendType sbta)
    {
        ALL_PASSES(setSeparateSceneBlending(sbt, sbta));
    }
    // --------------------------------------------------------------------
    void Technique::setSceneBlending(const SceneBlendFactor sourceFactor, const SceneBlendFactor destFactor)
    {
        ALL_PASSES(setSceneBlending(sourceFactor, destFactor));
    }
    // --------------------------------------------------------------------
    void Technique::setSeparateSceneBlending( const SceneBlendFactor sourceFactor, const SceneBlendFactor destFactor, const SceneBlendFactor sourceFactorAlpha, const SceneBlendFactor destFactorAlpha)
    {
        ALL_PASSES(setSeparateSceneBlending(sourceFactor, destFactor, sourceFactorAlpha, destFactorAlpha));
    }
    #undef ALL_PASSES

    // --------------------------------------------------------------------
    void Technique::setName(StringView name)
    {
        mName = name;
    }


    //-----------------------------------------------------------------------
    void Technique::_notifyNeedsRecompile()
    {
        // Disable require to recompile when splitting illumination passes
        if (mIlluminationPassesCompilationPhase != IPS_COMPILE_DISABLED)
        {
            mParent->_notifyNeedsRecompile();
        }
    }
    //-----------------------------------------------------------------------
    void Technique::setLodIndex(unsigned short index)
    {
        mLodIndex = index;
        _notifyNeedsRecompile();
    }
    //-----------------------------------------------------------------------
    void Technique::setSchemeName(StringView schemeName)
    {
        mSchemeIndex = MaterialManager::getSingleton()._getSchemeIndex(schemeName);
        _notifyNeedsRecompile();
    }
    //-----------------------------------------------------------------------
    auto Technique::getSchemeName() const noexcept -> StringView
    {
        return MaterialManager::getSingleton()._getSchemeName(mSchemeIndex);
    }
    //-----------------------------------------------------------------------
    auto Technique::_getSchemeIndex() const -> unsigned short
    {
        return mSchemeIndex;
    }
    //---------------------------------------------------------------------
    auto Technique::checkManuallyOrganisedIlluminationPasses() -> bool
    {
        // first check whether all passes have manually assigned illumination
        for (auto & mPasse : mPasses)
        {
            if (mPasse->getIlluminationStage() == IS_UNKNOWN)
                return false;
        }

        // ok, all manually controlled, so just use that
        for (auto & mPasse : mPasses)
        {
            auto* iPass = new IlluminationPass();
            iPass->destroyOnShutdown = false;
            iPass->originalPass = iPass->pass = mPasse;
            iPass->stage = mPasse->getIlluminationStage();
            mIlluminationPasses.push_back(iPass);
        }

        return true;
    }
    //-----------------------------------------------------------------------
    void Technique::_compileIlluminationPasses()
    {
        clearIlluminationPasses();

        if (!checkManuallyOrganisedIlluminationPasses())
        {
            // Build based on our own heuristics

            Passes::iterator i, iend;
            iend = mPasses.end();
            i = mPasses.begin();

            IlluminationStage iStage = IS_AMBIENT;

            bool haveAmbient = false;
            while (i != iend)
            {
                IlluminationPass* iPass;
                Pass* p = *i;
                switch(iStage)
                {
                case IS_AMBIENT:
                    // Keep looking for ambient only
                    if (p->isAmbientOnly())
                    {
                        // Add this pass wholesale
                        iPass = new IlluminationPass();
                        iPass->destroyOnShutdown = false;
                        iPass->originalPass = iPass->pass = p;
                        iPass->stage = iStage;
                        mIlluminationPasses.push_back(iPass);
                        haveAmbient = true;
                        // progress to next pass
                        ++i;
                    }
                    else
                    {
                        // Split off any ambient part
                        if (p->getAmbient() != ColourValue::Black ||
                            p->getSelfIllumination() != ColourValue::Black ||
                            p->getAlphaRejectFunction() != CMPF_ALWAYS_PASS)
                        {
                            // Copy existing pass
                            Pass* newPass = new Pass(this, p->getIndex(), *p);
                            if (newPass->getAlphaRejectFunction() != CMPF_ALWAYS_PASS)
                            {
                                // Alpha rejection passes must retain their transparency, so
                                // we allow the texture units, but override the colour functions
                                for (auto tus : newPass->getTextureUnitStates())
                                {
                                    tus->setColourOperationEx(LBX_SOURCE1, LBS_CURRENT);
                                }
                            }
                            else
                            {
                                // Remove any texture units
                                newPass->removeAllTextureUnitStates();
                            }
                            // Remove any fragment program
                            if (newPass->hasFragmentProgram())
                                newPass->setFragmentProgram("");
                            // We have to leave vertex program alone (if any) and
                            // just trust that the author is using light bindings, which
                            // we will ensure there are none in the ambient pass
                            newPass->setDiffuse(0, 0, 0, newPass->getDiffuse().a);  // Preserving alpha
                            newPass->setSpecular(ColourValue::Black);

                            // Calculate hash value for new pass, because we are compiling
                            // illumination passes on demand, which will loss hash calculate
                            // before it add to render queue first time.
                            newPass->_recalculateHash();

                            iPass = new IlluminationPass();
                            iPass->destroyOnShutdown = true;
                            iPass->originalPass = p;
                            iPass->pass = newPass;
                            iPass->stage = iStage;

                            mIlluminationPasses.push_back(iPass);
                            haveAmbient = true;

                        }

                        if (!haveAmbient)
                        {
                            // Make up a new basic pass
                            Pass* newPass = new Pass(this, p->getIndex());
                            newPass->setAmbient(ColourValue::Black);
                            newPass->setDiffuse(ColourValue::Black);

                            // Calculate hash value for new pass, because we are compiling
                            // illumination passes on demand, which will loss hash calculate
                            // before it add to render queue first time.
                            newPass->_recalculateHash();

                            iPass = new IlluminationPass();
                            iPass->destroyOnShutdown = true;
                            iPass->originalPass = p;
                            iPass->pass = newPass;
                            iPass->stage = iStage;
                            mIlluminationPasses.push_back(iPass);
                            haveAmbient = true;
                        }
                        // This means we're done with ambients, progress to per-light
                        iStage = IS_PER_LIGHT;
                    }
                    break;
                case IS_PER_LIGHT:
                    if (p->getIteratePerLight())
                    {
                        // If this is per-light already, use it directly
                        iPass = new IlluminationPass();
                        iPass->destroyOnShutdown = false;
                        iPass->originalPass = iPass->pass = p;
                        iPass->stage = iStage;
                        mIlluminationPasses.push_back(iPass);
                        // progress to next pass
                        ++i;
                    }
                    else
                    {
                        // Split off per-light details (can only be done for one)
                        if (p->getLightingEnabled() &&
                            (p->getDiffuse() != ColourValue::Black ||
                            p->getSpecular() != ColourValue::Black))
                        {
                            // Copy existing pass
                            Pass* newPass = new Pass(this, p->getIndex(), *p);
                            if (newPass->getAlphaRejectFunction() != CMPF_ALWAYS_PASS)
                            {
                                // Alpha rejection passes must retain their transparency, so
                                // we allow the texture units, but override the colour functions
                                for (auto tus : newPass->getTextureUnitStates())
                                {
                                    tus->setColourOperationEx(LBX_SOURCE1, LBS_CURRENT);
                                }
                            }
                            else
                            {
                                // remove texture units
                                newPass->removeAllTextureUnitStates();
                            }
                            // remove fragment programs
                            if (newPass->hasFragmentProgram())
                                newPass->setFragmentProgram("");
                            // Cannot remove vertex program, have to assume that
                            // it will process diffuse lights, ambient will be turned off
                            newPass->setAmbient(ColourValue::Black);
                            newPass->setSelfIllumination(ColourValue::Black);
                            // must be additive
                            newPass->setSceneBlending(SBF_ONE, SBF_ONE);

                            // Calculate hash value for new pass, because we are compiling
                            // illumination passes on demand, which will loss hash calculate
                            // before it add to render queue first time.
                            newPass->_recalculateHash();

                            iPass = new IlluminationPass();
                            iPass->destroyOnShutdown = true;
                            iPass->originalPass = p;
                            iPass->pass = newPass;
                            iPass->stage = iStage;

                            mIlluminationPasses.push_back(iPass);

                        }
                        // This means the end of per-light passes
                        iStage = IS_DECAL;
                    }
                    break;
                case IS_DECAL:
                    // We just want a 'lighting off' pass to finish off
                    // and only if there are texture units
                    if (p->getNumTextureUnitStates() > 0)
                    {
                        if (!p->getLightingEnabled())
                        {
                            // we assume this pass already combines as required with the scene
                            iPass = new IlluminationPass();
                            iPass->destroyOnShutdown = false;
                            iPass->originalPass = iPass->pass = p;
                            iPass->stage = iStage;
                            mIlluminationPasses.push_back(iPass);
                        }
                        else
                        {
                            // Copy the pass and tweak away the lighting parts
                            Pass* newPass = new Pass(this, p->getIndex(), *p);
                            newPass->setAmbient(ColourValue::Black);
                            newPass->setDiffuse(0, 0, 0, newPass->getDiffuse().a);  // Preserving alpha
                            newPass->setSpecular(ColourValue::Black);
                            newPass->setSelfIllumination(ColourValue::Black);
                            newPass->setLightingEnabled(false);
                            newPass->setIteratePerLight(false, false);
                            // modulate
                            newPass->setSceneBlending(SBF_DEST_COLOUR, SBF_ZERO);

                            // Calculate hash value for new pass, because we are compiling
                            // illumination passes on demand, which will loss hash calculate
                            // before it add to render queue first time.
                            newPass->_recalculateHash();

                            // NB there is nothing we can do about vertex & fragment
                            // programs here, so people will just have to make their
                            // programs friendly-like if they want to use this technique
                            iPass = new IlluminationPass();
                            iPass->destroyOnShutdown = true;
                            iPass->originalPass = p;
                            iPass->pass = newPass;
                            iPass->stage = iStage;
                            mIlluminationPasses.push_back(iPass);

                        }
                    }
                    ++i; // always increment on decal, since nothing more to do with this pass

                    break;
                case IS_UNKNOWN:
                    break;
                }
            }
        }

    }
    //-----------------------------------------------------------------------
    void Technique::clearIlluminationPasses()
    {
        if(MaterialManager::getSingletonPtr())
            MaterialManager::getSingleton()._notifyBeforeIlluminationPassesCleared(this);

        for (auto & mIlluminationPasse : mIlluminationPasses)
        {
            if (mIlluminationPasse->destroyOnShutdown)
            {
                mIlluminationPasse->pass->queueForDeletion();
            }
            delete mIlluminationPasse;
        }
        mIlluminationPasses.clear();
    }
    //-----------------------------------------------------------------------
    auto
    Technique::getIlluminationPasses() noexcept -> const IlluminationPassList&
    {
        IlluminationPassesState targetState = IPS_COMPILED;
        if(mIlluminationPassesCompilationPhase != targetState
        && mIlluminationPassesCompilationPhase != IPS_COMPILE_DISABLED)
        {
            // prevents parent->_notifyNeedsRecompile() call during compile
            mIlluminationPassesCompilationPhase = IPS_COMPILE_DISABLED;
            // Splitting the passes into illumination passes
            _compileIlluminationPasses();
            // Post notification, so that technique owner can post-process created passes
            if(MaterialManager::getSingletonPtr())
                MaterialManager::getSingleton()._notifyAfterIlluminationPassesCreated(this);
            // Mark that illumination passes compilation finished
            mIlluminationPassesCompilationPhase = targetState;
        }

        return mIlluminationPasses;
    }
    //-----------------------------------------------------------------------
    auto Technique::getResourceGroup() const noexcept -> StringView
    {
        return mParent->getGroup();
    }
    //-----------------------------------------------------------------------
    auto  Technique::getShadowCasterMaterial() const -> Ogre::MaterialPtr 
    { 
        return mShadowCasterMaterial; 
    }
    //-----------------------------------------------------------------------
    void  Technique::setShadowCasterMaterial(Ogre::MaterialPtr val) 
    { 
        if (!val)
        {
            mShadowCasterMaterial.reset();
            mShadowCasterMaterialName.clear();
        }
        else
        {
            // shadow caster material should never receive shadows
            val->setReceiveShadows(false); // should we warn if this is not set?
            mShadowCasterMaterial = val; 
            mShadowCasterMaterialName = val->getName();
        }
    }
    //-----------------------------------------------------------------------
    void  Technique::setShadowCasterMaterial(StringView name) 
    { 
        setShadowCasterMaterial(MaterialManager::getSingleton().getByName(name));
    }
    //-----------------------------------------------------------------------
    auto  Technique::getShadowReceiverMaterial() const -> Ogre::MaterialPtr 
    { 
        return mShadowReceiverMaterial; 
    }
    //-----------------------------------------------------------------------
    void  Technique::setShadowReceiverMaterial(Ogre::MaterialPtr val) 
    { 
        if (!val)
        {
            mShadowReceiverMaterial.reset();
            mShadowReceiverMaterialName.clear();
        }
        else
        {
            mShadowReceiverMaterial = val; 
            mShadowReceiverMaterialName = val->getName();
        }
    }
    //-----------------------------------------------------------------------
    void  Technique::setShadowReceiverMaterial(StringView name)  
    { 
        mShadowReceiverMaterialName = name;
        mShadowReceiverMaterial = MaterialManager::getSingleton().getByName(name);
    }
    //---------------------------------------------------------------------
    void Technique::addGPUVendorRule(GPUVendor vendor, Technique::IncludeOrExclude includeOrExclude)
    {
        addGPUVendorRule(GPUVendorRule(vendor, includeOrExclude));
    }
    //---------------------------------------------------------------------
    void Technique::addGPUVendorRule(const Technique::GPUVendorRule& rule)
    {
        // remove duplicates
        removeGPUVendorRule(rule.vendor);
        mGPUVendorRules.push_back(rule);
    }
    //---------------------------------------------------------------------
    void Technique::removeGPUVendorRule(GPUVendor vendor)
    {
        for (auto i = mGPUVendorRules.begin(); i != mGPUVendorRules.end(); )
        {
            if (i->vendor == vendor)
                i = mGPUVendorRules.erase(i);
            else
                ++i;
        }
    }
    //---------------------------------------------------------------------
    void Technique::addGPUDeviceNameRule(StringView devicePattern, 
        Technique::IncludeOrExclude includeOrExclude, bool caseSensitive)
    {
        addGPUDeviceNameRule(GPUDeviceNameRule(devicePattern, includeOrExclude, caseSensitive));
    }
    //---------------------------------------------------------------------
    void Technique::addGPUDeviceNameRule(const Technique::GPUDeviceNameRule& rule)
    {
        // remove duplicates
        removeGPUDeviceNameRule(rule.devicePattern);
        mGPUDeviceNameRules.push_back(rule);
    }
    //---------------------------------------------------------------------
    void Technique::removeGPUDeviceNameRule(StringView devicePattern)
    {
        for (auto i = mGPUDeviceNameRules.begin(); i != mGPUDeviceNameRules.end(); )
        {
            if (i->devicePattern == devicePattern)
                i = mGPUDeviceNameRules.erase(i);
            else
                ++i;
        }
    }
}
