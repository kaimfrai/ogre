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

#include <cassert>

module Ogre.Core;

import :DataStream;
import :Exception;
import :GpuProgramManager;
import :GpuProgramParams;
import :LogManager;
import :RenderSystem;
import :RenderSystemCapabilities;
import :Root;
import :StreamSerialiser;
import :UnifiedHighLevelGpuProgram;

import <memory>;
import <utility>;

namespace Ogre {
namespace {
    uint32 CACHE_CHUNK_ID = StreamSerialiser::makeIdentifier("OGPC"); // Ogre Gpu Program cache

    String sNullLang = "null";
    class NullProgram : public GpuProgram
    {
    protected:
        /** Internal load implementation, must be implemented by subclasses.
        */
        void loadFromSource() override {}
        void unloadImpl() override {}

    public:
        NullProgram(ResourceManager* creator,
            const String& name, ResourceHandle handle, const String& group,
            bool isManual, ManualResourceLoader* loader)
            : GpuProgram(creator, name, handle, group, isManual, loader){}
        ~NullProgram() override = default;
        /// Overridden from GpuProgram - never supported
        auto isSupported() const -> bool override { return false; }
        /// Overridden from GpuProgram
        auto getLanguage() const -> const String& override { return sNullLang; }
        auto calculateSize() const -> size_t override { return 0; }

        /// Overridden from StringInterface
        auto setParameter(const String& name, const String& value) -> bool
        {
            // always silently ignore all parameters so as not to report errors on
            // unsupported platforms
            return true;
        }

    };
    class NullProgramFactory : public HighLevelGpuProgramFactory
    {
    public:
        NullProgramFactory() = default;
        ~NullProgramFactory() override = default;
        /// Get the name of the language this factory creates programs for
        [[nodiscard]]
        auto getLanguage() const -> const String& override
        {
            return sNullLang;
        }
        auto create(ResourceManager* creator,
            const String& name, ResourceHandle handle,
            const String& group, bool isManual, ManualResourceLoader* loader) -> GpuProgram* override
        {
            return new NullProgram(creator, name, handle, group, isManual, loader);
        }
    };
}

    auto GpuProgramManager::createImpl(const String& name, ResourceHandle handle, const String& group,
                                            bool isManual, ManualResourceLoader* loader,
                                            const NameValuePairList* params) -> Resource*
    {
        auto langIt = params->find("language");
        auto typeIt = params->find("type");

        if(langIt == params->end())
            langIt = params->find("syntax");

        if (!params || langIt == params->end() || typeIt == params->end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "You must supply 'language' or 'syntax' and 'type' parameters");
        }

        auto ret = getFactory(langIt->second)->create(this, name, handle, group, isManual, loader);

        if (typeIt->second == "vertex_program")
        {
            ret->setType(GPT_VERTEX_PROGRAM);
        }
        else if (typeIt->second == "geometry_program")
        {
            ret->setType(GPT_GEOMETRY_PROGRAM);
        }
        else
        {
            ret->setType(GPT_FRAGMENT_PROGRAM);
        }

        return ret;
    }

    //-----------------------------------------------------------------------
    template<> GpuProgramManager* Singleton<GpuProgramManager>::msSingleton = nullptr;
    auto GpuProgramManager::getSingletonPtr() -> GpuProgramManager*
    {
        return msSingleton;
    }
    auto GpuProgramManager::getSingleton() -> GpuProgramManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    auto GpuProgramManager::getByName(const String& name, const String& group) const -> GpuProgramPtr
    {
        return static_pointer_cast<GpuProgram>(getResourceByName(name, group));
    }
    //---------------------------------------------------------------------------
    GpuProgramManager::GpuProgramManager()
    {
        // Loading order
        mLoadOrder = 50.0f;
        // Resource type
        mResourceType = "GpuProgram";
        mSaveMicrocodesToCache = false;
        mCacheDirty = false;

        mNullFactory = std::make_unique<NullProgramFactory>();
        addFactory(mNullFactory.get());
        mUnifiedFactory = std::make_unique<UnifiedHighLevelGpuProgramFactory>();
        addFactory(mUnifiedFactory.get());

        ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
    }
    //---------------------------------------------------------------------------
    GpuProgramManager::~GpuProgramManager()
    {
        ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    }
    //---------------------------------------------------------------------------
    auto GpuProgramManager::load(const String& name,
        const String& groupName, const String& filename, 
        GpuProgramType gptype, const String& syntaxCode) -> GpuProgramPtr
    {
        GpuProgramPtr prg;
        {
            prg = getByName(name, groupName);
            if (!prg)
            {
                prg = createProgram(name, groupName, filename, gptype, syntaxCode);
            }

        }
        prg->load();
        return prg;
    }
    //---------------------------------------------------------------------------
    auto GpuProgramManager::loadFromString(const String& name, 
        const String& groupName, const String& code, 
        GpuProgramType gptype, const String& syntaxCode) -> GpuProgramPtr
    {
        GpuProgramPtr prg;
        {
            prg = getByName(name, groupName);
            if (!prg)
            {
                prg = createProgramFromString(name, groupName, code, gptype, syntaxCode);
            }

        }
        prg->load();
        return prg;
    }
    //---------------------------------------------------------------------------
    auto GpuProgramManager::create(const String& name, const String& group, GpuProgramType gptype,
                                            const String& syntaxCode, bool isManual,
                                            ManualResourceLoader* loader) -> GpuProgramPtr
    {
        auto prg = getFactory(syntaxCode)->create(this, name, getNextHandle(), group, isManual, loader);
        prg->setType(gptype);
        prg->setSyntaxCode(syntaxCode);

        ResourcePtr ret(prg);
        addImpl(ret);
        // Tell resource group manager
        if(ret)
            ResourceGroupManager::getSingleton()._notifyResourceCreated(ret);
        return static_pointer_cast<GpuProgram>(ret);
    }
    //---------------------------------------------------------------------------
    auto GpuProgramManager::createProgram(const String& name, const String& groupName,
                                                   const String& filename, GpuProgramType gptype,
                                                   const String& syntaxCode) -> GpuProgramPtr
    {
        GpuProgramPtr prg = createProgram(name, groupName, syntaxCode, gptype);
        prg->setSourceFile(filename);
        return prg;
    }
    //---------------------------------------------------------------------------
    auto GpuProgramManager::createProgramFromString(const String& name, 
        const String& groupName, const String& code, GpuProgramType gptype, 
        const String& syntaxCode) -> GpuProgramPtr
    {
        GpuProgramPtr prg = createProgram(name, groupName, syntaxCode, gptype);
        prg->setSource(code);
        return prg;
    }
    //---------------------------------------------------------------------------
    auto GpuProgramManager::getSupportedSyntax() -> const GpuProgramManager::SyntaxCodes&
    {
        // Use the current render system
        RenderSystem* rs = Root::getSingleton().getRenderSystem();

        // Get the supported syntaxed from RenderSystemCapabilities 
        return rs->getCapabilities()->getSupportedShaderProfiles();
    }

    //---------------------------------------------------------------------------
    auto GpuProgramManager::isSyntaxSupported(const String& syntaxCode) -> bool
    {
        // Use the current render system
        RenderSystem* rs = Root::getSingleton().getRenderSystem();

        // Get the supported syntax from RenderSystemCapabilities 
        return rs && rs->getCapabilities()->isShaderProfileSupported(syntaxCode);
    }
    //-----------------------------------------------------------------------------
    auto GpuProgramManager::createParameters() -> GpuProgramParametersSharedPtr
    {
        return GpuProgramParametersSharedPtr(new GpuProgramParameters());
    }
    //---------------------------------------------------------------------
    auto GpuProgramManager::createSharedParameters(const String& name) -> GpuSharedParametersPtr
    {
        if (mSharedParametersMap.find(name) != mSharedParametersMap.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
                ::std::format("The shared parameter set '{}' already exists!", name ), 
                "GpuProgramManager::createSharedParameters");
        }
        GpuSharedParametersPtr ret(new GpuSharedParameters(name));
        mSharedParametersMap[name] = ret;
        return ret;
    }
    //---------------------------------------------------------------------
    auto GpuProgramManager::getSharedParameters(const String& name) const -> GpuSharedParametersPtr
    {
        auto i = mSharedParametersMap.find(name);
        if (i == mSharedParametersMap.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
                ::std::format("No shared parameter set with name '{}'!", name ), 
                "GpuProgramManager::getSharedParameters");
        }
        return i->second;
    }
    //---------------------------------------------------------------------
    auto 
    GpuProgramManager::getAvailableSharedParameters() const -> const GpuProgramManager::SharedParametersMap&
    {
        return mSharedParametersMap;
    }
    //---------------------------------------------------------------------
    auto GpuProgramManager::getSaveMicrocodesToCache() const -> bool
    {
        return mSaveMicrocodesToCache;
    }
    //---------------------------------------------------------------------
    auto GpuProgramManager::canGetCompiledShaderBuffer() -> bool
    {
        // Use the current render system
        RenderSystem* rs = Root::getSingleton().getRenderSystem();

        // Check if the supported  
        return rs->getCapabilities()->hasCapability(RSC_CAN_GET_COMPILED_SHADER_BUFFER);
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::setSaveMicrocodesToCache( const bool val )
    {
        // Check that saving shader microcode is supported
        if(!canGetCompiledShaderBuffer())
            mSaveMicrocodesToCache = false;
        else
            mSaveMicrocodesToCache = val;
    }
    //---------------------------------------------------------------------
    auto GpuProgramManager::isCacheDirty( ) const -> bool
    {
        return mCacheDirty;     
    }
    //---------------------------------------------------------------------
    auto GpuProgramManager::addRenderSystemToName( const String & name ) -> String
    {
        // Use the current render system
        RenderSystem* rs = Root::getSingleton().getRenderSystem();

        return  ::std::format("{}_{}", rs->getName(), name);
    }
    //---------------------------------------------------------------------
    auto GpuProgramManager::isMicrocodeAvailableInCache( uint32 id ) const -> bool
    {
        return mMicrocodeCache.find(id) != mMicrocodeCache.end();
    }
    //---------------------------------------------------------------------
    auto GpuProgramManager::getMicrocodeFromCache( uint32 id ) const -> const GpuProgramManager::Microcode &
    {
        return mMicrocodeCache.find(id)->second;
    }
    //---------------------------------------------------------------------
    auto GpuProgramManager::createMicrocode( size_t size ) const -> GpuProgramManager::Microcode
    {   
        return Microcode(new MemoryDataStream(size));  
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::addMicrocodeToCache( uint32 id, const GpuProgramManager::Microcode & microcode )
    {   
        auto foundIter = mMicrocodeCache.find(id);
        if ( foundIter == mMicrocodeCache.end() )
        {
            mMicrocodeCache.insert(make_pair(id, microcode));
            // if cache is modified, mark it as dirty.
            mCacheDirty = true;
        }
        else
        {
            foundIter->second = microcode;

        }       
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::removeMicrocodeFromCache( uint32 id )
    {
        auto foundIter = mMicrocodeCache.find(id);

        if (foundIter != mMicrocodeCache.end())
        {
            mMicrocodeCache.erase( foundIter );
            mCacheDirty = true;
        }
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::saveMicrocodeCache( DataStreamPtr stream ) const
    {
        if (!mCacheDirty)
            return; 

        if (!stream->isWriteable())
        {
            OGRE_EXCEPT(Exception::ERR_CANNOT_WRITE_TO_FILE,
                ::std::format("Unable to write to stream {}", stream->getName()),
                "GpuProgramManager::saveMicrocodeCache");
        }
        
        StreamSerialiser serialiser(stream);
        serialiser.writeChunkBegin(CACHE_CHUNK_ID, 2);

        // write the size of the array
        auto sizeOfArray = static_cast<uint32>(mMicrocodeCache.size());
        serialiser.write(&sizeOfArray);
        
        // loop the array and save it
        for ( const auto& entry : mMicrocodeCache )
        {
            // saves the id of the shader
            serialiser.write(&entry.first);

            // saves the microcode
            const Microcode & microcodeOfShader = entry.second;
            auto microcodeLength = static_cast<uint32>(microcodeOfShader->size());
            serialiser.write(&microcodeLength);
            serialiser.writeData(microcodeOfShader->getPtr(), 1, microcodeLength);
        }

        serialiser.writeChunkEnd(CACHE_CHUNK_ID);
    }
    //---------------------------------------------------------------------
    void GpuProgramManager::loadMicrocodeCache( DataStreamPtr stream )
    {
        mMicrocodeCache.clear();

        StreamSerialiser serialiser(stream);
        const StreamSerialiser::Chunk* chunk;

        try
        {
            chunk = serialiser.readChunkBegin();
        }
        catch (const InvalidStateException& e)
        {
            LogManager::getSingleton().logWarning(
                ::std::format("Could not load Microcode Cache: {}", e.getDescription()));
            return;
        }

        if(chunk->id != CACHE_CHUNK_ID || chunk->version != 2)
        {
            LogManager::getSingleton().logWarning("Invalid Microcode Cache");
            return;
        }
        // write the size of the array
        uint32 sizeOfArray = 0;
        serialiser.read(&sizeOfArray);
        
        // loop the array and load it
        for ( uint32 i = 0 ; i < sizeOfArray ; i++ )
        {
            // loads the id of the shader
            uint32 id;
            serialiser.read(&id);

            // loads the microcode
            uint32 microcodeLength = 0;
            serialiser.read(&microcodeLength);

            Microcode microcodeOfShader(new MemoryDataStream(microcodeLength));
            microcodeOfShader->seek(0);
            serialiser.readData(microcodeOfShader->getPtr(), 1, microcodeLength);

            mMicrocodeCache.insert(make_pair(id, microcodeOfShader));
        }
        serialiser.readChunkEnd(CACHE_CHUNK_ID);

        // if cache is not modified, mark it as clean.
        mCacheDirty = false;
        
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------------
    void GpuProgramManager::addFactory(GpuProgramFactory* factory)
    {
        // deliberately allow later plugins to override earlier ones
        mFactories[factory->getLanguage()] = factory;
    }
    //---------------------------------------------------------------------------
    void GpuProgramManager::removeFactory(GpuProgramFactory* factory)
    {
        // Remove only if equal to registered one, since it might overridden
        // by other plugins
        auto it = mFactories.find(factory->getLanguage());
        if (it != mFactories.end() && it->second == factory)
        {
            mFactories.erase(it);
        }
    }
    //---------------------------------------------------------------------------
    auto GpuProgramManager::getFactory(const String& language) -> GpuProgramFactory*
    {
        auto i = mFactories.find(language);

        if (i == mFactories.end())
        {
            // use the null factory to create programs that will never be supported
            i = mFactories.find(sNullLang);
        }
        return i->second;
    }
    //---------------------------------------------------------------------
    auto GpuProgramManager::isLanguageSupported(const String& lang) const -> bool
    {
        return mFactories.find(lang) != mFactories.end();
    }
}
