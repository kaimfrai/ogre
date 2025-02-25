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
// Ogre includes
module;

#include <cassert>
#include <cstdlib>

module Ogre.Core;

import :ASTCCodec;
import :ArchiveFactory;
import :ArchiveManager;
import :BillboardChain;
import :BillboardSet;
import :BuiltinMovableFactories;
import :Common;
import :CompositorManager;
import :ConfigDialog;
import :ConfigFile;
import :ConfigOptionMap;
import :ControllerManager;
import :ConvexBody;
import :DDSCodec;
import :DefaultWorkQueueStandard;
import :DynLib;
import :DynLibManager;
import :ETCCodec;
import :Entity;
import :Exception;
import :ExternalTextureSourceManager;
import :FileSystem;
import :FileSystemLayer;
import :FrameListener;
import :GpuProgramManager;
import :HardwareBufferManager;
import :Light;
import :LodStrategyManager;
import :LogManager;
import :ManualObject;
import :MaterialManager;
import :Math;
import :MeshManager;
import :MovableObject;
import :ParticleSystemManager;
import :Platform;
import :PlatformInformation;
import :Plugin;
import :Prerequisites;
import :Profiler;
import :RenderSystem;
import :RenderSystemCapabilities;
import :RenderSystemCapabilitiesManager;
import :RenderTarget;
import :RenderWindow;
import :ResourceBackgroundQueue;
import :ResourceGroupManager;
import :RibbonTrail;
import :Root;
import :SceneManager;
import :SceneManagerEnumerator;
import :ScriptCompiler;
import :ShadowTextureManager;
import :ShadowVolumeExtrudeProgram;
import :SharedPtr;
import :Singleton;
import :SkeletonManager;
import :StaticGeometry;
import :String;
import :StringConverter;
import :StringInterface;
import :StringVector;
import :TextureManager;
import :Timer;
import :WorkQueue;
import :Zip;

import <algorithm>;
import <deque>;
import <format>;
import <map>;
import <memory>;
import <ostream>;
import <ranges>;
import <set>;
import <string>;
import <thread>;
import <utility>;
import <vector>;

namespace Ogre {
    //-----------------------------------------------------------------------
    template<> Root* Singleton<Root>::msSingleton = nullptr;
    auto Root::getSingletonPtr() noexcept -> Root*
    {
        return msSingleton;
    }
    auto Root::getSingleton() noexcept -> Root&
    {
        assert( msSingleton );  return ( *msSingleton );
    }

    using DLL_START_PLUGIN = void (*)();
    using DLL_STOP_PLUGIN = void (*)();

    //-----------------------------------------------------------------------
    Root::Root(std::string_view pluginFileName, std::string_view configFileName,
        std::string_view logFileName, ulong frameCount)
      : 
       mFrameCount(frameCount)
       
    {
        // superclass will do singleton checking

        // Init
        mActiveRenderer = nullptr;
        mVersion = ::std::format("{}.{}.{}{} ({})",
            /*OGRE_VERSION_MAJOR*/13,
            /*OGRE_VERSION_MINOR*/3,
            /*OGRE_VERSION_PATCH*/3,
            /*OGRE_VERSION_SUFFIX*/"Modernized",
            /*OGRE_VERSION_NAME*/"Tsathoggua");
        mConfigFileName = configFileName;

        // Create log manager and default log file if there is no log manager yet
        if(!LogManager::getSingletonPtr())
        {
            mLogManager = std::make_unique<LogManager>();

            mLogManager->createLog(logFileName, true, true);
        }

        mDynLibManager = std::make_unique<DynLibManager>();
        mArchiveManager = std::make_unique<ArchiveManager>();
        mResourceGroupManager = std::make_unique<ResourceGroupManager>();

        // WorkQueue (note: users can replace this if they want)
        auto* defaultQ = new DefaultWorkQueue("Root");
        // never process responses in main thread for longer than 10ms by default
        defaultQ->setResponseProcessingTimeLimit(10);
        // match threads to hardware
        int threadCount = std::thread::hardware_concurrency();
        // but clamp it at 2 by default - we dont scale much beyond that currently
        // yet it helps on android where it needlessly burns CPU
        threadCount = Math::Clamp(threadCount, 1, 2);
        defaultQ->setWorkerThreadCount(threadCount);

        defaultQ->setWorkersCanAccessRenderSystem(false);
        mWorkQueue.reset(defaultQ);

        // ResourceBackgroundQueue
        mResourceBackgroundQueue = std::make_unique<ResourceBackgroundQueue>();

        // Create SceneManager enumerator (note - will be managed by singleton)
        mSceneManagerEnum = std::make_unique<SceneManagerEnumerator>();
        mShadowTextureManager = std::make_unique<ShadowTextureManager>();
        mRenderSystemCapabilitiesManager = std::make_unique<RenderSystemCapabilitiesManager>();
        mMaterialManager = std::make_unique<MaterialManager>();
        mMeshManager = std::make_unique<MeshManager>();
        mSkeletonManager = std::make_unique<SkeletonManager>();
        mParticleManager = std::make_unique<ParticleSystemManager>();
        mTimer = std::make_unique<Timer>();
        mLodStrategyManager = std::make_unique<LodStrategyManager>();

        // Profiler
        mProfiler = std::make_unique<Profiler>();
        Profiler::getSingleton().setTimer(mTimer.get());

        mFileSystemArchiveFactory = std::make_unique<FileSystemArchiveFactory>();
        ArchiveManager::getSingleton().addArchiveFactory( mFileSystemArchiveFactory.get() );

        mZipArchiveFactory = std::make_unique<ZipArchiveFactory>();
        ArchiveManager::getSingleton().addArchiveFactory( mZipArchiveFactory.get() );
        mEmbeddedZipArchiveFactory = std::make_unique<EmbeddedZipArchiveFactory>();
        ArchiveManager::getSingleton().addArchiveFactory( mEmbeddedZipArchiveFactory.get() );

        // Register image codecs
        DDSCodec::startup();
        ETCCodec::startup();
        ASTCCodec::startup();

        mGpuProgramManager = std::make_unique<GpuProgramManager>();
        mExternalTextureSourceManager = std::make_unique<ExternalTextureSourceManager>();
        mCompositorManager = std::make_unique<CompositorManager>();
        mCompilerManager = std::make_unique<ScriptCompilerManager>();

        // Auto window
        mAutoWindow = nullptr;

        // instantiate and register base movable factories
        mEntityFactory = std::make_unique<EntityFactory>();
        addMovableObjectFactory(mEntityFactory.get());
        mLightFactory = std::make_unique<LightFactory>();
        addMovableObjectFactory(mLightFactory.get());
        mBillboardSetFactory = std::make_unique<BillboardSetFactory>();
        addMovableObjectFactory(mBillboardSetFactory.get());
        mManualObjectFactory = std::make_unique<ManualObjectFactory>();
        addMovableObjectFactory(mManualObjectFactory.get());
        mBillboardChainFactory = std::make_unique<BillboardChainFactory>();
        addMovableObjectFactory(mBillboardChainFactory.get());
        mRibbonTrailFactory = std::make_unique<RibbonTrailFactory>();
        addMovableObjectFactory(mRibbonTrailFactory.get());
        mStaticGeometryFactory = std::make_unique<StaticGeometryFactory>();
        addMovableObjectFactory(mStaticGeometryFactory.get());
        mRectangle2DFactory = std::make_unique<Rectangle2DFactory>();
        addMovableObjectFactory(mRectangle2DFactory.get());

        // Load plugins
        if (!pluginFileName.empty())
            loadPlugins(pluginFileName);

        LogManager::getSingleton().logMessage("*-*-* OGRE Initialising");
        LogManager::getSingleton().logMessage(::std::format("*-*-* Version {}", mVersion));

        // Can't create managers until initialised
        mControllerManager = nullptr;

        mFirstTimePostWindowInit = false;
    }

    //-----------------------------------------------------------------------
    Root::~Root()
    {
        shutdown();

        DDSCodec::shutdown();
        ETCCodec::shutdown();
        ASTCCodec::shutdown();

		mCompositorManager.reset(); // needs rendersystem
        mParticleManager.reset(); // may use plugins
        mGpuProgramManager.reset(); // may use plugins
        unloadPlugins();

        mAutoWindow = nullptr;

        StringInterface::cleanupDictionary();
    }

    //-----------------------------------------------------------------------
    void Root::saveConfig()
    {
        if (mConfigFileName.empty())
            return;

        std::ofstream of(mConfigFileName.c_str());

        if (!of)
            OGRE_EXCEPT(ExceptionCodes::CANNOT_WRITE_TO_FILE, "Cannot create settings file.",
            "Root::saveConfig");

        if (mActiveRenderer)
        {
            of << "Render System=" << mActiveRenderer->getName() << std::endl;
        }
        else
        {
            of << "Render System=" << std::endl;
        }

        for (auto rs : getAvailableRenderers())
        {
            of << std::endl;
            of << "[" << rs->getName() << "]" << std::endl;
            const ConfigOptionMap& opts = rs->getConfigOptions();
            for (const auto & opt : opts)
            {
                of << opt.first << "=" << opt.second.currentValue << std::endl;
            }
        }

        of.close();

    }
    //-----------------------------------------------------------------------
    auto Root::restoreConfig() -> bool
    {
        if (mConfigFileName.empty ())
            return true;

        // Restores configuration from saved state
        // Returns true if a valid saved configuration is
        //   available, and false if no saved config is
        //   stored, or if there has been a problem
        ConfigFile cfg;

        try {
            // Don't trim whitespace
            cfg.load(mConfigFileName, "\t:=", false);
        }
        catch (FileNotFoundException&)
        {
            return false;
        }

        bool optionError = false;
        for (auto const& [renderSystem, settings] : cfg.getSettingsBySection()) {

            RenderSystem* rs = getRenderSystemByName(renderSystem);
            if (!rs)
            {
                // Unrecognised render system
                continue;
            }

            for (auto const& [key, value] : settings)
            {
                try
                {
                    rs->setConfigOption(key, value);
                }
                catch(const InvalidParametersException& e)
                {
                    LogManager::getSingleton().logError(e.getDescription());
                    optionError = true;
                    continue;
                }
            }
        }

        RenderSystem* rs = getRenderSystemByName(cfg.getSetting("Render System"));
        if (!rs)
        {
            // Unrecognised render system
            return false;
        }

        String err = rs->validateConfigOptions();
        if (err.length() > 0)
            return false;

        setRenderSystem(rs);

        // Successful load
        return !optionError;
    }

    //-----------------------------------------------------------------------
    auto Root::showConfigDialog(ConfigDialog* dialog) -> bool {
        if(dialog) {
            if(!mActiveRenderer)
                restoreConfig();

            if (dialog->display()) {
                saveConfig();
                return true;
            }

            return false;
        }

        // just select the first available render system
        if (!mRenderers.empty())
        {
            setRenderSystem(mRenderers.front());
            return true;
        }

        return false;
    }

    //-----------------------------------------------------------------------
    auto Root::getAvailableRenderers() noexcept -> const RenderSystemList&
    {
        // Returns a vector of renders

        return mRenderers;

    }

    //-----------------------------------------------------------------------
    auto Root::getRenderSystemByName(std::string_view name) -> RenderSystem*
    {
        if (name.empty())
        {
            // No render system
            return nullptr;
        }

        for (auto rs : getAvailableRenderers())
        {
            if (rs->getName() == name)
                return rs;
        }

        // Unrecognised render system
        return nullptr;
    }

    //-----------------------------------------------------------------------
    void Root::setRenderSystem(RenderSystem* system)
    {
        // Sets the active rendering system
        // Can be called direct or will be called by
        //   standard config dialog

        // Is there already an active renderer?
        // If so, disable it and init the new one
        if( mActiveRenderer && mActiveRenderer != system )
        {
            mActiveRenderer->shutdown();
        }

        mActiveRenderer = system;
        // Tell scene managers
        if(mSceneManagerEnum)
            mSceneManagerEnum->setRenderSystem(system);

        if(RenderSystem::Listener* ls = RenderSystem::getSharedListener())
            ls->eventOccurred("RenderSystemChanged");
    }
    //-----------------------------------------------------------------------
    void Root::addRenderSystem(RenderSystem *newRend)
    {
        mRenderers.push_back(newRend);
    }
    //-----------------------------------------------------------------------
    auto Root::getRenderSystem() noexcept -> RenderSystem*
    {
        // Gets the currently active renderer
        return mActiveRenderer;

    }

    //-----------------------------------------------------------------------
    auto Root::initialise(bool autoCreateWindow, std::string_view windowTitle, std::string_view customCapabilitiesConfig) -> RenderWindow*
    {
        OgreAssert(mActiveRenderer, "Cannot initialise");

        if (!mControllerManager)
            mControllerManager = std::make_unique<ControllerManager>();

        // .rendercaps manager
        RenderSystemCapabilitiesManager& rscManager = RenderSystemCapabilitiesManager::getSingleton();
        // caller wants to load custom RenderSystemCapabilities form a config file
        if(!customCapabilitiesConfig.empty())
        {
            ConfigFile cfg;
            cfg.load(customCapabilitiesConfig, "\t:=", false);

            // Capabilities Database setting must be in the same format as
            // resources.cfg in Ogre examples.
            const ConfigFile::SettingsMultiMap& dbs = cfg.getSettings("Capabilities Database");
            for(auto const& [archType, filename] : dbs)
            {
                rscManager.parseCapabilitiesFromArchive(filename, archType, true);
            }

            auto const capsName = cfg.getSetting("Custom Capabilities");
            // The custom capabilities have been parsed, let's retrieve them
            RenderSystemCapabilities* rsc = rscManager.loadParsedCapabilities(capsName);
            if(rsc == nullptr)
            {
                OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                    std::format("Cannot load a RenderSystemCapability named {}", capsName),
                    "Root::initialise");
            }

            // Tell RenderSystem to use the comon rsc
            useCustomRenderSystemCapabilities(rsc);
        }


        PlatformInformation::log(LogManager::getSingleton().getDefaultLog());
        mActiveRenderer->_initialise();

        // Initialise timer
        mTimer->reset();

        // Init pools
        ConvexBody::_initialisePool();

        mIsInitialised = true;

        if (autoCreateWindow)
        {
            auto desc = mActiveRenderer->getRenderWindowDescription();
            desc.name = windowTitle;
            mAutoWindow = createRenderWindow(desc);
        }

        return mAutoWindow;

    }
    //-----------------------------------------------------------------------
    void Root::useCustomRenderSystemCapabilities(RenderSystemCapabilities* capabilities)
    {
        mActiveRenderer->useCustomRenderSystemCapabilities(capabilities);
    }
    //-----------------------------------------------------------------------
    void Root::addSceneManagerFactory(SceneManagerFactory* fact)
    {
        mSceneManagerEnum->addFactory(fact);
    }
    //-----------------------------------------------------------------------
    void Root::removeSceneManagerFactory(SceneManagerFactory* fact)
    {
        mSceneManagerEnum->removeFactory(fact);
    }
    //-----------------------------------------------------------------------
    auto Root::getSceneManagerMetaData(std::string_view typeName) const -> const SceneManagerMetaData*
    {
        return mSceneManagerEnum->getMetaData(typeName);
    }

    //-----------------------------------------------------------------------
    auto
    Root::getSceneManagerMetaData() const noexcept -> const SceneManagerEnumerator::MetaDataList&
    {
        return mSceneManagerEnum->getMetaData();
    }
    //-----------------------------------------------------------------------
    auto Root::createSceneManager(std::string_view typeName,
        std::string_view instanceName) -> SceneManager*
    {
        return mSceneManagerEnum->createSceneManager(typeName, instanceName);
    }
    //-----------------------------------------------------------------------
    void Root::destroySceneManager(SceneManager* sm)
    {
        mSceneManagerEnum->destroySceneManager(sm);
    }
    //-----------------------------------------------------------------------
    auto Root::getSceneManager(std::string_view instanceName) const -> SceneManager*
    {
        return mSceneManagerEnum->getSceneManager(instanceName);
    }
    //---------------------------------------------------------------------
    auto Root::hasSceneManager(std::string_view instanceName) const -> bool
    {
        return mSceneManagerEnum->hasSceneManager(instanceName);
    }

    //-----------------------------------------------------------------------
    auto Root::getSceneManagers() const noexcept -> const SceneManagerEnumerator::Instances&
    {
        return mSceneManagerEnum->getSceneManagers();
    }
    //-----------------------------------------------------------------------
    auto Root::getTextureManager() noexcept -> TextureManager*
    {
        return &TextureManager::getSingleton();
    }
    //-----------------------------------------------------------------------
    auto Root::getMeshManager() noexcept -> MeshManager*
    {
        return &MeshManager::getSingleton();
    }
    //-----------------------------------------------------------------------
    void Root::addFrameListener(FrameListener* newListener)
    {
        mRemovedFrameListeners.erase(newListener);
        mAddedFrameListeners.insert(newListener);
    }
    //-----------------------------------------------------------------------
    void Root::removeFrameListener(FrameListener* oldListener)
    {
        mAddedFrameListeners.erase(oldListener);
        mRemovedFrameListeners.insert(oldListener);
    }
    //-----------------------------------------------------------------------
    void Root::_syncAddedRemovedFrameListeners()
    {
        for (auto mRemovedFrameListener : mRemovedFrameListeners)
            mFrameListeners.erase(mRemovedFrameListener);
        mRemovedFrameListeners.clear();

        for (auto mAddedFrameListener : mAddedFrameListeners)
            mFrameListeners.insert(mAddedFrameListener);
        mAddedFrameListeners.clear();
    }
    //-----------------------------------------------------------------------
    auto Root::_fireFrameStarted(FrameEvent& evt) -> bool
    {
        _syncAddedRemovedFrameListeners();

        // Tell all listeners
        for (auto mFrameListener : mFrameListeners)
        {
            if(mRemovedFrameListeners.find(mFrameListener) != mRemovedFrameListeners.end())
                continue;

            if (!mFrameListener->frameStarted(evt))
                return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------
    auto Root::_fireFrameRenderingQueued(FrameEvent& evt) -> bool
    {
        // Increment next frame number
        ++mNextFrame;
        _syncAddedRemovedFrameListeners();

        // Tell all listeners
        for (auto mFrameListener : mFrameListeners)
        {
            if(mRemovedFrameListeners.find(mFrameListener) != mRemovedFrameListeners.end())
                continue;

            if (!mFrameListener->frameRenderingQueued(evt))
                return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------
    auto Root::_fireFrameEnded(FrameEvent& evt) -> bool
    {
        _syncAddedRemovedFrameListeners();

        // Tell all listeners
        bool ret = true;
        for (auto mFrameListener : mFrameListeners)
        {
            if(mRemovedFrameListeners.find(mFrameListener) != mRemovedFrameListeners.end())
                continue;

            if (!mFrameListener->frameEnded(evt))
            {
                ret = false;
                break;
            }
        }

        // Tell buffer manager to free temp buffers used this frame
        if (HardwareBufferManager::getSingletonPtr())
            HardwareBufferManager::getSingleton()._releaseBufferCopies();

        // Tell the queue to process responses
        mWorkQueue->processResponses();

        return ret;
    }
    //-----------------------------------------------------------------------
    auto Root::_fireFrameStarted() -> bool
    {
        FrameEvent evt;
        populateFrameEvent(FrameEventTimeType::STARTED, evt);

        return _fireFrameStarted(evt);
    }
    //-----------------------------------------------------------------------
    auto Root::_fireFrameRenderingQueued() -> bool
    {
        FrameEvent evt;
        populateFrameEvent(FrameEventTimeType::QUEUED, evt);

        return _fireFrameRenderingQueued(evt);
    }
    //-----------------------------------------------------------------------
    auto Root::_fireFrameEnded() -> bool
    {
        FrameEvent evt;
        populateFrameEvent(FrameEventTimeType::ENDED, evt);
        return _fireFrameEnded(evt);
    }
    //---------------------------------------------------------------------
    void Root::populateFrameEvent(FrameEventTimeType type, FrameEvent& evtToUpdate)
    {
        unsigned long now = mTimer->getMilliseconds();
        auto const timeSinceLastEvent = calculateEventTime(now, FrameEventTimeType::ANY);
        auto const timeSinceLastFrame = calculateEventTime(now, type);
//         evtToUpdate.timeSinceLastEvent = timeSinceLastEvent;
//         evtToUpdate.timeSinceLastFrame = timeSinceLastFrame;
        if  (timeSinceLastFrame <= 0.0)
        {   evtToUpdate.timeSinceLastEvent = 0.0f;
            evtToUpdate.timeSinceLastFrame = 0.0f;
        }
        else
        {
            evtToUpdate.timeSinceLastEvent =
                1.0 / (32.0 * timeSinceLastFrame) *
                timeSinceLastEvent;
            evtToUpdate.timeSinceLastFrame =
                1.0/32.0;
        }
    }
    //-----------------------------------------------------------------------
    auto Root::calculateEventTime(unsigned long now, FrameEventTimeType type) -> Real
    {
        // Calculate the average time passed between events of the given type
        // during the last mFrameSmoothingTime seconds.

        EventTimesQueue& times = mEventTimes[std::to_underlying(type)];
        times.push_back(now);

        if(times.size() == 1)
            return 0;

        // Times up to mFrameSmoothingTime seconds old should be kept
        auto discardThreshold =
            static_cast<unsigned long>(mFrameSmoothingTime * 1000.0f);

        // Find the oldest time to keep
        auto it = times.begin(),
            end = times.end()-2; // We need at least two times
        while(it != end)
        {
            if (now - *it > discardThreshold)
                ++it;
            else
                break;
        }

        // Remove old times
        times.erase(times.begin(), it);

        return Real(times.back() - times.front()) / ((times.size()-1) * 1000);
    }
    //-----------------------------------------------------------------------
    void Root::queueEndRendering(bool state /* = true */)
    {
        mQueuedEnd = state;
    }
    //-----------------------------------------------------------------------
    auto Root::endRenderingQueued() -> bool
    {
        return mQueuedEnd;
    }
    //-----------------------------------------------------------------------
    void Root::startRendering()
    {
        OgreAssert(mActiveRenderer, "no RenderSystem");

        mActiveRenderer->_initRenderTargets();

        // Clear event times
        clearEventTimes();

        // Infinite loop, until broken out of by frame listeners
        // or break out by calling queueEndRendering()
        mQueuedEnd = false;

        for(::std::size_t frame = 0; frame < mFrameCount && !mQueuedEnd; ++frame)
        {
            Ogre::Profile frameProfile("Frame", ProfileGroupMask::GENERAL);
            if (!renderOneFrame())
            {
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    auto Root::renderOneFrame() -> bool
    {
        if(!_fireFrameStarted())
            return false;

        if (!_updateAllRenderTargets())
            return false;

        return _fireFrameEnded();
    }
    //---------------------------------------------------------------------
    auto Root::renderOneFrame(Real timeSinceLastFrame) -> bool
    {
        FrameEvent evt;
        evt.timeSinceLastFrame = timeSinceLastFrame;

        unsigned long now = mTimer->getMilliseconds();
        evt.timeSinceLastEvent = calculateEventTime(now, FrameEventTimeType::ANY);

        if(!_fireFrameStarted(evt))
            return false;

        if (!_updateAllRenderTargets(evt))
            return false;

        now = mTimer->getMilliseconds();
        evt.timeSinceLastEvent = calculateEventTime(now, FrameEventTimeType::ANY);

        return _fireFrameEnded(evt);
    }
    //-----------------------------------------------------------------------
    void Root::shutdown()
    {
        if(mActiveRenderer)
            mActiveRenderer->_setViewport(nullptr);

        // Since background thread might be access resources,
        // ensure shutdown before destroying resource manager.
        mResourceBackgroundQueue->shutdown();
        mWorkQueue->shutdown();

        if(mSceneManagerEnum)
            mSceneManagerEnum->shutdownAll();
        if(mFirstTimePostWindowInit)
        {
            shutdownPlugins();
            mParticleManager->removeAllTemplates(true);
            mFirstTimePostWindowInit = false;
        }
        mSceneManagerEnum.reset();
        mShadowTextureManager.reset();

        ShadowVolumeExtrudeProgram::shutdown();
        ResourceGroupManager::getSingleton().shutdownAll();

        // Destroy pools
        ConvexBody::_destroyPool();


        mIsInitialised = false;

        LogManager::getSingleton().logMessage("*-*-* OGRE Shutdown");
    }
    //-----------------------------------------------------------------------
    void Root::loadPlugins( std::string_view pluginsfile )
    {
        StringVector pluginList;
        String pluginDir;
        ConfigFile cfg;

        try {
            cfg.load( pluginsfile );
        }
        catch (Exception& e)
        {
            LogManager::getSingleton().logError(::std::format("{} - skipping automatic plugin loading", e.getDescription()));
            return;
        }

        pluginDir = cfg.getSetting("PluginFolder");
        pluginList = cfg.getMultiSetting("Plugin");

        StringUtil::trim(pluginDir);
        if(pluginDir.empty() || pluginDir[0] == '.')
        {
            // resolve relative path with regards to configfile
            std::string_view baseDir, filename;
            StringUtil::splitFilename(pluginsfile, filename, baseDir);
            pluginDir = std::format("{}{}", baseDir, pluginDir);
        }

        if(char* val = getenv("OGRE_PLUGIN_DIR"))
        {
            pluginDir = val;
            LogManager::getSingleton().logMessage(
                "setting PluginFolder from OGRE_PLUGIN_DIR environment variable");
        }

        pluginDir = FileSystemLayer::resolveBundlePath(pluginDir);

        if (!pluginDir.empty() && *pluginDir.rbegin() != '/' && *pluginDir.rbegin() != '\\')
        {
            pluginDir += "/";
        }

        for(auto & it : pluginList)
        {
            loadPlugin(pluginDir + it);
        }

    }
    //-----------------------------------------------------------------------
    void Root::shutdownPlugins()
    {
        // NB Shutdown plugins in reverse order to enforce dependencies
        for (auto & mPlugin : std::ranges::reverse_view(mPlugins))
        {
            mPlugin->shutdown();
        }
    }
    //-----------------------------------------------------------------------
    void Root::initialisePlugins()
    {
        for (auto & mPlugin : mPlugins)
        {
            mPlugin->initialise();
        }
    }
    //-----------------------------------------------------------------------
    void Root::unloadPlugins()
    {
        // unload dynamic libs first
        for (auto & mPluginLib : std::ranges::reverse_view(mPluginLibs))
        {
            // Call plugin shutdown
            #ifdef __GNUC__
            __extension__
            #endif
            DLL_STOP_PLUGIN pFunc = reinterpret_cast<DLL_STOP_PLUGIN>(mPluginLib->getSymbol("dllStopPlugin"));
            // this will call uninstallPlugin
            pFunc();
            // Unload library & destroy
            DynLibManager::getSingleton().unload(mPluginLib);

        }
        mPluginLibs.clear();

        // now deal with any remaining plugins that were registered through other means
        for (auto & mPlugin : std::ranges::reverse_view(mPlugins))
        {
            // Note this does NOT call uninstallPlugin - this shutdown is for the
            // detail objects
            mPlugin->uninstall();
        }
        mPlugins.clear();
    }
    //---------------------------------------------------------------------
    auto Root::createFileStream(std::string_view filename, std::string_view groupName,
        bool overwrite, std::string_view locationPattern) -> DataStreamPtr
    {
        // Does this file include path specifiers?
        std::string_view path, basename;
        StringUtil::splitFilename(filename, basename, path);

        // no path elements, try the resource system first
        DataStreamPtr stream;
        if (path.empty())
        {
            try
            {
                stream = ResourceGroupManager::getSingleton().createResource(
                    filename, groupName, overwrite, locationPattern);
            }
            catch (...) {}

        }

        if (!stream)
        {
            // save direct in filesystem
            stream = _openFileStream(filename, std::ios::out | std::ios::binary);
        }

        return stream;

    }
    //---------------------------------------------------------------------
    auto Root::openFileStream(std::string_view filename, std::string_view groupName) -> DataStreamPtr
    {
        auto ret = ResourceGroupManager::getSingleton().openResource(filename, groupName, nullptr, false);
        if(ret)
            return ret;

        return _openFileStream(filename, std::ios::in | std::ios::binary);
    }
    //-----------------------------------------------------------------------
    auto Root::getAutoCreatedWindow() noexcept -> RenderWindow*
    {
        return mAutoWindow;
    }
    //-----------------------------------------------------------------------
    auto Root::createRenderWindow(std::string_view name, unsigned int width, unsigned int height,
            bool fullScreen, const NameValuePairList *miscParams) -> RenderWindow*
    {
        OgreAssert(mIsInitialised,
                   "Cannot create window! Make sure to call Root::initialise before creating a window");
        OgreAssert(mActiveRenderer, "Cannot create window");

        RenderWindow* ret;
        ret = mActiveRenderer->_createRenderWindow(name, width, height, fullScreen, miscParams);

        // Initialisation for classes dependent on first window created
        if(!mFirstTimePostWindowInit)
        {
            oneTimePostWindowInit();
            ret->_setPrimary();
        }

        return ret;

    }
    //-----------------------------------------------------------------------
    auto Root::detachRenderTarget(RenderTarget* target) -> RenderTarget*
    {
        OgreAssert(mActiveRenderer, "Cannot detach target");
        return mActiveRenderer->detachRenderTarget( target->getName() );
    }
    //-----------------------------------------------------------------------
    auto Root::detachRenderTarget(std::string_view name) -> RenderTarget*
    {
        OgreAssert(mActiveRenderer, "Cannot detach target");
        return mActiveRenderer->detachRenderTarget( name );
    }
    //-----------------------------------------------------------------------
    void Root::destroyRenderTarget(RenderTarget* target)
    {
        detachRenderTarget(target);
        delete target;
    }
    //-----------------------------------------------------------------------
    void Root::destroyRenderWindow(RenderWindow* target)
    {
        mActiveRenderer->destroyRenderWindow(target->getName());
    }
    //-----------------------------------------------------------------------
    void Root::destroyRenderTarget(std::string_view name)
    {
        RenderTarget* target = getRenderTarget(name);
        destroyRenderTarget(target);
    }
    //-----------------------------------------------------------------------
    auto Root::getRenderTarget(std::string_view name) -> RenderTarget*
    {
        OgreAssert(mActiveRenderer, "Cannot get target");
        return mActiveRenderer->getRenderTarget(name);
    }
    //---------------------------------------------------------------------
    void Root::installPlugin(Plugin* plugin)
    {
        LogManager::getSingleton().logMessage(::std::format("Installing plugin: {}", plugin->getName()));

        mPlugins.push_back(plugin);
        plugin->install();

        // if rendersystem is already initialised, call rendersystem init too
        if (mIsInitialised)
        {
            plugin->initialise();
        }

        LogManager::getSingleton().logMessage("Plugin successfully installed");
    }
    //---------------------------------------------------------------------
    void Root::uninstallPlugin(Plugin* plugin)
    {
        LogManager::getSingleton().logMessage(::std::format("Uninstalling plugin: {}", plugin->getName()));
        auto i =
            std::ranges::find(mPlugins, plugin);
        if (i != mPlugins.end())
        {
            if (mIsInitialised)
                plugin->shutdown();
            plugin->uninstall();
            mPlugins.erase(i);
        }
        LogManager::getSingleton().logMessage("Plugin successfully uninstalled");

    }
    //-----------------------------------------------------------------------
    void Root::loadPlugin(std::string_view pluginName)
    {
    }
    //-----------------------------------------------------------------------
    void Root::unloadPlugin(std::string_view pluginName)
    {

    }
    //-----------------------------------------------------------------------
    auto Root::getTimer() noexcept -> Timer*
    {
        return mTimer.get();
    }
    //-----------------------------------------------------------------------
    void Root::oneTimePostWindowInit()
    {
        // log RenderSystem caps
        mActiveRenderer->getCapabilities()->log(LogManager::getSingleton().getDefaultLog());

        // Background loader
        mResourceBackgroundQueue->initialise();
        mWorkQueue->startup();
        // Initialise material manager
        mMaterialManager->initialise();
        // Init particle systems manager
        mParticleManager->_initialise();
        // Init mesh manager
        MeshManager::getSingleton()._initialise();
        // Init plugins - after window creation so rsys resources available
        initialisePlugins();
        mFirstTimePostWindowInit = true;
    }
    //-----------------------------------------------------------------------
    auto Root::_updateAllRenderTargets() -> bool
    {
        // update all targets but don't swap buffers
        mActiveRenderer->_updateAllRenderTargets(false);
        // give client app opportunity to use queued GPU time
        bool ret = _fireFrameRenderingQueued();
        // block for final swap
        mActiveRenderer->_swapAllRenderTargetBuffers();

        // This belongs here, as all render targets must be updated before events are
        // triggered, otherwise targets could be mismatched.  This could produce artifacts,
        // for instance, with shadows.
        for (const auto & it : getSceneManagers())
            it.second->_handleLodEvents();

        return ret;
    }
    //---------------------------------------------------------------------
    auto Root::_updateAllRenderTargets(FrameEvent& evt) -> bool
    {
        // update all targets but don't swap buffers
        mActiveRenderer->_updateAllRenderTargets(false);
        // give client app opportunity to use queued GPU time
        bool ret = _fireFrameRenderingQueued(evt);
        // block for final swap
        mActiveRenderer->_swapAllRenderTargetBuffers();

        // This belongs here, as all render targets must be updated before events are
        // triggered, otherwise targets could be mismatched.  This could produce artifacts,
        // for instance, with shadows.
        for (const auto & it : getSceneManagers())
            it.second->_handleLodEvents();

        return ret;
    }
    //-----------------------------------------------------------------------
    void Root::clearEventTimes()
    {
        // Clear event times
        for(auto & mEventTime : mEventTimes)
            mEventTime.clear();
    }
    //---------------------------------------------------------------------
    void Root::addMovableObjectFactory(MovableObjectFactory* fact,
        bool overrideExisting)
    {
        auto facti = mMovableObjectFactoryMap.find(
            fact->getType());
        if (!overrideExisting && facti != mMovableObjectFactoryMap.end())
        {
            OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM,
                ::std::format("A factory of type '{}' already exists.", fact->getType() ),
                "Root::addMovableObjectFactory");
        }

        if (fact->requestTypeFlags())
        {
            if (facti != mMovableObjectFactoryMap.end() && facti->second->requestTypeFlags())
            {
                // Copy type flags from the factory we're replacing
                fact->_notifyTypeFlags(facti->second->getTypeFlags());
            }
            else
            {
                // Allocate new
                fact->_notifyTypeFlags(_allocateNextMovableObjectTypeFlag());
            }
        }

        // Save
        mMovableObjectFactoryMap[fact->getType()] = fact;

        LogManager::getSingleton().logMessage(std::format("MovableObjectFactory for type '{}' registered.", fact->getType()));

    }
    //---------------------------------------------------------------------
    auto Root::hasMovableObjectFactory(std::string_view typeName) const -> bool
    {
        return !(mMovableObjectFactoryMap.find(typeName) == mMovableObjectFactoryMap.end());
    }
    //---------------------------------------------------------------------
    auto Root::getMovableObjectFactory(std::string_view typeName) -> MovableObjectFactory*
    {
        auto i =
            mMovableObjectFactoryMap.find(typeName);
        if (i == mMovableObjectFactoryMap.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                ::std::format("MovableObjectFactory of type {} does not exist", typeName ),
                "Root::getMovableObjectFactory");
        }
        return i->second;
    }
    //---------------------------------------------------------------------
    auto Root::_allocateNextMovableObjectTypeFlag() -> QueryTypeMask
    {
        if (mNextMovableObjectTypeFlag == QueryTypeMask::USER_LIMIT)
        {
            OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM,
                "Cannot allocate a type flag since "
                "all the available flags have been used.",
                "Root::_allocateNextMovableObjectTypeFlag");

        }
        auto ret = mNextMovableObjectTypeFlag;
        mNextMovableObjectTypeFlag <<= 1;
        return ret;

    }
    //---------------------------------------------------------------------
    void Root::removeMovableObjectFactory(MovableObjectFactory* fact)
    {
        auto i = mMovableObjectFactoryMap.find(
            fact->getType());
        if (i != mMovableObjectFactoryMap.end())
        {
            mMovableObjectFactoryMap.erase(i);
        }

    }

    //---------------------------------------------------------------------
    void Root::setWorkQueue(WorkQueue* queue)
    {
        if (mWorkQueue.get() != queue)
        {
            mWorkQueue.reset(queue);
            if (mIsInitialised)
                mWorkQueue->startup();

        }
    }
}
