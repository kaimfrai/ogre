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
#include <cstdint>
#include <fstream> // IWYU pragma: keep
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
// IWYU pragma: no_include <sstream>

#include "OgreCommon.hpp"
#include "OgreException.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramManager.hpp"
#include "OgreMurmurHash3.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreRoot.hpp"
#include "OgreShaderFunction.hpp"
#include "OgreShaderGLSLProgramProcessor.hpp"
#include "OgreShaderGenerator.hpp"
#include "OgreShaderHLSLProgramProcessor.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderProgram.hpp"
#include "OgreShaderProgramManager.hpp"
#include "OgreShaderProgramProcessor.hpp"
#include "OgreShaderProgramSet.hpp"
#include "OgreShaderProgramWriter.hpp"
#include "OgreShaderProgramWriterManager.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreSingleton.hpp"
#include "OgreString.hpp"
#include "OgreStringConverter.hpp"

namespace Ogre {

//-----------------------------------------------------------------------
template<> 
RTShader::ProgramManager* Singleton<RTShader::ProgramManager>::msSingleton = nullptr;

namespace RTShader {


//-----------------------------------------------------------------------
auto ProgramManager::getSingletonPtr() noexcept -> ProgramManager*
{
    return msSingleton;
}

//-----------------------------------------------------------------------
auto ProgramManager::getSingleton() noexcept -> ProgramManager&
{
    assert( msSingleton );  
    return ( *msSingleton );
}

//-----------------------------------------------------------------------------
ProgramManager::ProgramManager()
{
    createDefaultProgramProcessors();
}

//-----------------------------------------------------------------------------
ProgramManager::~ProgramManager()
{
    flushGpuProgramsCache();
    destroyDefaultProgramProcessors();  
}

//-----------------------------------------------------------------------------
void ProgramManager::releasePrograms(const ProgramSet* programSet)
{
    GpuProgramPtr vsProgram(programSet->getGpuProgram(GpuProgramType::VERTEX_PROGRAM));
    GpuProgramPtr psProgram(programSet->getGpuProgram(GpuProgramType::FRAGMENT_PROGRAM));

    auto itVsGpuProgram = !vsProgram ? mVertexShaderMap.end() : mVertexShaderMap.find(vsProgram->getName());
    auto itFsGpuProgram = !psProgram ? mFragmentShaderMap.end() : mFragmentShaderMap.find(psProgram->getName());

    if (itVsGpuProgram != mVertexShaderMap.end())
    {
        if (itVsGpuProgram->second.use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS + 1)
        {
            destroyGpuProgram(itVsGpuProgram->second);
            mVertexShaderMap.erase(itVsGpuProgram);
        }
    }

    if (itFsGpuProgram != mFragmentShaderMap.end())
    {
        if (itFsGpuProgram->second.use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS + 1)
        {
            destroyGpuProgram(itFsGpuProgram->second);
            mFragmentShaderMap.erase(itFsGpuProgram);
        }
    }
}
//-----------------------------------------------------------------------------
void ProgramManager::flushGpuProgramsCache()
{
    flushGpuProgramsCache(mVertexShaderMap);
    flushGpuProgramsCache(mFragmentShaderMap);
}

auto ProgramManager::getShaderCount(GpuProgramType type) const -> size_t
{
    using enum GpuProgramType;
    switch(type)
    {
    case VERTEX_PROGRAM:
        return mVertexShaderMap.size();
    case FRAGMENT_PROGRAM:
        return mFragmentShaderMap.size();
    default:
        return 0;
    }
}
//-----------------------------------------------------------------------------
void ProgramManager::flushGpuProgramsCache(GpuProgramsMap& gpuProgramsMap)
{
    while (gpuProgramsMap.size() > 0)
    {
        auto it = gpuProgramsMap.begin();

        destroyGpuProgram(it->second);
        gpuProgramsMap.erase(it);
    }
}
//-----------------------------------------------------------------------------
void ProgramManager::createDefaultProgramProcessors()
{
    // Add standard shader processors
    mDefaultProgramProcessors.emplace_back(new GLSLProgramProcessor);
    addProgramProcessor("glsles", mDefaultProgramProcessors.back().get());
    addProgramProcessor("glslang", mDefaultProgramProcessors.back().get());

    addProgramProcessor("glsl", mDefaultProgramProcessors.back().get());
    mDefaultProgramProcessors.emplace_back(new HLSLProgramProcessor);
    addProgramProcessor("hlsl", mDefaultProgramProcessors.back().get());
}

//-----------------------------------------------------------------------------
void ProgramManager::destroyDefaultProgramProcessors()
{
    // removing unknown is not an error
    for(auto lang : {"glsl", "glsles", "glslang", "hlsl"})
        removeProgramProcessor(lang);
    mDefaultProgramProcessors.clear();
}

//-----------------------------------------------------------------------------
void ProgramManager::createGpuPrograms(ProgramSet* programSet)
{
    // Before we start we need to make sure that the pixel shader input
    //  parameters are the same as the vertex output, this required by 
    //  shader models 4 and 5.
    // This change may incrase the number of register used in older shader
    //  models - this is why the check is present here.
    bool isVs4 = GpuProgramManager::getSingleton().isSyntaxSupported("vs_4_0_level_9_1");
    if (isVs4)
    {
        synchronizePixelnToBeVertexOut(programSet);
    }

    // Grab the matching writer.
    std::string_view language = ShaderGenerator::getSingleton().getTargetLanguage();

    auto programWriter = ProgramWriterManager::getSingleton().getProgramWriter(language);

    auto itProcessor = mProgramProcessorsMap.find(language);
    ProgramProcessor* programProcessor = nullptr;

    if (itProcessor == mProgramProcessorsMap.end())
    {
        OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM,
            ::std::format("Could not find processor for language '{}", language),
            "ProgramManager::createGpuPrograms");       
    }

    programProcessor = itProcessor->second;
    
    // Call the pre creation of GPU programs method.
    if (!programProcessor->preCreateGpuPrograms(programSet))
        OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, "preCreateGpuPrograms failed");
    
    // Create the shader programs
    for(auto type : {GpuProgramType::VERTEX_PROGRAM, GpuProgramType::FRAGMENT_PROGRAM})
    {
        auto gpuProgram = createGpuProgram(programSet->getCpuProgram(type), programWriter, language,
                                           ShaderGenerator::getSingleton().getShaderProfiles(type),
                                           ShaderGenerator::getSingleton().getShaderCachePath());
        programSet->setGpuProgram(gpuProgram);
    }

    //update flags
    programSet->getGpuProgram(GpuProgramType::VERTEX_PROGRAM)->setSkeletalAnimationIncluded(
        programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM)->getSkeletalAnimationIncluded());

    // Call the post creation of GPU programs method.
    if(!programProcessor->postCreateGpuPrograms(programSet))
        OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR, "postCreateGpuPrograms failed");
}

//-----------------------------------------------------------------------------
auto ProgramManager::createGpuProgram(Program* shaderProgram, 
                                               ProgramWriter* programWriter,
                                               std::string_view language,
                                               std::string_view profiles,
                                               std::string_view cachePath) -> GpuProgramPtr
{
    std::stringstream sourceCodeStringStream;

    // Generate source code.
    programWriter->writeSourceCode(sourceCodeStringStream, shaderProgram);
    String source = sourceCodeStringStream.str();

    // Generate program name.
    String programName = generateHash(source, shaderProgram->getPreprocessorDefines());

    if (shaderProgram->getType() == GpuProgramType::VERTEX_PROGRAM)
    {
        programName += "_VS";
    }
    else if (shaderProgram->getType() == GpuProgramType::FRAGMENT_PROGRAM)
    {
        programName += "_FS";
    }

    // Try to get program by name.
    HighLevelGpuProgramPtr pGpuProgram =
        HighLevelGpuProgramManager::getSingleton().getByName(
            programName, ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);

    if(pGpuProgram) {
        return static_pointer_cast<GpuProgram>(pGpuProgram);
    }

    // Case the program doesn't exist yet.
    // Create new GPU program.
    pGpuProgram = HighLevelGpuProgramManager::getSingleton().createProgram(programName,
        ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, language, shaderProgram->getType());

    // Case cache directory specified -> create program from file.
    if (!cachePath.empty())
    {
        auto const programFullName = ::std::format("{}.{}", programName , programWriter->getTargetLanguage());
        auto const programFileName = std::format("{}{}", cachePath, programFullName);
        std::ifstream programFile;

        // Check if program file already exist.
        programFile.open(programFileName.c_str());

        // Case we have to write the program to a file.
        if (!programFile)
        {
            std::ofstream outFile(programFileName.c_str());

            if (!outFile)
                return {};

            outFile << source;
            outFile.close();
        }
        else
        {
            // use program file version
            StringStream buffer;
            programFile >> buffer.rdbuf();
            source = buffer.str();
        }
    }

    pGpuProgram->setSource(source);
    pGpuProgram->setParameter("preprocessor_defines", shaderProgram->getPreprocessorDefines());
    pGpuProgram->setParameter("entry_point", "main");

    if (language == "hlsl")
    {
        pGpuProgram->setParameter("target", profiles);
        pGpuProgram->setParameter("enable_backwards_compatibility", "true");
        pGpuProgram->setParameter("column_major_matrices", StringConverter::toString(shaderProgram->getUseColumnMajorMatrices()));
    }
    else if (language == "glsl")
    {
        auto* rs = Root::getSingleton().getRenderSystem();
        if( rs && rs->getNativeShadingLanguageVersion() >= 420)
            pGpuProgram->setParameter("has_sampler_binding", "true");
    }

    pGpuProgram->load();

    // Add the created GPU program to local cache.
    if (pGpuProgram->getType() == GpuProgramType::VERTEX_PROGRAM)
    {
        mVertexShaderMap[programName] = pGpuProgram;
    }
    else if (pGpuProgram->getType() == GpuProgramType::FRAGMENT_PROGRAM)
    {
        mFragmentShaderMap[programName] = pGpuProgram;
    }
    
    return static_pointer_cast<GpuProgram>(pGpuProgram);
}


//-----------------------------------------------------------------------------
auto ProgramManager::generateHash(std::string_view programString, std::string_view defines) -> String
{
    //Different programs must have unique hash values.
    uint32_t hash[4];
    uint32_t seed = FastHash(defines.data(), defines.size());
    MurmurHash3_128(programString.data(), programString.size(), seed, hash);

    //Generate the string
    return std::format("{:#08x}{:#08x}{:#08x}{:#08x}", hash[0], hash[1], hash[2], hash[3]);
}


//-----------------------------------------------------------------------------
void ProgramManager::addProgramProcessor(std::string_view lang, ProgramProcessor* processor)
{
    
    auto itFind = mProgramProcessorsMap.find(lang);

    if (itFind != mProgramProcessorsMap.end())
    {
        OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM, ::std::format("A processor for language '{}' already exists.", lang ));
    }

    mProgramProcessorsMap[lang] = processor;
}

//-----------------------------------------------------------------------------
void ProgramManager::removeProgramProcessor(std::string_view lang)
{
    auto itFind = mProgramProcessorsMap.find(lang);

    if (itFind != mProgramProcessorsMap.end())
        mProgramProcessorsMap.erase(itFind);

}

//-----------------------------------------------------------------------------
void ProgramManager::destroyGpuProgram(GpuProgramPtr& gpuProgram)
{       
    GpuProgramManager::getSingleton().remove(gpuProgram);
}

//-----------------------------------------------------------------------
void ProgramManager::synchronizePixelnToBeVertexOut( ProgramSet* programSet )
{
    Function* vertexMain = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM)->getMain();
    Function* pixelMain = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM)->getMain();

    // save the pixel program original input parameters
    auto pixelOriginalInParams = pixelMain->getInputParameters();

    // set the pixel Input to be the same as the vertex prog output
    pixelMain->deleteAllInputParameters();

    // Loop the vertex shader output parameters and make sure that
    //   all of them exist in the pixel shader input.
    // If the parameter type exist in the original output - use it
    // If the parameter doesn't exist - use the parameter from the
    //   vertex shader input.
    // The order will be based on the vertex shader parameters order
    // Write output parameters.
    for (const auto& curOutParemter : vertexMain->getOutputParameters())
    {
        ParameterPtr paramToAdd = Function::_getParameterBySemantic(
            pixelOriginalInParams, curOutParemter->getSemantic(), curOutParemter->getIndex());

        erase(pixelOriginalInParams, paramToAdd);

        if (!paramToAdd)
        {
            // param not found - we will add the one from the vertex shader
            paramToAdd = curOutParemter;
        }

        pixelMain->addInputParameter(paramToAdd);
    }

    for(const auto& paramToAdd : pixelOriginalInParams)
    {
        pixelMain->addInputParameter(paramToAdd);
    }
}

/** @} */
/** @} */
}
}
