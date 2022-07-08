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
#include <cassert>
#include <cstddef>
#include <map>
#include <string>
#include <utility>

#include "OgreBillboardParticleRenderer.hpp"
#include "OgreCommon.hpp"
#include "OgreException.hpp"
#include "OgreFactoryObj.hpp"
#include "OgreLogManager.hpp"
#include "OgreParticleAffector.hpp"
#include "OgreParticleAffectorFactory.hpp"
#include "OgreParticleEmitter.hpp"
#include "OgreParticleEmitterFactory.hpp"
#include "OgreParticleSystem.hpp"
#include "OgreParticleSystemManager.hpp"
#include "OgreParticleSystemRenderer.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreRoot.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreSingleton.hpp"
#include "OgreStringConverter.hpp"
#include "OgreStringVector.hpp"

namespace Ogre {

    //-----------------------------------------------------------------------
    // Shortcut to set up billboard particle renderer
    BillboardParticleRendererFactory* mBillboardRendererFactory = nullptr;
    //-----------------------------------------------------------------------
    template<> ParticleSystemManager* Singleton<ParticleSystemManager>::msSingleton = nullptr;
    auto ParticleSystemManager::getSingletonPtr() noexcept -> ParticleSystemManager*
    {
        return msSingleton;
    }
    auto ParticleSystemManager::getSingleton() noexcept -> ParticleSystemManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::ParticleSystemManager()
    {
        mFactory = ::std::make_unique<ParticleSystemFactory>();
        Root::getSingleton().addMovableObjectFactory(mFactory.get());
    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::~ParticleSystemManager()
    {
        removeAllTemplates(true); // Destroy all templates
        ResourceGroupManager::getSingleton()._unregisterScriptLoader(this);
        // delete billboard factory
        if (mBillboardRendererFactory)
        {
            delete mBillboardRendererFactory;
            mBillboardRendererFactory = nullptr;
        }

        if (mFactory)
        {
            // delete particle system factory
            Root::getSingleton().removeMovableObjectFactory(mFactory.get());
        }

    }
    //-----------------------------------------------------------------------
    auto ParticleSystemManager::getScriptPatterns() const noexcept -> const StringVector&
    {
        return mScriptPatterns;
    }
    //-----------------------------------------------------------------------
    auto ParticleSystemManager::getLoadingOrder() const -> Real
    {
        /// Load late
        return 1000.0f;
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::parseScript(DataStreamPtr& stream, std::string_view groupName)
    {
        ScriptCompilerManager::getSingleton().parseScript(stream, groupName);
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addEmitterFactory(ParticleEmitterFactory* factory)
    {
        String name = factory->getName();
        mEmitterFactories[name] = factory;
        LogManager::getSingleton().logMessage(::std::format("Particle Emitter Type '{}' registered", name ));
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addAffectorFactory(ParticleAffectorFactory* factory)
    {
        String name = factory->getName();
        mAffectorFactories[name] = factory;
        LogManager::getSingleton().logMessage(::std::format("Particle Affector Type '{}' registered", name ));
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addRendererFactory(ParticleSystemRendererFactory* factory)
    {
        String name = factory->getType();
        mRendererFactories[name] = factory;
        LogManager::getSingleton().logMessage(::std::format("Particle Renderer Type '{}' registered", name ));
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addTemplate(std::string_view name, ParticleSystem* sysTemplate)
    {
        // check name
        if (mSystemTemplates.find(name) != mSystemTemplates.end())
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
                ::std::format("ParticleSystem template with name '{}' already exists.", name ), 
                "ParticleSystemManager::addTemplate");
        }

        mSystemTemplates[name] = sysTemplate;
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::removeTemplate(std::string_view name, bool deleteTemplate)
    {
        auto itr = mSystemTemplates.find(name);
        if (itr == mSystemTemplates.end())
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                ::std::format("ParticleSystem template with name '{}' cannot be found.", name ),
                "ParticleSystemManager::removeTemplate");

        if (deleteTemplate)
            delete itr->second;

        mSystemTemplates.erase(itr);
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::removeAllTemplates(bool deleteTemplate)
    {
        if (deleteTemplate)
        {
            for (auto & mSystemTemplate : mSystemTemplates)
                delete mSystemTemplate.second;
        }

        mSystemTemplates.clear();
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::removeTemplatesByResourceGroup(std::string_view resourceGroup)
    {
        auto i = mSystemTemplates.begin();
        while (i != mSystemTemplates.end())
        {
            auto icur = i++;

            if(icur->second->getResourceGroupName() == resourceGroup)
            {
                delete icur->second;
                mSystemTemplates.erase(icur);
            }
        }    
    }
    //-----------------------------------------------------------------------
    auto ParticleSystemManager::createTemplate(std::string_view name, 
        std::string_view resourceGroup) -> ParticleSystem*
    {
        // check name
        if (mSystemTemplates.find(name) != mSystemTemplates.end())
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
                ::std::format("ParticleSystem template with name '{}' already exists.", name ), 
                "ParticleSystemManager::createTemplate");
        }

        auto* tpl = new ParticleSystem(name, resourceGroup);
        addTemplate(name, tpl);
        return tpl;

    }
    //-----------------------------------------------------------------------
    auto ParticleSystemManager::getTemplate(std::string_view name) -> ParticleSystem*
    {
        auto i = mSystemTemplates.find(name);
        if (i != mSystemTemplates.end())
        {
            return i->second;
        }
        else
        {
            return nullptr;
        }
    }
    //-----------------------------------------------------------------------
    auto ParticleSystemManager::createSystemImpl(std::string_view name,
        size_t quota, std::string_view resourceGroup) -> ParticleSystem*
    {
        auto* sys = new ParticleSystem(name, resourceGroup);
        sys->setParticleQuota(quota);
        return sys;
    }
    //-----------------------------------------------------------------------
    auto ParticleSystemManager::createSystemImpl(std::string_view name, 
        std::string_view templateName) -> ParticleSystem*
    {
        // Look up template
        ParticleSystem* pTemplate = getTemplate(templateName);
        if (!pTemplate)
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, ::std::format("Cannot find required template '{}'", templateName ), "ParticleSystemManager::createSystem");
        }

        ParticleSystem* sys = createSystemImpl(name, pTemplate->getParticleQuota(), 
            pTemplate->getResourceGroupName());
        // Copy template settings
        *sys = *pTemplate;
        return sys;
        
    }
    //-----------------------------------------------------------------------
    auto ParticleSystemManager::_createEmitter(
        std::string_view emitterType, ParticleSystem* psys) -> ParticleEmitter*
    {
        // Locate emitter type
        auto pFact = mEmitterFactories.find(emitterType);

        if (pFact == mEmitterFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, ::std::format("Cannot find emitter type '{}'", emitterType ));
        }

        return pFact->second->createEmitter(psys);
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_destroyEmitter(ParticleEmitter* emitter)
    {
        if(!emitter)
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot destroy a null ParticleEmitter.", "ParticleSystemManager::_destroyEmitter");

        // Destroy using the factory which created it
        auto pFact = mEmitterFactories.find(emitter->getType());

        if (pFact == mEmitterFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot find emitter factory to destroy emitter.", 
                "ParticleSystemManager::_destroyEmitter");
        }

        pFact->second->destroyEmitter(emitter);
    }
    //-----------------------------------------------------------------------
    auto ParticleSystemManager::_createAffector(
        std::string_view affectorType, ParticleSystem* psys) -> ParticleAffector*
    {
        // Locate affector type
        auto pFact = mAffectorFactories.find(affectorType);

        if (pFact == mAffectorFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, ::std::format("Cannot find affector type '{}'", affectorType ));
        }

        return pFact->second->createAffector(psys);

    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_destroyAffector(ParticleAffector* affector)
    {
        if(!affector)
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot destroy a null ParticleAffector.", "ParticleSystemManager::_destroyAffector");

        // Destroy using the factory which created it
        auto pFact = mAffectorFactories.find(affector->getType());

        if (pFact == mAffectorFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot find affector factory to destroy affector.", 
                "ParticleSystemManager::_destroyAffector");
        }

        pFact->second->destroyAffector(affector);
    }
    //-----------------------------------------------------------------------
    auto ParticleSystemManager::_createRenderer(std::string_view rendererType) -> ParticleSystemRenderer*
    {
        // Locate affector type
        auto pFact = mRendererFactories.find(rendererType);

        if (pFact == mRendererFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot find requested renderer type.", 
                "ParticleSystemManager::_createRenderer");
        }

        return pFact->second->createInstance(rendererType);
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_destroyRenderer(ParticleSystemRenderer* renderer)
    {
        if(!renderer)
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot destroy a null ParticleSystemRenderer.", "ParticleSystemManager::_destroyRenderer");

        // Destroy using the factory which created it
        auto pFact = mRendererFactories.find(renderer->getType());

        if (pFact == mRendererFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot find renderer factory to destroy renderer.", 
                "ParticleSystemManager::_destroyRenderer");
        }

        pFact->second->destroyInstance(renderer);
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_initialise()
    {
        // Create Billboard renderer factory
        mBillboardRendererFactory = new BillboardParticleRendererFactory();
        addRendererFactory(mBillboardRendererFactory);

    }
    //-----------------------------------------------------------------------
    auto 
    ParticleSystemManager::getAffectorFactoryIterator() -> ParticleSystemManager::ParticleAffectorFactoryIterator
    {
        return {
            mAffectorFactories.begin(), mAffectorFactories.end()};
    }
    //-----------------------------------------------------------------------
    auto 
    ParticleSystemManager::getEmitterFactoryIterator() -> ParticleSystemManager::ParticleEmitterFactoryIterator
    {
        return {
            mEmitterFactories.begin(), mEmitterFactories.end()};
    }
    //-----------------------------------------------------------------------
    auto 
    ParticleSystemManager::getRendererFactoryIterator() -> ParticleSystemManager::ParticleRendererFactoryIterator
    {
        return {
            mRendererFactories.begin(), mRendererFactories.end()};
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String ParticleSystemFactory::FACTORY_TYPE_NAME = "ParticleSystem";
    //-----------------------------------------------------------------------
    auto ParticleSystemFactory::createInstanceImpl( std::string_view name, 
            const NameValuePairList* params) -> MovableObject*
    {
        if (params != nullptr)
        {
            auto ni = params->find("templateName");
            if (ni != params->end())
            {
                String templateName = ni->second;
                // create using manager
                return ParticleSystemManager::getSingleton().createSystemImpl(
                        name, templateName);
            }
        }
        // Not template based, look for quota & resource name
        size_t quota = 500;
        String resourceGroup = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
        if (params != nullptr)
        {
            auto ni = params->find("quota");
            if (ni != params->end())
            {
                quota = StringConverter::parseUnsignedInt(ni->second);
            }
            ni = params->find("resourceGroup");
            if (ni != params->end())
            {
                resourceGroup = ni->second;
            }
        }
        // create using manager
        return ParticleSystemManager::getSingleton().createSystemImpl(
                name, quota, resourceGroup);
                

    }
    //-----------------------------------------------------------------------
    auto ParticleSystemFactory::getType() const noexcept -> std::string_view 
    {
        return FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
}
