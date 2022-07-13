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
#include <cstddef>
#include <format>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "OgreException.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderFunction.hpp"
#include "OgreShaderFunctionAtom.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreString.hpp"
#include "OgreStringConverter.hpp"

namespace Ogre::RTShader {

static auto typeFromContent(Parameter::Content content) -> GpuConstantType
{
    switch (content)
    {
    case Parameter::Content::BLEND_INDICES:
        return GpuConstantType::UINT4;
    case Parameter::Content::COLOR_DIFFUSE:
    case Parameter::Content::COLOR_SPECULAR:
    case Parameter::Content::POSITION_PROJECTIVE_SPACE:
    case Parameter::Content::POSITION_OBJECT_SPACE:
    case Parameter::Content::BLEND_WEIGHTS:
    case Parameter::Content::POSITION_LIGHT_SPACE0:
    case Parameter::Content::POSITION_LIGHT_SPACE1:
    case Parameter::Content::POSITION_LIGHT_SPACE2:
    case Parameter::Content::POSITION_LIGHT_SPACE3:
    case Parameter::Content::POSITION_LIGHT_SPACE4:
    case Parameter::Content::POSITION_LIGHT_SPACE5:
    case Parameter::Content::POSITION_LIGHT_SPACE6:
    case Parameter::Content::POSITION_LIGHT_SPACE7:
        return GpuConstantType::FLOAT4;
    case Parameter::Content::NORMAL_TANGENT_SPACE:
    case Parameter::Content::NORMAL_OBJECT_SPACE:
    case Parameter::Content::NORMAL_WORLD_SPACE:
    case Parameter::Content::NORMAL_VIEW_SPACE:
    case Parameter::Content::TANGENT_OBJECT_SPACE:
    case Parameter::Content::POSTOCAMERA_TANGENT_SPACE:
    case Parameter::Content::POSTOCAMERA_OBJECT_SPACE:
    case Parameter::Content::POSTOCAMERA_VIEW_SPACE:
    case Parameter::Content::POSITION_VIEW_SPACE:
    case Parameter::Content::POSITION_WORLD_SPACE:
    case Parameter::Content::LIGHTDIRECTION_OBJECT_SPACE0:
    case Parameter::Content::LIGHTDIRECTION_OBJECT_SPACE1:
    case Parameter::Content::LIGHTDIRECTION_OBJECT_SPACE2:
    case Parameter::Content::LIGHTDIRECTION_OBJECT_SPACE3:
    case Parameter::Content::LIGHTDIRECTION_OBJECT_SPACE4:
    case Parameter::Content::LIGHTDIRECTION_OBJECT_SPACE5:
    case Parameter::Content::LIGHTDIRECTION_OBJECT_SPACE6:
    case Parameter::Content::LIGHTDIRECTION_OBJECT_SPACE7:
    case Parameter::Content::POSTOLIGHT_OBJECT_SPACE0:
    case Parameter::Content::POSTOLIGHT_OBJECT_SPACE1:
    case Parameter::Content::POSTOLIGHT_OBJECT_SPACE2:
    case Parameter::Content::POSTOLIGHT_OBJECT_SPACE3:
    case Parameter::Content::POSTOLIGHT_OBJECT_SPACE4:
    case Parameter::Content::POSTOLIGHT_OBJECT_SPACE5:
    case Parameter::Content::POSTOLIGHT_OBJECT_SPACE6:
    case Parameter::Content::POSTOLIGHT_OBJECT_SPACE7:
    case Parameter::Content::LIGHTDIRECTION_TANGENT_SPACE0:
    case Parameter::Content::LIGHTDIRECTION_TANGENT_SPACE1:
    case Parameter::Content::LIGHTDIRECTION_TANGENT_SPACE2:
    case Parameter::Content::LIGHTDIRECTION_TANGENT_SPACE3:
    case Parameter::Content::LIGHTDIRECTION_TANGENT_SPACE4:
    case Parameter::Content::LIGHTDIRECTION_TANGENT_SPACE5:
    case Parameter::Content::LIGHTDIRECTION_TANGENT_SPACE6:
    case Parameter::Content::LIGHTDIRECTION_TANGENT_SPACE7:
    case Parameter::Content::POSTOLIGHT_TANGENT_SPACE0:
    case Parameter::Content::POSTOLIGHT_TANGENT_SPACE1:
    case Parameter::Content::POSTOLIGHT_TANGENT_SPACE2:
    case Parameter::Content::POSTOLIGHT_TANGENT_SPACE3:
    case Parameter::Content::POSTOLIGHT_TANGENT_SPACE4:
    case Parameter::Content::POSTOLIGHT_TANGENT_SPACE5:
    case Parameter::Content::POSTOLIGHT_TANGENT_SPACE6:
    case Parameter::Content::POSTOLIGHT_TANGENT_SPACE7:
    case Parameter::Content::LIGHTDIRECTION_VIEW_SPACE0:
        return GpuConstantType::FLOAT3;
    case Parameter::Content::POINTSPRITE_COORDINATE:
        return GpuConstantType::FLOAT2;
    case Parameter::Content::POINTSPRITE_SIZE:
    case Parameter::Content::DEPTH_VIEW_SPACE:
        return GpuConstantType::FLOAT1;
    case Parameter::Content::FRONT_FACING:
        return GpuConstantType::FLOAT1;
    default:
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "cannot derive type from content");
        break;
    }
}

static auto semanticFromContent(Parameter::Content content, bool isVSOut = false) -> Parameter::Semantic
{
    switch (content)
    {
    case Parameter::Content::COLOR_DIFFUSE:
    case Parameter::Content::COLOR_SPECULAR:
        return Parameter::Semantic::COLOR;
    case Parameter::Content::POSITION_PROJECTIVE_SPACE:
        return Parameter::Semantic::POSITION;
    case Parameter::Content::BLEND_INDICES:
        return Parameter::Semantic::BLEND_INDICES;
    case Parameter::Content::BLEND_WEIGHTS:
        return Parameter::Semantic::BLEND_WEIGHTS;
    case Parameter::Content::POINTSPRITE_COORDINATE:
        return Parameter::Semantic::TEXTURE_COORDINATES;
    case Parameter::Content::BINORMAL_OBJECT_SPACE:
        return Parameter::Semantic::BINORMAL;
    case Parameter::Content::FRONT_FACING:
        return Parameter::Semantic::FRONT_FACING;
    case Parameter::Content::TANGENT_OBJECT_SPACE:
        if(!isVSOut) return Parameter::Semantic::TANGENT;
        [[fallthrough]];
    case Parameter::Content::POSITION_OBJECT_SPACE:
        if(!isVSOut) return Parameter::Semantic::POSITION;
        [[fallthrough]];
    case Parameter::Content::NORMAL_OBJECT_SPACE:
        if(!isVSOut) return Parameter::Semantic::NORMAL;
        [[fallthrough]];
    // the remaining types are VS output types only (or indeed texcoord)
    // for out types we use the TEXCOORD[n] semantics for compatibility
    // with Cg, HLSL SM2.0 where they are the only multivariate semantics
    default:
        return Parameter::Semantic::TEXTURE_COORDINATES;
    }
}

/// fixed index for texcoords, next free semantic slot else
static auto indexFromContent(Parameter::Content content) -> int
{
    if(content < Parameter::Content::TEXTURE_COORDINATE0 || content > Parameter::Content::TEXTURE_COORDINATE7)
        return -1;

    return std::to_underlying(content) - std::to_underlying(Parameter::Content::TEXTURE_COORDINATE0);
}

void FunctionStageRef::callFunction(const char* name, const InOut& inout) const
{
    callFunction(name, std::vector<Operand>{inout});
}

void FunctionStageRef::callFunction(const char* name, const std::vector<Operand>& params) const
{
    auto function = new FunctionInvocation(name, mStage);
    function->setOperands(params);
    mParent->addAtomInstance(function);
}

void FunctionStageRef::sampleTexture(const std::vector<Operand>& params) const
{
    auto function = new SampleTextureAtom(mStage);
    function->setOperands(params);
    mParent->addAtomInstance(function);
}

void FunctionStageRef::assign(const std::vector<Operand>& params) const
{
    auto function = new AssignmentAtom(mStage);
    function->setOperands(params);
    mParent->addAtomInstance(function);
}

void FunctionStageRef::binaryOp(char op, const std::vector<Operand>& params) const
{
    auto function = new BinaryOpAtom(op, mStage);
    function->setOperands(params);
    mParent->addAtomInstance(function);
}

//-----------------------------------------------------------------------------
Function::~Function()
{
    mAtomInstances.clear();

    for (auto & mInputParameter : mInputParameters)
        mInputParameter.reset();
    mInputParameters.clear();

    for (auto & mOutputParameter : mOutputParameters)
        mOutputParameter.reset();
    mOutputParameters.clear();

    for (auto & mLocalParameter : mLocalParameters)
        mLocalParameter.reset();
    mLocalParameters.clear();

}

static auto getParameterName(const char* prefix, Parameter::Semantic semantic, int index) -> String
{
    const char* name = "";
    switch (semantic)
    {
    case Parameter::Semantic::POSITION:
        name = "Pos";
        break;
    case Parameter::Semantic::BLEND_WEIGHTS:
        name = "BlendWeights";
        break;
    case Parameter::Semantic::BLEND_INDICES:
        name = "BlendIndices";
        break;
    case Parameter::Semantic::NORMAL:
        name = "Normal";
        break;
    case Parameter::Semantic::COLOR:
        name = "Color";
        break;
    case Parameter::Semantic::TEXTURE_COORDINATES:
        name = "Texcoord";
        break;
    case Parameter::Semantic::BINORMAL:
        name = "BiNormal";
        break;
    case Parameter::Semantic::TANGENT:
        name = "Tangent";
        break;
    case Parameter::Semantic::FRONT_FACING:
        name = "FrontFacing";
        break;
    case Parameter::Semantic::UNKNOWN:
        name = "Param";
        break;
    };
    return std::format("{}{}_{}", prefix, name, index);
}

//-----------------------------------------------------------------------------
auto Function::resolveInputParameter(Parameter::Semantic semantic,
                                        int index,
                                        const Parameter::Content content,
                                        GpuConstantType type) -> ParameterPtr
{
    if(type == GpuConstantType::UNKNOWN)
        type = typeFromContent(content);

    ParameterPtr param;

    // Check if desired parameter already defined.
    param = _getParameterByContent(mInputParameters, content, type);
    if (param.get() != nullptr)
        return param;

    if(semantic == Parameter::Semantic::UNKNOWN)
    {
        semantic = semanticFromContent(content);
        index = indexFromContent(content); // create new parameter for this content
    }

    // Case we have to create new parameter.
    if (index == -1)
    {
        index = 0;

        // Find the next available index of the target semantic.
        for (auto const& it : mInputParameters)
        {
            if (it->getSemantic() == semantic)
            {
                index++;
            }
        }
    }
    else
    {
        // Check if desired parameter already defined.
        param = _getParameterBySemantic(mInputParameters, semantic, index);
        if (param.get() != nullptr && param->getContent() == content)
        {
            if (param->getType() == type)
            {
                return param;
            }
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        std::format("Can not resolve parameter due to type mismatch: semantic: {}, index: {}",
                            semantic, index));
        }
    }

    
    // No parameter found -> create new one.
    OgreAssert(semantic != Parameter::Semantic::UNKNOWN, "unknown semantic");
    param =
        std::make_shared<Parameter>(type, getParameterName("i", semantic, index), semantic, index, content);
    addInputParameter(param);

    return param;
}

//-----------------------------------------------------------------------------
auto Function::resolveOutputParameter(Parameter::Semantic semantic,
                                            int index,
                                            Parameter::Content content,                                         
                                            GpuConstantType type) -> ParameterPtr
{
    if(type == GpuConstantType::UNKNOWN)
        type = typeFromContent(content);

    ParameterPtr param;

    // Check if desired parameter already defined.
    param = _getParameterByContent(mOutputParameters, content, type);
    if (param.get() != nullptr)
        return param;

    if(semantic == Parameter::Semantic::UNKNOWN)
    {
        semantic = semanticFromContent(content, true);
        index = -1; // create new parameter for this content
    }

    // Case we have to create new parameter.
    if (index == -1)
    {
        index = 0;

        // Find the next available index of the target semantic.
        for (auto const& it : mOutputParameters)
        {
            if (it->getSemantic() == semantic)
            {
                index++;
            }
        }
    }
    else
    {
        // Check if desired parameter already defined.
        param = _getParameterBySemantic(mOutputParameters, semantic, index);
        if (param.get() != nullptr && param->getContent() == content)
        {
            if (param->getType() == type)
            {
                return param;
            }
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        std::format("Can not resolve parameter due to type mismatch: semantic: {}, index: {}",
                            semantic, index));
        }
    }
    

    // No parameter found -> create new one.
    switch (semantic)
    {
    case Parameter::Semantic::TEXTURE_COORDINATES:
    case Parameter::Semantic::COLOR:
    case Parameter::Semantic::POSITION:
        param = std::make_shared<Parameter>(type, getParameterName("o", semantic, index), semantic, index,
                                            content);
        break;

    default:
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                    std::format("Semantic not supported as output parameter: {}", semantic));
        break;
    }

    addOutputParameter(param);

    return param;
}

//-----------------------------------------------------------------------------
auto Function::resolveLocalParameter(GpuConstantType type, std::string_view name) -> ParameterPtr
{
    ParameterPtr param;

    param = _getParameterByName(mLocalParameters, name);
    if (param.get() != nullptr)
    {
        if (param->getType() == type)
        {
            return param;
        }
        else 
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "Can not resolve local parameter due to type mismatch");
        }       
    }
        
    param = std::make_shared<Parameter>(type, name, Parameter::Semantic::UNKNOWN, 0, Parameter::Content::UNKNOWN);
    addParameter(mLocalParameters, param);
            
    return param;
}

//-----------------------------------------------------------------------------
auto Function::resolveLocalParameter(const Parameter::Content content, GpuConstantType type) -> ParameterPtr
{
    ParameterPtr param;

    if(type == GpuConstantType::UNKNOWN) type = typeFromContent(content);

    param = _getParameterByContent(mLocalParameters, content, type);
    if (param.get() != nullptr)    
        return param;

    param = std::make_shared<Parameter>(
        type, getParameterName("l", semanticFromContent(content), mLocalParameters.size()),
        Parameter::Semantic::UNKNOWN, 0, content);
    addParameter(mLocalParameters, param);

    return param;
}

//-----------------------------------------------------------------------------
void Function::addInputParameter(ParameterPtr parameter)
{

    // Check that parameter with the same semantic and index in input parameters list.
    if (_getParameterBySemantic(mInputParameters, parameter->getSemantic(), parameter->getIndex()).get() != nullptr)
    {
        OGRE_EXCEPT( ExceptionCodes::INVALIDPARAMS, 
            ::std::format("Parameter '{}' has equal semantic parameter", parameter->getName() ));
    }

    addParameter(mInputParameters, parameter);
}

//-----------------------------------------------------------------------------
void Function::addOutputParameter(ParameterPtr parameter)
{
    // Check that parameter with the same semantic and index in output parameters list.
    if (_getParameterBySemantic(mOutputParameters, parameter->getSemantic(), parameter->getIndex()).get() != nullptr)
    {
        OGRE_EXCEPT( ExceptionCodes::INVALIDPARAMS, 
            ::std::format("Parameter '{}' has equal semantic parameter", parameter->getName() ));
    }

    addParameter(mOutputParameters, parameter);
}

//-----------------------------------------------------------------------------
void Function::deleteInputParameter(ParameterPtr parameter)
{
    deleteParameter(mInputParameters, parameter);
}

//-----------------------------------------------------------------------------
void Function::deleteOutputParameter(ParameterPtr parameter)
{
    deleteParameter(mOutputParameters, parameter);
}

//-----------------------------------------------------------------------------
void Function::deleteAllInputParameters()
{
    mInputParameters.clear();
}

//-----------------------------------------------------------------------------
void Function::deleteAllOutputParameters()
{
    mOutputParameters.clear();
}
//-----------------------------------------------------------------------------
void Function::addParameter(ShaderParameterList& parameterList, ParameterPtr parameter)
                                        
{
    // Check that parameter with the same name doest exist in input parameters list.
    if (_getParameterByName(mInputParameters, parameter->getName()).get() != nullptr)
    {
        OGRE_EXCEPT( ExceptionCodes::INVALIDPARAMS, 
            ::std::format("Parameter '{}' already declared", parameter->getName() ));
    }

    // Check that parameter with the same name doest exist in output parameters list.
    if (_getParameterByName(mOutputParameters, parameter->getName()).get() != nullptr)
    {
        OGRE_EXCEPT( ExceptionCodes::INVALIDPARAMS, 
            ::std::format("Parameter '{}' already declared", parameter->getName() ));
    }


    // Add to given parameters list.
    parameterList.push_back(parameter);
}

//-----------------------------------------------------------------------------
void Function::deleteParameter(ShaderParameterList& parameterList, ParameterPtr parameter)
{
    for (auto it = parameterList.begin(); it != parameterList.end(); ++it)
    {
        if (*it == parameter)
        {
            (*it).reset();
            parameterList.erase(it);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
auto Function::_getParameterByName( const ShaderParameterList& parameterList, std::string_view name ) -> ParameterPtr
{
    for (auto const& it : parameterList)
    {
        if (it->getName() == name)
        {
            return it;
        }
    }

    return {};
}

//-----------------------------------------------------------------------------
auto Function::_getParameterBySemantic(const ShaderParameterList& parameterList,
                                                const Parameter::Semantic semantic, 
                                                int index) -> ParameterPtr
{
    for (auto const& it : parameterList)
    {
        if (it->getSemantic() == semantic &&
            it->getIndex() == index)
        {
            return it;
        }
    }

    return {};
}

//-----------------------------------------------------------------------------
auto Function::_getParameterByContent(const ShaderParameterList& parameterList, const Parameter::Content content, GpuConstantType type) -> ParameterPtr
{
    if(type == GpuConstantType::UNKNOWN)
        type = typeFromContent(content);

    // Search only for known content.
    if (content != Parameter::Content::UNKNOWN)  
    {
        for (auto const& it : parameterList)
        {
            if (it->getContent() == content &&
                it->getType() == type)
            {
                return it;
            }
        }
    }
    
    return {};
}


//-----------------------------------------------------------------------------
void Function::addAtomInstance(FunctionAtom* atomInstance)
{
    mAtomInstances[atomInstance->getGroupExecutionOrder()].emplace_back(atomInstance);
    mSortedAtomInstances.clear();
}

//-----------------------------------------------------------------------------
auto Function::deleteAtomInstance(FunctionAtom* atomInstance) -> bool
{
    size_t g = atomInstance->getGroupExecutionOrder();
    for (auto it=mAtomInstances[g].begin(); it != mAtomInstances[g].end(); ++it)
    {
        if (it->get() == atomInstance)
        {
            mAtomInstances[g].erase(it);
            mSortedAtomInstances.clear();
            return true;
        }       
    }

    return false;
    
}

//-----------------------------------------------------------------------------
auto Function::getAtomInstances() noexcept -> const FunctionAtomInstanceList&
{
    if(!mSortedAtomInstances.empty())
        return mSortedAtomInstances;

    // put atom instances into order
    for(auto & mAtomInstance : mAtomInstances)
    {
        ::std::ranges::transform
        (   mAtomInstance.second,
            ::std::inserter(mSortedAtomInstances, mSortedAtomInstances.end()),
            ::std::mem_fn(&::std::unique_ptr<FunctionAtom>::get)
        );
    }

    return mSortedAtomInstances;
}

}
