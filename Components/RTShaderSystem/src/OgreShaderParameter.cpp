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
#include <cstddef>
#include <format>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "OgreGpuProgramParams.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreString.hpp"
#include "OgreStringConverter.hpp"
#include "OgreVector.hpp"

namespace Ogre::RTShader {

    //-----------------------------------------------------------------------
    // Define some ConstParameterTypes
    //-----------------------------------------------------------------------

    /** ConstParameterVec2 represents a Vector2 constant.
    */
    class ConstParameterVec2 : public ConstParameter<Vector2>
    {
    public:
        ConstParameterVec2( Vector2 val, 
            GpuConstantType type, 
            const Semantic& semantic,  
            const Content& content) 
            : ConstParameter<Vector2>(val, type, semantic, content)
        {
        }

        ~ConstParameterVec2() override = default;

        /** 
        @see Parameter::toString.
        */
        [[nodiscard]] auto toString () const noexcept -> String override
        {
            StringStream str;
            str << "vec2(" << std::showpoint << mValue.x << "," << mValue.y << ")";
            return str.str();
        }
    };

    /** ConstParameterVec3 represents a Vector3 constant.
    */
    class ConstParameterVec3 : public ConstParameter<Vector3>
    {
    public:
        ConstParameterVec3( Vector3 val, 
            GpuConstantType type, 
            const Semantic& semantic,  
            const Content& content) 
            : ConstParameter<Vector3>(val, type, semantic, content)
        {
        }
        ~ConstParameterVec3() override = default;

        /** 
        @see Parameter::toString.
        */
        [[nodiscard]] auto toString () const noexcept -> String override
        {
            StringStream str;
            str << "vec3(" << std::showpoint << mValue.x << "," << mValue.y << "," << mValue.z << ")";
            return str.str();
        }
    };

    /** ConstParameterVec4 represents a Vector2 Vector4.
    */
    class ConstParameterVec4 : public ConstParameter<Vector4>
    {
    public:
        ConstParameterVec4( Vector4 val, 
            GpuConstantType type, 
            const Semantic& semantic,  
            const Content& content) 
            : ConstParameter<Vector4>(val, type, semantic, content)
        {
        }
        ~ConstParameterVec4() override = default;

        /** 
        @see Parameter::toString.
        */
        [[nodiscard]] auto toString () const noexcept -> String override
        {
            StringStream str;
            str << "vec4(" << std::showpoint << mValue.x << "," << mValue.y << "," << mValue.z << "," << mValue.w << ")";
            return str.str();
        }
    };

    /** ConstParameterFloat represents a float constant.
    */
    class ConstParameterFloat : public ConstParameter<float>
    {
    public:
        ConstParameterFloat(float val, 
            GpuConstantType type, 
            const Semantic& semantic,  
            const Content& content) 
            : ConstParameter<float>(val, type, semantic, content)
        {
        }

        ~ConstParameterFloat() override = default;

        /** 
        @see Parameter::toString.
        */
        [[nodiscard]] auto toString () const noexcept -> String override
        {
            return StringConverter::toString(mValue, 6, 0, ' ', std::ios::showpoint);
        }
    };
    /** ConstParameterInt represents an int constant.
    */
    class ConstParameterInt : public ConstParameter<int>
    {
    public:
        ConstParameterInt(int val, 
            GpuConstantType type, 
            const Semantic& semantic,  
            const Content& content) 
            : ConstParameter<int>(val, type, semantic, content)
        {
        }

        ~ConstParameterInt() override = default;

        /** 
        @see Parameter::toString.
        */
        [[nodiscard]] auto toString () const noexcept -> String override
        {
            return Ogre::StringConverter::toString(mValue);
        }
    };

//-----------------------------------------------------------------------
Parameter::Parameter() : mName(""), mType(GpuConstantType::UNKNOWN), mSemantic(Semantic::UNKNOWN), mIndex(0), mContent(Content::UNKNOWN), mSize(0), mUsed(false)
{
}

//-----------------------------------------------------------------------
Parameter::Parameter(GpuConstantType type, std::string_view name,
            const Semantic& semantic, int index, 
            const Content& content, size_t size) :
    mName(name), mType(type), mSemantic(semantic), mIndex(index), mContent(content), mSize(size), mUsed(false)
{
}

//-----------------------------------------------------------------------
UniformParameter::UniformParameter(GpuConstantType type, std::string_view name,
                 const Semantic& semantic, int index, 
                 const Content& content,
                 GpuParamVariability variability, size_t size) : Parameter(type, name, semantic, index, content, size)
{
    mAutoConstantData    = ::std::monostate{};
    mVariability            = variability;
    mParamsPtr              = nullptr;
    mPhysicalIndex          = -1;
    mAutoConstantType       = GpuProgramParameters::AutoConstantType::UNKNOWN;
}

static auto getGCType(const GpuProgramParameters::AutoConstantDefinition* def) -> GpuConstantType
{
    assert(def->elementType == GpuProgramParameters::ElementType::REAL);

    switch (def->elementCount)
    {
    default:
    case 1:
        return GpuConstantType::FLOAT1;
    case 2:
        return GpuConstantType::FLOAT2;
    case 3:
        return GpuConstantType::FLOAT3;
    case 4:
        return GpuConstantType::FLOAT4;
    case 8:
        return GpuConstantType::MATRIX_2X4;
    case 9:
        return GpuConstantType::MATRIX_3X3;
    case 12:
        return GpuConstantType::MATRIX_3X4;
    case 16:
        return GpuConstantType::MATRIX_4X4;
    }
}

//-----------------------------------------------------------------------
UniformParameter::UniformParameter(GpuProgramParameters::AutoConstantType autoType, float fAutoConstantData, size_t size)
{
    auto parameterDef = GpuProgramParameters::getAutoConstantDefinition(autoType);
    assert(parameterDef);

    mName               = parameterDef->name;
    if (fAutoConstantData != 0.0)
    {
        mName += StringConverter::toString(fAutoConstantData);
        //replace possible illegal point character in name
        std::ranges::replace(mName, '.', '_'); 
    }
    mType               = getGCType(parameterDef);
    mSemantic           = Semantic::UNKNOWN;
    mIndex              = -1;
    mContent            = Content::UNKNOWN;
    mAutoConstantType   = autoType;
    mAutoConstantData = fAutoConstantData;
    mVariability        = GpuParamVariability::GLOBAL;
    mParamsPtr           = nullptr;
    mPhysicalIndex       = -1;
    mSize                = size;
}

//-----------------------------------------------------------------------
UniformParameter::UniformParameter(GpuProgramParameters::AutoConstantType autoType, float fAutoConstantData, size_t size, GpuConstantType type)
{
    auto parameterDef = GpuProgramParameters::getAutoConstantDefinition(autoType);
    assert(parameterDef);

    mName               = parameterDef->name;
    if (fAutoConstantData != 0.0)
    {
        mName += StringConverter::toString(fAutoConstantData);
        //replace possible illegal point character in name
        std::ranges::replace(mName, '.', '_'); 
    }
    mType               = type;
    mSemantic           = Semantic::UNKNOWN;
    mIndex              = -1;
    mContent            = Content::UNKNOWN;
    mAutoConstantType   = autoType;
    mAutoConstantData = fAutoConstantData;
    mVariability        = GpuParamVariability::GLOBAL;
    mParamsPtr           = nullptr;
    mPhysicalIndex       = -1;
    mSize                = size;
}

//-----------------------------------------------------------------------
UniformParameter::UniformParameter(GpuProgramParameters::AutoConstantType autoType, uint32 nAutoConstantData, size_t size)
{
    auto parameterDef = GpuProgramParameters::getAutoConstantDefinition(autoType);
    assert(parameterDef);

    mName               = parameterDef->name;
    if (nAutoConstantData != 0)
        mName += StringConverter::toString(nAutoConstantData);
    mType               = getGCType(parameterDef);
    mSemantic           = Semantic::UNKNOWN;
    mIndex              = -1;
    mContent            = Content::UNKNOWN;
    mAutoConstantType   = autoType;
    mAutoConstantData = nAutoConstantData;
    mVariability        = GpuParamVariability::GLOBAL;
    mParamsPtr           = nullptr;
    mPhysicalIndex       = -1;
    mSize                = size;
}

//-----------------------------------------------------------------------
UniformParameter::UniformParameter(GpuProgramParameters::AutoConstantType autoType, uint32 nAutoConstantData, size_t size, GpuConstantType type)
{
    auto parameterDef = GpuProgramParameters::getAutoConstantDefinition(autoType);
    assert(parameterDef);

    mName               = parameterDef->name;
    if (nAutoConstantData != 0)
        mName += StringConverter::toString(nAutoConstantData);
    mType               = type;
    mSemantic           = Semantic::UNKNOWN;
    mIndex              = -1;
    mContent            = Content::UNKNOWN;
    mAutoConstantType   = autoType;
    mAutoConstantData = nAutoConstantData;
    mVariability        = GpuParamVariability::GLOBAL;
    mParamsPtr           = nullptr;
    mPhysicalIndex       = -1;
    mSize                = size;
}

//-----------------------------------------------------------------------
void UniformParameter::bind(GpuProgramParametersSharedPtr paramsPtr)
{
    // do not throw on failure: some RS optimize unused uniforms away. Also unit tests run without any RS
    const GpuConstantDefinition* def = paramsPtr->_findNamedConstantDefinition(mBindName.empty() ? mName : mBindName, false);

    if (def != nullptr)
    {
        mParamsPtr = paramsPtr.get();
        mPhysicalIndex = def->physicalIndex;
        mElementSize = def->elementSize;
        mVariability = def->variability;
    }
}

//-----------------------------------------------------------------------
auto ParameterFactory::createInPosition(int index, Parameter::Content content) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT4, ::std::format("iPos_{}", index),
                                       Parameter::Semantic::POSITION, index,
                                       content);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createOutPosition(int index) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT4, ::std::format("oPos_{}", index),
        Parameter::Semantic::POSITION, index,
        Parameter::Content::POSITION_PROJECTIVE_SPACE);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createInNormal(int index) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT3, ::std::format("iNormal_{}", index),
        Parameter::Semantic::NORMAL, index,
        Parameter::Content::NORMAL_OBJECT_SPACE);
}


//-----------------------------------------------------------------------
auto ParameterFactory::createInWeights(int index) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT4, ::std::format("iBlendWeights_{}", index),
        Parameter::Semantic::BLEND_WEIGHTS, index,
        Parameter::Content::BLEND_WEIGHTS);
}


//-----------------------------------------------------------------------
auto ParameterFactory::createInIndices(int index) -> ParameterPtr
{
	return std::make_shared<Parameter>(
		GpuConstantType::UINT4
	, ::std::format("iBlendIndices_{}", index),
        Parameter::Semantic::BLEND_INDICES, index,
        Parameter::Content::BLEND_INDICES);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createInBiNormal(int index) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT3, ::std::format("iBiNormal_{}", index),
        Parameter::Semantic::BINORMAL, index,
        Parameter::Content::BINORMAL_OBJECT_SPACE);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createInTangent(int index) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT3, ::std::format("iTangent_{}", index),
        Parameter::Semantic::TANGENT, index,
        Parameter::Content::TANGENT_OBJECT_SPACE);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createOutNormal(int index) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT3, ::std::format("oNormal_{}", index),
        Parameter::Semantic::NORMAL, index,
        Parameter::Content::NORMAL_OBJECT_SPACE);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createOutBiNormal(int index) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT3, ::std::format("oBiNormal_{}", index),
        Parameter::Semantic::BINORMAL, index,
        Parameter::Content::BINORMAL_OBJECT_SPACE);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createOutTangent(int index) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT3, ::std::format("oTangent_{}", index),
        Parameter::Semantic::TANGENT, index,
        Parameter::Content::TANGENT_OBJECT_SPACE);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createInColor(int index) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT4, ::std::format("iColor_{}", index),
        Parameter::Semantic::COLOR, index,
        index == 0 ? Parameter::Content::COLOR_DIFFUSE : Parameter::Content::COLOR_SPECULAR);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createOutColor(int index) -> ParameterPtr
{
    return std::make_shared<Parameter>(GpuConstantType::FLOAT4, ::std::format("oColor_{}", index),
        Parameter::Semantic::COLOR, index,
        index == 0 ? Parameter::Content::COLOR_DIFFUSE : Parameter::Content::COLOR_SPECULAR);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createInTexcoord(GpuConstantType type, int index, Parameter::Content content) -> ParameterPtr
{
    switch (type)
    {
    case GpuConstantType::FLOAT1:
    case GpuConstantType::FLOAT2:
    case GpuConstantType::FLOAT3:
    case GpuConstantType::FLOAT4:
    case GpuConstantType::MATRIX_2X2:
    case GpuConstantType::MATRIX_2X3:
    case GpuConstantType::MATRIX_2X4:
    case GpuConstantType::MATRIX_3X2:
    case GpuConstantType::MATRIX_3X3:
    case GpuConstantType::MATRIX_3X4:
    case GpuConstantType::MATRIX_4X2:
    case GpuConstantType::MATRIX_4X3:
    case GpuConstantType::MATRIX_4X4:
    case GpuConstantType::INT1:
    case GpuConstantType::INT2:
    case GpuConstantType::INT3:
    case GpuConstantType::INT4:
    case GpuConstantType::UINT1:
    case GpuConstantType::UINT2:
    case GpuConstantType::UINT3:
    case GpuConstantType::UINT4:
        return std::make_shared<Parameter>(type, std::format("iTexcoord_{}", index),
                                           Parameter::Semantic::TEXTURE_COORDINATES, index, content);
    default:
    case GpuConstantType::SAMPLER1D:
    case GpuConstantType::SAMPLER2D:
    case GpuConstantType::SAMPLER2DARRAY:
    case GpuConstantType::SAMPLER3D:
    case GpuConstantType::SAMPLERCUBE:
    case GpuConstantType::SAMPLER1DSHADOW:
    case GpuConstantType::SAMPLER2DSHADOW:
    case GpuConstantType::UNKNOWN:
        break;
    }

    return {};
}

//-----------------------------------------------------------------------
auto ParameterFactory::createOutTexcoord(GpuConstantType type, int index, Parameter::Content content) -> ParameterPtr
{
    switch (type)
    {
    case GpuConstantType::FLOAT1:
    case GpuConstantType::FLOAT2:
    case GpuConstantType::FLOAT3:
    case GpuConstantType::FLOAT4:
        return std::make_shared<Parameter>(type, std::format("oTexcoord_{}", index),
                                           Parameter::Semantic::TEXTURE_COORDINATES, index, content);
    default:
    case GpuConstantType::SAMPLER1D:
    case GpuConstantType::SAMPLER2D:
    case GpuConstantType::SAMPLER2DARRAY:
    case GpuConstantType::SAMPLER3D:
    case GpuConstantType::SAMPLERCUBE:
    case GpuConstantType::SAMPLER1DSHADOW:
    case GpuConstantType::SAMPLER2DSHADOW:
    case GpuConstantType::MATRIX_2X2:
    case GpuConstantType::MATRIX_2X3:
    case GpuConstantType::MATRIX_2X4:
    case GpuConstantType::MATRIX_3X2:
    case GpuConstantType::MATRIX_3X3:
    case GpuConstantType::MATRIX_3X4:
    case GpuConstantType::MATRIX_4X2:
    case GpuConstantType::MATRIX_4X3:
    case GpuConstantType::MATRIX_4X4:
    case GpuConstantType::INT1:
    case GpuConstantType::INT2:
    case GpuConstantType::INT3:
    case GpuConstantType::INT4:
    case GpuConstantType::UINT1:
    case GpuConstantType::UINT2:
    case GpuConstantType::UINT3:
    case GpuConstantType::UINT4:
    case GpuConstantType::UNKNOWN:
        break;
    }

    return {};
}

//-----------------------------------------------------------------------
auto ParameterFactory::createSampler(GpuConstantType type, int index) -> UniformParameterPtr
{
    switch (type)
    {
    case GpuConstantType::SAMPLER1D:
        return createSampler1D(index);

    case GpuConstantType::SAMPLER2D:
        return createSampler2D(index);

    case GpuConstantType::SAMPLER2DARRAY:
        return createSampler2DArray(index);

    case GpuConstantType::SAMPLER3D:
        return createSampler3D(index);

    case GpuConstantType::SAMPLERCUBE:
        return createSamplerCUBE(index);

    default:
    case GpuConstantType::SAMPLER1DSHADOW:
    case GpuConstantType::SAMPLER2DSHADOW:
    case GpuConstantType::MATRIX_2X2:
    case GpuConstantType::MATRIX_2X3:
    case GpuConstantType::MATRIX_2X4:
    case GpuConstantType::MATRIX_3X2:
    case GpuConstantType::MATRIX_3X3:
    case GpuConstantType::MATRIX_3X4:
    case GpuConstantType::MATRIX_4X2:
    case GpuConstantType::MATRIX_4X3:
    case GpuConstantType::MATRIX_4X4:
    case GpuConstantType::INT1:
    case GpuConstantType::INT2:
    case GpuConstantType::INT3:
    case GpuConstantType::INT4:
    case GpuConstantType::UINT1:
    case GpuConstantType::UINT2:
    case GpuConstantType::UINT3:
    case GpuConstantType::UINT4:
    case GpuConstantType::UNKNOWN:
        break;
    }

    return {};
    
}

//-----------------------------------------------------------------------
auto ParameterFactory::createSampler1D(int index) -> UniformParameterPtr
{
    return std::make_shared<UniformParameter>(GpuConstantType::SAMPLER1D, ::std::format("gSampler1D_{}", index),
        Parameter::Semantic::UNKNOWN, index,
        Parameter::Content::UNKNOWN,
        GpuParamVariability::GLOBAL, 1);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createSampler2D(int index) -> UniformParameterPtr
{
    return std::make_shared<UniformParameter>(GpuConstantType::SAMPLER2D, ::std::format("gSampler2D_{}", index),
        Parameter::Semantic::UNKNOWN, index,
        Parameter::Content::UNKNOWN,
        GpuParamVariability::GLOBAL, 1);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createSampler2DArray(int index) -> UniformParameterPtr
{
    return std::make_shared<UniformParameter>(GpuConstantType::SAMPLER2DARRAY, ::std::format("gSampler2DArray_{}", index),
                                                         Parameter::Semantic::UNKNOWN, index,
                                                         Parameter::Content::UNKNOWN,
                                                         GpuParamVariability::GLOBAL, 1);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createSampler3D(int index) -> UniformParameterPtr
{
    return std::make_shared<UniformParameter>(GpuConstantType::SAMPLER3D, ::std::format("gSampler3D_{}", index),
        Parameter::Semantic::UNKNOWN, index,
        Parameter::Content::UNKNOWN,
        GpuParamVariability::GLOBAL, 1);
}

//-----------------------------------------------------------------------
auto ParameterFactory::createSamplerCUBE(int index) -> UniformParameterPtr
{
    return std::make_shared<UniformParameter>(GpuConstantType::SAMPLERCUBE, ::std::format("gSamplerCUBE_{}", index),
        Parameter::Semantic::UNKNOWN, index,
        Parameter::Content::UNKNOWN,
        GpuParamVariability::GLOBAL, 1);
}
//-----------------------------------------------------------------------
auto ParameterFactory::createConstParam(const Vector2& val) -> ParameterPtr
{
    return ParameterPtr(new ConstParameterVec2(val, GpuConstantType::FLOAT2, Parameter::Semantic::UNKNOWN,
                                                    Parameter::Content::UNKNOWN));
}

//-----------------------------------------------------------------------
auto ParameterFactory::createConstParam(const Vector3& val) -> ParameterPtr
{
    return ParameterPtr(new ConstParameterVec3(val, GpuConstantType::FLOAT3, Parameter::Semantic::UNKNOWN,
                                                    Parameter::Content::UNKNOWN));
}

//-----------------------------------------------------------------------
auto ParameterFactory::createConstParam(const Vector4& val) -> ParameterPtr
{
    return ParameterPtr(new ConstParameterVec4(val, GpuConstantType::FLOAT4, Parameter::Semantic::UNKNOWN,
                                                    Parameter::Content::UNKNOWN));
}

//-----------------------------------------------------------------------
auto ParameterFactory::createConstParam(float val) -> ParameterPtr
{
    return ParameterPtr(new ConstParameterFloat(val, GpuConstantType::FLOAT1, Parameter::Semantic::UNKNOWN,
                                                     Parameter::Content::UNKNOWN));
}

//-----------------------------------------------------------------------
auto ParameterFactory::createUniform(GpuConstantType type, 
                                             int index, GpuParamVariability variability,
                                             std::string_view suggestedName,
                                             size_t size) -> UniformParameterPtr
{
    UniformParameterPtr param;
    
    param = std::make_shared<UniformParameter>(type, std::format("{}{}", suggestedName, index),
        Parameter::Semantic::UNKNOWN, index,
        Parameter::Content::UNKNOWN, variability, size);
        
    return param;
}

}
