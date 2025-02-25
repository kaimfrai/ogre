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

module Ogre.Components.RTShaderSystem;

import :ShaderFFPRenderState;
import :ShaderFunction;
import :ShaderFunctionAtom;
import :ShaderGenerator;
import :ShaderParameter;
import :ShaderPrerequisites;
import :ShaderProgram;
import :ShaderProgramProcessor;

import Ogre.Core;

import <map>;
import <memory>;
import <string>;
import <utility>;
import <vector>;

namespace Ogre::RTShader {

//-----------------------------------------------------------------------------
ProgramProcessor::ProgramProcessor()
{
    mMaxTexCoordSlots = 16;
    mMaxTexCoordFloats = mMaxTexCoordSlots * 4;
    
}

//-----------------------------------------------------------------------------
ProgramProcessor::~ProgramProcessor()
= default;

//-----------------------------------------------------------------------------
void ProgramProcessor::bindAutoParameters(Program* pCpuProgram, GpuProgramPtr pGpuProgram)
{
    GpuProgramParametersSharedPtr pGpuParams = pGpuProgram->getDefaultParameters();


    for (const UniformParameterList& progParams = pCpuProgram->getParameters();
         const UniformParameterPtr& pCurParam : progParams)
    {
        const GpuConstantDefinition* gpuConstDef = pGpuParams->_findNamedConstantDefinition(pCurParam->getName());
    
        if (gpuConstDef != nullptr)
        {
            // Handle auto parameters.
            if (pCurParam->isAutoConstantParameter())
            {
                if (pCurParam->isAutoConstantRealParameter())
                {                   
                    pGpuParams->setNamedAutoConstantReal(pCurParam->getName(), 
                        pCurParam->getAutoConstantType(), 
                        pCurParam->getAutoConstantRealData());
                                        
                }
                else if (pCurParam->isAutoConstantIntParameter())
                {                   
                    pGpuParams->setNamedAutoConstant(pCurParam->getName(), 
                        pCurParam->getAutoConstantType(), 
                        pCurParam->getAutoConstantIntData());                                   
                }                       
            }

            // Case this is not auto constant - we have to update its variability ourself.
            else
            {                           
                gpuConstDef->variability |= pCurParam->getVariability();

                // Update variability in the float map.
                if (gpuConstDef->isSampler() == false)
                {
                    GpuLogicalBufferStructPtr floatLogical = pGpuParams->getLogicalBufferStruct();
                    if (floatLogical.get())
                    {
                        for (auto & i : floatLogical->map)
                        {
                            if (i.second.physicalIndex == gpuConstDef->physicalIndex)
                            {
                                i.second.variability |= gpuConstDef->variability;
                                break;
                            }
                        }
                    }
                }                                           
            }       
        }           
    }
}

//-----------------------------------------------------------------------------
auto ProgramProcessor::compactVsOutputs(Function* vsMain, Function* fsMain) -> bool
{

    int outTexCoordSlots;
    int outTexCoordFloats;

    // Count vertex shader texcoords outputs.
    countVsTexcoordOutputs(vsMain, outTexCoordSlots, outTexCoordFloats);

    // Case the total number of used floats is bigger than maximum - nothing we can do.
    if (outTexCoordFloats > mMaxTexCoordFloats) 
        return false;

    // Only one slot used -> nothing to compact.
    if (outTexCoordSlots <= 1)  
        return true;    

    // Case compact policy is low and output slots are enough -> quit compacting process.
    if (ShaderGenerator::getSingleton().getVertexShaderOutputsCompactPolicy() == VSOutputCompactPolicy::LOW && outTexCoordSlots <= mMaxTexCoordSlots)
        return true;

    // Build output parameter tables - each row represents different parameter type (GpuConstantType::FLOAT1-4).
    ShaderParameterList vsOutParamsTable[4];
    ShaderParameterList fsInParamsTable[4];

    buildTexcoordTable(vsMain->getOutputParameters(), vsOutParamsTable);
    buildTexcoordTable(fsMain->getInputParameters(), fsInParamsTable);


    // Create merge parameters entries using the predefined merge combinations.
    MergeParameterList vsMergedParamsList;
    MergeParameterList fsMergedParamsList;
    ShaderParameterList vsSplitParams;
    ShaderParameterList fsSplitParams;
    bool hasMergedParameters = false;
    
    mergeParameters(vsOutParamsTable, vsMergedParamsList, vsSplitParams);

    
    // Check if any parameter has been merged - means at least two parameters takes the same slot.
    for (auto & i : vsMergedParamsList)
    {
        if (i.getSourceParameterCount() > 1)
        {
            hasMergedParameters = true;
            break;
        }
    }

    // Case no parameter has been merged -> quit compacting process.
    if (hasMergedParameters == false)
        return true;

    mergeParameters(fsInParamsTable, fsMergedParamsList, fsSplitParams);

    // Generate local params for split source parameters.
    LocalParameterMap vsLocalParamsMap;
    LocalParameterMap fsLocalParamsMap;

    generateLocalSplitParameters(vsMain, GpuProgramType::VERTEX_PROGRAM, vsMergedParamsList, vsSplitParams, vsLocalParamsMap);
    generateLocalSplitParameters(fsMain, GpuProgramType::FRAGMENT_PROGRAM, fsMergedParamsList, fsSplitParams, fsLocalParamsMap);

    
    // Rebuild functions parameter lists.
    rebuildParameterList(vsMain, Operand::OpSemantic::OUT, vsMergedParamsList);
    rebuildParameterList(fsMain, Operand::OpSemantic::IN, fsMergedParamsList);

    // Adjust function invocations operands to reference the new merged parameters.
    rebuildFunctionInvocations(vsMain->getAtomInstances(), vsMergedParamsList, vsLocalParamsMap);
    rebuildFunctionInvocations(fsMain->getAtomInstances(), fsMergedParamsList, fsLocalParamsMap);


    return true;
}

//-----------------------------------------------------------------------------
void ProgramProcessor::countVsTexcoordOutputs(Function* vsMain, 
                                              int& outTexCoordSlots, 
                                              int& outTexCoordFloats)
{
    outTexCoordSlots = 0;
    outTexCoordFloats = 0;

    // Grab vertex shader output information.
    for (const ShaderParameterList& vsOutputs = vsMain->getOutputParameters();
        const ParameterPtr& curParam : vsOutputs)
    {
        if (curParam->getSemantic() == Parameter::Semantic::TEXTURE_COORDINATES)
        {
            outTexCoordSlots++;
            outTexCoordFloats += getParameterFloatCount(curParam->getType());
        }
    }
}

//-----------------------------------------------------------------------------
void ProgramProcessor::buildTexcoordTable(const ShaderParameterList& paramList, ShaderParameterList outParamsTable[4])
{
    for (const ParameterPtr& curParam : paramList)
    {
        if (curParam->getSemantic() == Parameter::Semantic::TEXTURE_COORDINATES)
        {
            using enum GpuConstantType;

            switch (curParam->getType())
            {
            case FLOAT1:
                outParamsTable[0].push_back(curParam);
                break;

            case FLOAT2:
                outParamsTable[1].push_back(curParam);
                break;

            case FLOAT3:
                outParamsTable[2].push_back(curParam);
                break;

            case FLOAT4:
                outParamsTable[3].push_back(curParam);
                break;
            case SAMPLER1D:
            case SAMPLER2D:
            case SAMPLER2DARRAY:
            case SAMPLER3D:
            case SAMPLERCUBE:
            case SAMPLER1DSHADOW:
            case SAMPLER2DSHADOW:
            case MATRIX_2X2:
            case MATRIX_2X3:
            case MATRIX_2X4:
            case MATRIX_3X2:
            case MATRIX_3X3:
            case MATRIX_3X4:
            case MATRIX_4X2:
            case MATRIX_4X3:
            case MATRIX_4X4:
            case INT1:
            case INT2:
            case INT3:
            case INT4:
            case UINT1:
            case UINT2:
            case UINT3:
            case UINT4:
            case UNKNOWN:
            default:
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------
void ProgramProcessor::mergeParameters(ShaderParameterList paramsTable[4], MergeParameterList& mergedParams, 
                                      ShaderParameterList& splitParams)
{
    // Merge using the predefined combinations.
    mergeParametersByPredefinedCombinations(paramsTable, mergedParams);

    // Merge the reminders parameters if such left.
    if (paramsTable[0].size() + paramsTable[1].size() + 
        paramsTable[2].size() + paramsTable[3].size() > 0)          
    {
        mergeParametersReminders(paramsTable, mergedParams, splitParams);
    }   
}

//-----------------------------------------------------------------------------
void ProgramProcessor::mergeParametersByPredefinedCombinations(ShaderParameterList paramsTable[4], 
                                                               MergeParameterList& mergedParams)
{

    // Make sure the merge combinations are ready.
    if (mParamMergeCombinations.empty())
    {
        buildMergeCombinations();
    }

    // Create the full used merged params - means FLOAT4 params that all of their components are used.
    for (auto & curCombination : mParamMergeCombinations)
    {
        // Case all parameters have been merged.
        if (paramsTable[0].empty() && paramsTable[1].empty() &&
            paramsTable[2].empty() && paramsTable[3].empty())
            return;     

        MergeParameter curMergeParam;

        while (mergeParametersByCombination(curCombination, paramsTable, &curMergeParam))
        {
            mergedParams.push_back(curMergeParam);
            curMergeParam.clear();
        }
    }

    // Case low/medium compacting policy -> use these simplified combinations in order to prevent splits.
    if (ShaderGenerator::getSingleton().getVertexShaderOutputsCompactPolicy() == VSOutputCompactPolicy::LOW ||
        ShaderGenerator::getSingleton().getVertexShaderOutputsCompactPolicy() == VSOutputCompactPolicy::MEDIUM)
    {
        const int curUsedSlots = static_cast<int>(mergedParams.size());
        const int float1ParamCount = static_cast<int>(paramsTable[0].size());
        const int float2ParamCount = static_cast<int>(paramsTable[1].size());
        const int float3ParamCount = static_cast<int>(paramsTable[2].size());
        int       reqSlots = 0;

        // Compute the required slots.
        
        // Add all float3 since each one of them require one slot for himself.
        reqSlots += float3ParamCount;

        // Add the float2 count -> at max it will be 1 since all pairs have been merged previously.
        if (float2ParamCount > 1)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                "Invalid float2 reminder count.",
                "ProgramProcessor::mergeParametersByPredefinedCombinations");
        }
        
        reqSlots += float2ParamCount;

        // Compute how much space needed for the float1(s) that left -> at max it will be 3.
        if (float1ParamCount > 0)
        {
            if (float2ParamCount > 3)
            {
                OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                    "Invalid float1 reminder count.",
                    "ProgramProcessor::mergeParametersByPredefinedCombinations");
            }

            // No float2 -> we need one more slot for these float1(s).
            if (float2ParamCount == 0)
            {
                reqSlots += 1;
            }
            else
            {
                // Float2 exists -> there must be at max 1 float1.
                if (float1ParamCount > 1)
                {
                    OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "Invalid float1 reminder count.",
                        "ProgramProcessor::mergeParametersByPredefinedCombinations");
                }
            }
        }

        // Case maximum slot count will be exceeded -> fall back to full compaction.
        if (curUsedSlots + reqSlots > mMaxTexCoordSlots)        
            return; 

        MergeCombination simpleCombinations[6] = 
        {
            // Deal with the float3 parameters.
            MergeCombination{
            0,
            0,
            1,
            0},

            // Deal with float2 + float1 combination.
            MergeCombination{
            1,
            1,
            0,
            0},

            // Deal with the float2 parameter.
            MergeCombination{
            0,
            1,
            0,
            0},

            // Deal with the 3 float1 combination.
            MergeCombination{
            3,
            0,
            0,
            0},

            // Deal with the 2 float1 combination.
            MergeCombination{
            2,
            0,
            0,
            0},

            // Deal with the 1 float1 combination.
            MergeCombination{
            1,
            0,
            0,
            0},
            
        };

        for (const auto & curCombination : simpleCombinations)
        {
            // Case all parameters have been merged.
            if (paramsTable[0].size() + paramsTable[1].size() + paramsTable[2].size() + paramsTable[3].empty())     
                break;      

            MergeParameter curMergeParam;

            while (mergeParametersByCombination(curCombination, paramsTable, &curMergeParam))
            {
                mergedParams.push_back(curMergeParam);
                curMergeParam.clear();
            }
        }
    }
}

//-----------------------------------------------------------------------------
auto ProgramProcessor::mergeParametersByCombination(const MergeCombination& combination, 
                                    ShaderParameterList paramsTable[4], 
                                    MergeParameter* mergedParameter) -> bool
{
    // Make sure we have enough parameters to combine.
    if (combination.srcParameterTypeCount[0] > paramsTable[0].size() ||
        combination.srcParameterTypeCount[1] > paramsTable[1].size() ||
        combination.srcParameterTypeCount[2] > paramsTable[2].size() ||
        combination.srcParameterTypeCount[3] > paramsTable[3].size())
    {
        return false;
    }

    // Create the new output parameter.
    for (int i=0; i < 4; ++i)
    {
        ShaderParameterList& curParamList = paramsTable[i];     
        int srcParameterTypeCount = static_cast<int>(combination.srcParameterTypeCount[i]);
        int srcParameterCount = 0;

        while (srcParameterTypeCount > 0)
        {
            mergedParameter->addSourceParameter(curParamList.back(), combination.srcParameterMask[srcParameterCount]);
            curParamList.pop_back();
            srcParameterCount++;
            --srcParameterTypeCount;
        }
    }


    return true;
}

//-----------------------------------------------------------------------------
void ProgramProcessor::mergeParametersReminders(ShaderParameterList paramsTable[4], MergeParameterList& mergedParams, ShaderParameterList& splitParams)
{
    
    // Handle reminders parameters - All of the parameters that could not packed perfectly. 
    const size_t mergedParamsBaseIndex      = mergedParams.size();
    const size_t remindersFloatCount        = (1 * paramsTable[0].size()) + (2 * paramsTable[1].size()) + (3 * paramsTable[2].size()) + (4 * paramsTable[3].size());
    const size_t remindersFloatMod          = remindersFloatCount % 4;
    const size_t remindersFullSlotCount     = remindersFloatCount / 4;
    const size_t remindersPartialSlotCount  = remindersFloatMod > 0 ? 1 : 0;
    const size_t remindersTotalSlotCount    = remindersFullSlotCount + remindersPartialSlotCount;

    // First pass -> fill the slots with the largest reminders parameters.
    for (unsigned int slot=0; slot < remindersTotalSlotCount; ++slot)
    {
        MergeParameter curMergeParam;

        for (unsigned int row=0; row < 4; ++row)
        {
            ShaderParameterList& curParamList = paramsTable[3 - row];       

            // Case this list contains parameters -> pop it out and add to merged params.
            if (curParamList.size() > 0)
            {
                curMergeParam.addSourceParameter(curParamList.back(), Operand::OpMask::ALL);
                curParamList.pop_back();
                mergedParams.push_back(curMergeParam);
                break;
            }
        }
    }

    // Second pass -> merge the reminders parameters.
    for (unsigned int row=0; row < 4; ++row)
    {
        ShaderParameterList& curParamList = paramsTable[3 - row];       

        // Merge the all the parameters of the current list.
        while (curParamList.size() > 0)
        {
            ParameterPtr srcParameter  = curParamList.back();
            int splitCount             = 0;     // How many times the source parameter has been split.

            auto srcParameterFloats = getParameterFloatCount(srcParameter->getType());
            auto curSrcParameterFloats = srcParameterFloats;
            auto srcParameterComponents = getParameterMaskByType(srcParameter->getType());

            
            // While this parameter has remaining components -> split it.
            while (curSrcParameterFloats > 0)
            {           
                for (unsigned int slot=0; slot < remindersTotalSlotCount && curSrcParameterFloats > 0; ++slot)
                {
                    MergeParameter& curMergeParam = mergedParams[mergedParamsBaseIndex + slot];
                    const int freeFloatCount = 4 - curMergeParam.getUsedFloatCount();

                    // Case this slot has free space.
                    if (freeFloatCount > 0)
                    {
                        // Case current components of source parameter can go all into this slot without split.
                        if (srcParameterFloats < freeFloatCount && splitCount == 0)
                        {                               
                            curMergeParam.addSourceParameter(srcParameter, Operand::OpMask::ALL);
                        }

                        // Case we have to split the current source parameter.
                        else
                        {
                            // Create the mask that tell us which part of the source component is added to the merged parameter.
                            auto srcComponentsMask = getParameterMaskByFloatCount(freeFloatCount) << splitCount;

                            // Add the partial source parameter to merged parameter.
                            curMergeParam.addSourceParameter(srcParameter, srcComponentsMask & srcParameterComponents);
                        }
                        splitCount++;

                        // Update left floats count.
                        if (srcParameterFloats < freeFloatCount)
                        {
                            curSrcParameterFloats -= srcParameterFloats;
                        }
                        else
                        {
                            curSrcParameterFloats -= freeFloatCount;
                        }                        
                    }
                }
            }                                   

            // Add to split params list.
            if (splitCount > 1)
                splitParams.push_back(srcParameter);


            curParamList.pop_back();
        }
    }               
}

//-----------------------------------------------------------------------------
void ProgramProcessor::rebuildParameterList(Function* func, Operand::OpSemantic paramsUsage, MergeParameterList& mergedParams)
{
    // Delete the old merged parameters.
    for (auto & curMergeParameter : mergedParams)
    {
        for (unsigned int j=0; j < curMergeParameter.getSourceParameterCount(); ++j)
        {
            ParameterPtr curSrcParam = curMergeParameter.getSourceParameter(j);

            if (paramsUsage == Operand::OpSemantic::OUT)
            {
                func->deleteOutputParameter(curSrcParam);
            }
            else if (paramsUsage == Operand::OpSemantic::IN)
            {
                func->deleteInputParameter(curSrcParam);
            }
        }
    }

    // Add the new combined parameters.
    for (unsigned int i=0; i < mergedParams.size(); ++i)
    {
        MergeParameter& curMergeParameter = mergedParams[i];
        
        if (paramsUsage == Operand::OpSemantic::OUT)
        {           
            func->addOutputParameter(curMergeParameter.getDestinationParameter(paramsUsage, i));
        }
        else if (paramsUsage == Operand::OpSemantic::IN)
        {
            func->addInputParameter(curMergeParameter.getDestinationParameter(paramsUsage, i));
        }
    }
}

//-----------------------------------------------------------------------------
void ProgramProcessor::generateLocalSplitParameters(Function* func, GpuProgramType progType,
                                                   MergeParameterList& mergedParams, 
                                                   ShaderParameterList& splitParams, LocalParameterMap& localParamsMap)
{
    // No split params created.
    if (splitParams.empty())    
        return; 

    // Create the local parameters + map from source to local.
    for (auto srcParameter : splitParams)
    {
        ParameterPtr localParameter = func->resolveLocalParameter(srcParameter->getType(), ::std::format("lsplit_{}", srcParameter->getName()));

        localParamsMap[srcParameter.get()] = localParameter;        
    }   

    // Establish link between the local parameter to the merged parameter.
    for (unsigned int i=0; i < mergedParams.size(); ++i)
    {
        MergeParameter& curMergeParameter = mergedParams[i];

        for (unsigned int p=0; p < curMergeParameter.getSourceParameterCount(); ++p)
        {
            ParameterPtr srcMergedParameter    = curMergeParameter.getSourceParameter(p);
            auto itFind = localParamsMap.find(srcMergedParameter.get());

            // Case the source parameter is split parameter.
            if (itFind != localParamsMap.end())
            {
                // Case it is the vertex shader -> assign the local parameter to the output merged parameter.
                if (progType == GpuProgramType::VERTEX_PROGRAM)
                {
                    FunctionAtom* curFuncInvocation = new AssignmentAtom(std::to_underlying(FFPVertexShaderStage::POST_PROCESS));
                    
                    curFuncInvocation->pushOperand(itFind->second, Operand::OpSemantic::IN, curMergeParameter.getSourceParameterMask(p));
                    curFuncInvocation->pushOperand(curMergeParameter.getDestinationParameter(Operand::OpSemantic::OUT, i), Operand::OpSemantic::OUT, curMergeParameter.getDestinationParameterMask(p));
                    func->addAtomInstance(curFuncInvocation);       
                }
                else if (progType == GpuProgramType::FRAGMENT_PROGRAM)
                {
                    FunctionAtom* curFuncInvocation = new AssignmentAtom(std::to_underlying(FFPFragmentShaderStage::PRE_PROCESS));
                    
                    curFuncInvocation->pushOperand(curMergeParameter.getDestinationParameter(Operand::OpSemantic::IN, i), Operand::OpSemantic::IN, curMergeParameter.getDestinationParameterMask(p));
                    curFuncInvocation->pushOperand(itFind->second, Operand::OpSemantic::OUT, curMergeParameter.getSourceParameterMask(p));
                    func->addAtomInstance(curFuncInvocation);       
                }
            }
        }
    }               
}

//-----------------------------------------------------------------------------
void ProgramProcessor::rebuildFunctionInvocations(const FunctionAtomInstanceList& funcAtomList,
                                                  MergeParameterList& mergedParams,
                                                  LocalParameterMap& localParamsMap)
{   
    ParameterOperandMap paramsRefMap;

    // Build reference map of source parameters.
    buildParameterReferenceMap(funcAtomList, paramsRefMap);

    // Replace references to old parameters with references to new parameters.
    replaceParametersReferences(mergedParams, paramsRefMap);


    // Replace references to old parameters with references to new split parameters.
    replaceSplitParametersReferences(localParamsMap, paramsRefMap);

}

//-----------------------------------------------------------------------------
void ProgramProcessor::buildParameterReferenceMap(const FunctionAtomInstanceList& funcAtomList, ParameterOperandMap& paramsRefMap)
{
    for (const auto& func : funcAtomList)
    {
        for (Operand& curOperand : func->getOperandList())
        {
            paramsRefMap[curOperand.getParameter().get()].push_back(&curOperand);
        }
    }
}

//-----------------------------------------------------------------------------
void ProgramProcessor::replaceParametersReferences(MergeParameterList& mergedParams, ParameterOperandMap& paramsRefMap)
{
    for (unsigned int i=0; i < mergedParams.size(); ++i)
    {
        MergeParameter& curMergeParameter = mergedParams[i];
        int paramBitMaskOffset = 0;

        for (unsigned int j=0; j < curMergeParameter.getSourceParameterCount(); ++j)
        {
            ParameterPtr      curSrcParam  = curMergeParameter.getSourceParameter(j);
            auto itParamRefs = paramsRefMap.find(curSrcParam.get());

            // Case the source parameter has some references
            if (itParamRefs != paramsRefMap.end())
            {
                OperandPtrVector& srcParamRefs = itParamRefs->second;
                ParameterPtr dstParameter;

                // Case the source parameter is fully contained within the destination merged parameter.
                if (curMergeParameter.getSourceParameterMask(j) == Operand::OpMask::ALL)
                {
                    dstParameter = curMergeParameter.getDestinationParameter(Operand::OpSemantic::INOUT, i);

                    for (auto srcOperandPtr : srcParamRefs)
                    {
                        Operand::OpMask       dstOpMask;

                        if (srcOperandPtr->getMask() == Operand::OpMask::ALL)
                        {
                            // Case the merged parameter contains only one source - no point in adding special mask.
                            if (curMergeParameter.getSourceParameterCount() == 1)
                            {
                                dstOpMask = Operand::OpMask::ALL;
                            }
                            else
                            {
                                dstOpMask = getParameterMaskByType(curSrcParam->getType()) << paramBitMaskOffset;                           
                            }
                        }
                        else
                        {
                            dstOpMask = srcOperandPtr->getMask() << paramBitMaskOffset;
                        }

                        // Replace the original source operand with a new operand the reference the new merged parameter.                       
                        *srcOperandPtr = Operand(dstParameter, srcOperandPtr->getSemantic(), dstOpMask);
                    }
                }
            }   


            // Update the bit mask offset. 
            paramBitMaskOffset += getParameterFloatCount(curSrcParam->getType());           
        }
    }
}

//-----------------------------------------------------------------------------
void ProgramProcessor::replaceSplitParametersReferences(LocalParameterMap& localParamsMap, ParameterOperandMap& paramsRefMap)
{
    for (auto const& [curSrcParam, dstParameter] : localParamsMap)
    {
        auto itParamRefs = paramsRefMap.find(curSrcParam);

        if (itParamRefs != paramsRefMap.end())
        {
            OperandPtrVector& srcParamRefs = itParamRefs->second;

            for (auto srcOperandPtr : srcParamRefs)
            {
                Operand::OpMask dstOpMask;

                if (srcOperandPtr->getMask() == Operand::OpMask::ALL)
                {                   
                    dstOpMask = getParameterMaskByType(curSrcParam->getType());
                }
                else
                {
                    dstOpMask = srcOperandPtr->getMask();
                }

                // Replace the original source operand with a new operand the reference the new merged parameter.                       
                *srcOperandPtr = Operand(dstParameter, srcOperandPtr->getSemantic(), dstOpMask);
            }
        }
    }
}

//-----------------------------------------------------------------------------
auto ProgramProcessor::getParameterFloatCount(GpuConstantType type) -> int
{
    int floatCount = 0;

    using enum GpuConstantType;
    switch (type)
    {
    case FLOAT1: floatCount = 1; break;
    case FLOAT2: floatCount = 2; break;
    case FLOAT3: floatCount = 3; break;
    case FLOAT4: floatCount = 4; break;
    default:
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
            "Invalid parameter float type.",
            "ProgramProcessor::getParameterFloatCount");
    }

    return floatCount;
}

//-----------------------------------------------------------------------------
auto ProgramProcessor::getParameterMaskByType(GpuConstantType type) -> Operand::OpMask
{
    using enum GpuConstantType;
    switch (type)
    {
    case FLOAT1: return Operand::OpMask::X;
    case FLOAT2: return Operand::OpMask::XY;
    case FLOAT3: return Operand::OpMask::XYZ;
    case FLOAT4: return Operand::OpMask::XYZW;
    default:
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Invalid parameter type.");
    }
}

//-----------------------------------------------------------------------------
auto ProgramProcessor::getParameterMaskByFloatCount(int floatCount) -> Operand::OpMask
{
    switch (floatCount)
    {
    case 1: return Operand::OpMask::X;
    case 2: return Operand::OpMask::XY;
    case 3: return Operand::OpMask::XYZ;
    case 4: return Operand::OpMask::XYZW;
    default:
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Invalid parameter float type");
    }
}



//-----------------------------------------------------------------------------
void ProgramProcessor::buildMergeCombinations()
{
    mParamMergeCombinations.push_back(
        MergeCombination{
        1,
        0,
        1,
        0});

    mParamMergeCombinations.push_back(
        MergeCombination{
        2,
        1,
        0,
        0});

    mParamMergeCombinations.push_back(
        MergeCombination{
        4,
        0,
        0,
        0});

    mParamMergeCombinations.push_back(
        MergeCombination{
        0,
        2,
        0,
        0});

    mParamMergeCombinations.push_back(
        MergeCombination{
        0,
        0,
        0,
        1});
}

//-----------------------------------------------------------------------------
ProgramProcessor::MergeParameter::MergeParameter()
{
    clear();
}

//-----------------------------------------------------------------------------
void ProgramProcessor::MergeParameter::addSourceParameter(ParameterPtr srcParam, Operand::OpMask mask)
{
    // Case source count exceeded maximum
    if (mSrcParameterCount >= 4)
    {
        OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR,
            "Merged parameter source parameters overflow",
            "MergeParameter::addSourceParameter");
    }
    
    mSrcParameter[mSrcParameterCount]     = srcParam;
    mSrcParameterMask[mSrcParameterCount] = mask;

    if (mask == Operand::OpMask::ALL)
    {
        mDstParameterMask[mSrcParameterCount] = mask;

        mUsedFloatCount += getParameterFloatCount(srcParam->getType()); 
    }
    else
    {       
        int srcParamFloatCount = Operand::getFloatCount(mask);

        mDstParameterMask[mSrcParameterCount] = Operand::OpMask(getParameterMaskByFloatCount(srcParamFloatCount) << mUsedFloatCount);
        mUsedFloatCount += srcParamFloatCount;
    }
    
    mSrcParameterCount++;


    // Case float count exceeded maximum
    if (mUsedFloatCount > 4)
    {
        OGRE_EXCEPT(ExceptionCodes::INTERNAL_ERROR,
            "Merged parameter floats overflow",
            "MergeParameter::addSourceParameter");
    }

}

//-----------------------------------------------------------------------------
auto ProgramProcessor::MergeParameter::getUsedFloatCount() noexcept -> int
{
    return mUsedFloatCount;
}

//-----------------------------------------------------------------------------
void ProgramProcessor::MergeParameter::createDestinationParameter(Operand::OpSemantic usage, int index)
{
    GpuConstantType dstParamType = GpuConstantType::UNKNOWN;

    switch (getUsedFloatCount())
    {
    case 1:
        dstParamType = GpuConstantType::FLOAT1;
        break;

    case 2:
        dstParamType = GpuConstantType::FLOAT2;
        break;

    case 3:
        dstParamType = GpuConstantType::FLOAT3;
        break;

    case 4:
        dstParamType = GpuConstantType::FLOAT4;
        break;

    }


    if (usage == Operand::OpSemantic::IN)
    {
        mDstParameter = ParameterFactory::createInTexcoord(dstParamType, index, Parameter::Content::UNKNOWN);
    }
    else if (usage == Operand::OpSemantic::OUT)
    {
        mDstParameter = ParameterFactory::createOutTexcoord(dstParamType, index, Parameter::Content::UNKNOWN);
    }
}

//-----------------------------------------------------------------------------
auto ProgramProcessor::MergeParameter::getDestinationParameter( Operand::OpSemantic usage, int index) -> Ogre::RTShader::ParameterPtr
{
    if (!mDstParameter)
        createDestinationParameter(usage, index);

    return mDstParameter;
}

//-----------------------------------------------------------------------------
void ProgramProcessor::MergeParameter::clear()
{
    mDstParameter.reset();
    for (unsigned int i=0; i < 4; ++i)
    {
        mSrcParameter[i].reset();
        mSrcParameterMask[i] = Operand::OpMask::NONE;
        mDstParameterMask[i] = Operand::OpMask::NONE;
    }   
    mSrcParameterCount = 0;
    mUsedFloatCount = 0;
}

}
