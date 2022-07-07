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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_GENERATOR_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_GENERATOR_H

#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreMaterial.hpp"
#include "OgreMaterialSerializer.hpp"
#include "OgrePass.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreSceneManager.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderScriptTranslator.hpp"
#include "OgreSingleton.hpp"

namespace Ogre {
class AutoParamDataSource;
class FileSystemLayer;
class Renderable;
class ScriptTranslator;
class Technique;
class TextureUnitState;
class Viewport;


namespace RTShader {

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

class SGRenderObjectListener;
class SGSceneManagerListener;
class SGScriptTranslatorManager;
class SGResourceGroupListener;
class FFPRenderStateBuilder;
class ProgramManager;
class ProgramWriterManager;
class RenderState;
class SGMaterialSerializerListener;
class SubRenderState;
class SubRenderStateFactory;

/** Shader generator system main interface. This singleton based class
enables automatic generation of shader code based on existing material techniques.
*/
class ShaderGenerator : public Singleton<ShaderGenerator>, public RTShaderSystemAlloc
{
// Interface.
public:

    /** 
    Initialize the Shader Generator System.
    Return true upon success.
    */
    static auto initialize() -> bool;

    /** 
    Destroy the Shader Generator instance.
    */
    static void destroy();


    /** Override standard Singleton retrieval.
    @remarks
    Why do we do this? Well, it's because the Singleton
    implementation is in a .h file, which means it gets compiled
    into anybody who includes it. This is needed for the
    Singleton template to work, but we actually only want it
    compiled into the implementation of the class based on the
    Singleton, not all of them. If we don't change this, we get
    link errors when trying to use the Singleton-based class from
    an outside dll.
    @par
    This method just delegates to the template version anyway,
    but the implementation stays in this single compilation unit,
    preventing link errors.
    */
    static auto getSingleton() noexcept -> ShaderGenerator&; 


    /// @copydoc Singleton::getSingleton()
    static auto getSingletonPtr() noexcept -> ShaderGenerator*;

    /** 
    Add a scene manager to the shader generator scene managers list.
    @param sceneMgr The scene manager to add to the list.
    */
    void addSceneManager(SceneManager* sceneMgr);

    /** 
    Remove a scene manager from the shader generator scene managers list.
    @param sceneMgr The scene manager to remove from the list.
    */
    void removeSceneManager(SceneManager* sceneMgr);

    /** 
    Get the active scene manager that is doint the actual scene rendering.
    This attribute will be update on the call to preFindVisibleObjects. 
    */
    auto getActiveSceneManager() noexcept -> SceneManager*;

    /** 
    Set the active scene manager against which new render states are compiled.
    Note that normally the setting of the active scene manager is updated through the
    preFindVisibleObjects method.
    */

    void _setActiveSceneManager(SceneManager* sceneManager);

    /** 
    Set the target shader language.
    @param shaderLanguage The output shader language to use.
    @remarks The default shader language is cg.
    */
    void setTargetLanguage(std::string_view shaderLanguage);

    /** 
    Return the target shader language currently in use.     
    */
    [[nodiscard]] auto getTargetLanguage() const noexcept -> std::string_view{ return mShaderLanguage; }

    /** 
    Set the output shader target profiles.
    @param type shader type
    @param shaderProfiles The target profiles for the shader.
    */
    void setShaderProfiles(GpuProgramType type, std::string_view shaderProfiles);

    /** 
    Get the output shader target profiles.
    */
    [[nodiscard]] auto getShaderProfiles(GpuProgramType type) const -> std::string_view;

    /** 
    Set the output shader cache path. Generated shader code will be written to this path.
    In case of empty cache path shaders will be generated directly from system memory.
    @param cachePath The cache path of the shader.  
    The default is empty cache path.
    */
    void setShaderCachePath(std::string_view cachePath);

    /** 
    Get the output shader cache path.
    */
    [[nodiscard]] auto getShaderCachePath() const noexcept -> std::string_view{ return mShaderCachePath; }

    /** 
    Flush the shader cache. This operation will cause all active schemes to be invalidated and will
    destroy any CPU/GPU program that created by this shader generator.
    */
    void flushShaderCache();

    /** 
    Return a global render state associated with the given scheme name.
    Modifying this render state will affect all techniques that belongs to that scheme.
    This is the best way to apply global changes to all techniques.
    After altering the render state one should call invalidateScheme method in order to 
    regenerate shaders.
    @param schemeName The destination scheme name.
    */
    auto getRenderState(std::string_view schemeName) -> RenderState*;


    using RenderStateCreateOrRetrieveResult = std::pair<RenderState *, bool>;
    /** 
    Returns a requested render state. If the render state does not exist this function creates it.
    @param schemeName The scheme name to retrieve.
    */
    auto createOrRetrieveRenderState(std::string_view schemeName) -> RenderStateCreateOrRetrieveResult;


    /** 
    Tells if a given render state exists
    @param schemeName The scheme name to check.
    */
    [[nodiscard]] auto hasRenderState(std::string_view schemeName) const -> bool;
    
    /**
     Get render state of specific pass.
     Using this method allows the user to customize the behavior of a specific pass.
     @param schemeName The destination scheme name.
     @param materialName The specific material name.
     @param groupName The specific material name.
     @param passIndex The pass index.
     */
    auto getRenderState(std::string_view schemeName, std::string_view materialName, std::string_view groupName, unsigned short passIndex) -> RenderState*;

    /// @overload
    auto getRenderState(std::string_view schemeName, const Material& mat, uint16 passIndex = 0) -> RenderState*
    {
        return getRenderState(schemeName, mat.getName(), mat.getGroup(), passIndex);
    }

    /** 
    Add sub render state factory. Plugins or 3d party applications may implement sub classes of
    SubRenderState interface. Add the matching factory will allow the application to create instances 
    of these sub classes.
    @param factory The factory to add.
    */
    void addSubRenderStateFactory(SubRenderStateFactory* factory);

    /** 
    Returns the number of existing factories
    */
    [[nodiscard]] auto getNumSubRenderStateFactories() const -> size_t;

    /** 
    Returns a sub render state factory by index
    @note index must be lower than the value returned by getNumSubRenderStateFactories()
    */
    auto getSubRenderStateFactory(size_t index) -> SubRenderStateFactory*;

    /** 
    Returns a sub render state factory by name
    */
    auto getSubRenderStateFactory(std::string_view type) -> SubRenderStateFactory*;

    /** 
    Remove sub render state factory. 
    @param factory The factory to remove.
    */
    void removeSubRenderStateFactory(SubRenderStateFactory* factory);

    /** 
    Create an instance of sub render state from a given type. 
    @param type The type of sub render state to create.
    */
    auto createSubRenderState(std::string_view type) -> SubRenderState*;

    /// @overload
    template<typename T>
    auto createSubRenderState() -> T*
    {
        return static_cast<T*>(createSubRenderState(T::Type));
    }

    /** 
    Destroy an instance of sub render state. 
    @param subRenderState The instance to destroy.
    */
    void destroySubRenderState(SubRenderState* subRenderState);

    /**
     Checks if a shader based technique has been created for a given technique.
     Return true if exist. False if not.
     @param materialName The source material name.
     @param groupName The source group name.
     @param srcTechniqueSchemeName The source technique scheme name.
     @param dstTechniqueSchemeName The destination shader based technique scheme name.
     */
    [[nodiscard]] auto hasShaderBasedTechnique(std::string_view materialName, std::string_view groupName, std::string_view srcTechniqueSchemeName, std::string_view dstTechniqueSchemeName) const -> bool;

    /// @overload
    [[nodiscard]] auto hasShaderBasedTechnique(const Material& mat, std::string_view srcTechniqueSchemeName, std::string_view dstTechniqueSchemeName) const -> bool
    {
        return hasShaderBasedTechnique(mat.getName(), mat.getGroup(), srcTechniqueSchemeName, dstTechniqueSchemeName);
    }

    /**
    Create shader based technique from a given technique.
    Return true upon success. Failure may occur if the source technique is not FFP pure, or different
    source technique is mapped to the requested destination scheme.
    @param srcMat The source material.
    @param srcTechniqueSchemeName The source technique scheme name.
    @param dstTechniqueSchemeName The destination shader based technique scheme name.
    @param overProgrammable If true a shader will be created even if the pass already has shaders
    */
    auto createShaderBasedTechnique(const Material& srcMat, std::string_view srcTechniqueSchemeName, std::string_view dstTechniqueSchemeName, bool overProgrammable = false) -> bool;

    /// @overload
    auto createShaderBasedTechnique(const Technique* srcTech, std::string_view dstTechniqueSchemeName, bool overProgrammable = false) -> bool;

    /**
     Remove shader based technique from a given technique.
     Return true upon success. Failure may occur if the given source technique was not previously
     registered successfully using the createShaderBasedTechnique method.
     @param srcTech The source technique.
     @param dstTechniqueSchemeName The destination shader based technique scheme name.
     */
    auto removeShaderBasedTechnique(const Technique* srcTech, std::string_view dstTechniqueSchemeName) -> bool;

    /** 
    Remove all shader based techniques of the given material. 
    Return true upon success.
    @param materialName The source material name.   
    @param groupName The source group name. 
    */
    auto removeAllShaderBasedTechniques(std::string_view materialName, std::string_view groupName OGRE_RESOURCE_GROUP_INIT) -> bool;

    /// @overload
    auto removeAllShaderBasedTechniques(const Material& mat) -> bool
    {
        return removeAllShaderBasedTechniques(mat.getName(), mat.getGroup());
    }

    /** 
    Clone all shader based techniques from one material to another.
    This function can be used in conjunction with the Material::clone() function to copy 
    both material properties and RTSS state from one material to another.
    @param srcMat The source material
    @param dstMat The destination material
    @return True if successful
    */
    auto cloneShaderBasedTechniques(const Material& srcMat, Material& dstMat) -> bool;

    /** 
    Remove all shader based techniques that created by this shader generator.   
    */
    void removeAllShaderBasedTechniques();

    /** 
    Create a scheme.
    @param schemeName The scheme name to create.
    */
    void createScheme(std::string_view schemeName);

    /** 
    Invalidate a given scheme. This action will lead to shader regeneration of all techniques belongs to the
    given scheme name.
    @param schemeName The scheme to invalidate.
    */
    void invalidateScheme(std::string_view schemeName);

    /** 
    Validate a given scheme. This action will generate shader programs for all techniques of the
    given scheme name.
    @param schemeName The scheme to validate.
    */
    auto validateScheme(std::string_view schemeName) -> bool;
    
    /** 
    Invalidate specific material scheme. This action will lead to shader regeneration of the technique belongs to the
    given scheme name.
    @param schemeName The scheme to invalidate.
    @param materialName The material to invalidate.
    @param groupName The source group name. 
    */
    void invalidateMaterial(std::string_view schemeName, std::string_view materialName, std::string_view groupName OGRE_RESOURCE_GROUP_INIT);

    /// @overload
    void invalidateMaterial(std::string_view schemeName, const Material& mat)
    {
        invalidateMaterial(schemeName, mat.getName(), mat.getGroup());
    }

    /** 
    Validate specific material scheme. This action will generate shader programs for the technique of the
    given scheme name.
    @param schemeName The scheme to validate.
    @param materialName The material to validate.
    @param groupName The source group name. 
    */
    auto validateMaterial(std::string_view schemeName, std::string_view materialName, std::string_view groupName OGRE_RESOURCE_GROUP_INIT) -> bool;

    /// @overload
    void validateMaterial(std::string_view schemeName, const Material& mat)
    {
        validateMaterial(schemeName, mat.getName(), mat.getGroup());
    }

	/**
	Invalidate specific material scheme. This action will lead to shader regeneration of the technique belongs to the
	given scheme name.
	@param schemeName The scheme to invalidate.
	@param materialName The material to invalidate.
	@param groupName The source group name.
	*/
    void invalidateMaterialIlluminationPasses(std::string_view schemeName, std::string_view materialName, std::string_view groupName OGRE_RESOURCE_GROUP_INIT);

	/**
	Validate specific material scheme. This action will generate shader programs illumination passes of the technique of the
	given scheme name.
	@param schemeName The scheme to validate.
	@param materialName The material to validate.
	@param groupName The source group name.
	*/
	auto validateMaterialIlluminationPasses(std::string_view schemeName, std::string_view materialName, std::string_view groupName OGRE_RESOURCE_GROUP_INIT) -> bool;

    /** 
    Return custom material Serializer of the shader generator.
    This is useful when you'd like to export certain material that contains shader generator effects.
    I.E - when writing an exporter you may want mark your material as shader generated material 
    so in the next time you will load it by your application it will automatically generate shaders with custom
    attributes you wanted. To do it you'll have to do the following steps:
    1. Create shader based technique for you material via the createShaderBasedTechnique() method.
    2. Create MaterialSerializer instance.
    3. Add the return instance of serializer listener to the MaterialSerializer.
    4. Call one of the export methods of MaterialSerializer.
    */
    auto getMaterialSerializerListener() noexcept -> MaterialSerializer::Listener*;

    /** Return the current number of generated shaders. */
    [[nodiscard]] auto getShaderCount(GpuProgramType type) const -> size_t;

    /** Set the vertex shader outputs compaction policy. 
    @see VSOutputCompactPolicy.
    @param policy The policy to set.
    */
    void setVertexShaderOutputsCompactPolicy(VSOutputCompactPolicy policy)  { mVSOutputCompactPolicy = policy; }
    
    /** Get the vertex shader outputs compaction policy. 
    @see VSOutputCompactPolicy. 
    */
    [[nodiscard]] auto getVertexShaderOutputsCompactPolicy() const noexcept -> VSOutputCompactPolicy { return mVSOutputCompactPolicy; }


    /** Sets whether shaders are created for passes with shaders.
    Note that this only refers to when the system parses the materials itself.
    Not for when calling the createShaderBasedTechnique() function directly
    @param value The value to set this attribute pass.  
    */
    void setCreateShaderOverProgrammablePass(bool value) { mCreateShaderOverProgrammablePass = value; }

    /** Returns whether shaders are created for passes with shaders.
    @see setCreateShaderOverProgrammablePass(). 
    */
    [[nodiscard]] auto getCreateShaderOverProgrammablePass() const noexcept -> bool { return mCreateShaderOverProgrammablePass; }


    /** Returns the amount of schemes used in the for RT shader generation
    */
    [[nodiscard]] auto getRTShaderSchemeCount() const -> size_t;

    /** Returns the scheme name used in the for RT shader generation by index
    */
    [[nodiscard]] auto getRTShaderScheme(size_t index) const -> std::string_view;

    /// Default material scheme of the shader generator.
    static String DEFAULT_SCHEME_NAME;

private:
    class SGPass;
    class SGTechnique;
    class SGMaterial;
    class SGScheme;

    using MatGroupPair = std::pair<String, String>;
    struct MatGroupPair_less
    {
        // ensure we arrange the list first by material name then by group name
        auto operator()(const MatGroupPair& p1, const MatGroupPair& p2) const -> bool
        {
            int cmpVal = strcmp(p1.first.c_str(),p2.first.c_str());
            return (cmpVal < 0) || ((cmpVal == 0) && (strcmp(p1.second.c_str(),p2.second.c_str()) < 0));
        }
    };

    using SGPassList = std::vector<SGPass *>;
    using SGPassIterator = SGPassList::iterator;
    using SGPassConstIterator = SGPassList::const_iterator;

    using SGTechniqueList = std::vector<SGTechnique *>;
    using SGTechniqueIterator = SGTechniqueList::iterator;
    using SGTechniqueConstIterator = SGTechniqueList::const_iterator;

    using SGTechniqueMap = std::map<SGTechnique *, SGTechnique *>;
    using SGTechniqueMapIterator = SGTechniqueMap::iterator;
    
    using SGMaterialMap = std::map<MatGroupPair, SGMaterial *, MatGroupPair_less>;
    using SGMaterialIterator = SGMaterialMap::iterator;
    using SGMaterialConstIterator = SGMaterialMap::const_iterator;

    using SGSchemeMap = std::map<std::string_view, SGScheme *>;
    using SGSchemeIterator = SGSchemeMap::iterator;
    using SGSchemeConstIterator = SGSchemeMap::const_iterator;

    using SGScriptTranslatorMap = std::map<uint32, ScriptTranslator *>;
    using SGScriptTranslatorIterator = SGScriptTranslatorMap::iterator;
    using SGScriptTranslatorConstIterator = SGScriptTranslatorMap::const_iterator;


    
    /** Shader generator pass wrapper class. */
    class SGPass : public RTShaderSystemAlloc
    {
    public:
		SGPass(SGTechnique* parent, Pass* srcPass, Pass* dstPass, IlluminationStage stage);
        ~SGPass();
    
        /** Build the render state and acquire the CPU/GPU programs */
        void buildTargetRenderState();

        /** Get source pass. */
        auto getSrcPass() noexcept -> Pass* { return mSrcPass; }

        /** Get destination pass. */
        auto getDstPass() noexcept -> Pass* { return mDstPass; }

		/** Get illumination stage. */
		auto getIlluminationStage() noexcept -> IlluminationStage { return mStage; }

		/** Get illumination state. */
		auto isIlluminationPass() noexcept -> bool { return mStage != IS_UNKNOWN; }

        /** Get custom render state of this pass. */
        auto getCustomRenderState() noexcept -> RenderState* { return mCustomRenderState; }

        /** Set the custom render state of this pass. */
        void setCustomRenderState(RenderState* customRenderState) { mCustomRenderState = customRenderState; }

        [[nodiscard]] auto getParent() const noexcept -> const SGTechnique* { return mParent; }
    protected:
        // Parent technique.
        SGTechnique* mParent;
        // Source pass.
        Pass* mSrcPass;
        // Destination pass.
        Pass* mDstPass;
		// Illumination stage
		IlluminationStage mStage;
        // Custom render state.
        RenderState* mCustomRenderState;
    };

    
    /** Shader generator technique wrapper class. */
    class SGTechnique : public RTShaderSystemAlloc
    {
    public:
        SGTechnique(SGMaterial* parent, const Technique* srcTechnique,
                    std::string_view dstTechniqueSchemeName, bool overProgrammable);
        ~SGTechnique();
        
        /** Get the parent SGMaterial */
        [[nodiscard]] auto getParent() const noexcept -> const SGMaterial* { return mParent; }
        
        /** Get the source technique. */
        auto getSourceTechnique() noexcept -> const Technique* { return mSrcTechnique; }

        /** Get the destination technique. */
        auto getDestinationTechnique() noexcept -> Technique* { return mDstTechnique; }

        /** Get the destination technique scheme name. */
        [[nodiscard]] auto getDestinationTechniqueSchemeName() const noexcept -> std::string_view{ return mDstTechniqueSchemeName; }
        
        /** Build the render state. */
        void buildTargetRenderState();

		/** Build the render state for illumination passes. */
		void buildIlluminationTargetRenderState();

		/** Destroy the illumination passes entries. */
		void destroyIlluminationSGPasses();

        /** Release the CPU/GPU programs of this technique. */
        void releasePrograms();

        /** Tells the technique that it needs to generate shader code. */
        void setBuildDestinationTechnique(bool buildTechnique)  { mBuildDstTechnique = buildTechnique; }        

        /** Tells if the destination technique should be build. */
        [[nodiscard]] auto getBuildDestinationTechnique() const noexcept -> bool { return mBuildDstTechnique; }

        /** Get render state of specific pass.
        @param passIndex The pass index.
        */
        auto getRenderState(unsigned short passIndex) -> RenderState*;
        /** Tells if a custom render state exists for the given pass. */
        auto hasRenderState(unsigned short passIndex) -> bool;

        /// whether shaders are created for passes with shaders
        auto overProgrammablePass() noexcept -> bool { return mOverProgrammable; }

        [[nodiscard]] auto getPassList() const noexcept -> const SGPassList& { return mPassEntries; }

        // Key name for associating with a Technique instance.
        static String UserKey;

    protected:
        
        /** Create the passes entries. */
        void createSGPasses();

		/** Create the illumination passes entries. */
		void createIlluminationSGPasses();

        /** Destroy the passes entries. */
        void destroySGPasses();
        
    protected:
        // Parent material.     
        SGMaterial* mParent;
        // Source technique.
        const Technique* mSrcTechnique;
        // Destination technique.
        Technique* mDstTechnique{nullptr};
		// All passes entries, both normal and illumination.
        SGPassList mPassEntries;
        // The custom render states of all passes.
        using RenderStateList = std::vector<RenderState *>;
        RenderStateList mCustomRenderStates;
        // Flag that tells if destination technique should be build.        
        bool mBuildDstTechnique{true};
        // Scheme name of destination technique.
        String mDstTechniqueSchemeName;
        bool mOverProgrammable;
    };

    
    /** Shader generator material wrapper class. */
    class SGMaterial : public RTShaderSystemAlloc
    {   
    
    public:
        /** Class constructor. */
        SGMaterial(std::string_view materialName, std::string_view groupName) : mName(materialName), mGroup(groupName)
        {}

        /** Get the material name. */
        [[nodiscard]] auto getMaterialName() const noexcept -> std::string_view{ return mName; }
        
        /** Get the group name. */
        [[nodiscard]] auto getGroupName() const noexcept -> std::string_view{ return mGroup; }

        /** Get the const techniques list of this material. */
        [[nodiscard]] auto getTechniqueList() const noexcept -> const SGTechniqueList& { return mTechniqueEntries; }

        /** Get the techniques list of this material. */
        auto getTechniqueList() noexcept -> SGTechniqueList& { return mTechniqueEntries; }
    
    protected:
        // The material name.
        String mName;
        // The group name.
        String mGroup;
        // All passes entries.
        SGTechniqueList mTechniqueEntries;
    };

    
    /** Shader generator scheme class. */
    class SGScheme : public RTShaderSystemAlloc
    {   
    public:
        SGScheme(std::string_view schemeName);
        ~SGScheme();    


        /** Return true if this scheme dose not contains any techniques.
        */
        [[nodiscard]] auto empty() const -> bool  { return mTechniqueEntries.empty(); }
        
        /** Invalidate the whole scheme.
        @see ShaderGenerator::invalidateScheme.
        */
        void invalidate();

        /** Validate the whole scheme.
        @see ShaderGenerator::validateScheme.
        */
        void validate();

        /** Invalidate specific material.
        @see ShaderGenerator::invalidateMaterial.
        */
        void invalidate(std::string_view materialName, std::string_view groupName);

        /** Validate specific material.
        @see ShaderGenerator::validateMaterial.
        */
        auto validate(std::string_view materialName, std::string_view groupName) -> bool;

		/** Validate illumination passes of the specific material.
		@see ShaderGenerator::invalidateMaterialIlluminationPasses.
		*/
        void invalidateIlluminationPasses(std::string_view materialName, std::string_view groupName);

		/** Validate illumination passes of the specific material.
		@see ShaderGenerator::validateMaterialIlluminationPasses.
		*/
        auto validateIlluminationPasses(std::string_view materialName, std::string_view groupName) -> bool;

        /** Add a technique to current techniques list. */
        void addTechniqueEntry(SGTechnique* techEntry);

        /** Remove a technique from the current techniques list. */
        void removeTechniqueEntry(SGTechnique* techEntry);


        /** Get global render state of this scheme. 
        @see ShaderGenerator::getRenderState.
        */
        auto getRenderState() noexcept -> RenderState*;

        /** Get specific pass render state. 
        @see ShaderGenerator::getRenderState.
        */
        auto getRenderState(std::string_view materialName, std::string_view groupName, unsigned short passIndex) -> RenderState*;

    protected:
        /** Synchronize the current light settings of this scheme with the current settings of the scene. */
        void synchronizeWithLightSettings();

        /** Synchronize the fog settings of this scheme with the current settings of the scene. */
        void synchronizeWithFogSettings();


    protected:
        // Scheme name.
        String mName;
        // Technique entries.
        SGTechniqueList mTechniqueEntries;
        // Tells if this scheme is out of date.
        bool mOutOfDate{true};
        // The global render state of this scheme.
        std::unique_ptr<RenderState> mRenderState;
        // Current fog mode.
        FogMode mFogMode{FOG_NONE};
    };

    //-----------------------------------------------------------------------------
    using SubRenderStateFactoryMap = std::map<std::string_view, SubRenderStateFactory *>;
    using SubRenderStateFactoryIterator = SubRenderStateFactoryMap::iterator;
    using SubRenderStateFactoryConstIterator = SubRenderStateFactoryMap::const_iterator;

    //-----------------------------------------------------------------------------
    using SceneManagerMap = std::set<SceneManager *>;
    using SceneManagerIterator = SceneManagerMap::iterator;
    using SceneManagerConstIterator = SceneManagerMap::const_iterator;

    friend class SGRenderObjectListener;
    friend class SGSceneManagerListener;

    /** Class default constructor */
    ShaderGenerator();

    /** Class destructor */
    ~ShaderGenerator();

    /** Initialize the shader generator instance. */
    auto _initialize() -> bool;
    
    /** Destory the shader generator instance. */
    void _destroy();
 
    /** Called from the sub class of the RenderObjectLister when single object is rendered. */
    void notifyRenderSingleObject(Renderable* rend, const Pass* pass,  const AutoParamDataSource* source, const LightList* pLightList, bool suppressRenderStateChanges);

    /** Called from the sub class of the SceneManager::Listener when finding visible object process starts. */
    void preFindVisibleObjects(SceneManager* source, SceneManager::IlluminationRenderStage irs, Viewport* v);

    /** Create sub render state core extensions factories */
    void createBuiltinSRSFactories();

    /** Destroy sub render state core extensions factories */
    void destroyBuiltinSRSFactories();

    /** Create an instance of the SubRenderState based on script properties using the
    current sub render state factories.
    @see SubRenderStateFactory::createInstance  
    @param compiler The compiler instance.
    @param prop The abstract property node.
    @param pass The pass that is the parent context of this node.
    @param translator The translator for the specific SubRenderState
    */
    auto createSubRenderState(ScriptCompiler* compiler, PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) -> SubRenderState*;
    
    /** Create an instance of the SubRenderState based on script properties using the
    current sub render state factories.
    @see SubRenderStateFactory::createInstance  
    @param compiler The compiler instance.
    @param prop The abstract property node.
    @param texState The texture unit state that is the parent context of this node.
    @param translator The translator for the specific SubRenderState
    */
    auto createSubRenderState(ScriptCompiler* compiler, PropertyAbstractNode* prop, TextureUnitState* texState, SGScriptTranslator* translator) -> SubRenderState*;

    /** Return a matching script translator. */
    auto getTranslator(const AbstractNodePtr& node) -> ScriptTranslator*;


    /** This method called by instance of SGMaterialSerializerListener and 
    serialize a given pass entry attributes.
    @param ser The material serializer.
    @param passEntry The SGPass instance.
    */
    void serializePassAttributes(MaterialSerializer* ser, SGPass* passEntry);

    /** This method called by instance of SGMaterialSerializerListener and 
    serialize a given textureUnitState entry attributes.
    @param ser The material serializer.
    @param passEntry The SGPass instance.
    @param srcTextureUnit The TextureUnitState being serialized.
    */
    void serializeTextureUnitStateAttributes(MaterialSerializer* ser, SGPass* passEntry, const TextureUnitState* srcTextureUnit);

    /** Finds an entry iterator in the mMaterialEntriesMap map.
    This function is able to find materials with group specified as 
    AUTODETECT_RESOURCE_GROUP_NAME 
    */
    auto findMaterialEntryIt(std::string_view materialName, std::string_view groupName) -> SGMaterialIterator;
    [[nodiscard]] auto findMaterialEntryIt(std::string_view materialName, std::string_view groupName) const -> SGMaterialConstIterator;


    using SchemeCreateOrRetrieveResult = std::pair<SGScheme *, bool>;
    /** 
    Returns a requested scheme. If the scheme does not exist this function creates it.
    @param schemeName The scheme name to retrieve.
    */
    auto createOrRetrieveScheme(std::string_view schemeName) -> SchemeCreateOrRetrieveResult;

    /** Used to check if finalizing */
    [[nodiscard]] auto getIsFinalizing() const noexcept -> bool;

    /** Internal method that creates list of SGPass instances composing the given material. */
    auto createSGPassList(Material* mat) const -> SGPassList;

    // The active scene manager.
    SceneManager* mActiveSceneMgr{nullptr};
    // A map of all scene managers this generator is bound to.
    SceneManagerMap mSceneManagerMap;
    // Render object listener.
    std::unique_ptr<SGRenderObjectListener> mRenderObjectListener;
    // Scene manager listener.
    std::unique_ptr<SGSceneManagerListener> mSceneManagerListener;
    // Script translator manager.
    std::unique_ptr<SGScriptTranslatorManager> mScriptTranslatorManager;
    // Custom material Serializer listener - allows exporting material that contains shader generated techniques.
    std::unique_ptr<SGMaterialSerializerListener> mMaterialSerializerListener;
    // get notified if materials get dropped
    std::unique_ptr<SGResourceGroupListener> mResourceGroupListener;
    // The core translator of the RT Shader System.
    SGScriptTranslator mCoreScriptTranslator;
    // The target shader language (currently only cg supported).
    String mShaderLanguage;
    // The target vertex shader profile. Will be used as argument for program compilation.
    String mVertexShaderProfiles;
    // The target fragment shader profile. Will be used as argument for program compilation.
    String mFragmentShaderProfiles;
    // Path for caching the generated shaders.
    String mShaderCachePath;
    // Shader program manager.
    std::unique_ptr<ProgramManager> mProgramManager;
    // Shader program writer manager.
    std::unique_ptr<ProgramWriterManager> mProgramWriterManager;
    // File system layer manager.
    FileSystemLayer* mFSLayer{nullptr};
    // Fixed Function Render state builder.
    std::unique_ptr<FFPRenderStateBuilder> mFFPRenderStateBuilder;
    // Material entries map.
    SGMaterialMap mMaterialEntriesMap;
    // Scheme entries map.
    SGSchemeMap mSchemeEntriesMap;
    // All technique entries map.
    SGTechniqueMap mTechniqueEntriesMap;
    // Sub render state registered factories.
    SubRenderStateFactoryMap mSubRenderStateFactories;
    // Sub render state core extension factories.
    std::vector<SubRenderStateFactory*> mBuiltinSRSFactories;
    // True if active view port use a valid SGScheme.
    bool mActiveViewportValid{false};
    // Light count per light type.
    int mLightCount[3];
    // Vertex shader outputs compact policy.
    VSOutputCompactPolicy mVSOutputCompactPolicy{VSOCP_LOW};
    // Tells whether shaders are created for passes with shaders
    bool mCreateShaderOverProgrammablePass{false};
    // A flag to indicate finalizing
    bool mIsFinalizing{false};

    uint32 ID_RT_SHADER_SYSTEM;

    friend class SGPass;
    friend class FFPRenderStateBuilder;
    friend class SGScriptTranslatorManager;
    friend class SGScriptTranslator;
    friend class SGMaterialSerializerListener;
    
};

/** @} */
/** @} */

}
}

#endif

