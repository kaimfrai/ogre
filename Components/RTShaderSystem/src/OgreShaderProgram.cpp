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

import :ShaderFunction;
import :ShaderParameter;
import :ShaderPrerequisites;
import :ShaderProgram;

import Ogre.Core;

import <algorithm>;
import <format>;
import <memory>;
import <string>;
import <utility>;
import <vector>;

namespace Ogre::RTShader {

//-----------------------------------------------------------------------------
Program::Program(GpuProgramType type)
{
    mType               = type;
    // all programs must have an entry point
    mEntryPointFunction = ::std::make_unique<Function>();
    mSkeletalAnimation  = false;
    mColumnMajorMatrices = true;
}

//-----------------------------------------------------------------------------
Program::~Program()
{
    destroyParameters();
}

//-----------------------------------------------------------------------------
void Program::destroyParameters()
{
    mParameters.clear();
}

//-----------------------------------------------------------------------------
auto Program::getType() const -> GpuProgramType
{
    return mType;
}

//-----------------------------------------------------------------------------
void Program::addParameter(UniformParameterPtr parameter)
{
    if (getParameterByName(parameter->getName()).get() != nullptr)
    {
        OGRE_EXCEPT( ExceptionCodes::INVALIDPARAMS, 
            ::std::format("Parameter '{}' already declared in program.", parameter->getName() ),
            "Program::addParameter" );
    }

    mParameters.push_back(parameter);
}

//-----------------------------------------------------------------------------
void Program::removeParameter(UniformParameterPtr parameter)
{
    for (auto it = mParameters.begin(); it != mParameters.end(); ++it)
    {
        if ((*it) == parameter)
        {
            (*it).reset();
            mParameters.erase(it);
            break;
        }
    }
}

//-----------------------------------------------------------------------------

static auto isArray(GpuProgramParameters::AutoConstantType autoType) -> bool
{
    using enum GpuProgramParameters::AutoConstantType;
    switch (autoType)
    {
    case WORLD_MATRIX_ARRAY_3x4:
    case WORLD_MATRIX_ARRAY:
    case WORLD_DUALQUATERNION_ARRAY_2x4:
    case WORLD_SCALE_SHEAR_MATRIX_ARRAY_3x4:
    case LIGHT_DIFFUSE_COLOUR_ARRAY:
    case LIGHT_SPECULAR_COLOUR_ARRAY:
    case LIGHT_DIFFUSE_COLOUR_POWER_SCALED_ARRAY:
    case LIGHT_SPECULAR_COLOUR_POWER_SCALED_ARRAY:
    case LIGHT_ATTENUATION_ARRAY:
    case LIGHT_POSITION_ARRAY:
    case LIGHT_POSITION_OBJECT_SPACE_ARRAY:
    case LIGHT_POSITION_VIEW_SPACE_ARRAY:
    case LIGHT_DIRECTION_ARRAY:
    case LIGHT_DIRECTION_OBJECT_SPACE_ARRAY:
    case LIGHT_DIRECTION_VIEW_SPACE_ARRAY:
    case LIGHT_DISTANCE_OBJECT_SPACE_ARRAY:
    case LIGHT_POWER_SCALE_ARRAY:
    case SPOTLIGHT_PARAMS_ARRAY:
    case DERIVED_LIGHT_DIFFUSE_COLOUR_ARRAY:
    case DERIVED_LIGHT_SPECULAR_COLOUR_ARRAY:
    case LIGHT_CASTS_SHADOWS_ARRAY:
    case TEXTURE_VIEWPROJ_MATRIX_ARRAY:
    case TEXTURE_WORLDVIEWPROJ_MATRIX_ARRAY:
    case SPOTLIGHT_VIEWPROJ_MATRIX_ARRAY:
    case SPOTLIGHT_WORLDVIEWPROJ_MATRIX_ARRAY:
    case SHADOW_SCENE_DEPTH_RANGE_ARRAY:
        return true;
    default:
        return false;
    }
}

auto Program::resolveParameter(GpuProgramParameters::AutoConstantType autoType, uint32 data) -> UniformParameterPtr
{
    UniformParameterPtr param;

    // Check if parameter already exists.
    param = getParameterByAutoType(autoType);

    uint32 size = 0;
    if(isArray(autoType)) std::swap(size, data); // for array autotypes the extra parameter is the size

    if (param && param->getAutoConstantIntData() == data)
    {
        return param;
    }
    
    // Create new parameter
    param = std::make_shared<UniformParameter>(autoType, data, size);
    addParameter(param);

    return param;
}

auto Program::resolveAutoParameterReal(GpuProgramParameters::AutoConstantType autoType, 
                                                Real data, size_t size) -> UniformParameterPtr
{
    UniformParameterPtr param;

    // Check if parameter already exists.
    param = getParameterByAutoType(autoType);
    if (param.get() != nullptr)
    {
        if (param->isAutoConstantRealParameter() &&
            param->getAutoConstantRealData() == data)
        {
            param->setSize(std::max(size, param->getSize()));
            return param;
        }
    }
    
    // Create new parameter.
    param = std::make_shared<UniformParameter>(autoType, float(data), size);
    addParameter(param);

    return param;
}

//-----------------------------------------------------------------------------
auto Program::resolveAutoParameterReal(GpuProgramParameters::AutoConstantType autoType, GpuConstantType type,
                                                float data, size_t size) -> UniformParameterPtr
{
    UniformParameterPtr param;

    // Check if parameter already exists.
    param = getParameterByAutoType(autoType);
    if (param.get() != nullptr)
    {
        if (param->isAutoConstantRealParameter() &&
            param->getAutoConstantRealData() == data)
        {
            param->setSize(std::max(size, param->getSize()));
            return param;
        }
    }
    
    // Create new parameter.
    param = std::make_shared<UniformParameter>(autoType, data, size, type);
    addParameter(param);

    return param;
}

//-----------------------------------------------------------------------------
auto Program::resolveAutoParameterInt(GpuProgramParameters::AutoConstantType autoType, GpuConstantType type, 
                                           uint32 data, size_t size) -> UniformParameterPtr
{
    UniformParameterPtr param;

    // Check if parameter already exists.
    param = getParameterByAutoType(autoType);
    if (param.get() != nullptr)
    {
        if (param->isAutoConstantIntParameter() &&
            param->getAutoConstantIntData() == data)
        {
            param->setSize(std::max(size, param->getSize()));
            return param;
        }
    }

    // Create new parameter.
    param = std::make_shared<UniformParameter>(autoType, data, size, type);
    addParameter(param);

    return param;
}

//-----------------------------------------------------------------------------
auto Program::resolveParameter(GpuConstantType type, 
                                    int index, GpuParamVariability variability,
                                    std::string_view suggestedName,
                                    size_t size) -> UniformParameterPtr
{
    UniformParameterPtr param;

    if (index == -1)
    {
        index = 0;

        // Find the next available index of the target type.
        for (auto const& it : mParameters)
        {
            if (it->getType() == type &&
                it->isAutoConstantParameter() == false)
            {
                index++;
            }
        }
    }
    else
    {
        // Check if parameter already exists.
        param = getParameterByType(type, index);
        if (param.get() != nullptr)
        {       
            return param;       
        }
    }
    
    // Create new parameter.
    param = ParameterFactory::createUniform(type, index, variability, suggestedName, size);
    addParameter(param);

    return param;
}



//-----------------------------------------------------------------------------
auto Program::getParameterByName(std::string_view name) -> UniformParameterPtr
{
    for (auto const& it : mParameters)
    {
        if (it->getName() == name)
        {
            return it;
        }
    }

    return {};
}

//-----------------------------------------------------------------------------
auto Program::getParameterByType(GpuConstantType type, int index) -> UniformParameterPtr
{
    for (auto const& it : mParameters)
    {
        if (it->getType() == type &&
            it->getIndex() == index)
        {
            return it;
        }
    }

    return {};
}

//-----------------------------------------------------------------------------
auto Program::getParameterByAutoType(GpuProgramParameters::AutoConstantType autoType) -> UniformParameterPtr
{
    for (auto const& it : mParameters)
    {
        if (it->isAutoConstantParameter() && it->getAutoConstantType() == autoType)
        {
            return it;
        }
    }

    return {};
}

//-----------------------------------------------------------------------------
void Program::addDependency(std::string_view libFileName)
{
    for (auto & mDependencie : mDependencies)
    {
        if (mDependencie == libFileName)
        {
            return;
        }
    }
    mDependencies.emplace_back(libFileName);
}

void Program::addPreprocessorDefines(std::string_view defines)
{
    if (not mPreprocessorDefines.empty())
        mPreprocessorDefines += ',';

    mPreprocessorDefines += defines;
}

//-----------------------------------------------------------------------------
auto Program::getDependencyCount() const -> size_t
{
    return mDependencies.size();
}

//-----------------------------------------------------------------------------
auto Program::getDependency(unsigned int index) const -> std::string_view
{
    return mDependencies[index];
}

}
