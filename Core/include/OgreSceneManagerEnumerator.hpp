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
#ifndef OGRE_CORE_SCENEMANAGERENUMERATOR_H
#define OGRE_CORE_SCENEMANAGERENUMERATOR_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreIteratorWrapper.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSceneManager.hpp"
#include "OgreSingleton.hpp"

namespace Ogre {
class RenderSystem;
    
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */
    /// Factory for default scene manager
    class DefaultSceneManagerFactory : public SceneManagerFactory
    {
    protected:
        void initMetaData() const override;
    public:
        DefaultSceneManagerFactory() = default;
        ~DefaultSceneManagerFactory() override = default;
        /// Factory type name
        static const String FACTORY_TYPE_NAME;
        auto createInstance(std::string_view instanceName) -> SceneManager* override;
    };

    /// Default scene manager
    class DefaultSceneManager : public SceneManager
    {
    public:
        DefaultSceneManager(std::string_view name);
        ~DefaultSceneManager() override;
        auto getTypeName() const noexcept -> std::string_view override;
    };

    /** Enumerates the SceneManager classes available to applications.
        @remarks
            As described in the SceneManager class, SceneManagers are responsible
            for organising the scene and issuing rendering commands to the
            RenderSystem. Certain scene types can benefit from different
            rendering approaches, and it is intended that subclasses will
            be created to special case this.
        @par
            In order to give applications easy access to these implementations,
            this class has a number of methods to create or retrieve a SceneManager
            which is appropriate to the scene type. 
        @par
            SceneManagers are created by SceneManagerFactory instances. New factories
            for new types of SceneManager can be registered with this class to make
            them available to clients.
        @par
            Note that you can still plug in your own custom SceneManager without
            using a factory, should you choose, it's just not as flexible that way.
            Just instantiate your own SceneManager manually and use it directly.
    */
    class SceneManagerEnumerator : public Singleton<SceneManagerEnumerator>, public SceneMgtAlloc
    {
    public:
        /// Scene manager instances, indexed by instance name
        using Instances = std::map<String, SceneManager *>;
        /// List of available scene manager types as meta data
        using MetaDataList = std::vector<const SceneManagerMetaData *>;
    private:
        /// Scene manager factories
        using Factories = std::list<SceneManagerFactory *>;
        Factories mFactories;
        Instances mInstances;
        /// Stored separately to allow iteration
        MetaDataList mMetaDataList;
        /// Factory for default scene manager
        DefaultSceneManagerFactory mDefaultFactory;
        /// Count of creations for auto-naming
        unsigned long mInstanceCreateCount{0};
        /// Currently assigned render system
        RenderSystem* mCurrentRenderSystem{nullptr};


    public:
        SceneManagerEnumerator();
        ~SceneManagerEnumerator();

        /** Register a new SceneManagerFactory. 
        @remarks
            Plugins should call this to register as new SceneManager providers.
        */
        void addFactory(SceneManagerFactory* fact);

        /** Remove a SceneManagerFactory. 
        */
        void removeFactory(SceneManagerFactory* fact);

        /** Get more information about a given type of SceneManager.
        @remarks
            The metadata returned tells you a few things about a given type 
            of SceneManager, which can be created using a factory that has been
            registered already. 
        @param typeName The type name of the SceneManager you want to enquire on.
            If you don't know the typeName already, you can iterate over the 
            metadata for all types using getMetaDataIterator.
        */
        auto getMetaData(std::string_view typeName) const -> const SceneManagerMetaData*;

        /** get all types of SceneManager available for construction

            providing some information about each one.
        */
        auto getMetaData() const noexcept -> const MetaDataList& { return mMetaDataList; }

        using MetaDataIterator = ConstVectorIterator<MetaDataList>;

        /** Create a SceneManager instance of a given type.
        @remarks
            You can use this method to create a SceneManager instance of a 
            given specific type. You may know this type already, or you may
            have discovered it by looking at the results from getMetaDataIterator.
        @note
            This method throws an exception if the named type is not found.
        @param typeName String identifying a unique SceneManager type
        @param instanceName Optional name to given the new instance that is
            created. If you leave this blank, an auto name will be assigned.
        */
        auto createSceneManager(std::string_view typeName, 
            std::string_view instanceName = BLANKSTRING) -> SceneManager*;

        /** Destroy an instance of a SceneManager. */
        void destroySceneManager(SceneManager* sm);

        /** Get an existing SceneManager instance that has already been created,
            identified by the instance name.
        @param instanceName The name of the instance to retrieve.
        */
        auto getSceneManager(std::string_view instanceName) const -> SceneManager*;

        /** Identify if a SceneManager instance already exists.
        @param instanceName The name of the instance to retrieve.
        */
        auto hasSceneManager(std::string_view instanceName) const -> bool;

        using SceneManagerIterator = MapIterator<Instances>;

        /// Get all the existing SceneManager instances.
        auto getSceneManagers() const noexcept -> const Instances&;

        /** Notifies all SceneManagers of the destination rendering system.
        */
        void setRenderSystem(RenderSystem* rs);

        /// Utility method to control shutdown of the managers
        void shutdownAll();
        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> SceneManagerEnumerator&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> SceneManagerEnumerator*;

    };

    /** @} */
    /** @} */

}

#endif
