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
#include <any>
#include <cassert>
#include <cstdio>
#include <ios>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreException.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramManager.hpp"
#include "OgreLight.hpp"
#include "OgreLog.hpp"
#include "OgreLogManager.hpp"
#include "OgreMaterial.hpp"
#include "OgreMaterialManager.hpp"
#include "OgreMaterialSerializer.hpp"
#include "OgrePass.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderObjectListener.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreSceneManager.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreScriptTranslator.hpp"
#include "OgreShaderCookTorranceLighting.hpp"
#include "OgreShaderExGBuffer.hpp"
#include "OgreShaderExHardwareSkinning.hpp"
#include "OgreShaderExIntegratedPSSM3.hpp"
#include "OgreShaderExLayeredBlending.hpp"
#include "OgreShaderExNormalMapLighting.hpp"
#include "OgreShaderExPerPixelLighting.hpp"
#include "OgreShaderExTriplanarTexturing.hpp"
#include "OgreShaderExWBOIT.hpp"
#include "OgreShaderFFPAlphaTest.hpp"
#include "OgreShaderFFPColour.hpp"
#include "OgreShaderFFPFog.hpp"
#include "OgreShaderFFPLighting.hpp"
#include "OgreShaderFFPRenderStateBuilder.hpp"
#include "OgreShaderFFPTexturing.hpp"
#include "OgreShaderFFPTransform.hpp"
#include "OgreShaderGenerator.hpp"
#include "OgreShaderMaterialSerializerListener.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderProgramManager.hpp"
#include "OgreShaderProgramWriterManager.hpp"
#include "OgreShaderRenderState.hpp"
#include "OgreShaderScriptTranslator.hpp"
#include "OgreShaderSubRenderState.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreSingleton.hpp"
#include "OgreString.hpp"
#include "OgreStringConverter.hpp"
#include "OgreTechnique.hpp"
#include "OgreTextureUnitState.hpp"
#include "OgreUserObjectBindings.hpp"
#include "OgreVector.hpp"
#include "OgreViewport.hpp"

namespace Ogre {
class AutoParamDataSource;
class Renderable;

//-----------------------------------------------------------------------
template<> 
RTShader::ShaderGenerator* Singleton<RTShader::ShaderGenerator>::msSingleton = nullptr;

namespace RTShader {

/** Shader generator RenderObjectListener sub class. */
class SGRenderObjectListener : public RenderObjectListener, public RTShaderSystemAlloc
{
public:
    SGRenderObjectListener(ShaderGenerator* owner) { mOwner = owner; }

    /**
    Listener overridden function notify the shader generator when rendering single object.
    */
    void notifyRenderSingleObject(Renderable* rend, const Pass* pass,
                                          const AutoParamDataSource* source, const LightList* pLightList,
                                          bool suppressRenderStateChanges) override
    {
        mOwner->notifyRenderSingleObject(rend, pass, source, pLightList, suppressRenderStateChanges);
    }

protected:
    ShaderGenerator* mOwner;
};

/** Shader generator scene manager sub class. */
class SGSceneManagerListener : public SceneManager::Listener, public RTShaderSystemAlloc
{
public:
    SGSceneManagerListener(ShaderGenerator* owner) { mOwner = owner; }

    /**
    Listener overridden function notify the shader generator when finding visible objects process started.
    */
    void preFindVisibleObjects(SceneManager* source, SceneManager::IlluminationRenderStage irs,
                                       Viewport* v) override
    {
        mOwner->preFindVisibleObjects(source, irs, v);
    }
protected:
    // The shader generator instance.
    ShaderGenerator* mOwner;
};

/** Shader generator ScriptTranslatorManager sub class. */
class SGScriptTranslatorManager : public ScriptTranslatorManager
{
public:
    SGScriptTranslatorManager(ShaderGenerator* owner) { mOwner = owner; }

    /// Returns a manager for the given object abstract node, or null if it is not supported
    auto getTranslator(const AbstractNodePtr& node) -> ScriptTranslator* override
    {
        return mOwner->getTranslator(node);
    }

protected:
    // The shader generator instance.
    ShaderGenerator* mOwner;
};

class SGResourceGroupListener : public ResourceGroupListener
{
public:
    SGResourceGroupListener(ShaderGenerator* owner) { mOwner = owner; }

    /// sync our internal list if material gets dropped
    void resourceRemove(const ResourcePtr& resource) override
    {
        if (auto mat = dynamic_cast<Material*>(resource.get()))
        {
            mOwner->removeAllShaderBasedTechniques(*mat);
        }
    }

protected:
    // The shader generator instance.
    ShaderGenerator* mOwner;
};

String ShaderGenerator::DEFAULT_SCHEME_NAME     = "ShaderGeneratorDefaultScheme";
String ShaderGenerator::SGTechnique::UserKey    = "SGTechnique";

//-----------------------------------------------------------------------
auto ShaderGenerator::getSingletonPtr() noexcept -> ShaderGenerator*
{
    return msSingleton;
}

//-----------------------------------------------------------------------
auto ShaderGenerator::getSingleton() noexcept -> ShaderGenerator&
{
    assert( msSingleton );  
    return ( *msSingleton );
}

//-----------------------------------------------------------------------------
ShaderGenerator::ShaderGenerator() :
     mShaderLanguage("") 
{
    mLightCount[0]              = 0;
    mLightCount[1]              = 0;
    mLightCount[2]              = 0;

    HighLevelGpuProgramManager& hmgr = HighLevelGpuProgramManager::getSingleton();

    if (hmgr.isLanguageSupported("glsles"))
    {
        mShaderLanguage = "glsles";
    }
    else if (hmgr.isLanguageSupported("glsl"))
    {
        mShaderLanguage = "glsl";
    }
    else if (hmgr.isLanguageSupported("hlsl"))
    {
        mShaderLanguage = "hlsl";
    }
    else if (hmgr.isLanguageSupported("glslang"))
    {
        mShaderLanguage = "glslang";
    }
    else
    {
        mShaderLanguage = "null"; // falling back to HLSL, for unit tests mainly
        LogManager::getSingleton().logWarning("ShaderGenerator: No supported language found. Falling back to 'null'");
    }

    setShaderProfiles(GPT_VERTEX_PROGRAM, "vs_3_0 vs_2_a vs_2_0 vs_1_1");
    setShaderProfiles(GPT_FRAGMENT_PROGRAM, "ps_3_0 ps_2_a ps_2_b ps_2_0 ps_1_4 ps_1_3 ps_1_2 ps_1_1");
}

//-----------------------------------------------------------------------------
ShaderGenerator::~ShaderGenerator()
= default;

//-----------------------------------------------------------------------------
auto ShaderGenerator::initialize() -> bool
{
    if (msSingleton == nullptr)
    {
        msSingleton = new ShaderGenerator;
        if (false == msSingleton->_initialize())
        {
            delete msSingleton;
            msSingleton = nullptr;
            return false;
        }
    }
        
    return true;
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::_initialize() -> bool
{
    // Allocate program writer manager.
    mProgramWriterManager = std::make_unique<ProgramWriterManager>();

    // Allocate program manager.
    mProgramManager = std::make_unique<ProgramManager>();

    // Allocate and initialize FFP render state builder.
    mFFPRenderStateBuilder = std::make_unique<FFPRenderStateBuilder>();

    // Create extensions factories.
    createBuiltinSRSFactories();

    // Allocate script translator manager.
    mScriptTranslatorManager = std::make_unique<SGScriptTranslatorManager>(this);
    ScriptCompilerManager::getSingleton().addTranslatorManager(mScriptTranslatorManager.get());
    ID_RT_SHADER_SYSTEM = ScriptCompilerManager::getSingleton().registerCustomWordId("rtshader_system");

    // Create the default scheme.
    createScheme(DEFAULT_SCHEME_NAME);
	
	mResourceGroupListener = std::make_unique<SGResourceGroupListener>(this);
	ResourceGroupManager::getSingleton().addResourceGroupListener(mResourceGroupListener.get());

    return true;
}



//-----------------------------------------------------------------------------
void ShaderGenerator::createBuiltinSRSFactories()
{
    SubRenderStateFactory* curFactory;

    curFactory = new FFPTransformFactory;
    ShaderGenerator::getSingleton().addSubRenderStateFactory(curFactory);
    mBuiltinSRSFactories.push_back(curFactory);

    curFactory = new FFPColourFactory;
    ShaderGenerator::getSingleton().addSubRenderStateFactory(curFactory);
    mBuiltinSRSFactories.push_back(curFactory);

    curFactory = new FFPLightingFactory;
    ShaderGenerator::getSingleton().addSubRenderStateFactory(curFactory);
    mBuiltinSRSFactories.push_back(curFactory);

    curFactory = new FFPTexturingFactory;
    ShaderGenerator::getSingleton().addSubRenderStateFactory(curFactory);
    mBuiltinSRSFactories.push_back(curFactory);

    curFactory = new FFPFogFactory;
    ShaderGenerator::getSingleton().addSubRenderStateFactory(curFactory);
    mBuiltinSRSFactories.push_back(curFactory);

    curFactory = new FFPAlphaTestFactory;
    ShaderGenerator::getSingleton().addSubRenderStateFactory(curFactory);
    mBuiltinSRSFactories.push_back(curFactory);

    // check if we are running an old shader level in d3d11
    bool d3d11AndLowProfile = ( (GpuProgramManager::getSingleton().isSyntaxSupported("vs_4_0_level_9_1") ||
        GpuProgramManager::getSingleton().isSyntaxSupported("vs_4_0_level_9_3"))
        && !GpuProgramManager::getSingleton().isSyntaxSupported("vs_4_0"));
    if(!d3d11AndLowProfile)
    {
        curFactory = new PerPixelLightingFactory;
        addSubRenderStateFactory(curFactory);
        mBuiltinSRSFactories.push_back(curFactory);

        curFactory = new NormalMapLightingFactory;
        addSubRenderStateFactory(curFactory);
        mBuiltinSRSFactories.push_back(curFactory);

        curFactory = new CookTorranceLightingFactory;
        addSubRenderStateFactory(curFactory);
        mBuiltinSRSFactories.push_back(curFactory);

        curFactory = new IntegratedPSSM3Factory;
        addSubRenderStateFactory(curFactory);
        mBuiltinSRSFactories.push_back(curFactory);

        curFactory = new LayeredBlendingFactory;
        addSubRenderStateFactory(curFactory);
        mBuiltinSRSFactories.push_back(curFactory);

        curFactory = new HardwareSkinningFactory;
        addSubRenderStateFactory(curFactory);
        mBuiltinSRSFactories.push_back(curFactory);
    }
    
    curFactory = new TriplanarTexturingFactory;
    addSubRenderStateFactory(curFactory);
    mBuiltinSRSFactories.push_back(curFactory);

    curFactory = new GBufferFactory;
    addSubRenderStateFactory(curFactory);
    mBuiltinSRSFactories.push_back(curFactory);

    curFactory = new WBOITFactory;
    addSubRenderStateFactory(curFactory);
    mBuiltinSRSFactories.push_back(curFactory);
}

//-----------------------------------------------------------------------------
void ShaderGenerator::destroy()
{
    if (msSingleton != nullptr)
    {
        msSingleton->_destroy();

        delete msSingleton;
        msSingleton = nullptr;
    }
}

//-----------------------------------------------------------------------------
void ShaderGenerator::_destroy()
{
    mIsFinalizing = true;
    
    // Delete technique entries.
    for (auto & itTech : mTechniqueEntriesMap)
    {           
        delete (itTech.second);
    }
    mTechniqueEntriesMap.clear();

    // Delete material entries.
    for (auto & itMat : mMaterialEntriesMap)
    {       
        delete (itMat.second);
    }
    mMaterialEntriesMap.clear();

    // Delete scheme entries.
    for (auto & itScheme : mSchemeEntriesMap)
    {       
        delete (itScheme.second);
    }
    mSchemeEntriesMap.clear();

    // Destroy extensions factories.
    destroyBuiltinSRSFactories();

    mFFPRenderStateBuilder.reset();

    mProgramManager.reset();
    mProgramWriterManager.reset();

    // Delete script translator manager.
    if (mScriptTranslatorManager)
    {
        ScriptCompilerManager::getSingleton().removeTranslatorManager(mScriptTranslatorManager.get());
        mScriptTranslatorManager.reset();
    }

    mMaterialSerializerListener.reset();

    if (mResourceGroupListener)
    {
        ResourceGroupManager::getSingleton().removeResourceGroupListener(mResourceGroupListener.get());
        mResourceGroupListener.reset();
    }

    // Remove all scene managers.   
    while (mSceneManagerMap.empty() == false)
    {
        removeSceneManager(*mSceneManagerMap.begin());
    }

    mRenderObjectListener.reset();
    mSceneManagerListener.reset();
}

//-----------------------------------------------------------------------------
void ShaderGenerator::destroyBuiltinSRSFactories()
{
    for (auto f : mBuiltinSRSFactories)
    {
        removeSubRenderStateFactory(f);
        delete f;
    }
    mBuiltinSRSFactories.clear();
}

//-----------------------------------------------------------------------------
void ShaderGenerator::addSubRenderStateFactory(SubRenderStateFactory* factory)
{
    auto itFind = mSubRenderStateFactories.find(factory->getType());

    if (itFind != mSubRenderStateFactories.end())
    {
        OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM,
            ::std::format("A factory of type '{}' already exists.", factory->getType() ),
            "ShaderGenerator::addSubRenderStateFactory");
    }       
    
    mSubRenderStateFactories[factory->getType()] = factory;
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::getNumSubRenderStateFactories() const -> size_t
{
    return mSubRenderStateFactories.size();
}


//-----------------------------------------------------------------------------
auto  ShaderGenerator::getSubRenderStateFactory(size_t index) -> SubRenderStateFactory*
{
    {
        auto itFind = mSubRenderStateFactories.begin();
        for(; index != 0 && itFind != mSubRenderStateFactories.end(); --index , ++itFind);

        if (itFind != mSubRenderStateFactories.end())
        {
            return itFind->second;
        }
    }

    OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM,
        ::std::format("A factory on index {} does not exist.", index ),
        "ShaderGenerator::addSubRenderStateFactory");
        
    return nullptr;
}
//-----------------------------------------------------------------------------
auto ShaderGenerator::getSubRenderStateFactory(std::string_view type) -> SubRenderStateFactory*
{
    auto itFind = mSubRenderStateFactories.find(type);
    return (itFind != mSubRenderStateFactories.end()) ? itFind->second : NULL;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::removeSubRenderStateFactory(SubRenderStateFactory* factory)
{
    auto itFind = mSubRenderStateFactories.find(factory->getType());

    if (itFind != mSubRenderStateFactories.end())
        mSubRenderStateFactories.erase(itFind);

}

//-----------------------------------------------------------------------------
auto ShaderGenerator::createSubRenderState(std::string_view type) -> SubRenderState*
{
    auto itFind = mSubRenderStateFactories.find(type);

    if (itFind != mSubRenderStateFactories.end())
        return itFind->second->createInstance();


    OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
        ::std::format("A factory of type '{}' doesn't exists.", type ),
        "ShaderGenerator::createSubRenderState");

    return nullptr;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::destroySubRenderState(SubRenderState* subRenderState)
{
    auto itFind = mSubRenderStateFactories.find(subRenderState->getType());

    if (itFind != mSubRenderStateFactories.end())
    {
         itFind->second->destroyInstance(subRenderState);
    }   
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::createSubRenderState(ScriptCompiler* compiler, 
                                                      PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) -> SubRenderState*
{
    SubRenderState* subRenderState = nullptr;

    for (auto const& [key, value] : mSubRenderStateFactories)
    {
        subRenderState = value->createInstance(compiler, prop, pass, translator);
        if (subRenderState != nullptr)     
            break;              
    }   

    return subRenderState;
}


//-----------------------------------------------------------------------------
auto ShaderGenerator::createSubRenderState(ScriptCompiler* compiler, 
                                                      PropertyAbstractNode* prop, TextureUnitState* texState, SGScriptTranslator* translator) -> SubRenderState*
{
    SubRenderState* subRenderState = nullptr;

    for (auto const& [key, value] : mSubRenderStateFactories)
    {
        subRenderState = value->createInstance(compiler, prop, texState, translator);
        if (subRenderState != nullptr)     
            break;              
    }   

    return subRenderState;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::createScheme(std::string_view schemeName)
{
    createOrRetrieveScheme(schemeName);
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::getRenderState(std::string_view schemeName) -> RenderState*
{
    auto itFind = mSchemeEntriesMap.find(schemeName);
    
    if (itFind == mSchemeEntriesMap.end())
    {
        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
            ::std::format("A scheme named'{}' doesn't exists.", schemeName ),
            "ShaderGenerator::getRenderState"); 
    }   
    
    return itFind->second->getRenderState();
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::hasRenderState(std::string_view schemeName) const -> bool
{
    auto itFind = mSchemeEntriesMap.find(schemeName);
    return itFind != mSchemeEntriesMap.end();
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::createOrRetrieveRenderState(std::string_view schemeName) -> ShaderGenerator::RenderStateCreateOrRetrieveResult
{
    SchemeCreateOrRetrieveResult res = createOrRetrieveScheme(schemeName);
    return { res.first->getRenderState(),res.second };
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::createOrRetrieveScheme(std::string_view schemeName) -> ShaderGenerator::SchemeCreateOrRetrieveResult
{
    bool wasCreated = false;
    auto itScheme = mSchemeEntriesMap.find(schemeName);
    SGScheme* schemeEntry = nullptr;

    if (itScheme == mSchemeEntriesMap.end())
    {
        schemeEntry = new SGScheme(schemeName);
        mSchemeEntriesMap.emplace(schemeName, schemeEntry);
        wasCreated = true;
    }
    else
    {
        schemeEntry = itScheme->second;
    }

    return { schemeEntry, wasCreated };
}
//-----------------------------------------------------------------------------
auto ShaderGenerator::getRenderState(std::string_view schemeName, 
                                     std::string_view materialName, 
                                     std::string_view groupName, 
                                     unsigned short passIndex) -> RenderState*
{
    auto itFind = mSchemeEntriesMap.find(schemeName);

    if (itFind == mSchemeEntriesMap.end())
    {
        OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
            ::std::format("A scheme named'{}' doesn't exists.", schemeName ),
            "ShaderGenerator::getRenderState");
    }

    return itFind->second->getRenderState(materialName, groupName, passIndex);
}

//-----------------------------------------------------------------------------
void ShaderGenerator::addSceneManager(SceneManager* sceneMgr)
{
    // Make sure this scene manager not exists in the set.
    if (!mSceneManagerMap.insert(sceneMgr).second)
        return;

    if (!mRenderObjectListener)
        mRenderObjectListener = std::make_unique<SGRenderObjectListener>(this);
    
    sceneMgr->addRenderObjectListener(mRenderObjectListener.get());

    if (!mSceneManagerListener)
        mSceneManagerListener = std::make_unique<SGSceneManagerListener>(this);
    
    sceneMgr->addListener(mSceneManagerListener.get());

    // Update the active scene manager.
    if (mActiveSceneMgr == nullptr)
        mActiveSceneMgr = sceneMgr;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::removeSceneManager(SceneManager* sceneMgr)
{
    // Make sure this scene manager exists in the map.
    auto itFind = mSceneManagerMap.find(sceneMgr);
    
    if (itFind != mSceneManagerMap.end())
    {
        (*itFind)->removeRenderObjectListener(mRenderObjectListener.get());
        (*itFind)->removeListener(mSceneManagerListener.get());

        mSceneManagerMap.erase(itFind);

        // Update the active scene manager.
        if (mActiveSceneMgr == sceneMgr) {
            mActiveSceneMgr = nullptr;

            // force refresh global scene manager material
            invalidateMaterial(DEFAULT_SCHEME_NAME, "Ogre/TextureShadowReceiver", RGN_INTERNAL);
        }
    }
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::getActiveSceneManager() noexcept -> SceneManager*
{
    return mActiveSceneMgr;
}
//-----------------------------------------------------------------------------
void ShaderGenerator::_setActiveSceneManager(SceneManager* sceneManager)
{
    mActiveViewportValid &= (mActiveSceneMgr == sceneManager);
    mActiveSceneMgr = sceneManager;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::setShaderProfiles(GpuProgramType type, std::string_view shaderProfiles)
{
    switch(type)
    {
    case GPT_VERTEX_PROGRAM:
        mVertexShaderProfiles = shaderProfiles;
        break;
    case GPT_FRAGMENT_PROGRAM:
        mFragmentShaderProfiles = shaderProfiles;
        break;
    default:
        OgreAssert(false, "not implemented");
        break;
    }
}

auto ShaderGenerator::getShaderProfiles(GpuProgramType type) const -> std::string_view 
{
    switch(type)
    {
    case GPT_VERTEX_PROGRAM:
        return mVertexShaderProfiles;
    case GPT_FRAGMENT_PROGRAM:
        return mFragmentShaderProfiles;
    default:
        return BLANKSTRING;
    }
}
//-----------------------------------------------------------------------------
auto ShaderGenerator::hasShaderBasedTechnique(std::string_view materialName, 
                                                 std::string_view groupName, 
                                                 std::string_view srcTechniqueSchemeName, 
                                                 std::string_view dstTechniqueSchemeName) const -> bool
{
    // Make sure material exists;
    if (false == MaterialManager::getSingleton().resourceExists(materialName, groupName))
        return false;

    
    auto itMatEntry = findMaterialEntryIt(materialName, groupName);
    
    // Check if technique already created.
    if (itMatEntry != mMaterialEntriesMap.end())
    {
        for (const SGTechniqueList& techniqueEntires = itMatEntry->second->getTechniqueList();
             auto const& itTechEntry : techniqueEntires)
        {
            // Check requested mapping already exists.
            if (itTechEntry->getSourceTechnique()->getSchemeName() == srcTechniqueSchemeName &&
                itTechEntry->getDestinationTechniqueSchemeName() == dstTechniqueSchemeName)
            {
                return true;
            }           
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
static auto hasFixedFunctionPass(Technique* tech) -> bool
{
    for (unsigned short i=0; i < tech->getNumPasses(); ++i)
    {
        if (!tech->getPass(i)->isProgrammable())
        {
            return true;
        }
    }
    return false;
}

static auto findSourceTechnique(const Material& mat, std::string_view srcTechniqueSchemeName,
                                      bool overProgrammable) -> Technique*
{
    // Find the source technique
    for (Technique* curTechnique : mat.getTechniques())
    {
        if (curTechnique->getSchemeName() == srcTechniqueSchemeName &&
            (hasFixedFunctionPass(curTechnique) || overProgrammable))
        {
            return curTechnique;
        }
    }

    return nullptr;
}
//-----------------------------------------------------------------------------
auto ShaderGenerator::createShaderBasedTechnique(const Material& srcMat,
                                                 std::string_view srcTechniqueSchemeName, 
                                                 std::string_view dstTechniqueSchemeName,
                                                 bool overProgrammable) -> bool
{
    // No technique created -> check if one can be created from the given source technique scheme.  
    Technique* srcTechnique = findSourceTechnique(srcMat, srcTechniqueSchemeName, overProgrammable);

    // No appropriate source technique found.
    if (!srcTechnique)
    {
        return false;
    }

    return createShaderBasedTechnique(srcTechnique, dstTechniqueSchemeName, overProgrammable);
}
auto ShaderGenerator::createShaderBasedTechnique(const Technique* srcTechnique, std::string_view dstTechniqueSchemeName,
                                                 bool overProgrammable) -> bool
{
    // Update group name in case it is AUTODETECT_RESOURCE_GROUP_NAME
    Material* srcMat = srcTechnique->getParent();
    std::string_view materialName = srcMat->getName();
    std::string_view trueGroupName = srcMat->getGroup();

    auto itMatEntry = findMaterialEntryIt(materialName, trueGroupName);

    // Check if technique already created.
    if (itMatEntry != mMaterialEntriesMap.end())
    {
        for (const SGTechniqueList& techniqueEntires = itMatEntry->second->getTechniqueList();
             auto const& itTechEntry : techniqueEntires)
        {
            // Case the requested mapping already exists.
            if (itTechEntry->getSourceTechnique()->getSchemeName() == srcTechnique->getSchemeName() &&
                itTechEntry->getDestinationTechniqueSchemeName() == dstTechniqueSchemeName)
            {
                return true;
            }


            // Case a shader based technique with the same scheme name already defined based
            // on different source technique.
            // This state might lead to conflicts during shader generation - we prevent it by returning false here.
            else if (itTechEntry->getDestinationTechniqueSchemeName() == dstTechniqueSchemeName)
            {
                return false;
            }
        }
    }

    // Create shader based technique from the given source technique.
    SGMaterial* matEntry = nullptr;

    if (itMatEntry == mMaterialEntriesMap.end())
    {
        matEntry = new SGMaterial(materialName, trueGroupName);
        mMaterialEntriesMap.emplace(MatGroupPair(materialName, trueGroupName), matEntry);
    }
    else
    {
        matEntry = itMatEntry->second;
    }

    // Create the new technique entry.
    auto* techEntry =
        new SGTechnique(matEntry, srcTechnique, dstTechniqueSchemeName, overProgrammable);

    // Add to material entry map.
    matEntry->getTechniqueList().push_back(techEntry);

    // Add to all technique map.
    mTechniqueEntriesMap[techEntry] = techEntry;

    // Add to scheme.
    SGScheme* schemeEntry = createOrRetrieveScheme(dstTechniqueSchemeName).first;
    schemeEntry->addTechniqueEntry(techEntry);

    return true;
}

auto ShaderGenerator::removeShaderBasedTechnique(const Technique* srcTech, std::string_view dstTechniqueSchemeName) -> bool
{
    // Make sure scheme exists.
    auto itScheme = mSchemeEntriesMap.find(dstTechniqueSchemeName);
    SGScheme* schemeEntry = nullptr;

    if (itScheme == mSchemeEntriesMap.end())
        return false;

    schemeEntry = itScheme->second;

    // Find the material entry.
    Material* srcMat = srcTech->getParent();
    auto itMatEntry = findMaterialEntryIt(srcMat->getName(), srcMat->getGroup());

    // Case material not found.
    if (itMatEntry == mMaterialEntriesMap.end())
        return false;


    SGTechniqueList& matTechniqueEntires = itMatEntry->second->getTechniqueList();
    auto itTechEntry = matTechniqueEntires.begin();
    SGTechnique* dstTechnique = nullptr;

    // Remove destination technique entry from material techniques list.
    for (; itTechEntry != matTechniqueEntires.end(); ++itTechEntry)
    {
        if ((*itTechEntry)->getSourceTechnique()->getSchemeName() == srcTech->getSchemeName() &&
            (*itTechEntry)->getDestinationTechniqueSchemeName() == dstTechniqueSchemeName)
        {
            dstTechnique = *itTechEntry;
            matTechniqueEntires.erase(itTechEntry);
            break;
        }
    }

    // Technique not found.
    if (dstTechnique == nullptr)
        return false;

    schemeEntry->removeTechniqueEntry(dstTechnique);

    auto itTechMap = mTechniqueEntriesMap.find(dstTechnique);

    if (itTechMap != mTechniqueEntriesMap.end())
        mTechniqueEntriesMap.erase(itTechMap);

    delete dstTechnique;

    return true;
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::removeAllShaderBasedTechniques(std::string_view materialName, std::string_view groupName) -> bool
{
    // Find the material entry.
    auto itMatEntry = findMaterialEntryIt(materialName, groupName);
    
    // Case material not found.
    if (itMatEntry == mMaterialEntriesMap.end())
        return false;


    SGTechniqueList& matTechniqueEntires = itMatEntry->second->getTechniqueList();
        
    // Remove all technique entries from material techniques list.
    while (matTechniqueEntires.empty() == false)
    {   
        auto itTechEntry = matTechniqueEntires.begin();

        removeShaderBasedTechnique((*itTechEntry)->getSourceTechnique(),
                                   (*itTechEntry)->getDestinationTechniqueSchemeName());
    }

    delete itMatEntry->second;
    mMaterialEntriesMap.erase(itMatEntry);
    
    return true;
}

auto ShaderGenerator::cloneShaderBasedTechniques(const Material& srcMat, Material& dstMat) -> bool
{
    if(&srcMat == &dstMat) return true; // nothing to do

    auto itSrcMatEntry = findMaterialEntryIt(srcMat.getName(), srcMat.getGroup());

    //remove any techniques in the destination material so the new techniques may be copied
    removeAllShaderBasedTechniques(dstMat);
    
    //
    //remove any techniques from the destination material which have RTSS associated schemes from
    //the source material. This code is performed in case the user performed a clone of a material
    //which has already generated RTSS techniques in the source material.
    //

    //first gather the techniques to remove
    std::set<unsigned short> schemesToRemove;
    for(Technique* pSrcTech : srcMat.getTechniques())
    {
        Pass* pSrcPass = pSrcTech->getNumPasses() > 0 ? pSrcTech->getPass(0) : nullptr;
        if (pSrcPass)
        {
            ::std::any const& passUserData = pSrcPass->getUserObjectBindings().getUserAny(TargetRenderState::UserKey);
            if (passUserData.has_value())
            {
                schemesToRemove.insert(pSrcTech->_getSchemeIndex());
            }
        }
    }
    //remove the techniques from the destination material
    auto techCount = dstMat.getNumTechniques();
    for(unsigned short ti = techCount - 1 ; ti != (unsigned short)-1 ; --ti)
    {
        Technique* pDstTech = dstMat.getTechnique(ti);
        if (schemesToRemove.find(pDstTech->_getSchemeIndex()) != schemesToRemove.end())
        {
            dstMat.removeTechnique(ti);
        }
    }
    
    //
    // Clone the render states from source to destination
    //

    // Check if RTSS techniques exist in the source material 
    if (itSrcMatEntry != mMaterialEntriesMap.end())
    {
        //Go over all rtss techniques in the source material
        for (const SGTechniqueList& techniqueEntires = itSrcMatEntry->second->getTechniqueList();
             auto const& itTechEntry : techniqueEntires)
        {
            String srcFromTechniqueScheme = itTechEntry->getSourceTechnique()->getSchemeName();
            String srcToTechniqueScheme = itTechEntry->getDestinationTechniqueSchemeName();
            
            //for every technique in the source material create a shader based technique in the 
            //destination material
            if (createShaderBasedTechnique(dstMat, srcFromTechniqueScheme, srcToTechniqueScheme))
            {
                //check for custom render states in the source material
                unsigned short passCount =  itTechEntry->getSourceTechnique()->getNumPasses();
                for(unsigned short pi = 0 ; pi < passCount ; ++pi)
                {
                    if (itTechEntry->hasRenderState(pi))
                    {
                        //copy the custom render state from the source material to the destination material
                        RenderState* srcRenderState = itTechEntry->getRenderState(pi);
                        RenderState* dstRenderState = getRenderState(srcToTechniqueScheme, dstMat, pi);

                        for(const SubRenderStateList& srcSubRenderState =
                                srcRenderState->getSubRenderStates();
                            SubRenderState* srcSubState : srcSubRenderState)
                        {
                            SubRenderState* dstSubState = createSubRenderState(srcSubState->getType());
                            (*dstSubState) = (*srcSubState);
                            dstRenderState->addTemplateSubRenderState(dstSubState);
                        }
                    }
                }
            }
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::removeAllShaderBasedTechniques()
{
    while (!mMaterialEntriesMap.empty())
    {
        auto itMatEntry = mMaterialEntriesMap.begin();

        removeAllShaderBasedTechniques(itMatEntry->first.first, itMatEntry->first.second);
    }
}

//-----------------------------------------------------------------------------
 void ShaderGenerator::notifyRenderSingleObject(Renderable* rend, 
     const Pass* pass,  
     const AutoParamDataSource* source, 
     const LightList* pLightList, bool suppressRenderStateChanges)
{
    if (mActiveViewportValid)
    {
        ::std::any const& passUserData = pass->getUserObjectBindings().getUserAny(TargetRenderState::UserKey);

        if (!passUserData.has_value() || suppressRenderStateChanges)
            return; 

        auto renderState = any_cast<TargetRenderStatePtr>(passUserData);
        renderState->updateGpuProgramsParams(rend, pass, source, pLightList);        
    }   
}


//-----------------------------------------------------------------------------
void ShaderGenerator::preFindVisibleObjects(SceneManager* source, 
                                            SceneManager::IlluminationRenderStage irs, 
                                            Viewport* v)
{
    std::string_view curMaterialScheme = v->getMaterialScheme();
        
    mActiveSceneMgr      = source;
    mActiveViewportValid = validateScheme(curMaterialScheme);
}

//-----------------------------------------------------------------------------
void ShaderGenerator::invalidateScheme(std::string_view schemeName)
{
    auto itScheme = mSchemeEntriesMap.find(schemeName);

    if (itScheme != mSchemeEntriesMap.end())    
        itScheme->second->invalidate(); 
    
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::validateScheme(std::string_view schemeName) -> bool
{
    auto itScheme = mSchemeEntriesMap.find(schemeName);

    // No such scheme exists.
    if (itScheme == mSchemeEntriesMap.end())    
        return false;

    itScheme->second->validate();

    return true;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::invalidateMaterial(std::string_view schemeName, std::string_view materialName, std::string_view groupName)
{
    auto itScheme = mSchemeEntriesMap.find(schemeName);
    
    if (itScheme != mSchemeEntriesMap.end())            
        itScheme->second->invalidate(materialName, groupName);  
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::validateMaterial(std::string_view schemeName, std::string_view materialName, std::string_view groupName) -> bool
{
    auto itScheme = mSchemeEntriesMap.find(schemeName);

    // No such scheme exists.
    if (itScheme == mSchemeEntriesMap.end())    
        return false;

    return itScheme->second->validate(materialName, groupName); 
}

//-----------------------------------------------------------------------------
void ShaderGenerator::invalidateMaterialIlluminationPasses(std::string_view schemeName, std::string_view materialName, std::string_view groupName)
{
	auto itScheme = mSchemeEntriesMap.find(schemeName);

	if(itScheme != mSchemeEntriesMap.end())
		itScheme->second->invalidateIlluminationPasses(materialName, groupName);
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::validateMaterialIlluminationPasses(std::string_view schemeName, std::string_view materialName, std::string_view groupName) -> bool
{
	auto itScheme = mSchemeEntriesMap.find(schemeName);

	// No such scheme exists.
	if(itScheme == mSchemeEntriesMap.end())
		return false;

	return itScheme->second->validateIlluminationPasses(materialName, groupName);
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::getMaterialSerializerListener() noexcept -> MaterialSerializer::Listener*
{
    if (!mMaterialSerializerListener)
        mMaterialSerializerListener.reset(new SGMaterialSerializerListener);

    return mMaterialSerializerListener.get();
}

//-----------------------------------------------------------------------------
void ShaderGenerator::flushShaderCache()
{
    // Release all programs.
    for (auto const& itTech : mTechniqueEntriesMap)
    {           
        itTech.second->releasePrograms();
    }

    ProgramManager::getSingleton().flushGpuProgramsCache();
    
    // Invalidate all schemes.
    for (auto const& itScheme : mSchemeEntriesMap)
    {
        itScheme.second->invalidate();
    }
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::getTranslator(const AbstractNodePtr& node) -> ScriptTranslator*
{
    if(node->type != ANT_OBJECT)
        return nullptr;
    
    auto *obj = static_cast<ObjectAbstractNode*>(node.get());

    if(obj->id == ID_RT_SHADER_SYSTEM)
        return &mCoreScriptTranslator;
    
    return nullptr;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::serializePassAttributes(MaterialSerializer* ser, SGPass* passEntry)
{
    
    // Write section header and begin it.
    ser->writeAttribute(3, "rtshader_system");
    ser->beginSection(3);

    // Grab the custom render state this pass uses.
    RenderState* customRenderState = passEntry->getCustomRenderState();

    if (customRenderState != nullptr)
    {
        // Write each of the sub-render states that composing the final render state.

        for (const SubRenderStateList& subRenderStates = customRenderState->getSubRenderStates();
            SubRenderState* curSubRenderState : subRenderStates)
        {
            auto itFactory = mSubRenderStateFactories.find(curSubRenderState->getType());

            if (itFactory != mSubRenderStateFactories.end())
            {
                SubRenderStateFactory* curFactory = itFactory->second;
                curFactory->writeInstance(ser, curSubRenderState, passEntry->getSrcPass(), passEntry->getDstPass());
            }
        }
    }
    
    // Write section end.
    ser->endSection(3);     
}



//-----------------------------------------------------------------------------
void ShaderGenerator::serializeTextureUnitStateAttributes(MaterialSerializer* ser, SGPass* passEntry, const TextureUnitState* srcTextureUnit)
{
    
    // Write section header and begin it.
    ser->writeAttribute(4, "rtshader_system");
    ser->beginSection(4);

    // Grab the custom render state this pass uses.
    RenderState* customRenderState = passEntry->getCustomRenderState();
            
    if (customRenderState != nullptr)
    {
        //retrive the destintion texture unit state
        TextureUnitState* dstTextureUnit = nullptr;
        unsigned short texIndex = srcTextureUnit->getParent()->getTextureUnitStateIndex(srcTextureUnit);
        if (texIndex < passEntry->getDstPass()->getNumTextureUnitStates())
        {
            dstTextureUnit = passEntry->getDstPass()->getTextureUnitState(texIndex);
        }
        
        // Write each of the sub-render states that composing the final render state.

        for (const SubRenderStateList& subRenderStates = customRenderState->getSubRenderStates();
            SubRenderState* curSubRenderState : subRenderStates)
        {
            auto itFactory = mSubRenderStateFactories.find(curSubRenderState->getType());

            if (itFactory != mSubRenderStateFactories.end())
            {
                SubRenderStateFactory* curFactory = itFactory->second;
                curFactory->writeInstance(ser, curSubRenderState, srcTextureUnit, dstTextureUnit);
            }
        }
    }
    
    // Write section end.
    ser->endSection(4);     
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::getShaderCount(GpuProgramType type) const -> size_t
{
    return mProgramManager->getShaderCount(type);
}

//-----------------------------------------------------------------------------
void ShaderGenerator::setTargetLanguage(std::string_view shaderLanguage)
{
    // Make sure that the shader language is supported.
    if (!mProgramWriterManager->isLanguageSupported(shaderLanguage))
    {
        OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, ::std::format("'{}' is not supported", shaderLanguage ));
    }

    // Case target language changed -> flush the shaders cache.
    if (mShaderLanguage != shaderLanguage)
    {
        mShaderLanguage = shaderLanguage;
        flushShaderCache();
    }
}

//-----------------------------------------------------------------------------
void ShaderGenerator::setShaderCachePath( std::string_view cachePath )
{
    String stdCachePath = cachePath;

    // Standardise the cache path in case of none empty string.
    if (stdCachePath.empty() == false)
        stdCachePath = StringUtil::standardisePath(stdCachePath);

    if (mShaderCachePath != stdCachePath)
    {
        mShaderCachePath = stdCachePath;

        // Case this is a valid file path -> add as resource location in order to make sure that
        // generated shaders could be loaded by the file system archive.
        if (mShaderCachePath.empty() == false)
        {   
            // Make sure this is a valid writable path.
            String outTestFileName(::std::format("{}ShaderGenerator.tst", mShaderCachePath));
            std::ofstream outFile(outTestFileName.c_str());
            
            if (!outFile)
            {
                OGRE_EXCEPT(Exception::ERR_CANNOT_WRITE_TO_FILE,
                    ::std::format("Could not create output files in the given shader cache path '{}", mShaderCachePath),
                    "ShaderGenerator::setShaderCachePath"); 
            }

            // Close and remove the test file.
            outFile.close();
            remove(outTestFileName.c_str());
        }
    }
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::findMaterialEntryIt(std::string_view materialName, std::string_view groupName) -> ShaderGenerator::SGMaterialIterator
{
    SGMaterialIterator itMatEntry;
    //check if we have auto detect request
    if (groupName == ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)
    {
        //find the possible first entry
        itMatEntry = mMaterialEntriesMap.lower_bound(MatGroupPair(materialName,""));
        if ((itMatEntry != mMaterialEntriesMap.end()) &&
            (itMatEntry->first.first != materialName))
        {
            //no entry found
            itMatEntry = mMaterialEntriesMap.end();
        }
    }
    else
    {
        //find entry with group name specified
        itMatEntry = mMaterialEntriesMap.find(MatGroupPair(materialName,groupName));
    }
    return itMatEntry;
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::findMaterialEntryIt(std::string_view materialName, std::string_view groupName) const -> ShaderGenerator::SGMaterialConstIterator
{
    SGMaterialConstIterator itMatEntry;
    //check if we have auto detect request
    if (groupName == ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)
    {
        //find the possible first entry
        itMatEntry = mMaterialEntriesMap.lower_bound(MatGroupPair(materialName,""));
        if ((itMatEntry != mMaterialEntriesMap.end()) &&
            (itMatEntry->first.first != materialName))
        {
            //no entry found
            itMatEntry = mMaterialEntriesMap.end();
        }
    }
    else
    {
        //find entry with group name specified
        itMatEntry = mMaterialEntriesMap.find(MatGroupPair(materialName,groupName));
    }
    return itMatEntry;
}
//-----------------------------------------------------------------------------
auto ShaderGenerator::getRTShaderSchemeCount() const -> size_t
{
    return mSchemeEntriesMap.size();
}
//-----------------------------------------------------------------------------
auto ShaderGenerator::getRTShaderScheme(size_t index) const -> std::string_view 
{
    auto it = mSchemeEntriesMap.begin();
    while ((index != 0) && (it != mSchemeEntriesMap.end()))
    {
        --index;
        ++it;
    }

    assert((it != mSchemeEntriesMap.end()) && "Index out of bounds");
    if (it != mSchemeEntriesMap.end())
        return it->first;
    else return BLANKSTRING;
}

//-----------------------------------------------------------------------------

auto ShaderGenerator::getIsFinalizing() const noexcept -> bool
{
    return mIsFinalizing;
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::createSGPassList(Material* mat) const -> ShaderGenerator::SGPassList
{
    SGPassList passList;

    auto itMatEntry = findMaterialEntryIt(mat->getName(), mat->getGroup());

    // Check if material is managed.
    if (itMatEntry == mMaterialEntriesMap.end())
        return passList;

    for (auto sgtech : itMatEntry->second->getTechniqueList())
    {
        for(auto sgpass : sgtech->getPassList())
        {
            passList.push_back(sgpass);
        }
    }

    return passList;
}

//-----------------------------------------------------------------------------
ShaderGenerator::SGPass::SGPass(SGTechnique* parent, Pass* srcPass, Pass* dstPass, IlluminationStage stage)
{
    mParent             = parent;
    mSrcPass            = srcPass;
	mDstPass			= dstPass;
	mStage				= stage;
    mCustomRenderState  = nullptr;
}

//-----------------------------------------------------------------------------
ShaderGenerator::SGPass::~SGPass()
{
    mDstPass->getUserObjectBindings().eraseUserAny(TargetRenderState::UserKey);
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGPass::buildTargetRenderState()
{   
    if(mSrcPass->isProgrammable() && !mParent->overProgrammablePass() && !isIlluminationPass()) return;
    std::string_view schemeName = mParent->getDestinationTechniqueSchemeName();
    const RenderState* renderStateGlobal = ShaderGenerator::getSingleton().getRenderState(schemeName);
    

    auto targetRenderState = std::make_shared<TargetRenderState>();

    // Set light properties.
    Vector3i lightCount(0, 0, 0);

    // Use light count definitions of the custom render state if exists.
    if (mCustomRenderState != nullptr && mCustomRenderState->getLightCountAutoUpdate() == false)
    {
        lightCount = mCustomRenderState->getLightCount();
    }

    // Use light count definitions of the global render state if exists.
    else if (renderStateGlobal != nullptr)
    {
        lightCount = renderStateGlobal->getLightCount();
    }
    
    
    targetRenderState->setLightCount(lightCount);

    // Link the target render state with the custom render state of this pass if exists.
    if (mCustomRenderState != nullptr)
    {
        targetRenderState->link(*mCustomRenderState, mSrcPass, mDstPass);
    }

    // Link the target render state with the scheme render state of the shader generator.
    if (renderStateGlobal != nullptr)
    {
        targetRenderState->link(*renderStateGlobal, mSrcPass, mDstPass);
    }

    // Build the FFP state.
    FFPRenderStateBuilder::buildRenderState(this, targetRenderState.get());

    targetRenderState->acquirePrograms(mDstPass);
    mDstPass->getUserObjectBindings().setUserAny(TargetRenderState::UserKey, targetRenderState);
}

//-----------------------------------------------------------------------------
ShaderGenerator::SGTechnique::SGTechnique(SGMaterial* parent, const Technique* srcTechnique,
                                          std::string_view dstTechniqueSchemeName,
                                          bool overProgrammable)
    : mParent(parent), mSrcTechnique(srcTechnique), 
      mDstTechniqueSchemeName(dstTechniqueSchemeName), mOverProgrammable(overProgrammable)
{
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGTechnique::createSGPasses()
{
    // Create pass entry for each pass.
    for (unsigned short i=0; i < mSrcTechnique->getNumPasses(); ++i)
    {
        Pass* srcPass = mSrcTechnique->getPass(i);
        Pass* dstPass = mDstTechnique->getPass(i);

		auto* passEntry = new SGPass(this, srcPass, dstPass, IS_UNKNOWN);

        if (i < mCustomRenderStates.size())
            passEntry->setCustomRenderState(mCustomRenderStates[i]);
        mPassEntries.push_back(passEntry);
    }
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGTechnique::createIlluminationSGPasses()
{
	// Create pass entry for each illumination pass.
	const IlluminationPassList& passes = mDstTechnique->getIlluminationPasses();

	for(auto p : passes)
	{
		// process only autogenerated illumination passes
			if(p->pass == p->originalPass)
			continue;

		auto* passEntry = new SGPass(this, p->pass, p->pass, p->stage);

		::std::any const& origPassUserData = p->originalPass->getUserObjectBindings().getUserAny(TargetRenderState::UserKey);
		if(origPassUserData.has_value())
		{
            for(auto sgp : mPassEntries)
            {
                if(sgp->getDstPass() == p->originalPass)
                {
                    passEntry->setCustomRenderState(sgp->getCustomRenderState());
                    break;
                }
            }
		}

		mPassEntries.push_back(passEntry);
	}
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGTechnique::destroyIlluminationSGPasses()
{
	for(auto itPass = mPassEntries.begin(); itPass != mPassEntries.end(); /* no increment*/ )
	{
		if((*itPass)->isIlluminationPass())
		{
			delete(*itPass);
			itPass = mPassEntries.erase(itPass);
		}
		else
		{
			++itPass;
		}
	}
}

//-----------------------------------------------------------------------------
ShaderGenerator::SGTechnique::~SGTechnique()
{
    std::string_view materialName = mParent->getMaterialName();
    std::string_view groupName = mParent->getGroupName();

    // Destroy the passes.
    destroySGPasses();

    if (MaterialManager::getSingleton().resourceExists(materialName, groupName))
    {
        MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName, groupName);
    
        // Remove the destination technique from parent material.
        for (ushort i=0; i < mat->getNumTechniques(); ++i)
        {
            if (mDstTechnique == mat->getTechnique(i))
            {
                // Unload the generated technique in order tor free referenced resources.
                mDstTechnique->_unload();

                // Remove the generated technique in order to restore the material to its original state.
                mat->removeTechnique(i);

                // touch when finalizing - will reload the textures - so no touch if finalizing
                if (ShaderGenerator::getSingleton().getIsFinalizing() == false)
                {
                    // Make sure the material goes back to its original state.
                    mat->touch();
                }
                break;
            }       
        }
    }

    // Delete the custom render states of each pass if exist.
    for (auto & mCustomRenderState : mCustomRenderStates)
    {
        if (mCustomRenderState != nullptr)
        {
            delete mCustomRenderState;
            mCustomRenderState = nullptr;
        }       
    }
    mCustomRenderStates.clear();
    
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGTechnique::destroySGPasses()
{
    for (auto & mPassEntrie : mPassEntries)
    {
        delete mPassEntrie;
    }
    mPassEntries.clear();
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGTechnique::buildTargetRenderState()
{
    // Remove existing destination technique and passes
    // in order to build it again from scratch.
    if (mDstTechnique != nullptr)
    {
        Material* mat = mSrcTechnique->getParent();

        for (unsigned short i=0; i < mat->getNumTechniques(); ++i)
        {
            if (mat->getTechnique(i) == mDstTechnique)
            {
                mat->removeTechnique(i);
                break;
            }
        }
        destroySGPasses();      
    }
        
    // Create the destination technique and passes.
    mDstTechnique   = mSrcTechnique->getParent()->createTechnique();
    mDstTechnique->getUserObjectBindings().setUserAny(SGTechnique::UserKey, this);
    *mDstTechnique  = *mSrcTechnique;
    mDstTechnique->setSchemeName(mDstTechniqueSchemeName);
    createSGPasses();


    // Build render state for each pass.
    for (auto & mPassEntrie : mPassEntries)
    {
		assert(!mPassEntrie->isIlluminationPass()); // this is not so important, but intended to be so here.
        mPassEntrie->buildTargetRenderState();
    }

    // Turn off the build destination technique flag.
    mBuildDstTechnique = false;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGTechnique::buildIlluminationTargetRenderState()
{
	assert(mDstTechnique != nullptr);
	assert(!getBuildDestinationTechnique());

	// Create the illumination passes.
	createIlluminationSGPasses();

	// Build render state for each pass.
	for(auto & mPassEntrie : mPassEntries)
	{
		if(mPassEntrie->isIlluminationPass())
			mPassEntrie->buildTargetRenderState();
	}
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGTechnique::releasePrograms()
{
    // Remove destination technique.
    if (mDstTechnique != nullptr)
    {
        Material* mat = mSrcTechnique->getParent();

        for (unsigned short i=0; i < mat->getNumTechniques(); ++i)
        {
            if (mat->getTechnique(i) == mDstTechnique)
            {
                mat->removeTechnique(i);
                break;
            }
        }
        mDstTechnique = nullptr;
    }

    // Destroy the passes.
    destroySGPasses();
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::SGTechnique::getRenderState(unsigned short passIndex) -> RenderState*
{
    RenderState* renderState = nullptr;

    if (passIndex >= mCustomRenderStates.size())    
        mCustomRenderStates.resize(passIndex + 1, nullptr);        

    renderState = mCustomRenderStates[passIndex];
    if (renderState == nullptr)
    {
        renderState = new RenderState;
        mCustomRenderStates[passIndex] = renderState;
    }
    
    return renderState;
}
//-----------------------------------------------------------------------------
auto ShaderGenerator::SGTechnique::hasRenderState(unsigned short passIndex) -> bool
{
    return (passIndex < mCustomRenderStates.size()) && (mCustomRenderStates[passIndex] != nullptr);
}


//-----------------------------------------------------------------------------
ShaderGenerator::SGScheme::SGScheme(std::string_view schemeName) :
    mName(schemeName)
{
}

//-----------------------------------------------------------------------------
ShaderGenerator::SGScheme::~SGScheme()
= default;

//-----------------------------------------------------------------------------
auto ShaderGenerator::SGScheme::getRenderState() noexcept -> RenderState*
{
    if (!mRenderState)
        mRenderState = std::make_unique<RenderState>();

    return mRenderState.get();
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::SGScheme::getRenderState(std::string_view materialName, std::string_view groupName, unsigned short passIndex) -> RenderState*
{
    // Find the desired technique.
    bool doAutoDetect = groupName == ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
    for (SGTechnique* curTechEntry : mTechniqueEntries)
    {
        Material* curMat = curTechEntry->getSourceTechnique()->getParent();
        if ((curMat->getName() == materialName) && 
            ((doAutoDetect == true) || (curMat->getGroup() == groupName)))
        {
            return curTechEntry->getRenderState(passIndex);         
        }
    }

    return nullptr;
}


//-----------------------------------------------------------------------------
void ShaderGenerator::SGScheme::addTechniqueEntry(SGTechnique* techEntry)
{
    mTechniqueEntries.push_back(techEntry);

    // Mark as out of data.
    mOutOfDate = true;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGScheme::removeTechniqueEntry(SGTechnique* techEntry)
{
    // Build render state for each technique.
    for (auto itTech = mTechniqueEntries.begin(); itTech != mTechniqueEntries.end(); ++itTech)
    {
        SGTechnique* curTechEntry = *itTech;

        if (curTechEntry == techEntry)
        {
            mTechniqueEntries.erase(itTech);
            break;
        }       
    }
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGScheme::validate()
{   
    // Synchronize with light settings.
    synchronizeWithLightSettings();

    // Synchronize with fog settings.
    synchronizeWithFogSettings();

    // The target scheme is up to date.
    if (mOutOfDate == false)
        return;

    // Build render state for each technique and acquire GPU programs.
    for (SGTechnique* curTechEntry : mTechniqueEntries)
    {
        if (curTechEntry->getBuildDestinationTechnique())
            curTechEntry->buildTargetRenderState();
    }
    
    // Mark this scheme as up to date.
    mOutOfDate = false;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGScheme::synchronizeWithLightSettings()
{
    SceneManager* sceneManager = ShaderGenerator::getSingleton().getActiveSceneManager();
    RenderState* curRenderState = getRenderState();

    if (curRenderState->getLightCountAutoUpdate())
    {
        OgreAssert(sceneManager, "no active SceneManager. Did you forget to call ShaderGenerator::addSceneManager?");

        const LightList& lightList =  sceneManager->_getLightsAffectingFrustum();
        
        Vector3i sceneLightCount(0, 0, 0);
        for (auto i : lightList)
        {
            sceneLightCount[i->getType()]++;
        }
        
        auto currLightCount = mRenderState->getLightCount();

        auto lightDiff = currLightCount - sceneLightCount;

        // Case new light appeared -> invalidate. But dont invalidate the other way as shader compilation is costly.
        if (!(Vector3i(-1) < lightDiff))
        {
            LogManager::getSingleton().stream(LML_TRIVIAL)
                << "RTSS: invalidating scheme " << mName << " - lights changed " << currLightCount
                << " -> " << sceneLightCount;
            curRenderState->setLightCount(sceneLightCount);
            invalidate();
        }
    }
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGScheme::synchronizeWithFogSettings()
{
    SceneManager* sceneManager = ShaderGenerator::getSingleton().getActiveSceneManager();

    if (sceneManager != nullptr && sceneManager->getFogMode() != mFogMode)
    {
        LogManager::getSingleton().stream(LML_TRIVIAL)
            << "RTSS: invalidating scheme " << mName << " - fog settings changed";
        mFogMode = sceneManager->getFogMode();
        invalidate();
    }
}

//-----------------------------------------------------------------------------
auto ShaderGenerator::SGScheme::validate(std::string_view materialName, std::string_view groupName) -> bool
{
    // Synchronize with light settings.
    synchronizeWithLightSettings();

    // Synchronize with fog settings.
    synchronizeWithFogSettings();


    // Find the desired technique.
    bool doAutoDetect = groupName == ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
    for (SGTechnique* curTechEntry : mTechniqueEntries)
    {
        const SGMaterial* curMat = curTechEntry->getParent();
        if ((curMat->getMaterialName() == materialName) && 
            ((doAutoDetect == true) || (curMat->getGroupName() == groupName)) &&
            (curTechEntry->getBuildDestinationTechnique()))
        {       
            // Build render state for each technique and Acquire the CPU/GPU programs.
            curTechEntry->buildTargetRenderState();

			return true;
        }                   
    }

    return false;
}
//-----------------------------------------------------------------------------
auto ShaderGenerator::SGScheme::validateIlluminationPasses(std::string_view materialName, std::string_view groupName) -> bool
{
	// Synchronize with light settings.
	synchronizeWithLightSettings();

	// Synchronize with fog settings.
	synchronizeWithFogSettings();

	// Find the desired technique.
	bool doAutoDetect = groupName == ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
	for(SGTechnique* curTechEntry : mTechniqueEntries)
	{
		const SGMaterial* curMat = curTechEntry->getParent();
		if((curMat->getMaterialName() == materialName) &&
			((doAutoDetect == true) || (curMat->getGroupName() == groupName)))
		{
			// Build render state for each technique and Acquire the CPU/GPU programs.
			curTechEntry->buildIlluminationTargetRenderState();

			return true;
		}
	}

	return false;
}
//-----------------------------------------------------------------------------
void ShaderGenerator::SGScheme::invalidateIlluminationPasses(std::string_view materialName, std::string_view groupName)
{
	// Find the desired technique.
	bool doAutoDetect = groupName == ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
	for(SGTechnique* curTechEntry : mTechniqueEntries)
	{
		const SGMaterial* curMat = curTechEntry->getParent();
		if((curMat->getMaterialName() == materialName) &&
			((doAutoDetect == true) || (curMat->getGroupName() == groupName)))
		{
			curTechEntry->destroyIlluminationSGPasses();
		}
	}
}
//-----------------------------------------------------------------------------
void ShaderGenerator::SGScheme::invalidate(std::string_view materialName, std::string_view groupName)
{
    // Find the desired technique.
    bool doAutoDetect = groupName == ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
    for (SGTechnique* curTechEntry : mTechniqueEntries)
    {
        const SGMaterial* curMaterial = curTechEntry->getParent();
        if ((curMaterial->getMaterialName() == materialName) &&
            ((doAutoDetect == true) || (curMaterial->getGroupName() == groupName))) 
        {           
            // Turn on the build destination technique flag.
            curTechEntry->setBuildDestinationTechnique(true);
            break;          
        }                   
    }

    mOutOfDate = true;
}

//-----------------------------------------------------------------------------
void ShaderGenerator::SGScheme::invalidate()
{
    // Turn on the build destination technique flag of all techniques.
    for (SGTechnique* curTechEntry : mTechniqueEntries)
    {
        curTechEntry->setBuildDestinationTechnique(true);
    }

    mOutOfDate = true;          
}

}
}
