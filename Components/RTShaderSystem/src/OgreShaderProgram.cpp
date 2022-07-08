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
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "OgreException.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderFunction.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderProgram.hpp"
#include "OgreStringVector.hpp"

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
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, 
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
    switch (autoType)
    {
    case GpuProgramParameters::ACT_WORLD_MATRIX_ARRAY_3x4:
    case GpuProgramParameters::ACT_WORLD_MATRIX_ARRAY:
    case GpuProgramParameters::ACT_WORLD_DUALQUATERNION_ARRAY_2x4:
    case GpuProgramParameters::ACT_WORLD_SCALE_SHEAR_MATRIX_ARRAY_3x4:
    case GpuProgramParameters::ACT_LIGHT_DIFFUSE_COLOUR_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_SPECULAR_COLOUR_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_DIFFUSE_COLOUR_POWER_SCALED_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_SPECULAR_COLOUR_POWER_SCALED_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_ATTENUATION_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_POSITION_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_POSITION_OBJECT_SPACE_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_POSITION_VIEW_SPACE_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_DIRECTION_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_DIRECTION_OBJECT_SPACE_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_DIRECTION_VIEW_SPACE_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_DISTANCE_OBJECT_SPACE_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_POWER_SCALE_ARRAY:
    case GpuProgramParameters::ACT_SPOTLIGHT_PARAMS_ARRAY:
    case GpuProgramParameters::ACT_DERIVED_LIGHT_DIFFUSE_COLOUR_ARRAY:
    case GpuProgramParameters::ACT_DERIVED_LIGHT_SPECULAR_COLOUR_ARRAY:
    case GpuProgramParameters::ACT_LIGHT_CASTS_SHADOWS_ARRAY:
    case GpuProgramParameters::ACT_TEXTURE_VIEWPROJ_MATRIX_ARRAY:
    case GpuProgramParameters::ACT_TEXTURE_WORLDVIEWPROJ_MATRIX_ARRAY:
    case GpuProgramParameters::ACT_SPOTLIGHT_VIEWPROJ_MATRIX_ARRAY:
    case GpuProgramParameters::ACT_SPOTLIGHT_WORLDVIEWPROJ_MATRIX_ARRAY:
    case GpuProgramParameters::ACT_SHADOW_SCENE_DEPTH_RANGE_ARRAY:
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
                                    int index, uint16 variability,
                                    StringView suggestedName,
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
auto Program::getParameterByName(StringView name) -> UniformParameterPtr
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
void Program::addDependency(StringView libFileName)
{
    for (auto & mDependencie : mDependencies)
    {
        if (mDependencie == libFileName)
        {
            return;
        }
    }
    mDependencies.push_back(libFileName);
}

void Program::addPreprocessorDefines(StringView defines)
{
    mPreprocessorDefines +=
        mPreprocessorDefines.empty() ? std::string{defines} : std::format(",{}", defines);
}

//-----------------------------------------------------------------------------
auto Program::getDependencyCount() const -> size_t
{
    return mDependencies.size();
}

//-----------------------------------------------------------------------------
auto Program::getDependency(unsigned int index) const -> StringView
{
    return mDependencies[index];
}

}
