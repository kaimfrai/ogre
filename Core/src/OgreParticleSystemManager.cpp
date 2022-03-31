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
#include <condition_variable>
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "OgreBillboardParticleRenderer.h"
#include "OgreCommon.h"
#include "OgreException.h"
#include "OgreFactoryObj.h"
#include "OgreLogManager.h"
#include "OgreMemoryAllocatorConfig.h"
#include "OgreParticleAffector.h"
#include "OgreParticleAffectorFactory.h"
#include "OgreParticleEmitter.h"
#include "OgreParticleEmitterFactory.h"
#include "OgreParticleSystem.h"
#include "OgreParticleSystemManager.h"
#include "OgreParticleSystemRenderer.h"
#include "OgrePlatform.h"
#include "OgrePrerequisites.h"
#include "OgreResourceGroupManager.h"
#include "OgreRoot.h"
#include "OgreScriptCompiler.h"
#include "OgreSingleton.h"
#include "OgreStableHeaders.h"
#include "OgreStringConverter.h"
#include "OgreStringVector.h"

namespace Ogre {
    class MovableObject;

    //-----------------------------------------------------------------------
    // Shortcut to set up billboard particle renderer
    BillboardParticleRendererFactory* mBillboardRendererFactory = 0;
    //-----------------------------------------------------------------------
    template<> ParticleSystemManager* Singleton<ParticleSystemManager>::msSingleton = 0;
    ParticleSystemManager* ParticleSystemManager::getSingletonPtr(void)
    {
        return msSingleton;
    }
    ParticleSystemManager& ParticleSystemManager::getSingleton(void)
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::ParticleSystemManager()
    {
        mFactory = new ParticleSystemFactory();
        Root::getSingleton().addMovableObjectFactory(mFactory);
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
            mBillboardRendererFactory = 0;
        }

        if (mFactory)
        {
            // delete particle system factory
            Root::getSingleton().removeMovableObjectFactory(mFactory);
            delete mFactory;
            mFactory = 0;
        }

    }
    //-----------------------------------------------------------------------
    const StringVector& ParticleSystemManager::getScriptPatterns(void) const
    {
        return mScriptPatterns;
    }
    //-----------------------------------------------------------------------
    Real ParticleSystemManager::getLoadingOrder(void) const
    {
        /// Load late
        return 1000.0f;
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::parseScript(DataStreamPtr& stream, const String& groupName)
    {
        ScriptCompilerManager::getSingleton().parseScript(stream, groupName);
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addEmitterFactory(ParticleEmitterFactory* factory)
    {
        String name = factory->getName();
        mEmitterFactories[name] = factory;
        LogManager::getSingleton().logMessage("Particle Emitter Type '" + name + "' registered");
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addAffectorFactory(ParticleAffectorFactory* factory)
    {
        String name = factory->getName();
        mAffectorFactories[name] = factory;
        LogManager::getSingleton().logMessage("Particle Affector Type '" + name + "' registered");
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addRendererFactory(ParticleSystemRendererFactory* factory)
    {
        String name = factory->getType();
        mRendererFactories[name] = factory;
        LogManager::getSingleton().logMessage("Particle Renderer Type '" + name + "' registered");
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::addTemplate(const String& name, ParticleSystem* sysTemplate)
    {
        // check name
        if (mSystemTemplates.find(name) != mSystemTemplates.end())
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
                "ParticleSystem template with name '" + name + "' already exists.", 
                "ParticleSystemManager::addTemplate");
        }

        mSystemTemplates[name] = sysTemplate;
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::removeTemplate(const String& name, bool deleteTemplate)
    {
        ParticleTemplateMap::iterator itr = mSystemTemplates.find(name);
        if (itr == mSystemTemplates.end())
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                "ParticleSystem template with name '" + name + "' cannot be found.",
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
            ParticleTemplateMap::iterator itr;
            for (itr = mSystemTemplates.begin(); itr != mSystemTemplates.end(); ++itr)
                delete itr->second;
        }

        mSystemTemplates.clear();
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::removeTemplatesByResourceGroup(const String& resourceGroup)
    {
        ParticleTemplateMap::iterator i = mSystemTemplates.begin();
        while (i != mSystemTemplates.end())
        {
            ParticleTemplateMap::iterator icur = i++;

            if(icur->second->getResourceGroupName() == resourceGroup)
            {
                delete icur->second;
                mSystemTemplates.erase(icur);
            }
        }    
    }
    //-----------------------------------------------------------------------
    ParticleSystem* ParticleSystemManager::createTemplate(const String& name, 
        const String& resourceGroup)
    {
        // check name
        if (mSystemTemplates.find(name) != mSystemTemplates.end())
        {
            OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, 
                "ParticleSystem template with name '" + name + "' already exists.", 
                "ParticleSystemManager::createTemplate");
        }

        ParticleSystem* tpl = new ParticleSystem(name, resourceGroup);
        addTemplate(name, tpl);
        return tpl;

    }
    //-----------------------------------------------------------------------
    ParticleSystem* ParticleSystemManager::getTemplate(const String& name)
    {
        ParticleTemplateMap::iterator i = mSystemTemplates.find(name);
        if (i != mSystemTemplates.end())
        {
            return i->second;
        }
        else
        {
            return 0;
        }
    }
    //-----------------------------------------------------------------------
    ParticleSystem* ParticleSystemManager::createSystemImpl(const String& name,
        size_t quota, const String& resourceGroup)
    {
        ParticleSystem* sys = new ParticleSystem(name, resourceGroup);
        sys->setParticleQuota(quota);
        return sys;
    }
    //-----------------------------------------------------------------------
    ParticleSystem* ParticleSystemManager::createSystemImpl(const String& name, 
        const String& templateName)
    {
        // Look up template
        ParticleSystem* pTemplate = getTemplate(templateName);
        if (!pTemplate)
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot find required template '" + templateName + "'", "ParticleSystemManager::createSystem");
        }

        ParticleSystem* sys = createSystemImpl(name, pTemplate->getParticleQuota(), 
            pTemplate->getResourceGroupName());
        // Copy template settings
        *sys = *pTemplate;
        return sys;
        
    }
    //-----------------------------------------------------------------------
    ParticleEmitter* ParticleSystemManager::_createEmitter(
        const String& emitterType, ParticleSystem* psys)
    {
        // Locate emitter type
        ParticleEmitterFactoryMap::iterator pFact = mEmitterFactories.find(emitterType);

        if (pFact == mEmitterFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot find emitter type '" + emitterType + "'");
        }

        return pFact->second->createEmitter(psys);
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_destroyEmitter(ParticleEmitter* emitter)
    {
        if(!emitter)
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot destroy a null ParticleEmitter.", "ParticleSystemManager::_destroyEmitter");

        // Destroy using the factory which created it
        ParticleEmitterFactoryMap::iterator pFact = mEmitterFactories.find(emitter->getType());

        if (pFact == mEmitterFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot find emitter factory to destroy emitter.", 
                "ParticleSystemManager::_destroyEmitter");
        }

        pFact->second->destroyEmitter(emitter);
    }
    //-----------------------------------------------------------------------
    ParticleAffector* ParticleSystemManager::_createAffector(
        const String& affectorType, ParticleSystem* psys)
    {
        // Locate affector type
        ParticleAffectorFactoryMap::iterator pFact = mAffectorFactories.find(affectorType);

        if (pFact == mAffectorFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot find affector type '" + affectorType + "'");
        }

        return pFact->second->createAffector(psys);

    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_destroyAffector(ParticleAffector* affector)
    {
        if(!affector)
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot destroy a null ParticleAffector.", "ParticleSystemManager::_destroyAffector");

        // Destroy using the factory which created it
        ParticleAffectorFactoryMap::iterator pFact = mAffectorFactories.find(affector->getType());

        if (pFact == mAffectorFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot find affector factory to destroy affector.", 
                "ParticleSystemManager::_destroyAffector");
        }

        pFact->second->destroyAffector(affector);
    }
    //-----------------------------------------------------------------------
    ParticleSystemRenderer* ParticleSystemManager::_createRenderer(const String& rendererType)
    {
        // Locate affector type
        ParticleSystemRendererFactoryMap::iterator pFact = mRendererFactories.find(rendererType);

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
        ParticleSystemRendererFactoryMap::iterator pFact = mRendererFactories.find(renderer->getType());

        if (pFact == mRendererFactories.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot find renderer factory to destroy renderer.", 
                "ParticleSystemManager::_destroyRenderer");
        }

        pFact->second->destroyInstance(renderer);
    }
    //-----------------------------------------------------------------------
    void ParticleSystemManager::_initialise(void)
    {
        // Create Billboard renderer factory
        mBillboardRendererFactory = new BillboardParticleRendererFactory();
        addRendererFactory(mBillboardRendererFactory);

    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::ParticleAffectorFactoryIterator 
    ParticleSystemManager::getAffectorFactoryIterator(void)
    {
        return ParticleAffectorFactoryIterator(
            mAffectorFactories.begin(), mAffectorFactories.end());
    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::ParticleEmitterFactoryIterator 
    ParticleSystemManager::getEmitterFactoryIterator(void)
    {
        return ParticleEmitterFactoryIterator(
            mEmitterFactories.begin(), mEmitterFactories.end());
    }
    //-----------------------------------------------------------------------
    ParticleSystemManager::ParticleRendererFactoryIterator 
    ParticleSystemManager::getRendererFactoryIterator(void)
    {
        return ParticleRendererFactoryIterator(
            mRendererFactories.begin(), mRendererFactories.end());
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String ParticleSystemFactory::FACTORY_TYPE_NAME = "ParticleSystem";
    //-----------------------------------------------------------------------
    MovableObject* ParticleSystemFactory::createInstanceImpl( const String& name, 
            const NameValuePairList* params)
    {
        if (params != 0)
        {
            NameValuePairList::const_iterator ni = params->find("templateName");
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
        if (params != 0)
        {
            NameValuePairList::const_iterator ni = params->find("quota");
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
    const String& ParticleSystemFactory::getType(void) const
    {
        return FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
}
