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
#include <cstddef>

module Ogre.Core;

import :BlendMode;
import :ColourValue;
import :Common;
import :Config;
import :Exception;
import :GpuProgram;
import :GpuProgramParams;
import :GpuProgramUsage;
import :Light;
import :Material;
import :Pass;
import :Platform;
import :Prerequisites;
import :SharedPtr;
import :String;
import :Technique;
import :TextureUnitState;
import :Vector;

import <algorithm>;
import <iterator>;
import <memory>;
import <set>;
import <string>;
import <vector>;

namespace Ogre {

    /** Default pass hash function.
    @remarks
        Tries to minimise the number of texture changes.
    */
    struct MinTextureStateChangeHashFunc : public Pass::HashFunc
    {
        auto operator()(const Pass* p) const -> uint32 override
        {
            uint32 hash = 0;
            ushort c = p->getNumTextureUnitStates();

            for (ushort i = 0; i < c; ++i)
            {
                const TextureUnitState* tus = nullptr;
                tus = p->getTextureUnitState(i);
                hash = FastHash(tus->getTextureName().data(), tus->getTextureName().size(), hash);
            }

            return hash;
        }
    };
    MinTextureStateChangeHashFunc sMinTextureStateChangeHashFunc;
    /** Alternate pass hash function.
    @remarks
        Tries to minimise the number of GPU program changes.
    */
    struct MinGpuProgramChangeHashFunc : public Pass::HashFunc
    {
        auto operator()(const Pass* p) const -> uint32 override
        {
            uint32 hash = 0;

            for(int i = 0; i < std::to_underlying(GpuProgramType::COUNT); i++)
            {
                std::string_view name = p->getGpuProgramName(GpuProgramType(i));
                if(!name.empty()) {
                    hash = FastHash(name.data(), name.size(), hash);
                }
            }

            return hash;
        }
    };
    MinGpuProgramChangeHashFunc sMinGpuProgramChangeHashFunc;
    //-----------------------------------------------------------------------------
    Pass::PassSet Pass::msDirtyHashList;
    Pass::PassSet Pass::msPassGraveyard;

    Pass::HashFunc* Pass::msHashFunc = &sMinGpuProgramChangeHashFunc;
    //-----------------------------------------------------------------------------
    auto Pass::getBuiltinHashFunction(BuiltinHashFunction builtin) -> Pass::HashFunc*
    {
        Pass::HashFunc* hashFunc = nullptr;

        switch(builtin)
        {
        case MIN_TEXTURE_CHANGE:
            hashFunc = &sMinTextureStateChangeHashFunc;
            break;
        case MIN_GPU_PROGRAM_CHANGE:
            hashFunc = &sMinGpuProgramChangeHashFunc;
            break;
        }

        return hashFunc;
    }
    //-----------------------------------------------------------------------------
    void Pass::setHashFunction(BuiltinHashFunction builtin)
    {
        switch(builtin)
        {
        case MIN_TEXTURE_CHANGE:
            msHashFunc = &sMinTextureStateChangeHashFunc;
            break;
        case MIN_GPU_PROGRAM_CHANGE:
            msHashFunc = &sMinGpuProgramChangeHashFunc;
            break;
        }
    }
    //-----------------------------------------------------------------------------
    Pass::Pass(Technique* parent, unsigned short index)
        : mParent(parent)
        , mHash(0)
        , mAmbient(ColourValue::White)
        , mDiffuse(ColourValue::White)
        , mSpecular(ColourValue::Black)
        , mEmissive(ColourValue::Black)
        , mShininess(0)
        , mTracking(TrackVertexColourEnum::NONE)
        , mHashDirtyQueued(false)
        , mDepthCheck(true)
        , mDepthWrite(true)
        , mAlphaToCoverageEnabled(false)
        , mTransparentSorting(true)
        , mTransparentSortingForced(false)
        , mLightingEnabled(true)
        , mIteratePerLight(false)
        , mRunOnlyForOneLightType(false)
        , mNormaliseNormals(false)
        , mPolygonModeOverrideable(true)
        , mFogOverride(false)
        , mQueuedForDeletion(false)
        , mLightScissoring(false)
        , mLightClipPlanes(false)
        , mPointSpritesEnabled(false)
        , mPointAttenuationEnabled(false)
        , mContentTypeLookupBuilt(false)
        , mAlphaRejectVal(0)
        , mDepthBiasConstant(0.0f)
        , mDepthBiasSlopeScale(0.0f)
        , mDepthBiasPerIteration(0.0f)
        , mDepthFunc(CompareFunction::LESS_EQUAL)
        , mAlphaRejectFunc(CompareFunction::ALWAYS_PASS)
        , mCullMode(CullingMode::CLOCKWISE)
        , mManualCullMode(ManualCullingMode::BACK)
        , mMaxSimultaneousLights(OGRE_MAX_SIMULTANEOUS_LIGHTS)
        , mStartLight(0)
        , mLightsPerIteration(1)
        , mIndex(index)
        , mLightMask{0xFFFFFFFF}
        , mFogColour(ColourValue::White)
        , mFogStart(0.0)
        , mFogEnd(1.0)
        , mFogDensity(0.001)
        , mLineWidth(1.0f)
        , mPassIterationCount(1)
        , mPointMinSize(0.0f)
        , mPointMaxSize(0.0f)
        , mPointAttenution{1.0f, 1.0f, 0.0f, 0.0f}
        , mShadeOptions(ShadeOptions::GOURAUD)
        , mPolygonMode(PolygonMode::SOLID)
        , mIlluminationStage(IlluminationStage::UNKNOWN)
        , mOnlyLightType(Light::LightTypes::POINT)
        , mFogMode(FogMode::NONE)
    {
        // init the hash inline
        _recalculateHash();
   }

    //-----------------------------------------------------------------------------
    Pass::Pass(Technique *parent, unsigned short index, const Pass& oth)
        : mParent(parent), mQueuedForDeletion(false), mIndex(index), mPassIterationCount(1)
    {
        *this = oth;
        mParent = parent;
        mIndex = index;
        mQueuedForDeletion = false;

        // init the hash inline
        _recalculateHash();
    }
    Pass::~Pass() = default; // ensure unique_ptr destructors are in cpp
    //-----------------------------------------------------------------------------
    auto Pass::operator=(const Pass& oth) -> Pass&
    {
        mName = oth.mName;
        mHash = oth.mHash;
        mAmbient = oth.mAmbient;
        mDiffuse = oth.mDiffuse;
        mSpecular = oth.mSpecular;
        mEmissive = oth.mEmissive;
        mShininess = oth.mShininess;
        mTracking = oth.mTracking;

        // Copy fog parameters
        mFogOverride = oth.mFogOverride;
        mFogMode = oth.mFogMode;
        mFogColour = oth.mFogColour;
        mFogStart = oth.mFogStart;
        mFogEnd = oth.mFogEnd;
        mFogDensity = oth.mFogDensity;

        // Default blending (overwrite)
        mBlendState = oth.mBlendState;

        mDepthCheck = oth.mDepthCheck;
        mDepthWrite = oth.mDepthWrite;
        mAlphaRejectFunc = oth.mAlphaRejectFunc;
        mAlphaRejectVal = oth.mAlphaRejectVal;
        mAlphaToCoverageEnabled = oth.mAlphaToCoverageEnabled;
        mTransparentSorting = oth.mTransparentSorting;
        mTransparentSortingForced = oth.mTransparentSortingForced;
        mDepthFunc = oth.mDepthFunc;
        mDepthBiasConstant = oth.mDepthBiasConstant;
        mDepthBiasSlopeScale = oth.mDepthBiasSlopeScale;
        mDepthBiasPerIteration = oth.mDepthBiasPerIteration;
        mCullMode = oth.mCullMode;
        mManualCullMode = oth.mManualCullMode;
        mLightingEnabled = oth.mLightingEnabled;
        mMaxSimultaneousLights = oth.mMaxSimultaneousLights;
        mStartLight = oth.mStartLight;
        mIteratePerLight = oth.mIteratePerLight;
        mLightsPerIteration = oth.mLightsPerIteration;
        mRunOnlyForOneLightType = oth.mRunOnlyForOneLightType;
        mNormaliseNormals = oth.mNormaliseNormals;
        mOnlyLightType = oth.mOnlyLightType;
        mShadeOptions = oth.mShadeOptions;
        mPolygonMode = oth.mPolygonMode;
        mPolygonModeOverrideable = oth.mPolygonModeOverrideable;
        mPassIterationCount = oth.mPassIterationCount;
        mLineWidth = oth.mLineWidth;
        mPointAttenution = oth.mPointAttenution;
        mPointMinSize = oth.mPointMinSize;
        mPointMaxSize = oth.mPointMaxSize;
        mPointSpritesEnabled = oth.mPointSpritesEnabled;
        mPointAttenuationEnabled = oth.mPointAttenuationEnabled;
        mShadowContentTypeLookup = oth.mShadowContentTypeLookup;
        mContentTypeLookupBuilt = oth.mContentTypeLookupBuilt;
        mLightScissoring = oth.mLightScissoring;
        mLightClipPlanes = oth.mLightClipPlanes;
        mIlluminationStage = oth.mIlluminationStage;
        mLightMask = oth.mLightMask;

        for(int i = 0; i < std::to_underlying(GpuProgramType::COUNT); i++)
        {
            auto& programUsage = mProgramUsage[i];
            auto& othUsage = oth.mProgramUsage[i];
            if  (othUsage)
                programUsage = std::make_unique<GpuProgramUsage>(*othUsage, this);
            else
                programUsage.reset();
        }

        // Clear texture units but doesn't notify need recompilation in the case
        // we are cloning, The parent material will take care of this.
        for (auto & mTextureUnitState : mTextureUnitStates)
        {
            delete mTextureUnitState;
        }

        mTextureUnitStates.clear();

        // Copy texture units
        for (auto const& i : oth.mTextureUnitStates)
        {
            auto* t = new TextureUnitState(this, *i);
            mTextureUnitStates.push_back(t);
        }

        _dirtyHash();

        return *this;
    }
    //-----------------------------------------------------------------------------
    auto Pass::calculateSize() const -> size_t
    {
        size_t memSize = 0;

        // Tally up TU states
        for (auto mTextureUnitState : mTextureUnitStates)
        {
            memSize += mTextureUnitState->calculateSize();
        }
        for(const auto& u : mProgramUsage)
            memSize += u ? u->calculateSize() : 0;

        return memSize;
    }
    //-----------------------------------------------------------------------
    void Pass::setPointAttenuation(bool enabled, float constant, float linear, float quadratic)
    {
        mPointAttenuationEnabled = enabled;
        mPointAttenution[1] = enabled ? constant : 1.0f;
        mPointAttenution[2] = enabled ? linear : 0.0f;
        mPointAttenution[3] = enabled ? quadratic : 0.0f;
    }
    //-----------------------------------------------------------------------
    void Pass::setPointMinSize(Real min)
    {
        mPointMinSize = min;
    }
    //-----------------------------------------------------------------------
    auto Pass::getPointMinSize() const -> Real
    {
        return mPointMinSize;
    }
    //-----------------------------------------------------------------------
    void Pass::setPointMaxSize(Real max)
    {
        mPointMaxSize = max;
    }
    //-----------------------------------------------------------------------
    auto Pass::getPointMaxSize() const -> Real
    {
        return mPointMaxSize;
    }
    //-----------------------------------------------------------------------
    void Pass::setAmbient(float red, float green, float blue)
    {
        mAmbient.r = red;
        mAmbient.g = green;
        mAmbient.b = blue;

    }
    //-----------------------------------------------------------------------
    void Pass::setDiffuse(float red, float green, float blue, float alpha)
    {
        mDiffuse.r = red;
        mDiffuse.g = green;
        mDiffuse.b = blue;
        mDiffuse.a = alpha;
    }
    //-----------------------------------------------------------------------
    void Pass::setSpecular(float red, float green, float blue, float alpha)
    {
        mSpecular.r = red;
        mSpecular.g = green;
        mSpecular.b = blue;
        mSpecular.a = alpha;
    }
    //-----------------------------------------------------------------------
    void Pass::setSelfIllumination(float red, float green, float blue)
    {
        mEmissive.r = red;
        mEmissive.g = green;
        mEmissive.b = blue;
    }
    //-----------------------------------------------------------------------
    auto Pass::createTextureUnitState() -> TextureUnitState*
    {
        auto *t = new TextureUnitState(this);
        addTextureUnitState(t);
        mContentTypeLookupBuilt = false;
        return t;
    }
    //-----------------------------------------------------------------------
    auto Pass::createTextureUnitState(
        std::string_view textureName, unsigned short texCoordSet) -> TextureUnitState*
    {
        auto *t = new TextureUnitState(this);
        t->setTextureName(textureName);
        t->setTextureCoordSet(texCoordSet);
        addTextureUnitState(t);
        mContentTypeLookupBuilt = false;
        return t;
    }
    //-----------------------------------------------------------------------
    void Pass::addTextureUnitState(TextureUnitState* state)
    {
        OgreAssert(state , "TextureUnitState is NULL");

        // only attach TUS to pass if TUS does not belong to another pass
        OgreAssert(!state->getParent() || (state->getParent() == this), "TextureUnitState already attached to another pass");

        mTextureUnitStates.push_back(state);
        // Notify state
        state->_notifyParent(this);
        // if texture unit state name is empty then give it a default name based on its index
        if (state->getName().empty())
        {
            // its the last entry in the container so its index is size - 1
            size_t idx = mTextureUnitStates.size() - 1;

            // allow 8 digit hex number. there should never be that many texture units.
            // This sprintf replaced a call to StringConverter::toString for performance reasons
            state->setName( std::format("{:#08x}", static_cast<long>(idx)));
        }
        _notifyNeedsRecompile();
        _dirtyHash();

        mContentTypeLookupBuilt = false;
    }
    //-----------------------------------------------------------------------------
    auto Pass::getTextureUnitState(std::string_view name) const -> TextureUnitState*
    {
        TextureUnitState* foundTUS = nullptr;

        // iterate through TUS Container to find a match
        for (auto const& i : mTextureUnitStates)
        {
            if (i->getName() == name )
            {
                foundTUS = i;
                break;
            }
        }

        return foundTUS;
    }

    //-----------------------------------------------------------------------
    auto Pass::getTextureUnitStateIndex(const TextureUnitState* state) const -> unsigned short
    {
        assert(state && "state is 0 in Pass::getTextureUnitStateIndex()");

        // only find index for state attached to this pass
        OgreAssert(state->getParent() == this, "TextureUnitState is not attached to this pass");
        auto i = std::ranges::find(mTextureUnitStates, state);
        assert(i != mTextureUnitStates.end() && "state is supposed to attached to this pass");
        return static_cast<unsigned short>(std::distance(mTextureUnitStates.begin(), i));
    }

    //-----------------------------------------------------------------------
    void Pass::removeTextureUnitState(unsigned short index)
    {
        assert (index < mTextureUnitStates.size() && "Index out of bounds");

        auto i = mTextureUnitStates.begin() + index;
        delete *i;
        mTextureUnitStates.erase(i);
        _notifyNeedsRecompile();
        _dirtyHash();
        mContentTypeLookupBuilt = false;
    }
    //-----------------------------------------------------------------------
    void Pass::removeAllTextureUnitStates()
    {
        for (auto & mTextureUnitState : mTextureUnitStates)
        {
            delete mTextureUnitState;
        }
        mTextureUnitStates.clear();
        _notifyNeedsRecompile();
        _dirtyHash();
        mContentTypeLookupBuilt = false;
    }
    //-----------------------------------------------------------------------
    void Pass::_getBlendFlags(SceneBlendType type, SceneBlendFactor& source, SceneBlendFactor& dest)
    {
        using enum SceneBlendType;
        switch ( type )
        {
        case TRANSPARENT_ALPHA:
            source = SceneBlendFactor::SOURCE_ALPHA;
            dest = SceneBlendFactor::ONE_MINUS_SOURCE_ALPHA;
            return;
        case TRANSPARENT_COLOUR:
            source = SceneBlendFactor::SOURCE_COLOUR;
            dest = SceneBlendFactor::ONE_MINUS_SOURCE_COLOUR;
            return;
        case MODULATE:
            source = SceneBlendFactor::DEST_COLOUR;
            dest = SceneBlendFactor::ZERO;
            return;
        case ADD:
            source = SceneBlendFactor::ONE;
            dest = SceneBlendFactor::ONE;
            return;
        case REPLACE:
            source = SceneBlendFactor::ONE;
            dest = SceneBlendFactor::ZERO;
            return;
        }

        // Default to SceneBlendType::REPLACE

        source = SceneBlendFactor::ONE;
        dest = SceneBlendFactor::ZERO;
    }
    //-----------------------------------------------------------------------
    void Pass::setSceneBlending(SceneBlendType sbt)
    {
        // Convert type into blend factors

        SceneBlendFactor source;
        SceneBlendFactor dest;
        _getBlendFlags(sbt, source, dest);

        // Set blend factors

        setSceneBlending(source, dest);
    }
    //-----------------------------------------------------------------------
    void Pass::setSeparateSceneBlending( const SceneBlendType sbt, const SceneBlendType sbta )
    {
        // Convert types into blend factors

        SceneBlendFactor source;
        SceneBlendFactor dest;
        _getBlendFlags(sbt, source, dest);

        SceneBlendFactor sourceAlpha;
        SceneBlendFactor destAlpha;
        _getBlendFlags(sbta, sourceAlpha, destAlpha);

        // Set blend factors

        setSeparateSceneBlending(source, dest, sourceAlpha, destAlpha);
    }

    //-----------------------------------------------------------------------
    void Pass::setSceneBlending(SceneBlendFactor sourceFactor, SceneBlendFactor destFactor)
    {
        mBlendState.sourceFactor = sourceFactor;
        mBlendState.sourceFactorAlpha = sourceFactor;
        mBlendState.destFactor = destFactor;
        mBlendState.destFactorAlpha = destFactor;
    }
    //-----------------------------------------------------------------------
    void Pass::setSeparateSceneBlending( const SceneBlendFactor sourceFactor, const SceneBlendFactor destFactor, const SceneBlendFactor sourceFactorAlpha, const SceneBlendFactor destFactorAlpha )
    {
        mBlendState.sourceFactor = sourceFactor;
        mBlendState.destFactor = destFactor;
        mBlendState.sourceFactorAlpha = sourceFactorAlpha;
        mBlendState.destFactorAlpha = destFactorAlpha;
    }
    //-----------------------------------------------------------------------
    void Pass::setSceneBlendingOperation(SceneBlendOperation op)
    {
        mBlendState.operation = op;
        mBlendState.alphaOperation = op;
    }
    //-----------------------------------------------------------------------
    void Pass::setSeparateSceneBlendingOperation(SceneBlendOperation op, SceneBlendOperation alphaOp)
    {
        mBlendState.operation = op;
        mBlendState.alphaOperation = alphaOp;
    }
    //-----------------------------------------------------------------------
    auto Pass::isTransparent() const noexcept -> bool
    {
        // Transparent if any of the destination colour is taken into account
        if (mBlendState.destFactor == SceneBlendFactor::ZERO &&
            mBlendState.sourceFactor != SceneBlendFactor::DEST_COLOUR &&
            mBlendState.sourceFactor != SceneBlendFactor::ONE_MINUS_DEST_COLOUR &&
            mBlendState.sourceFactor != SceneBlendFactor::DEST_ALPHA &&
            mBlendState.sourceFactor != SceneBlendFactor::ONE_MINUS_DEST_ALPHA)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    //-----------------------------------------------------------------------
    void Pass::setAlphaRejectSettings(CompareFunction func, unsigned char value, bool alphaToCoverage)
    {
        mAlphaRejectFunc = func;
        mAlphaRejectVal = value;
        mAlphaToCoverageEnabled = alphaToCoverage;
    }
    //-----------------------------------------------------------------------
    void Pass::setColourWriteEnabled(bool enabled)
    {
        mBlendState.writeR = enabled;
        mBlendState.writeG = enabled;
        mBlendState.writeB = enabled;
        mBlendState.writeA = enabled;
    }
    //-----------------------------------------------------------------------
    auto Pass::getColourWriteEnabled() const noexcept -> bool
    {
        return mBlendState.writeR || mBlendState.writeG || mBlendState.writeB ||
               mBlendState.writeA;
    }
    //-----------------------------------------------------------------------

    void Pass::setColourWriteEnabled(bool red, bool green, bool blue, bool alpha)
    {
        mBlendState.writeR = red;
        mBlendState.writeG = green;
        mBlendState.writeB = blue;
        mBlendState.writeA = alpha;
    }
    //-----------------------------------------------------------------------
    void Pass::getColourWriteEnabled(bool& red, bool& green, bool& blue, bool& alpha) const
    {
        red = mBlendState.writeR;
        green = mBlendState.writeG;
        blue = mBlendState.writeB;
        alpha = mBlendState.writeA;
    }
    //-----------------------------------------------------------------------
    void Pass::setIteratePerLight(bool enabled,
            bool onlyForOneLightType, Light::LightTypes lightType)
    {
        mIteratePerLight = enabled;
        mRunOnlyForOneLightType = onlyForOneLightType;
        mOnlyLightType = lightType;
    }
    //-----------------------------------------------------------------------
    void Pass::setManualCullingMode(ManualCullingMode mode)
    {
        mManualCullMode = mode;
    }
    //-----------------------------------------------------------------------
    auto Pass::getManualCullingMode() const -> ManualCullingMode
    {
        return mManualCullMode;
    }
    //-----------------------------------------------------------------------
    void Pass::setFog(bool overrideScene, FogMode mode, const ColourValue& colour, float density, float start, float end)
    {
        mFogOverride = overrideScene;
        if (overrideScene)
        {
            mFogMode = mode;
            mFogColour = colour;
            mFogStart = start;
            mFogEnd = end;
            mFogDensity = density;
        }
    }
    //-----------------------------------------------------------------------
    void Pass::setDepthBias(float constantBias, float slopeScaleBias)
    {
       mDepthBiasConstant = constantBias;
       mDepthBiasSlopeScale = slopeScaleBias;
    }
    //-----------------------------------------------------------------------
    auto Pass::_split(unsigned short numUnits) -> Pass*
    {
        OgreAssert(
            !isProgrammable(),
            "Programmable passes cannot be automatically split, define a fallback technique instead");

        if (mTextureUnitStates.size() > numUnits)
        {
            size_t start = mTextureUnitStates.size() - numUnits;

            Pass* newPass = mParent->createPass();

            TextureUnitStates::iterator istart, i, iend;
            iend = mTextureUnitStates.end();
            i = istart = mTextureUnitStates.begin() + start;
            // Set the new pass to fallback using scene blend
            newPass->setSceneBlending(
                (*i)->getColourBlendFallbackSrc(), (*i)->getColourBlendFallbackDest());
            // Fixup the texture unit 0   of new pass   blending method   to replace
            // all colour and alpha   with texture without adjustment, because we
            // assume it's detail texture.
            (*i)->setColourOperationEx(LayerBlendOperationEx::SOURCE1,   LayerBlendSource::TEXTURE, LayerBlendSource::CURRENT);
            (*i)->setAlphaOperation(LayerBlendOperationEx::SOURCE1, LayerBlendSource::TEXTURE, LayerBlendSource::CURRENT);

            // Add all the other texture unit states
            for (; i != iend; ++i)
            {
                // detach from parent first
                (*i)->_notifyParent(nullptr);
                newPass->addTextureUnitState(*i);
            }
            // Now remove texture units from this Pass, we don't need to delete since they've
            // been transferred
            mTextureUnitStates.erase(istart, iend);
            _dirtyHash();
            mContentTypeLookupBuilt = false;
            return newPass;
        }
        return nullptr;
    }
    //-----------------------------------------------------------------------------
    void Pass::_notifyIndex(unsigned short index)
    {
        if (mIndex != index)
        {
            mIndex = index;
            _dirtyHash();
        }
    }
    //-----------------------------------------------------------------------
    void Pass::_prepare()
    {
        // We assume the Technique only calls this when the material is being
        // prepared

        // prepare each TextureUnitState
        for (auto & mTextureUnitState : mTextureUnitStates)
        {
            mTextureUnitState->_prepare();
        }

    }
    //-----------------------------------------------------------------------
    void Pass::_unprepare()
    {
        // unprepare each TextureUnitState
        for (auto & mTextureUnitState : mTextureUnitStates)
        {
            mTextureUnitState->_unprepare();
        }

    }
    //-----------------------------------------------------------------------
    void Pass::_load()
    {
        // We assume the Technique only calls this when the material is being
        // loaded

        // Load each TextureUnitState
        for (auto & mTextureUnitState : mTextureUnitStates)
        {
            mTextureUnitState->_load();
        }

        // Load programs
        for (const auto& u : mProgramUsage)
            if(u) u->_load();

        if (mHashDirtyQueued)
        {
            _dirtyHash();
        }

    }
    //-----------------------------------------------------------------------
    void Pass::_unload()
    {
        // Unload each TextureUnitState
        for (auto & mTextureUnitState : mTextureUnitStates)
        {
            mTextureUnitState->_unload();
        }

        // TODO Unload programs
    }
    //-----------------------------------------------------------------------
    void Pass::setVertexProgram(std::string_view name, bool resetParams)
    {
        setGpuProgram(GpuProgramType::VERTEX_PROGRAM, name, resetParams);
    }
    //-----------------------------------------------------------------------
    void Pass::setGpuProgramParameters(GpuProgramType type, const GpuProgramParametersSharedPtr& params)
    {
        const auto& programUsage = getProgramUsage(type);
        if (!programUsage)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                "This pass does not have this program type assigned!");
        }
        programUsage->setParameters(params);
    }
    void Pass::setVertexProgramParameters(GpuProgramParametersSharedPtr params)
    {
        setGpuProgramParameters(GpuProgramType::VERTEX_PROGRAM, params);
    }
    //-----------------------------------------------------------------------
    void Pass::setGpuProgram(GpuProgramType type, const GpuProgramPtr& program, bool resetParams)
    {
        std::unique_ptr<GpuProgramUsage>& programUsage = getProgramUsage(type);

        // Turn off fragment program if name blank
        if (!program)
        {
            programUsage.reset();
        }
        else
        {
            if (!programUsage)
            {
                programUsage = std::make_unique<GpuProgramUsage>(type, this);
            }
            programUsage->setProgram(program, resetParams);
        }
        // Needs recompilation
        _notifyNeedsRecompile();

        if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_GPU_PROGRAM_CHANGE ) )
        {
            _dirtyHash();
        }
    }

    void Pass::setGpuProgram(GpuProgramType type, std::string_view name, bool resetParams)
    {
        if (getGpuProgramName(type) == name)
            return;

        GpuProgramPtr program;
        if (!name.empty())
            program = GpuProgramUsage::_getProgramByName(name, getResourceGroup(), type);

        setGpuProgram(type, program, resetParams);
    }

    void Pass::setFragmentProgram(std::string_view name, bool resetParams)
    {
        setGpuProgram(GpuProgramType::FRAGMENT_PROGRAM, name, resetParams);
    }
    //-----------------------------------------------------------------------
    void Pass::setFragmentProgramParameters(GpuProgramParametersSharedPtr params)
    {
        setGpuProgramParameters(GpuProgramType::FRAGMENT_PROGRAM, params);
    }
    //-----------------------------------------------------------------------
    void Pass::setGeometryProgram(std::string_view name, bool resetParams)
    {
        setGpuProgram(GpuProgramType::GEOMETRY_PROGRAM, name, resetParams);
    }
    //-----------------------------------------------------------------------
    void Pass::setGeometryProgramParameters(GpuProgramParametersSharedPtr params)
    {
        setGpuProgramParameters(GpuProgramType::GEOMETRY_PROGRAM, params);
    }
    //-----------------------------------------------------------------------
    void Pass::setTessellationHullProgram(std::string_view name, bool resetParams)
    {
        setGpuProgram(GpuProgramType::HULL_PROGRAM, name, resetParams);
    }
    //-----------------------------------------------------------------------
    void Pass::setTessellationHullProgramParameters(GpuProgramParametersSharedPtr params)
    {
        setGpuProgramParameters(GpuProgramType::HULL_PROGRAM, params);
    }
    //-----------------------------------------------------------------------
    void Pass::setTessellationDomainProgram(std::string_view name, bool resetParams)
    {
        setGpuProgram(GpuProgramType::DOMAIN_PROGRAM, name, resetParams);
    }
    //-----------------------------------------------------------------------
    void Pass::setTessellationDomainProgramParameters(GpuProgramParametersSharedPtr params)
    {
        setGpuProgramParameters(GpuProgramType::DOMAIN_PROGRAM, params);
    }
    //-----------------------------------------------------------------------
    void Pass::setComputeProgram(std::string_view name, bool resetParams)
    {
        setGpuProgram(GpuProgramType::COMPUTE_PROGRAM, name, resetParams);
    }
    //-----------------------------------------------------------------------
    void Pass::setComputeProgramParameters(GpuProgramParametersSharedPtr params)
    {
        setGpuProgramParameters(GpuProgramType::COMPUTE_PROGRAM, params);
    }
    //-----------------------------------------------------------------------
    auto Pass::getGpuProgramParameters(GpuProgramType type) const -> const GpuProgramParametersSharedPtr&
    {
        const auto& programUsage = getProgramUsage(type);
        if (!programUsage)
        {
            OGRE_EXCEPT (ExceptionCodes::INVALIDPARAMS,
                "This pass does not have this program type assigned!");
        }
        return programUsage->getParameters();
    }

    auto Pass::getVertexProgramParameters() const -> GpuProgramParametersSharedPtr
    {
        return getGpuProgramParameters(GpuProgramType::VERTEX_PROGRAM);
    }

    auto Pass::getProgramUsage(GpuProgramType programType) -> std::unique_ptr<GpuProgramUsage>& {
        return mProgramUsage[std::to_underlying(programType)];
    }

    auto Pass::getProgramUsage(GpuProgramType programType) const -> const std::unique_ptr<GpuProgramUsage>&
    {
        return mProgramUsage[std::to_underlying(programType)];
    }

    auto Pass::hasGpuProgram(GpuProgramType programType) const -> bool {
        return getProgramUsage(programType) != nullptr;
    }
    auto Pass::getGpuProgram(GpuProgramType programType) const -> const GpuProgramPtr&
	{
        OgreAssert(mProgramUsage[std::to_underlying(programType)], "check whether program is available using hasGpuProgram()");
        return mProgramUsage[std::to_underlying(programType)]->getProgram();
	}
    //-----------------------------------------------------------------------
    auto Pass::getGpuProgramName(GpuProgramType type) const -> std::string_view
    {
        const std::unique_ptr<GpuProgramUsage>& programUsage = getProgramUsage(type);
        if (!programUsage)
            return BLANKSTRING;
        else
            return programUsage->getProgramName();
    }
    //-----------------------------------------------------------------------
    auto Pass::getFragmentProgramParameters() const -> GpuProgramParametersSharedPtr
    {
        return getGpuProgramParameters(GpuProgramType::FRAGMENT_PROGRAM);
    }
    //-----------------------------------------------------------------------
    auto Pass::getGeometryProgramParameters() const -> GpuProgramParametersSharedPtr
    {
        return getGpuProgramParameters(GpuProgramType::GEOMETRY_PROGRAM);
    }
    //-----------------------------------------------------------------------
    auto Pass::getTessellationHullProgramParameters() const -> GpuProgramParametersSharedPtr
    {
        return getGpuProgramParameters(GpuProgramType::HULL_PROGRAM);
    }
    //-----------------------------------------------------------------------
    auto Pass::getTessellationDomainProgramParameters() const -> GpuProgramParametersSharedPtr
    {
        return getGpuProgramParameters(GpuProgramType::DOMAIN_PROGRAM);
    }
    //-----------------------------------------------------------------------
    auto Pass::getComputeProgramParameters() const -> GpuProgramParametersSharedPtr
    {
        return getGpuProgramParameters(GpuProgramType::COMPUTE_PROGRAM);
    }
    //-----------------------------------------------------------------------
    auto Pass::isLoaded() const noexcept -> bool
    {
        return mParent->isLoaded();
    }
    //-----------------------------------------------------------------------
    void Pass::_recalculateHash()
    {
        /* Hash format is 32-bit, divided as follows (high to low bits)
           bits   purpose
            4     Pass index (i.e. max 16 passes!)
           28     Pass contents
       */
        mHash = (*msHashFunc)(this);

        // overwrite the 4 upper bits with pass index
        mHash = (uint32(mIndex) << 28) | (mHash >> 4);
    }
    //-----------------------------------------------------------------------
    void Pass::_dirtyHash()
    {
        if (mQueuedForDeletion)
            return;

        Material* mat = mParent->getParent();
        if (mat->isLoading() || mat->isLoaded())
        {
            // Mark this hash as for follow up
            msDirtyHashList.insert(this);
            mHashDirtyQueued = false;
        }
        else
        {
            mHashDirtyQueued = true;
        }
    }
    //---------------------------------------------------------------------
    void Pass::clearDirtyHashList() 
    { 
        msDirtyHashList.clear(); 
    }
    //-----------------------------------------------------------------------
    void Pass::_notifyNeedsRecompile()
    {
        if (!mQueuedForDeletion)
            mParent->_notifyNeedsRecompile();
    }
    //-----------------------------------------------------------------------
    void Pass::setTextureFiltering(TextureFilterOptions filterType)
    {
        for (auto & mTextureUnitState : mTextureUnitStates)
        {
            mTextureUnitState->setTextureFiltering(filterType);
        }
    }
    // --------------------------------------------------------------------
    void Pass::setTextureAnisotropy(unsigned int maxAniso)
    {
        for (auto & mTextureUnitState : mTextureUnitStates)
        {
            mTextureUnitState->setTextureAnisotropy(maxAniso);
        }
    }
    //-----------------------------------------------------------------------
    void Pass::_updateAutoParams(const AutoParamDataSource* source, GpuParamVariability mask) const
    {
        for(int i = 0; i < std::to_underlying(GpuProgramType::COUNT); i++)
        {
            const auto& programUsage = getProgramUsage(GpuProgramType(i));
            if (programUsage)
            {
                // Update program auto params
                programUsage->getParameters()->_updateAutoParams(source, mask);
            }
        }
    }
    //-----------------------------------------------------------------------
    void Pass::processPendingPassUpdates()
    {

        // Delete items in the graveyard
        for (auto i : msPassGraveyard)
        {
            delete i;
        }
        msPassGraveyard.clear();

        PassSet tempDirtyHashList;

        // The dirty ones will have been removed from the groups above using the old hash now
        tempDirtyHashList.swap(msDirtyHashList);

        for (auto p : tempDirtyHashList)
        {
            p->_recalculateHash();
        }
    }
    //-----------------------------------------------------------------------
    void Pass::queueForDeletion()
    {
        mQueuedForDeletion = true;

        removeAllTextureUnitStates();
        for (auto& u : mProgramUsage)
            u.reset();

        // remove from dirty list, if there
        msDirtyHashList.erase(this);

        msPassGraveyard.insert(this);
    }
    //-----------------------------------------------------------------------
    auto Pass::isAmbientOnly() const noexcept -> bool
    {
        // treat as ambient if lighting is off, or colour write is off,
        // or all non-ambient (& emissive) colours are black
        // NB a vertex program could override this, but passes using vertex
        // programs are expected to indicate they are ambient only by
        // setting the state so it matches one of the conditions above, even
        // though this state is not used in rendering.
        return (!mLightingEnabled || !getColourWriteEnabled() ||
            (mDiffuse == ColourValue::Black &&
             mSpecular == ColourValue::Black));
    }
    //-----------------------------------------------------------------------
    auto Pass::getResourceGroup() const noexcept -> std::string_view
    {
        return mParent->getResourceGroup();
    }
    //-----------------------------------------------------------------------
    auto Pass::_getTextureUnitWithContentTypeIndex(
        TextureUnitState::ContentType contentType, unsigned short index) const -> unsigned short
    {
        if (!mContentTypeLookupBuilt)
        {
            mShadowContentTypeLookup.clear();
            for (unsigned short i = 0; i < mTextureUnitStates.size(); ++i)
            {
                if (mTextureUnitStates[i]->getContentType() == TextureUnitState::ContentType::SHADOW)
                {
                    mShadowContentTypeLookup.push_back(i);
                }
            }
            mContentTypeLookupBuilt = true;
        }

        switch(contentType)
        {
        using enum TextureUnitState::ContentType;
        case SHADOW:
            if (index < mShadowContentTypeLookup.size())
            {
                return mShadowContentTypeLookup[index];
            }
            break;
        default:
            // Simple iteration
            for (unsigned short i = 0; i < mTextureUnitStates.size(); ++i)
            {
                if (mTextureUnitStates[i]->getContentType() == TextureUnitState::ContentType::SHADOW)
                {
                    if (index == 0)
                    {
                        return i;
                    }
                    else
                    {
                        --index;
                    }
                }
            }
            break;
        }

        // not found - return out of range
        return static_cast<unsigned short>(mTextureUnitStates.size() + 1);

    }
}
