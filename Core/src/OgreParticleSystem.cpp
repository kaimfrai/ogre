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
#include <span>
#include <utility>

#include "OgreCamera.hpp"
#include "OgreCommon.hpp"
#include "OgreController.hpp"
#include "OgreControllerManager.hpp"
#include "OgreException.hpp"
#include "OgreLogManager.hpp"
#include "OgreMaterial.hpp"
#include "OgreMaterialManager.hpp"
#include "OgreMath.hpp"
#include "OgreMatrix4.hpp"
#include "OgreNode.hpp"
#include "OgreParticle.hpp"
#include "OgreParticleAffector.hpp"
#include "OgreParticleAffectorFactory.hpp"
#include "OgreParticleEmitter.hpp"
#include "OgreParticleSystem.hpp"
#include "OgreParticleSystemManager.hpp"
#include "OgreParticleSystemRenderer.hpp"
#include "OgreRadixSort.hpp"
#include "OgreRoot.hpp"
#include "OgreSceneManager.hpp"
#include "OgreStringConverter.hpp"

namespace Ogre {
class RenderQueue;

    /** Command object for quota (see ParamCommand).*/
    class CmdQuota : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /** Command object for emittedEmitterQuota (see ParamCommand).*/
    class CmdEmittedEmitterQuota : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /** Command object for material (see ParamCommand).*/
    class CmdMaterial : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /** Command object for cull_each (see ParamCommand).*/
    class CmdCull : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /** Command object for particle_width (see ParamCommand).*/
    class CmdWidth : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /** Command object for particle_height (see ParamCommand).*/
    class CmdHeight : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /** Command object for renderer (see ParamCommand).*/
    class CmdRenderer : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /** Command object for sorting (see ParamCommand).*/
    class CmdSorted : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /** Command object for local space (see ParamCommand).*/
    class CmdLocalSpace : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /** Command object for iteration interval(see ParamCommand).*/
    class CmdIterationInterval : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /** Command object for nonvisible timeout (see ParamCommand).*/
    class CmdNonvisibleTimeout : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, const String& val) override;
    };
    /// Command objects
    static CmdCull msCullCmd;
    static CmdHeight msHeightCmd;
    static CmdMaterial msMaterialCmd;
    static CmdQuota msQuotaCmd;
    static CmdEmittedEmitterQuota msEmittedEmitterQuotaCmd;
    static CmdWidth msWidthCmd;
    static CmdRenderer msRendererCmd;
    static CmdSorted msSortedCmd;
    static CmdLocalSpace msLocalSpaceCmd;
    static CmdIterationInterval msIterationIntervalCmd;
    static CmdNonvisibleTimeout msNonvisibleTimeoutCmd;

    Real ParticleSystem::msDefaultIterationInterval = 0;
    Real ParticleSystem::msDefaultNonvisibleTimeout = 0;

    //-----------------------------------------------------------------------
    // Local class for updating based on time
    class ParticleSystemUpdateValue : public ControllerValue<Real>
    {
    protected:
        ParticleSystem* mTarget;
    public:
        ParticleSystemUpdateValue(ParticleSystem* target) : mTarget(target) {}

        [[nodiscard]] auto getValue() const noexcept -> Real override { return 0; } // N/A

        void setValue(Real value) override { mTarget->_update(value); }

    };
    //-----------------------------------------------------------------------
    ParticleSystem::ParticleSystem() 
      : mAABB(),
        mBoundingRadius(1.0f),
        mBoundsAutoUpdate(true),
        mBoundsUpdateTime(10.0f),
        mUpdateRemainTime(0),
        mResourceGroupName(ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME),
        mIsRendererConfigured(false),
        mSpeedFactor(1.0f),
        mIterationInterval(0),
        mIterationIntervalSet(false),
        mSorted(false),
        mLocalSpace(false),
        mNonvisibleTimeout(0),
        mNonvisibleTimeoutSet(false),
        mTimeSinceLastVisible(0),
        mLastVisibleFrame(0),
        mTimeController(nullptr),
        mEmittedEmitterPoolInitialised(false),
        mIsEmitting(true),
        mRenderer(nullptr),
        mCullIndividual(false),
        mPoolSize(0),
        mEmittedEmitterPoolSize(0)
    {
        initParameters();

        // Default to billboard renderer
        setRenderer("billboard");
        mCastShadows = false;
    }
    //-----------------------------------------------------------------------
    ParticleSystem::ParticleSystem(const String& name, String  resourceGroup)
      : MovableObject(name),
        mAABB(),
        mBoundingRadius(1.0f),
        mBoundsAutoUpdate(true),
        mBoundsUpdateTime(10.0f),
        mUpdateRemainTime(0),
        mResourceGroupName(std::move(resourceGroup)),
        mIsRendererConfigured(false),
        mSpeedFactor(1.0f),
        mIterationInterval(0),
        mIterationIntervalSet(false),
        mSorted(false),
        mLocalSpace(false),
        mNonvisibleTimeout(0),
        mNonvisibleTimeoutSet(false),
        mTimeSinceLastVisible(0),
        mLastVisibleFrame(Root::getSingleton().getNextFrameNumber()),
        mTimeController(nullptr),
        mEmittedEmitterPoolInitialised(false),
        mIsEmitting(true),
        mRenderer(nullptr), 
        mCullIndividual(false),
        mPoolSize(0),
        mEmittedEmitterPoolSize(0)
    {
        setDefaultDimensions( 100, 100 );
        mMaterial = MaterialManager::getSingleton().getDefaultMaterial();
        // Default to 10 particles, expect app to specify (will only be increased, not decreased)
        setParticleQuota( 10 );
        setEmittedEmitterQuota( 3 );
        initParameters();

        // Default to billboard renderer
        setRenderer("billboard");
        mCastShadows = false;
    }
    //-----------------------------------------------------------------------
    ParticleSystem::~ParticleSystem()
    {
        if (mTimeController)
        {
            // Destroy controller
            ControllerManager::getSingleton().destroyController(mTimeController);
            mTimeController = nullptr;
        }

        // Arrange for the deletion of emitters & affectors
        removeAllEmitters();
        removeAllEmittedEmitters();
        removeAllAffectors();

        if (mRenderer)
        {
            ParticleSystemManager::getSingleton()._destroyRenderer(mRenderer);
            mRenderer = nullptr;
        }

    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::addEmitter(const String& emitterType) -> ParticleEmitter*
    {
        ParticleEmitter* em = 
            ParticleSystemManager::getSingleton()._createEmitter(emitterType, this);
        mEmitters.push_back(em);
        return em;
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getEmitter(unsigned short index) const -> ParticleEmitter*
    {
        assert(index < mEmitters.size() && "Emitter index out of bounds!");
        return mEmitters[index];
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getNumEmitters() const noexcept -> unsigned short
    {
        return static_cast< unsigned short >( mEmitters.size() );
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::removeEmitter(unsigned short index)
    {
        assert(index < mEmitters.size() && "Emitter index out of bounds!");
        auto ei = mEmitters.begin() + index;
        ParticleSystemManager::getSingleton()._destroyEmitter(*ei);
        mEmitters.erase(ei);
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::removeEmitter(ParticleEmitter* emitter)
    {
        auto ei = std::ranges::find(mEmitters, emitter);
        OgreAssert(ei != mEmitters.end(), "Emitter is not a part of ParticleSystem!");
        ParticleSystemManager::getSingleton()._destroyEmitter(*ei);
        mEmitters.erase(ei);
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::removeAllEmitters()
    {
        // DON'T delete directly, we don't know what heap these have been created on
        for (auto e : mEmitters)
        {
            ParticleSystemManager::getSingleton()._destroyEmitter(e);
        }
        mEmitters.clear();
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::addAffector(const String& affectorType) -> ParticleAffector*
    {
        ParticleAffector* af = 
            ParticleSystemManager::getSingleton()._createAffector(affectorType, this);
        mAffectors.push_back(af);
        return af;
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getAffector(unsigned short index) const -> ParticleAffector*
    {
        assert(index < mAffectors.size() && "Affector index out of bounds!");
        return mAffectors[index];
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getNumAffectors() const noexcept -> unsigned short
    {
        return static_cast< unsigned short >( mAffectors.size() );
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::removeAffector(unsigned short index)
    {
        assert(index < mAffectors.size() && "Affector index out of bounds!");
        auto ai = mAffectors.begin() + index;
        ParticleSystemManager::getSingleton()._destroyAffector(*ai);
        mAffectors.erase(ai);
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::removeAllAffectors()
    {
        // DON'T delete directly, we don't know what heap these have been created on
        for (auto a : mAffectors)
        {
            ParticleSystemManager::getSingleton()._destroyAffector(a);
        }
        mAffectors.clear();
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::operator=(const ParticleSystem& rhs) -> ParticleSystem&
    {
        // Blank this system's emitters & affectors
        removeAllEmitters();
        removeAllEmittedEmitters();
        removeAllAffectors();

        // Copy emitters
        for(unsigned short i = 0; i < rhs.getNumEmitters(); ++i)
        {
            ParticleEmitter* rhsEm = rhs.getEmitter(i);
            ParticleEmitter* newEm = addEmitter(rhsEm->getType());
            rhsEm->copyParametersTo(newEm);
        }
        // Copy affectors
        for(unsigned short i = 0; i < rhs.getNumAffectors(); ++i)
        {
            ParticleAffector* rhsAf = rhs.getAffector(i);
            ParticleAffector* newAf = addAffector(rhsAf->getType());
            rhsAf->copyParametersTo(newAf);
        }
        setParticleQuota(rhs.getParticleQuota());
        setEmittedEmitterQuota(rhs.getEmittedEmitterQuota());
        setMaterialName(rhs.getMaterialName());
        setDefaultDimensions(rhs.mDefaultWidth, rhs.mDefaultHeight);
        mCullIndividual = rhs.mCullIndividual;
        mSorted = rhs.mSorted;
        mLocalSpace = rhs.mLocalSpace;
        mIterationInterval = rhs.mIterationInterval;
        mIterationIntervalSet = rhs.mIterationIntervalSet;
        mNonvisibleTimeout = rhs.mNonvisibleTimeout;
        mNonvisibleTimeoutSet = rhs.mNonvisibleTimeoutSet;
        // last frame visible and time since last visible should be left default

        setRenderer(rhs.getRendererName());
        // Copy settings
        if (mRenderer && rhs.getRenderer())
        {
            rhs.getRenderer()->copyParametersTo(mRenderer);
        }

        return *this;

    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getNumParticles() const -> size_t
    {
        return mActiveParticles.size();
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getParticleQuota() const -> size_t
    {
        return mPoolSize;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setParticleQuota(size_t size)
    {
        // Never shrink below size()
        size_t currSize = mParticlePool.size();

        if( currSize < size )
        {
            // Will allocate particles on demand
            mPoolSize = size;
            
        }
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getEmittedEmitterQuota() const -> size_t
    {
        return mEmittedEmitterPoolSize;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setEmittedEmitterQuota(size_t size)
    {
        // Never shrink below size()
        size_t currSize = 0;
        for (auto& kv : mEmittedEmitterPool)
        {
            currSize += kv.second.size();
        }

        if( currSize < size )
        {
            // Will allocate emitted emitters on demand
            mEmittedEmitterPoolSize = size;
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setNonVisibleUpdateTimeout(Real timeout)
    {
        mNonvisibleTimeout = timeout;
        mNonvisibleTimeoutSet = true;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setIterationInterval(Real interval)
    {
        mIterationInterval = interval;
        mIterationIntervalSet = true;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_update(Real timeElapsed)
    {
        // Only update if attached to a node
        if (!mParentNode)
            return;

        Real nonvisibleTimeout = mNonvisibleTimeoutSet ?
            mNonvisibleTimeout : msDefaultNonvisibleTimeout;

        if (nonvisibleTimeout > 0)
        {
            // Check whether it's been more than one frame (update is ahead of
            // camera notification by one frame because of the ordering)
            long frameDiff = Root::getSingleton().getNextFrameNumber() - mLastVisibleFrame;
            if (frameDiff > 1 || frameDiff < 0) // < 0 if wrap only
            {
                mTimeSinceLastVisible += timeElapsed;
                if (mTimeSinceLastVisible >= nonvisibleTimeout)
                {
                    // No update
                    return;
                }
            }
        }

        // Scale incoming speed for the rest of the calculation
        timeElapsed *= mSpeedFactor;

        // Init renderer if not done already
        configureRenderer();

        // Initialise emitted emitters list if not done already
        initialiseEmittedEmitters();

        Real iterationInterval = mIterationIntervalSet ? 
            mIterationInterval : msDefaultIterationInterval;
        if (iterationInterval > 0)
        {
            mUpdateRemainTime += timeElapsed;

            while (mUpdateRemainTime >= iterationInterval)
            {
                // Update existing particles
                _expire(iterationInterval);
                _triggerAffectors(iterationInterval);
                _applyMotion(iterationInterval);

                if(mIsEmitting)
                {
                    // Emit new particles
                    _triggerEmitters(iterationInterval);
                }

                mUpdateRemainTime -= iterationInterval;
            }
        }
        else
        {
            // Update existing particles
            _expire(timeElapsed);
            _triggerAffectors(timeElapsed);
            _applyMotion(timeElapsed);

            if(mIsEmitting)
            {
                // Emit new particles
                _triggerEmitters(timeElapsed);
            }
        }

        if (!mBoundsAutoUpdate && mBoundsUpdateTime > 0.0f)
            mBoundsUpdateTime -= timeElapsed; // count down 
        _updateBounds();

    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_expire(Real timeElapsed)
    {
        Particle* pParticle;
        ParticleEmitter* pParticleEmitter;

        auto iend = mActiveParticles.end();
        for (auto i = mActiveParticles.begin(); i != iend;)
        {
            pParticle = static_cast<Particle*>(*i);
            if (pParticle->mTimeToLive < timeElapsed)
            {
                // Notify renderer
                mRenderer->_notifyParticleExpired(pParticle);

                // Identify the particle type
                if (pParticle->mParticleType == Particle::Visual)
                {
                    // add back to free list
                    mFreeParticles.push_back(pParticle);
                }
                else
                {
                    // For now, it can only be an emitted emitter
                    pParticleEmitter = static_cast<ParticleEmitter*>(*i);
                    std::list<ParticleEmitter*>* fee = findFreeEmittedEmitter(pParticleEmitter->getName());
                    fee->push_back(pParticleEmitter);

                    // Also erase from mActiveEmittedEmitters
                    removeFromActiveEmittedEmitters (pParticleEmitter);
                }

                // And remove from mActiveParticles
                *i = std::move(*(--iend));
            }
            else
            {
                // Decrement TTL
                pParticle->mTimeToLive -= timeElapsed;
                ++i;
            }
        }

        mActiveParticles.erase(iend, mActiveParticles.end());
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_triggerEmitters(Real timeElapsed)
    {
        // Add up requests for emission
        static std::vector<unsigned> requested;
        static std::vector<unsigned> emittedRequested;

        if( requested.size() != mEmitters.size() )
            requested.resize( mEmitters.size() );
        if( emittedRequested.size() != mEmittedEmitterPoolSize)
            emittedRequested.resize( mEmittedEmitterPoolSize );

        size_t totalRequested, emitterCount, emittedEmitterCount, emissionAllowed;

        emitterCount = mEmitters.size();
        emittedEmitterCount=mActiveEmittedEmitters.size();
        emissionAllowed = mFreeParticles.size();
        totalRequested = 0;

        // Count up total requested emissions for regular emitters (and exclude the ones that are used as
        // a template for emitted emitters)
        for (size_t i = 0;
            auto const& itEmit : mEmitters)
        {
            if (!itEmit->isEmitted())
            {
                requested[i] = itEmit->_getEmissionCount(timeElapsed);
                totalRequested += requested[i];
            }
            ++i;
        }

        // Add up total requested emissions for (active) emitted emitters
        for (size_t  i=0;
            auto const& itActiveEmit : mActiveEmittedEmitters)
        {
            emittedRequested[i] = itActiveEmit->_getEmissionCount(timeElapsed);
            totalRequested += emittedRequested[i];
            ++i;
        }

        // Check if the quota will be exceeded, if so reduce demand
        Real ratio =  1.0f;
        if (totalRequested > emissionAllowed)
        {
            // Apportion down requested values to allotted values
            ratio =  (Real)emissionAllowed / (Real)totalRequested;
            for (size_t i = 0; i < emitterCount; ++i)
            {
                requested[i] = static_cast<unsigned>(requested[i] * ratio);
            }
            for (size_t i = 0; i < emittedEmitterCount; ++i)
            {
                emittedRequested[i] = static_cast<unsigned>(emittedRequested[i] * ratio);
            }
        }

        // Emit
        // For each emission, apply a subset of the motion for the frame
        // this ensures an even distribution of particles when many are
        // emitted in a single frame
        for (size_t i = 0;
            auto const& itEmit : mEmitters)
        {
            // Trigger the emitters, but exclude the emitters that are already in the emitted emitters list; 
            // they are handled in a separate loop
            if (!itEmit->isEmitted())
                _executeTriggerEmitters (itEmit, static_cast<unsigned>(requested[i]), timeElapsed);
            ++i;
        }

        // Do the same with all active emitted emitters
        for (size_t i = 0;
             auto const& itActiveEmit : mActiveEmittedEmitters)
        {
            _executeTriggerEmitters (itActiveEmit, emittedRequested[i], timeElapsed);
            ++i;
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_executeTriggerEmitters(ParticleEmitter* emitter, unsigned requested, Real timeElapsed)
    {
        Real timePoint = 0.0f;


        // avoid any divide by zero conditions
        if(!requested) 
            return;

        Real timeInc = timeElapsed / requested;

        for (unsigned int j = 0; j < requested; ++j)
        {
            // Create a new particle & init using emitter
            // The particle is a visual particle if the emit_emitter property of the emitter isn't set 
            Particle* p = nullptr;
            String  emitterName = emitter->getEmittedEmitter();
            if (emitterName.empty())
                p = createParticle();
            else
                p = createEmitterParticle(emitterName);

            // Only continue if the particle was really created (not null)
            if (!p)
                return;

            emitter->_initParticle(p);

            // Translate position & direction into world space
            if (!mLocalSpace)
            {
                p->mPosition = mParentNode->convertLocalToWorldPosition(p->mPosition);
                p->mDirection = mParentNode->convertLocalToWorldDirection(p->mDirection, false);
            }

            // apply partial frame motion to this particle
            p->mPosition += (p->mDirection * timePoint);

            // apply particle initialization by the affectors
            for (auto a : mAffectors)
                a->_initParticle(p);

            // Increment time fragment
            timePoint += timeInc;

            // Notify renderer
            mRenderer->_notifyParticleEmitted(p);
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_applyMotion(Real timeElapsed)
    {
        for (auto pParticle : mActiveParticles)
        {
            pParticle->mPosition += (pParticle->mDirection * timeElapsed);
        }

        // Notify renderer
        mRenderer->_notifyParticleMoved(mActiveParticles);
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_triggerAffectors(Real timeElapsed)
    {
        for (auto a : mAffectors)
        {
            a->_affectParticles(this, timeElapsed);
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::increasePool(size_t size)
    {
        size_t oldSize = mParticlePool.size();

        // Increase size
        mParticlePool.resize(size);

        // Create new particles
        for( size_t i = oldSize; i < size; i++ )
        {
            mParticlePool[i] = ::std::make_unique<Particle>();
        }
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getParticle(size_t index) -> Particle* 
    {
        assert (index < mActiveParticles.size() && "Index out of bounds!");
        return mActiveParticles[index];
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::createParticle() -> Particle*
    {
        Particle* p = nullptr;
        if (!mFreeParticles.empty())
        {
            // Fast creation (don't use superclass since emitter will init)
            p = mFreeParticles.back();
            mActiveParticles.push_back(p);
            mFreeParticles.pop_back();
        }

        return p;

    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::createEmitterParticle(const String& emitterName) -> Particle*
    {
        // Get the appropriate list and retrieve an emitter 
        ParticleEmitter* p = nullptr;
        std::list<ParticleEmitter*>* fee = findFreeEmittedEmitter(emitterName);
        if (fee && !fee->empty())
        {
            p = fee->front();
            p->mParticleType = Particle::Emitter;
            fee->pop_front();
            mActiveParticles.push_back(p);

            // Also add to mActiveEmittedEmitters. This is needed to traverse through all active emitters
            // that are emitted. Don't use mActiveParticles for that (although they are added to
            // mActiveParticles also), because it would take too long to traverse.
            mActiveEmittedEmitters.push_back(p);
        }

        return p;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_updateRenderQueue(RenderQueue* queue)
    {
        if (mRenderer)
        {
            mRenderer->_updateRenderQueue(queue, mActiveParticles, mCullIndividual);
        }
    }
    //---------------------------------------------------------------------
    void ParticleSystem::visitRenderables(Renderable::Visitor* visitor, 
        bool debugRenderables)
    {
        if (mRenderer)
        {
            mRenderer->_notifyCastShadows(mCastShadows);
            mRenderer->visitRenderables(visitor, debugRenderables);
        }
    }
    //---------------------------------------------------------------------
    void ParticleSystem::initParameters()
    {
        if (createParamDictionary("ParticleSystem"))
        {
            ParamDictionary* dict = getParamDictionary();

            dict->addParameter(ParameterDef("quota", 
                "The maximum number of particles allowed at once in this system.",
                PT_UNSIGNED_INT),
                &msQuotaCmd);

            dict->addParameter(ParameterDef("emit_emitter_quota", 
                "The maximum number of emitters to be emitted at once in this system.",
                PT_UNSIGNED_INT),
                &msEmittedEmitterQuotaCmd);

            dict->addParameter(ParameterDef("material", 
                "The name of the material to be used to render all particles in this system.",
                PT_STRING),
                &msMaterialCmd);

            dict->addParameter(ParameterDef("particle_width", 
                "The width of particles in world units.",
                PT_REAL),
                &msWidthCmd);

            dict->addParameter(ParameterDef("particle_height", 
                "The height of particles in world units.",
                PT_REAL),
                &msHeightCmd);

            dict->addParameter(ParameterDef("cull_each", 
                "If true, each particle is culled in it's own right. If false, the entire system is culled as a whole.",
                PT_BOOL),
                &msCullCmd);

            dict->addParameter(ParameterDef("renderer", 
                "Sets the particle system renderer to use (default 'billboard').",
                PT_STRING),
                &msRendererCmd);

            dict->addParameter(ParameterDef("sorted", 
                "Sets whether particles should be sorted relative to the camera. ",
                PT_BOOL),
                &msSortedCmd);

            dict->addParameter(ParameterDef("local_space", 
                "Sets whether particles should be kept in local space rather than "
                "emitted into world space. ",
                PT_BOOL),
                &msLocalSpaceCmd);

            dict->addParameter(ParameterDef("iteration_interval", 
                "Sets a fixed update interval for the system, or 0 for the frame rate. ",
                PT_REAL),
                &msIterationIntervalCmd);

            dict->addParameter(ParameterDef("nonvisible_update_timeout", 
                "Sets a timeout on updates to the system if the system is not visible "
                "for the given number of seconds (0 to always update)",
                PT_REAL),
                &msNonvisibleTimeoutCmd);

        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_updateBounds()
    {

        if (mParentNode && (mBoundsAutoUpdate || mBoundsUpdateTime > 0.0f))
        {
            if (mActiveParticles.empty())
            {
                // No particles, reset to null if auto update bounds
                if (mBoundsAutoUpdate)
                {
                    mWorldAABB.setNull();
                }
            }
            else
            {
                Vector3 min;
                Vector3 max;
                if (!mBoundsAutoUpdate && mWorldAABB.isFinite())
                {
                    // We're on a limit, grow rather than reset each time
                    // so that we pick up the worst case scenario
                    min = mWorldAABB.getMinimum();
                    max = mWorldAABB.getMaximum();
                }
                else
                {
                    min.x = min.y = min.z = Math::POS_INFINITY;
                    max.x = max.y = max.z = Math::NEG_INFINITY;
                }
                Vector3 halfScale = Vector3::UNIT_SCALE * 0.5;
                for (auto p : mActiveParticles)
                {
                    Vector3 padding = halfScale * std::max(p->mWidth, p->mHeight);
                    min.makeFloor(p->mPosition - padding);
                    max.makeCeil(p->mPosition + padding);
                }
                mWorldAABB.setExtents(min, max);
            }


            if (mLocalSpace)
            {
                if (mBoundsAutoUpdate)
                    mAABB = mWorldAABB;
                else
                    // Merge calculated box with current AABB to preserve any user-set AABB
                    mAABB.merge(mWorldAABB);
            }
            else
            {
                // We've already put particles in world space to decouple them from the
                // node transform, so reverse transform back since we're expected to 
                // provide a local AABB
                AxisAlignedBox newAABB(mWorldAABB);
                newAABB.transform(mParentNode->_getFullTransform().inverse());

                if (mBoundsAutoUpdate)
                    mAABB = newAABB;
                else
                    // Merge calculated box with current AABB to preserve any user-set AABB
                    mAABB.merge(newAABB);
            }

            mParentNode->needUpdate();

            if (mRenderer)
                mRenderer->_notifyBoundingBox(mAABB);
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::fastForward(Real time, Real interval)
    {
        // First make sure all transforms are up to date
        auto steps = size_t(time/interval + 0.5f); // integer round
        for (size_t i = 0; i < steps; i++)
        {
            _update(interval);
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setEmitting(bool v)
    {
        mIsEmitting = v;
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getEmitting() const noexcept -> bool
    {
        return mIsEmitting;
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getMovableType() const noexcept -> const String&
    {
        return ParticleSystemFactory::FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setDefaultDimensions( Real width, Real height )
    {
        assert(width >= 0 && height >= 0 && "Particle dimensions can not be negative");
        mDefaultWidth = width;
        mDefaultHeight = height;
        if (mRenderer)
        {
            mRenderer->_notifyDefaultDimensions(width, height);
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setDefaultWidth(Real width)
    {
        assert(width >= 0 && "Particle dimensions can not be negative");
        mDefaultWidth = width;
        if (mRenderer)
        {
            mRenderer->_notifyDefaultDimensions(mDefaultWidth, mDefaultHeight);
        }
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getDefaultWidth() const -> Real
    {
        return mDefaultWidth;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setDefaultHeight(Real height)
    {
        assert(height >= 0 && "Particle dimensions can not be negative");
        mDefaultHeight = height;
        if (mRenderer)
        {
            mRenderer->_notifyDefaultDimensions(mDefaultWidth, mDefaultHeight);
        }
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getDefaultHeight() const -> Real
    {
        return mDefaultHeight;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_notifyCurrentCamera(Camera* cam)
    {
        MovableObject::_notifyCurrentCamera(cam);

        // Record visible
        if (isVisible())
        {           
            mLastVisibleFrame = Root::getSingleton().getNextFrameNumber();
            mTimeSinceLastVisible = 0.0f;

            if (mSorted)
            {
                _sortParticles(cam);
            }

            if (mRenderer)
            {
                if (!mIsRendererConfigured)
                    configureRenderer();

                mRenderer->_notifyCurrentCamera(cam);
            }
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_notifyAttached(Node* parent, bool isTagPoint)
    {
        MovableObject::_notifyAttached(parent, isTagPoint);
        if (mRenderer && mIsRendererConfigured)
        {
            mRenderer->_notifyAttached(parent, isTagPoint);
        }

        if (parent && !mTimeController)
        {
            // Assume visible
            mTimeSinceLastVisible = 0;
            mLastVisibleFrame = Root::getSingleton().getNextFrameNumber();

            // Create time controller when attached
            ControllerManager& mgr = ControllerManager::getSingleton(); 
            ControllerValueRealPtr updValue(new ParticleSystemUpdateValue(this));
            mTimeController = mgr.createFrameTimePassthroughController(updValue);
        }
        else if (!parent && mTimeController)
        {
            // Destroy controller
            ControllerManager::getSingleton().destroyController(mTimeController);
            mTimeController = nullptr;
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setMaterialName( const String& name, const String& groupName /* = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME */)
    {
        mMaterial = MaterialManager::getSingleton().getByName(name, groupName);
        if (!mMaterial)
        {
            LogManager::getSingleton().logError(
                ::std::format("Can't assign material {} to ParticleSystem {} because this "
                "Material does not exist in group {}. Have you forgotten to define it in a "
                ".material script?", name, mName, groupName));
            mMaterial = MaterialManager::getSingleton().getDefaultMaterial(false);
        }
        if (mIsRendererConfigured)
        {
            mMaterial->load();
            mRenderer->_setMaterial(mMaterial);
        }
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getMaterialName() const noexcept -> const String&
    {
        return mMaterial->getName();
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::clear()
    {
        // Notify renderer if exists
        if (mRenderer)
        {
            mRenderer->_notifyParticleCleared(mActiveParticles);
        }

        // reset active and free lists
        mActiveParticles.clear();
        mFreeParticles.clear();

        ::std::ranges::transform
        (   mParticlePool,
            ::std::inserter(mFreeParticles, mFreeParticles.end()),
            ::std::mem_fn(&::std::unique_ptr<Particle>::get)
        );

        // Add active emitted emitters to free list
        addActiveEmittedEmittersToFreeList();

        // Remove all active emitted emitter instances
        mActiveEmittedEmitters.clear();

        // Reset update remain time
        mUpdateRemainTime = 0;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setRenderer(const String& rendererName)
    {
        if (mRenderer)
        {
            // Destroy existing
            ParticleSystemManager::getSingleton()._destroyRenderer(mRenderer);
            mRenderer = nullptr;
        }

        if (!rendererName.empty())
        {
            mRenderer = ParticleSystemManager::getSingleton()._createRenderer(rendererName);
            mIsRendererConfigured = false;
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::configureRenderer()
    {
        // Actual allocate particles
        size_t currSize = mParticlePool.size();
        size_t size = mPoolSize;
        if( currSize < size )
        {
            this->increasePool(size);

            // Add new items to the queue
            ::std::ranges::transform
            (   std::span{mParticlePool.begin() + currSize, mParticlePool.end()},
                ::std::inserter(mFreeParticles, mFreeParticles.end()),
                ::std::mem_fn(&::std::unique_ptr<Particle>::get)
            );

            // Tell the renderer, if already configured
            if (mRenderer && mIsRendererConfigured)
            {
                mRenderer->_notifyParticleQuota(size);
            }
        }

        if (mRenderer && !mIsRendererConfigured)
        {
            mRenderer->_notifyParticleQuota(mParticlePool.size());
            mRenderer->_notifyAttached(mParentNode, mParentIsTagPoint);
            mRenderer->_notifyDefaultDimensions(mDefaultWidth, mDefaultHeight);
            mMaterial->load();
            mRenderer->_setMaterial(mMaterial);
            if (mRenderQueueIDSet)
                mRenderer->setRenderQueueGroup(mRenderQueueID);
            mRenderer->setKeepParticlesInLocalSpace(mLocalSpace);
            mIsRendererConfigured = true;
        }
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getRenderer() const noexcept -> ParticleSystemRenderer*
    {
        return mRenderer;
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getRendererName() const noexcept -> const String&
    {
        if (mRenderer)
        {
            return mRenderer->getType();
        }
        else
        {
            return BLANKSTRING;
        }
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getCullIndividually() const noexcept -> bool
    {
        return mCullIndividual;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setCullIndividually(bool cullIndividual)
    {
        mCullIndividual = cullIndividual;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setBounds(const AxisAlignedBox& aabb)
    {
        mAABB = aabb;
        mBoundingRadius = Math::boundingRadiusFromAABB(mAABB);

    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setBoundsAutoUpdated(bool autoUpdate, Real stopIn)
    {
        mBoundsAutoUpdate = autoUpdate;
        mBoundsUpdateTime = stopIn;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setRenderQueueGroup(uint8 queueID)
    {
        MovableObject::setRenderQueueGroup(queueID);
        if (mRenderer)
        {
            mRenderer->setRenderQueueGroup(queueID);
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setRenderQueueGroupAndPriority(uint8 queueID, ushort priority)
    {
        MovableObject::setRenderQueueGroupAndPriority(queueID, priority);
        if (mRenderer)
        {
            mRenderer->setRenderQueueGroupAndPriority(queueID, priority);
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::setKeepParticlesInLocalSpace(bool keepLocal)
    {
        mLocalSpace = keepLocal;
        if (mRenderer)
        {
            mRenderer->setKeepParticlesInLocalSpace(keepLocal);
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_sortParticles(Camera* cam)
    {
        static RadixSort<ParticlePool, Particle*, float> mRadixSorter;
        if (mRenderer)
        {
            SortMode sortMode =
                cam->getSortMode() == SM_DIRECTION ? SM_DIRECTION : mRenderer->_getSortMode();

            if (sortMode == SM_DIRECTION)
            {
                Vector3 camDir = cam->getDerivedDirection();
                if (mLocalSpace)
                {
                    // transform the camera direction into local space
                    camDir = mParentNode->convertWorldToLocalDirection(camDir, false);
                }
                mRadixSorter.sort(mActiveParticles, SortByDirectionFunctor(- camDir));
            }
            else if (sortMode == SM_DISTANCE)
            {
                Vector3 camPos = cam->getDerivedPosition();
                if (mLocalSpace)
                {
                    // transform the camera position into local space
                    camPos = mParentNode->convertWorldToLocalPosition(camPos);
                }
                mRadixSorter.sort(mActiveParticles, SortByDistanceFunctor(camPos));
            }
        }
    }
    ParticleSystem::SortByDirectionFunctor::SortByDirectionFunctor(const Vector3& dir)
        : sortDir(dir)
    {
    }
    auto ParticleSystem::SortByDirectionFunctor::operator()(Particle* p) const -> float
    {
        return sortDir.dotProduct(p->mPosition);
    }
    ParticleSystem::SortByDistanceFunctor::SortByDistanceFunctor(const Vector3& pos)
        : sortPos(pos)
    {
    }
    auto ParticleSystem::SortByDistanceFunctor::operator()(Particle* p) const -> float
    {
        // Sort descending by squared distance
        return - (sortPos - p->mPosition).squaredLength();
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::getTypeFlags() const noexcept -> uint32
    {
        return SceneManager::FX_TYPE_MASK;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::initialiseEmittedEmitters()
    {
        // Initialise the pool if needed
        size_t currSize = 0;
        if (mEmittedEmitterPool.empty())
        {
            if (mEmittedEmitterPoolInitialised)
            {
                // It was already initialised, but apparently no emitted emitters were used
                return;
            }
            else
            {
                initialiseEmittedEmitterPool();
            }
        }
        else
        {
            for (auto& kv : mEmittedEmitterPool)
            {
                currSize += kv.second.size();
            }
        }

        size_t size = mEmittedEmitterPoolSize;
        if( currSize < size && !mEmittedEmitterPool.empty())
        {
            // Increase the pool. Equally distribute over all vectors in the map
            increaseEmittedEmitterPool(size);
            
            // Add new items to the free list
            addFreeEmittedEmitters();
        }
    }

    //-----------------------------------------------------------------------
    void ParticleSystem::initialiseEmittedEmitterPool()
    {
        if (mEmittedEmitterPoolInitialised)
            return;

        // Run through mEmitters and add keys to the pool
        for (ParticleEmitter* emitter : mEmitters)
        {
            // Determine the names of all emitters that are emitted
            if (!emitter->getEmittedEmitter().empty())
            {
                // This one will be emitted, register its name and leave the vector empty!
                mEmittedEmitterPool[emitter->getEmittedEmitter()];
            }
        }

        // Determine whether the emitter itself will be emitted and set the 'mEmitted' attribute
        for (ParticleEmitter* emitter : mEmitters)
        {
            if (mEmittedEmitterPool.find(emitter->getName()) != mEmittedEmitterPool.end())
            {
                emitter->setEmitted(true);
            }
        }

        mEmittedEmitterPoolInitialised = true;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::increaseEmittedEmitterPool(size_t size)
    {
        // Don't proceed if the pool doesn't contain any keys of emitted emitters
        if (mEmittedEmitterPool.empty())
            return;

        ParticleEmitter* clonedEmitter = nullptr;
        size_t maxNumberOfEmitters = size / mEmittedEmitterPool.size(); // equally distribute the number for each emitted emitter list
    
        // Run through mEmittedEmitterPool and search for every key (=name) its corresponding emitter in mEmitters
        for (auto& kv : mEmittedEmitterPool)
        {
            const auto& name = kv.first;
            auto& e = kv.second;

            // Search the correct emitter in the mEmitters vector
            for (ParticleEmitter* emitter : mEmitters)
            {
                if (name == emitter->getName())
                {
                    // Found the right emitter, clone each emitter a number of times
                    size_t oldSize = e.size();
                    for (size_t t = oldSize; t < maxNumberOfEmitters; ++t)
                    {
                        clonedEmitter = ParticleSystemManager::getSingleton()._createEmitter(emitter->getType(), this);
                        emitter->copyParametersTo(clonedEmitter);
                        clonedEmitter->setEmitted(emitter->isEmitted()); // is always 'true' by the way, but just in case

                        // Initially deactivate the emitted emitter if duration/repeat_delay are set
                        if (clonedEmitter->getDuration() != 0.0f && clonedEmitter->getRepeatDelay() > 0.0f)
                            clonedEmitter->setEnabled(false);

                        // Add cloned emitters to the pool
                        e.push_back(clonedEmitter);
                    }
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::addFreeEmittedEmitters()
    {
        // Don't proceed if the EmittedEmitterPool is empty
        if (mEmittedEmitterPool.empty())
            return;

        // Copy all pooled emitters to the free list
        std::list<ParticleEmitter*>* fee = nullptr;

        // Run through the emittedEmitterPool map
        for (auto& kv : mEmittedEmitterPool)
        {
            const auto& name = kv.first;
            auto& emittedEmitters = kv.second;
            fee = findFreeEmittedEmitter(name);

            // If its not in the map, create an empty one
            if (!fee)
            {
                mFreeEmittedEmitters[name];
                fee = findFreeEmittedEmitter(name);
            }

            // Check anyway if its ok now
            if (!fee)
                return; // forget it!

            // Add all emitted emitters from the pool to the free list
            for(ParticleEmitter* emitter : emittedEmitters)
            {
                fee->push_back(emitter);
            }
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::removeAllEmittedEmitters()
    {
        for (auto& kv : mEmittedEmitterPool)
        {
            for (ParticleEmitter* emitter : kv.second)
            {
                ParticleSystemManager::getSingleton()._destroyEmitter(emitter);
            }
            kv.second.clear();
        }

        // Don't leave any references behind
        mEmittedEmitterPool.clear();
        mFreeEmittedEmitters.clear();
        mActiveEmittedEmitters.clear();
    }
    //-----------------------------------------------------------------------
    auto ParticleSystem::findFreeEmittedEmitter (const String& name) -> std::list<ParticleEmitter*>*
    {
        FreeEmittedEmitterMap::iterator it;
        it = mFreeEmittedEmitters.find (name);
        if (it != mFreeEmittedEmitters.end())
        {
            // Found it
            return &it->second;
        }

        return nullptr;
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::removeFromActiveEmittedEmitters (ParticleEmitter* emitter)
    {
        assert(emitter && "Emitter to be removed is 0!");
        for (auto itActiveEmit = mActiveEmittedEmitters.begin(); itActiveEmit != mActiveEmittedEmitters.end(); ++itActiveEmit)
        {
            if (emitter == (*itActiveEmit))
            {
                mActiveEmittedEmitters.erase(itActiveEmit);
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::addActiveEmittedEmittersToFreeList ()
    {
        for (auto & mActiveEmittedEmitter : mActiveEmittedEmitters)
        {
            std::list<ParticleEmitter*>* fee = findFreeEmittedEmitter (mActiveEmittedEmitter->getName());
            if (fee)
                fee->push_back(mActiveEmittedEmitter);
        }
    }
    //-----------------------------------------------------------------------
    void ParticleSystem::_notifyReorganiseEmittedEmitterData ()
    {
        removeAllEmittedEmitters();
        mEmittedEmitterPoolInitialised = false; // Don't rearrange immediately; it will be performed in the regular flow
    }
    //-----------------------------------------------------------------------
    auto CmdCull::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const ParticleSystem*>(target)->getCullIndividually() );
    }
    void CmdCull::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setCullIndividually(
            StringConverter::parseBool(val));
    }
    //-----------------------------------------------------------------------
    auto CmdHeight::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const ParticleSystem*>(target)->getDefaultHeight() );
    }
    void CmdHeight::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setDefaultHeight(
            StringConverter::parseReal(val));
    }
    //-----------------------------------------------------------------------
    auto CmdWidth::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const ParticleSystem*>(target)->getDefaultWidth() );
    }
    void CmdWidth::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setDefaultWidth(
            StringConverter::parseReal(val));
    }
    //-----------------------------------------------------------------------
    auto CmdMaterial::doGet(const void* target) const -> String
    {
        return static_cast<const ParticleSystem*>(target)->getMaterialName();
    }
    void CmdMaterial::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setMaterialName(val);
    }
    //-----------------------------------------------------------------------
    auto CmdQuota::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const ParticleSystem*>(target)->getParticleQuota() );
    }
    void CmdQuota::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setParticleQuota(
            StringConverter::parseUnsignedInt(val));
    }
    //-----------------------------------------------------------------------
    auto CmdEmittedEmitterQuota::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const ParticleSystem*>(target)->getEmittedEmitterQuota() );
    }
    void CmdEmittedEmitterQuota::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setEmittedEmitterQuota(
            StringConverter::parseUnsignedInt(val));
    }
    //-----------------------------------------------------------------------
    auto CmdRenderer::doGet(const void* target) const -> String
    {
        return static_cast<const ParticleSystem*>(target)->getRendererName();
    }
    void CmdRenderer::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setRenderer(val);
    }
    //-----------------------------------------------------------------------
    auto CmdSorted::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const ParticleSystem*>(target)->getSortingEnabled());
    }
    void CmdSorted::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setSortingEnabled(
            StringConverter::parseBool(val));
    }
    //-----------------------------------------------------------------------
    auto CmdLocalSpace::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const ParticleSystem*>(target)->getKeepParticlesInLocalSpace());
    }
    void CmdLocalSpace::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setKeepParticlesInLocalSpace(
            StringConverter::parseBool(val));
    }
    //-----------------------------------------------------------------------
    auto CmdIterationInterval::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const ParticleSystem*>(target)->getIterationInterval());
    }
    void CmdIterationInterval::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setIterationInterval(
            StringConverter::parseReal(val));
    }
    //-----------------------------------------------------------------------
    auto CmdNonvisibleTimeout::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const ParticleSystem*>(target)->getNonVisibleUpdateTimeout());
    }
    void CmdNonvisibleTimeout::doSet(void* target, const String& val)
    {
        static_cast<ParticleSystem*>(target)->setNonVisibleUpdateTimeout(
            StringConverter::parseReal(val));
    }
   //-----------------------------------------------------------------------
    ParticleAffector::~ParticleAffector() 
    = default;
    //-----------------------------------------------------------------------
    ParticleAffectorFactory::~ParticleAffectorFactory() 
    {
        // Destroy all affectors
        for (auto & mAffector : mAffectors)
        {
            delete mAffector;
        }
            
        mAffectors.clear();

    }
    //-----------------------------------------------------------------------
    void ParticleAffectorFactory::destroyAffector(ParticleAffector* e)
    {
        for (auto i = mAffectors.begin(); i != mAffectors.end(); ++i)
        {
            if ((*i) == e)
            {
                mAffectors.erase(i);
                delete e;
                break;
            }
        }
    }

}
