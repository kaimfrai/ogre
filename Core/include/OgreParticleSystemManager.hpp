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
module;

#include <cstddef>

export module Ogre.Core:ParticleSystemManager;

export import :Common;
export import :IteratorWrapper;
export import :MemoryAllocatorConfig;
export import :MovableObject;
export import :Prerequisites;
export import :ScriptLoader;
export import :Singleton;
export import :StringVector;

export import <map>;
export import <memory>;
export import <string>;

export
namespace Ogre {

    // Forward decl
    class ParticleSystemFactory;
class ParticleAffector;
class ParticleAffectorFactory;
class ParticleEmitter;
class ParticleEmitterFactory;
class ParticleSystem;
class ParticleSystemRenderer;
    
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
    /** Manages particle systems, particle system scripts (templates) and the 
        available emitter & affector factories.

        This singleton class is responsible for managing particle
        systems. Although, the user interface to creating them is via
        SceneManager. Remember that like all other MovableObject
        subclasses, ParticleSystems do not get rendered until they are 
        attached to a SceneNode object.

        This class also manages factories for ParticleEmitter and 
        ParticleAffector classes. To enable easy extensions to the types of 
        emitters (particle sources) and affectors (particle modifiers), the
        ParticleSystemManager lets plugins or applications register factory 
        classes which submit new subclasses to ParticleEmitter and 
        ParticleAffector. The actual implementations,
        such as cone, sphere and box-shaped emitters, and simple affectors such
        as constant directional force and colour faders are provided by the @ref ParticleFX Plugin shipped with %Ogre.
        However using this registration process, a custom plugin can create any behaviour required.

        This class also manages the loading and parsing of particle system 
        scripts, which are text files describing named particle system 
        templates. Instances of particle systems using these templates can
        then be created easily through the SceneManager::createParticleSystem method.
    */
    class ParticleSystemManager: 
        public Singleton<ParticleSystemManager>, public ScriptLoader, public FXAlloc
    {
        friend class ParticleSystemFactory;
    public:
        using ParticleTemplateMap = std::map<std::string_view, ParticleSystem *>;
        using ParticleAffectorFactoryMap = std::map<std::string_view, ParticleAffectorFactory *>;
        using ParticleEmitterFactoryMap = std::map<std::string_view, ParticleEmitterFactory *>;
        using ParticleSystemRendererFactoryMap = std::map<std::string_view, ParticleSystemRendererFactory *>;
    private:
        /// Templates based on scripts
        ParticleTemplateMap mSystemTemplates;
        
        /// Factories for named emitter types (can be extended using plugins)
        ParticleEmitterFactoryMap mEmitterFactories;

        /// Factories for named affector types (can be extended using plugins)
        ParticleAffectorFactoryMap mAffectorFactories;

        /// Map of renderer types to factories
        ParticleSystemRendererFactoryMap mRendererFactories;

        StringVector mScriptPatterns;

        // Factory instance
        ::std::unique_ptr<ParticleSystemFactory> mFactory;

        /// Internal implementation of createSystem
        auto createSystemImpl(std::string_view name, size_t quota, 
            std::string_view resourceGroup) -> ParticleSystem*;
        /// Internal implementation of createSystem
        auto createSystemImpl(std::string_view name, std::string_view templateName) -> ParticleSystem*;
        
    public:

        ParticleSystemManager();
        ~ParticleSystemManager() override;

        /** Adds a new 'factory' object for emitters to the list of available emitter types.
        @remarks
            This method allows plugins etc to add new particle emitter types to Ogre. Particle emitters
            are sources of particles, and generate new particles with their start positions, colours and
            momentums appropriately. Plugins would create new subclasses of ParticleEmitter which 
            emit particles a certain way, and register a subclass of ParticleEmitterFactory to create them (since multiple 
            emitters can be created for different particle systems).
        @par
            All particle emitter factories have an assigned name which is used to identify the emitter
            type. This must be unique.
        @par
            Note that the object passed to this function will not be destroyed by the ParticleSystemManager,
            since it may have been allocated on a different heap in the case of plugins. The caller must
            destroy the object later on, probably on plugin shutdown.
        @param factory
            Pointer to a ParticleEmitterFactory subclass created by the plugin or application code.
        */
        void addEmitterFactory(ParticleEmitterFactory* factory);

        /** Adds a new 'factory' object for affectors to the list of available affector types.
        @remarks
            This method allows plugins etc to add new particle affector types to Ogre. Particle
            affectors modify the particles in a system a certain way such as affecting their direction
            or changing their colour, lifespan etc. Plugins would
            create new subclasses of ParticleAffector which affect particles a certain way, and register
            a subclass of ParticleAffectorFactory to create them.
        @par
            All particle affector factories have an assigned name which is used to identify the affector
            type. This must be unique.
        @par
            Note that the object passed to this function will not be destroyed by the ParticleSystemManager,
            since it may have been allocated on a different heap in the case of plugins. The caller must
            destroy the object later on, probably on plugin shutdown.
        @param factory
            Pointer to a ParticleAffectorFactory subclass created by the plugin or application code.
        */
        void addAffectorFactory(ParticleAffectorFactory* factory);

        /** Registers a factory class for creating ParticleSystemRenderer instances. 
        @par
            Note that the object passed to this function will not be destroyed by the ParticleSystemManager,
            since it may have been allocated on a different heap in the case of plugins. The caller must
            destroy the object later on, probably on plugin shutdown.
        @param factory
            Pointer to a ParticleSystemRendererFactory subclass created by the plugin or application code.
        */
        void addRendererFactory(ParticleSystemRendererFactory* factory);

        /** Adds a new particle system template to the list of available templates. 
        @remarks
            Instances of particle systems in a scene are not normally unique - often you want to place the
            same effect in many places. This method allows you to register a ParticleSystem as a named template,
            which can subsequently be used to create instances using the createSystem method.
        @par
            Note that particle system templates can either be created programmatically by an application 
            and registered using this method, or they can be defined in a script file (*.particle) which is
            loaded by the engine at startup, very much like Material scripts.
        @param name
            The name of the template. Must be unique across all templates.
        @param sysTemplate
            A pointer to a particle system to be used as a template. The manager
            will take over ownership of this pointer.
            
        */
        void addTemplate(std::string_view name, ParticleSystem* sysTemplate);

        /** Removes a specified template from the ParticleSystemManager.
        @remarks
            This method removes a given template from the particle system manager, optionally deleting
            the template if the deleteTemplate method is called.  Throws an exception if the template
            could not be found.
        @param name
            The name of the template to remove.
        @param deleteTemplate
            Whether or not to delete the template before removing it.
        */
        void removeTemplate(std::string_view name, bool deleteTemplate = true);

        /** Removes a specified template from the ParticleSystemManager.
        @remarks
            This method removes all templates from the ParticleSystemManager.
        @param deleteTemplate
            Whether or not to delete the templates before removing them.
        */
        void removeAllTemplates(bool deleteTemplate = true);


        /** Removes all templates that belong to a secific Resource Group from the ParticleSystemManager.
        @remarks
            This method removes all templates that belong in a particular resource group from the ParticleSystemManager.
        @param resourceGroup
            Resource group to delete templates for
        */
        void removeTemplatesByResourceGroup(std::string_view resourceGroup);

        /** Create a new particle system template. 
        @remarks
            This method is similar to the addTemplate method, except this just creates a new template
            and returns a pointer to it to be populated. Use this when you don't already have a system
            to add as a template and just want to create a new template which you will build up in-place.
        @param name
            The name of the template. Must be unique across all templates.
        @param resourceGroup
            The name of the resource group which will be used to 
            load any dependent resources.
            
        */
        auto createTemplate(std::string_view name, std::string_view resourceGroup) -> ParticleSystem*;

        /** Retrieves a particle system template for possible modification. 
        @remarks
            Modifying a template does not affect the settings on any ParticleSystems already created
            from this template.
        */
        auto getTemplate(std::string_view name) -> ParticleSystem*;

        /** Internal method for creating a new emitter from a factory.
        @remarks
            Used internally by the engine to create new ParticleEmitter instances from named
            factories. Applications should use the ParticleSystem::addEmitter method instead, 
            which calls this method to create an instance.
        @param emitterType
            String name of the emitter type to be created. A factory of this type must have been registered.
        @param psys
            The particle system this is being created for
        */
        auto _createEmitter(std::string_view emitterType, ParticleSystem* psys) -> ParticleEmitter*;

        /** Internal method for destroying an emitter.
        @remarks
            Because emitters are created by factories which may allocate memory from separate heaps,
            the memory allocated must be freed from the same place. This method is used to ask the factory
            to destroy the instance passed in as a pointer.
        @param emitter
            Pointer to emitter to be destroyed. On return this pointer will point to invalid (freed) memory.
        */
        void _destroyEmitter(ParticleEmitter* emitter);

        /** Internal method for creating a new affector from a factory.
        @remarks
            Used internally by the engine to create new ParticleAffector instances from named
            factories. Applications should use the ParticleSystem::addAffector method instead, 
            which calls this method to create an instance.
        @param affectorType
            String name of the affector type to be created. A factory of this type must have been registered.
        @param psys
            The particle system it is being created for
        */
        auto _createAffector(std::string_view affectorType, ParticleSystem* psys) -> ParticleAffector*;

        /** Internal method for destroying an affector.
        @remarks
            Because affectors are created by factories which may allocate memory from separate heaps,
            the memory allocated must be freed from the same place. This method is used to ask the factory
            to destroy the instance passed in as a pointer.
        @param affector
            Pointer to affector to be destroyed. On return this pointer will point to invalid (freed) memory.
        */
        void _destroyAffector(ParticleAffector* affector);

        /** Internal method for creating a new renderer from a factory.
        @remarks
            Used internally by the engine to create new ParticleSystemRenderer instances from named
            factories. Applications should use the ParticleSystem::setRenderer method instead, 
            which calls this method to create an instance.
        @param rendererType
            String name of the renderer type to be created. A factory of this type must have been registered.
        */
        auto _createRenderer(std::string_view rendererType) -> ParticleSystemRenderer*;

        /** Internal method for destroying a renderer.
        @remarks
            Because renderer are created by factories which may allocate memory from separate heaps,
            the memory allocated must be freed from the same place. This method is used to ask the factory
            to destroy the instance passed in as a pointer.
        @param renderer
            Pointer to renderer to be destroyed. On return this pointer will point to invalid (freed) memory.
        */
        void _destroyRenderer(ParticleSystemRenderer* renderer);

        /** Init method to be called by OGRE system.
        @remarks
            Due to dependencies between various objects certain initialisation tasks cannot be done
            on construction. OGRE will call this method when the rendering subsystem is initialised.
        */
        void _initialise();

        /// @copydoc ScriptLoader::getScriptPatterns
        [[nodiscard]] auto getScriptPatterns() const noexcept -> const StringVector& override;
        /// @copydoc ScriptLoader::parseScript
        void parseScript(DataStreamPtr& stream, std::string_view groupName) override;
        /// @copydoc ScriptLoader::getLoadingOrder
        [[nodiscard]] auto getLoadingOrder() const -> Real override;

        using ParticleAffectorFactoryIterator = MapIterator<ParticleAffectorFactoryMap>;
        using ParticleEmitterFactoryIterator = MapIterator<ParticleEmitterFactoryMap>;
        using ParticleRendererFactoryIterator = MapIterator<ParticleSystemRendererFactoryMap>;
        /** Return an iterator over the affector factories currently registered */
        auto getAffectorFactoryIterator() -> ParticleAffectorFactoryIterator;
        /** Return an iterator over the emitter factories currently registered */
        auto getEmitterFactoryIterator() -> ParticleEmitterFactoryIterator;
        /** Return an iterator over the renderer factories currently registered */
        auto getRendererFactoryIterator() -> ParticleRendererFactoryIterator;


        using ParticleSystemTemplateIterator = MapIterator<ParticleTemplateMap>;
        /** Gets an iterator over the list of particle system templates. */
        auto getTemplateIterator() -> ParticleSystemTemplateIterator
        {
            return {
                mSystemTemplates.begin(), mSystemTemplates.end()};
        } 

        /** Get an instance of ParticleSystemFactory (internal use). */
        auto _getFactory() noexcept -> ParticleSystemFactory* { return mFactory.get(); }
        
        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> ParticleSystemManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> ParticleSystemManager*;

    };

    /** Factory object for creating ParticleSystem instances */
    class ParticleSystemFactory : public MovableObjectFactory
    {
    private:
        auto createInstanceImpl(std::string_view name, const NameValuePairList* params) -> MovableObject* override;
    public:
        ParticleSystemFactory() = default;
        ~ParticleSystemFactory() override = default;
        
        static std::string_view const FACTORY_TYPE_NAME;

        [[nodiscard]] auto getType() const noexcept -> std::string_view override;
    };
    /** @} */
    /** @} */

}
