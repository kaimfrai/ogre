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
#ifndef OGRE_CORE_GPUPROGRAMMANAGER_H
#define OGRE_CORE_GPUPROGRAMMANAGER_H

#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>

// Precompiler options
#include "OgreCommon.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResource.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreResourceManager.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreSingleton.hpp"

namespace Ogre {

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    /** Interface definition for factories of GpuProgram. */
    class GpuProgramFactory : public FactoryAlloc
    {
    public:
        virtual ~GpuProgramFactory() = default;
        /// Get the name of the language this factory creates programs for
        [[nodiscard]]
        virtual auto getLanguage() const -> const String& = 0;
        virtual auto create(ResourceManager* creator, const String& name, ResourceHandle handle,
                                   const String& group, bool isManual, ManualResourceLoader* loader) -> GpuProgram* = 0;
        virtual void destroy(GpuProgram* prog) { delete prog; }
    };


    /** This ResourceManager manages GPU shader programs

        This class not only manages the programs themselves, it also manages the factory
        classes which allow the creation of programs using a variety of
        syntaxes. Plugins can be created which register themselves as program
        factories and as such the engine can be extended to accept virtually any kind of
        program provided a plugin is written.
    */
    class GpuProgramManager : public ResourceManager, public Singleton<GpuProgramManager>
    {
        // silence warnings
        using ResourceManager::load;

        /// Factories capable of creating GpuProgram instances
        using FactoryMap = std::map<String, GpuProgramFactory *>;
        FactoryMap mFactories;

        /// Factory for dealing with programs for languages we can't create
        std::unique_ptr<GpuProgramFactory> mNullFactory;
        /// Factory for unified high-level programs
        std::unique_ptr<GpuProgramFactory> mUnifiedFactory;

        auto getFactory(const String& language) -> GpuProgramFactory*;

    public:

        using SyntaxCodes = std::set<String>;
        using SharedParametersMap = std::map<String, GpuSharedParametersPtr>;

        using Microcode = MemoryDataStreamPtr;

    protected:

        SharedParametersMap mSharedParametersMap;
        std::map<uint32, Microcode> mMicrocodeCache;
        bool mSaveMicrocodesToCache;
        bool mCacheDirty;           // When this is true the cache is 'dirty' and should be resaved to disk.
            
        static auto addRenderSystemToName( const String &  name ) -> String;

        /// Generic create method
        auto createImpl(const String& name, ResourceHandle handle,
            const String& group, bool isManual, ManualResourceLoader* loader,
            const NameValuePairList* createParams) -> Resource* override;
    public:
        GpuProgramManager();
        ~GpuProgramManager() override;

        /// Get a resource by name
        /// @see GpuProgramManager::getResourceByName
        auto getByName(const String& name, const String& group OGRE_RESOURCE_GROUP_INIT) const -> GpuProgramPtr;

        /** Loads a GPU program from a file
        @remarks
            This method creates a new program of the type specified as the second parameter.
            As with all types of ResourceManager, this class will search for the file in
            all resource locations it has been configured to look in.
        @param name The name of the GpuProgram
        @param groupName The name of the resource group
        @param filename The file to load
        @param gptype The type of program to create
        @param syntaxCode The name of the syntax to be used for this program e.g. arbvp1, vs_1_1
        */
        virtual auto load(const String& name, const String& groupName, 
            const String& filename, GpuProgramType gptype, 
            const String& syntaxCode) -> GpuProgramPtr;

        /** Loads a GPU program from a string
        @remarks
            The assembly code must be compatible with this manager - call the 
            getSupportedSyntax method for details of the supported syntaxes 
        @param name The identifying name to give this program, which can be used to
            retrieve this program later with getByName.
        @param groupName The name of the resource group
        @param code A string of assembly code which will form the program to run
        @param gptype The type of program to create.
        @param syntaxCode The name of the syntax to be used for this program e.g. arbvp1, vs_1_1
        */
        virtual auto loadFromString(const String& name, const String& groupName,
            const String& code, GpuProgramType gptype,
            const String& syntaxCode) -> GpuProgramPtr;

        /** Returns the syntaxes that the RenderSystem supports. */
        static auto getSupportedSyntax() -> const SyntaxCodes&;

        /** Returns whether a given syntax code (e.g. "glsl330", "vs_4_0", "arbvp1") is supported. */
        static auto isSyntaxSupported(const String& syntaxCode) -> bool;

        /** Returns whether a given high-level language (e.g. "glsl", "hlsl") is supported. */
        auto isLanguageSupported(const String& lang) const -> bool;

        /** Creates a new GpuProgramParameters instance which can be used to bind
            parameters to your programs.
        @remarks
            Program parameters can be shared between multiple programs if you wish.
        */
        virtual auto createParameters() -> GpuProgramParametersSharedPtr;
        
        /** Create a new, unloaded GpuProgram from a file of assembly. 
        @remarks    
            Use this method in preference to the 'load' methods if you wish to define
            a GpuProgram, but not load it yet; useful for saving memory.
        @par
            This method creates a new program of the type specified as the second parameter.
            As with all types of ResourceManager, this class will search for the file in
            all resource locations it has been configured to look in. 
        @param name The name of the program
        @param groupName The name of the resource group
        @param filename The file to load
        @param syntaxCode The name of the syntax to be used for this program e.g. arbvp1, vs_1_1
        @param gptype The type of program to create
        */
        virtual auto createProgram(const String& name, 
            const String& groupName, const String& filename, 
            GpuProgramType gptype, const String& syntaxCode) -> GpuProgramPtr;

        /** Create a GPU program from a string of assembly code.
        @remarks    
            Use this method in preference to the 'load' methods if you wish to define
            a GpuProgram, but not load it yet; useful for saving memory.
        @par
            The assembly code must be compatible with this manager - call the 
            getSupportedSyntax method for details of the supported syntaxes 
        @param name The identifying name to give this program, which can be used to
            retrieve this program later with getByName.
        @param groupName The name of the resource group
        @param code A string of assembly code which will form the program to run
        @param gptype The type of program to create.
        @param syntaxCode The name of the syntax to be used for this program e.g. arbvp1, vs_1_1
        */
        virtual auto createProgramFromString(const String& name, 
            const String& groupName, const String& code, 
            GpuProgramType gptype, const String& syntaxCode) -> GpuProgramPtr;

        /** General create method, using specific create parameters
            instead of name / value pairs. 
        */
        auto create(const String& name, const String& group,
            GpuProgramType gptype, const String& language, bool isManual = false,
            ManualResourceLoader* loader = nullptr) -> GpuProgramPtr;

        /** Create a new, unloaded GpuProgram.
        @par
            This method creates a new program of the type specified as the second and third parameters.
            You will have to call further methods on the returned program in order to
            define the program fully before you can load it.
        @param name The identifying name of the program
        @param groupName The name of the resource group which this program is
            to be a member of
        @param language Code of the language to use (e.g. "cg")
        @param gptype The type of program to create
        */
        auto createProgram(const String& name, const String& groupName, const String& language,
                                    GpuProgramType gptype) -> GpuProgramPtr
        {
            return create(name, groupName, gptype, language);
        }

        /** Create a new set of shared parameters, which can be used across many 
            GpuProgramParameters objects of different structures.
        @param name The name to give the shared parameters so you can refer to them
            later.
        */
        virtual auto createSharedParameters(const String& name) -> GpuSharedParametersPtr;

        /** Retrieve a set of shared parameters, which can be used across many 
        GpuProgramParameters objects of different structures.
        */
        virtual auto getSharedParameters(const String& name) const -> GpuSharedParametersPtr;

        /** Get (const) access to the available shared parameter sets. 
        */
        virtual auto getAvailableSharedParameters() const -> const SharedParametersMap&;

        /** Get if the microcode of a shader should be saved to a cache
        */
        auto getSaveMicrocodesToCache() const -> bool;
        /** Set if the microcode of a shader should be saved to a cache
        */
        void setSaveMicrocodesToCache( bool val );

        /** Returns true if the microcodecache changed during the run.
        */
        auto isCacheDirty() const -> bool;

        static auto canGetCompiledShaderBuffer() -> bool;
        /** Check if a microcode is available for a program in the microcode cache.
        @param id The id of the program.
        */
        auto isMicrocodeAvailableInCache(uint32 id) const -> bool;

        /** Returns a microcode for a program from the microcode cache.
        @param id The name of the program.
        */
        auto getMicrocodeFromCache(uint32 id) const -> const Microcode&;

        /** Creates a microcode to be later added to the cache.
        @param size The size of the microcode in bytes
        */
        auto createMicrocode( size_t size ) const -> Microcode;

        /** Adds a microcode for a program to the microcode cache.
        @param id The id of the program
        @param microcode the program binary
        */
        void addMicrocodeToCache(uint32 id, const Microcode& microcode);

        /** Removes a microcode for a program from the microcode cache.
        @param id The name of the program.
        */
        void removeMicrocodeFromCache(uint32 id);

        /** Saves the microcode cache to disk.
        @param stream The destination stream
        */
        void saveMicrocodeCache( DataStreamPtr stream ) const;
        /** Loads the microcode cache from disk.
        @param stream The source stream
        */
        void loadMicrocodeCache( DataStreamPtr stream );
        
        /** Add a new factory object for programs of a given language. */
        void addFactory(GpuProgramFactory* factory);
        /** Remove a factory object for programs of a given language. */
        void removeFactory(GpuProgramFactory* factory);

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() -> GpuProgramManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() -> GpuProgramManager*;
    


    };

    /** @} */
    /** @} */
}

#endif
