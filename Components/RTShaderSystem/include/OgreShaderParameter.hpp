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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_PARAMETER_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_PARAMETER_H

#include <cstddef>
#include <variant>
#include <vector>

#include "OgreGpuProgramParams.hpp"
#include "OgreMatrix4.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreVector.hpp"

namespace Ogre {
class ColourValue;
struct Matrix3;

namespace RTShader {

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** A class that represents a shader based program parameter.
*/
class Parameter : public RTShaderSystemAlloc
{
public:
    // Shader parameter semantic.
    enum class Semantic
    {
        /// Unknown semantic
        UNKNOWN = 0,
        /// Position
        POSITION = 1,
        /// Blending weights
        BLEND_WEIGHTS = 2,
        /// Blending indices
        BLEND_INDICES = 3,
        /// Normal, 3 reals per vertex
        NORMAL = 4,
        /// General floating point color.
        COLOR = 5,
        /// Texture coordinates
        TEXTURE_COORDINATES = 7,
        /// Binormal (Y axis if normal is Z)
        BINORMAL = 8,
        /// Tangent (X axis if normal is Z)
        TANGENT = 9,
        /// VFACE
        FRONT_FACING
    };

    /** Shader parameter content
     * 
     * used to resolve Parameters across different SubRenderState instances
     * Think of it as Semantic extended to the actual parameter content.
     */ 
    enum class Content
    {
        /// Unknown content
        UNKNOWN,

        /// Position in object space
        POSITION_OBJECT_SPACE,

        /// Position in world space
        POSITION_WORLD_SPACE,

        /// Position in view space
        POSITION_VIEW_SPACE,

        /// Position in projective space
        POSITION_PROJECTIVE_SPACE,

        /// Position in light space index 0-7
        POSITION_LIGHT_SPACE0,
        POSITION_LIGHT_SPACE1,
        POSITION_LIGHT_SPACE2,
        POSITION_LIGHT_SPACE3,
        POSITION_LIGHT_SPACE4,
        POSITION_LIGHT_SPACE5,
        POSITION_LIGHT_SPACE6,
        POSITION_LIGHT_SPACE7,

        /// Normal in object space
        NORMAL_OBJECT_SPACE,

        /// Normal in world space
        NORMAL_WORLD_SPACE,

        /// Normal in view space
        NORMAL_VIEW_SPACE,

        /// Normal in tangent space
        NORMAL_TANGENT_SPACE,

        /// View vector in object space
        POSTOCAMERA_OBJECT_SPACE,

        /// View vector in world space
        POSTOCAMERA_WORLD_SPACE,

        /// View vector in view space
        POSTOCAMERA_VIEW_SPACE,

        /// View vector in tangent space
        POSTOCAMERA_TANGENT_SPACE,

        /// Light vector in object space index 0-7
        POSTOLIGHT_OBJECT_SPACE0,
        POSTOLIGHT_OBJECT_SPACE1,
        POSTOLIGHT_OBJECT_SPACE2,
        POSTOLIGHT_OBJECT_SPACE3,
        POSTOLIGHT_OBJECT_SPACE4,
        POSTOLIGHT_OBJECT_SPACE5,
        POSTOLIGHT_OBJECT_SPACE6,
        POSTOLIGHT_OBJECT_SPACE7,

        /// Light vector in world space index 0-7
        POSTOLIGHT_WORLD_SPACE0,
        POSTOLIGHT_WORLD_SPACE1,
        POSTOLIGHT_WORLD_SPACE2,
        POSTOLIGHT_WORLD_SPACE3,
        POSTOLIGHT_WORLD_SPACE4,
        POSTOLIGHT_WORLD_SPACE5,
        POSTOLIGHT_WORLD_SPACE6,
        POSTOLIGHT_WORLD_SPACE7,

        /// Light vector in view space index 0-7
        POSTOLIGHT_VIEW_SPACE0,
        POSTOLIGHT_VIEW_SPACE1,
        POSTOLIGHT_VIEW_SPACE2,
        POSTOLIGHT_VIEW_SPACE3,
        POSTOLIGHT_VIEW_SPACE4,
        POSTOLIGHT_VIEW_SPACE5,
        POSTOLIGHT_VIEW_SPACE6,
        POSTOLIGHT_VIEW_SPACE7,

        /// Light vector in tangent space index 0-7
        POSTOLIGHT_TANGENT_SPACE0,
        POSTOLIGHT_TANGENT_SPACE1,
        POSTOLIGHT_TANGENT_SPACE2,
        POSTOLIGHT_TANGENT_SPACE3,
        POSTOLIGHT_TANGENT_SPACE4,
        POSTOLIGHT_TANGENT_SPACE5,
        POSTOLIGHT_TANGENT_SPACE6,
        POSTOLIGHT_TANGENT_SPACE7,

        /// Light direction in object space index 0-7
        LIGHTDIRECTION_OBJECT_SPACE0,
        LIGHTDIRECTION_OBJECT_SPACE1,
        LIGHTDIRECTION_OBJECT_SPACE2,
        LIGHTDIRECTION_OBJECT_SPACE3,
        LIGHTDIRECTION_OBJECT_SPACE4,
        LIGHTDIRECTION_OBJECT_SPACE5,
        LIGHTDIRECTION_OBJECT_SPACE6,
        LIGHTDIRECTION_OBJECT_SPACE7,

        /// Light direction in world space index 0-7
        LIGHTDIRECTION_WORLD_SPACE0,
        LIGHTDIRECTION_WORLD_SPACE1,
        LIGHTDIRECTION_WORLD_SPACE2,
        LIGHTDIRECTION_WORLD_SPACE3,
        LIGHTDIRECTION_WORLD_SPACE4,
        LIGHTDIRECTION_WORLD_SPACE5,
        LIGHTDIRECTION_WORLD_SPACE6,
        LIGHTDIRECTION_WORLD_SPACE7,

        /// Light direction in view space index 0-7
        LIGHTDIRECTION_VIEW_SPACE0,
        LIGHTDIRECTION_VIEW_SPACE1,
        LIGHTDIRECTION_VIEW_SPACE2,
        LIGHTDIRECTION_VIEW_SPACE3,
        LIGHTDIRECTION_VIEW_SPACE4,
        LIGHTDIRECTION_VIEW_SPACE5,
        LIGHTDIRECTION_VIEW_SPACE6,
        LIGHTDIRECTION_VIEW_SPACE7,

        /// Light direction in tangent space index 0-7
        LIGHTDIRECTION_TANGENT_SPACE0,
        LIGHTDIRECTION_TANGENT_SPACE1,
        LIGHTDIRECTION_TANGENT_SPACE2,
        LIGHTDIRECTION_TANGENT_SPACE3,
        LIGHTDIRECTION_TANGENT_SPACE4,
        LIGHTDIRECTION_TANGENT_SPACE5,
        LIGHTDIRECTION_TANGENT_SPACE6,
        LIGHTDIRECTION_TANGENT_SPACE7,

        /// Light position in object space index 0-7
        LIGHTPOSITION_OBJECT_SPACE0,
        LIGHTPOSITION_OBJECT_SPACE1,
        LIGHTPOSITION_OBJECT_SPACE2,
        LIGHTPOSITION_OBJECT_SPACE3,
        LIGHTPOSITION_OBJECT_SPACE4,
        LIGHTPOSITION_OBJECT_SPACE5,
        LIGHTPOSITION_OBJECT_SPACE6,
        LIGHTPOSITION_OBJECT_SPACE7,

        /// Light position in world space index 0-7
        LIGHTPOSITION_WORLD_SPACE0,
        LIGHTPOSITION_WORLD_SPACE1,
        LIGHTPOSITION_WORLD_SPACE2,
        LIGHTPOSITION_WORLD_SPACE3,
        LIGHTPOSITION_WORLD_SPACE4,
        LIGHTPOSITION_WORLD_SPACE5,
        LIGHTPOSITION_WORLD_SPACE6,
        LIGHTPOSITION_WORLD_SPACE7,

        /// Light position in view space index 0-7
        LIGHTPOSITIONVIEW_SPACE0,
        LIGHTPOSITIONVIEW_SPACE1,
        LIGHTPOSITIONVIEW_SPACE2,
        LIGHTPOSITIONVIEW_SPACE3,
        LIGHTPOSITIONVIEW_SPACE4,
        LIGHTPOSITIONVIEW_SPACE5,
        LIGHTPOSITIONVIEW_SPACE6,
        LIGHTPOSITIONVIEW_SPACE7,

        /// Light position in tangent space index 0-7
        LIGHTPOSITION_TANGENT_SPACE,

        /// Blending weights
        BLEND_WEIGHTS,

        /// Blending indices
        BLEND_INDICES,
        
        /// Tangent in object space
        TANGENT_OBJECT_SPACE,

        /// Tangent in world space
        TANGENT_WORLD_SPACE,

        /// Tangent in view space
        TANGENT_VIEW_SPACE,

        /// Tangent in tangent space
        TANGENT_TANGENT_SPACE,

        /// Binormal in object space
        BINORMAL_OBJECT_SPACE,

        /// Binormal in world space
        BINORMAL_WORLD_SPACE,

        /// Binormal in view space
        BINORMAL_VIEW_SPACE,

        /// Binormal in tangent space
        BINORMAL_TANGENT_SPACE,

        /// Diffuse color
        COLOR_DIFFUSE,

        /// Specular color
        COLOR_SPECULAR,

        /// Depth in object space
        DEPTH_OBJECT_SPACE,

        /// Depth in world space
        DEPTH_WORLD_SPACE,

        /// Depth in view space
        DEPTH_VIEW_SPACE,

        /// Depth in projective space
        DEPTH_PROJECTIVE_SPACE,

        /// Texture coordinate set index 0-7
        TEXTURE_COORDINATE0,
        TEXTURE_COORDINATE1,
        TEXTURE_COORDINATE2,
        TEXTURE_COORDINATE3,
        TEXTURE_COORDINATE4,
        TEXTURE_COORDINATE5,
        TEXTURE_COORDINATE6,
        TEXTURE_COORDINATE7,
		
        /// point sprite coordinates
        POINTSPRITE_COORDINATE,

        /// point sprite size
        POINTSPRITE_SIZE,

        /// gl_FrontFacing
        FRONT_FACING,

        /// Reserved custom content range to be used by user custom shader extensions.
        CUSTOM_CONTENT_BEGIN    = 1000,
        CUSTOM_CONTENT_END      = 2000
    };

// Interface.
public:
    /** */
    Parameter();

    /** Class constructor.
    @param type The type of this parameter.
    @param name The name of this parameter.
    @param semantic The semantic of this parameter.
    @param index The index of this parameter.
    @param content The content of this parameter.
    @param size
    */
    Parameter(GpuConstantType type, std::string_view name, 
        const Semantic& semantic, int index, 
        const Content& content, size_t size = 0);

    /** Class destructor */
    virtual ~Parameter() = default;;

    /** Get the name of this parameter. */
    [[nodiscard]] auto getName() const noexcept -> std::string_view { return mName; }

    /// internal function for aliasing to GLSL builtins e.g. gl_Position
    void _rename(std::string_view newName, bool onlyLocal = false)
    {
        if(onlyLocal)
            mBindName = mName;
        mName = newName;
    }

    /** Get the type of this parameter. */
    [[nodiscard]] auto getType() const noexcept -> GpuConstantType { return mType; }

    /** Get the semantic of this parameter. */
    [[nodiscard]] auto getSemantic() const noexcept -> const Semantic& { return mSemantic; }

    /** Get the index of this parameter. */
    [[nodiscard]] auto getIndex() const noexcept -> int { return mIndex; } 

    /** Return the content of this parameter. */
    [[nodiscard]] auto getContent() const noexcept -> Content { return mContent; }

    /** Returns true if this instance is a ConstParameter otherwise false. */
    [[nodiscard]] virtual auto isConstParameter() const noexcept -> bool { return false; }

    /** Returns the string representation of this parameter. */
    [[nodiscard]] virtual auto toString() const noexcept -> String { return mName; }
    
    /** Returns Whether this parameter is an array. */
    [[nodiscard]] auto isArray() const noexcept -> bool { return mSize > 0; }

    /** Returns the number of elements in the parameter (for arrays). */
    [[nodiscard]] auto getSize() const noexcept -> size_t { return mSize; }
    
    /** Sets the number of elements in the parameter (for arrays). */
    void setSize(size_t size) { mSize = size; }

    /// track whether this was used
    void setUsed(bool used) { mUsed = used; }
    auto isUsed() noexcept -> bool { return mUsed; }

// Attributes.
protected:
    // Name of this parameter.
    String mName;

    // only used for local renaming
    String mBindName;

    // Type of this parameter.
    GpuConstantType mType;
    // Semantic of this parameter.
    Semantic mSemantic;
    // Index of this parameter.
    int mIndex;
    // The content of this parameter.
    Content mContent;
    // Number of elements in the parameter (for arrays)
    size_t mSize;
    
    bool mUsed;
};

using ShaderParameterIterator = ShaderParameterList::iterator;
using ShaderParameterConstIterator = ShaderParameterList::const_iterator;

/** Uniform parameter class. Allow fast access to GPU parameter updates.
*/
class UniformParameter : public Parameter
{
public:

    /** Class constructor.
    @param type The type of this parameter.
    @param name The name of this parameter.
    @param semantic The semantic of this parameter.
    @param index The index of this parameter.
    @param content The content of this parameter.
    @param variability How this parameter varies (bitwise combination of GpuProgramVariability).
    @param size number of elements in the parameter.    
    */
    UniformParameter(GpuConstantType type, std::string_view name, 
        const Semantic& semantic, int index, 
        const Content& content,
        GpuParamVariability variability, size_t size);

    /** Class constructor.
    @param autoType The auto type of this parameter.
    @param fAutoConstantData The real data for this auto constant parameter.    
    @param size number of elements in the parameter.    
    */
    UniformParameter(GpuProgramParameters::AutoConstantType autoType, float fAutoConstantData, size_t size);
    
    /** Class constructor.
    @param autoType The auto type of this parameter.
    @param fAutoConstantData The real data for this auto constant parameter.    
    @param size number of elements in the parameter.
    @param type The desired data type of this auto constant parameter.
    */
    UniformParameter(GpuProgramParameters::AutoConstantType autoType, float fAutoConstantData, size_t size, GpuConstantType type);

    /** Class constructor.
    @param autoType The auto type of this parameter.
    @param nAutoConstantData The int data for this auto constant parameter. 
    @param size number of elements in the parameter.    
    */
    UniformParameter(GpuProgramParameters::AutoConstantType autoType, uint32 nAutoConstantData, size_t size);
    
    /** Class constructor.
    @param autoType The auto type of this parameter.
    @param nAutoConstantData The int data for this auto constant parameter. 
    @param size number of elements in the parameter.
    @param type The desired data type of this auto constant parameter.
    */
    UniformParameter(GpuProgramParameters::AutoConstantType autoType, uint32 nAutoConstantData, size_t size, GpuConstantType type);

    
    /** Get auto constant int data of this parameter, in case it is auto constant parameter. */
    [[nodiscard]] auto getAutoConstantIntData() const noexcept -> uint32
    { return get<uint32>(mAutoConstantData); }

    /** Get auto constant real data of this parameter, in case it is auto constant parameter. */
    [[nodiscard]] auto getAutoConstantRealData() const noexcept -> float
    { return get<float>(mAutoConstantData); }

    /** Return true if this parameter is a floating point type, false otherwise. */
    [[nodiscard]] auto isFloat() const noexcept -> bool
    { return GpuConstantDefinition::isFloat(mType); }

    /** Return true if this parameter is a texture sampler type, false otherwise. */
    [[nodiscard]] auto isSampler() const noexcept -> bool
    { return GpuConstantDefinition::isSampler(mType); }

    /** Return true if this parameter is an auto constant parameter, false otherwise. */
    [[nodiscard]] auto isAutoConstantParameter() const noexcept -> bool
    { return not holds_alternative<::std::monostate>(mAutoConstantData); }

    /** Return true if this parameter an auto constant with int data type, false otherwise. */
    [[nodiscard]] auto isAutoConstantIntParameter() const noexcept -> bool
    { return holds_alternative<uint32>(mAutoConstantData); }

    /** Return true if this parameter an auto constant with real data type, false otherwise. */
    [[nodiscard]] auto isAutoConstantRealParameter() const noexcept -> bool
    { return holds_alternative<float>(mAutoConstantData); }

    /** Return the auto constant type of this parameter. */
    [[nodiscard]] auto getAutoConstantType  () const noexcept -> GpuProgramParameters::AutoConstantType { return mAutoConstantType; }

    /** Return the variability of this parameter. */
    [[nodiscard]] auto getVariability() const noexcept -> GpuParamVariability { return mVariability; }

    /** Bind this parameter to the corresponding GPU parameter. */
    void bind(GpuProgramParametersSharedPtr paramsPtr);

public:

    /** Update the GPU parameter with the given value. */   
    void setGpuParameter(int val)
    { 
        if (mParamsPtr != nullptr)
        {
            mParamsPtr->_writeRawConstant(mPhysicalIndex, val);
        }
    }

    /** Update the GPU parameter with the given value. */   
    void setGpuParameter(Real val)  
    { 
        if (mParamsPtr != nullptr)
        {
            mParamsPtr->_writeRawConstant(mPhysicalIndex, val);
        }
    }

    /** Update the GPU parameter with the given value. */   
    void setGpuParameter(const ColourValue& val)  
    { 
        if (mParamsPtr != nullptr)
        {
            mParamsPtr->_writeRawConstant(mPhysicalIndex, val);
        }
    }

    /** Update the GPU parameter with the given value. */   
    void setGpuParameter(const Vector2& val)  
    { 
        if (mParamsPtr != nullptr)
        {
            mParamsPtr->_writeRawConstants(mPhysicalIndex, val.ptr(), 2);
        }
    }
    
    /** Update the GPU parameter with the given value. */   
    void setGpuParameter(const Vector3& val)  
    { 
        if (mParamsPtr != nullptr)
        {
            mParamsPtr->_writeRawConstant(mPhysicalIndex, val);
        }
    }

    /** Update the GPU parameter with the given value. */   
    void setGpuParameter(const Vector4& val)  
    { 
        if (mParamsPtr != nullptr)
        {
            mParamsPtr->_writeRawConstant(mPhysicalIndex, val);     
        }
    }

    void setGpuParameter(const Matrix3& val)
    {
        if (mParamsPtr == nullptr) return;

        if(mElementSize == 9) // check if tight packing is supported
        {
            mParamsPtr->_writeRawConstant(mPhysicalIndex, val, 9);
        }
        else
        {
            mParamsPtr->_writeRawConstant(mPhysicalIndex, Matrix4::FromMatrix3(val), mElementSize);
        }
    }

    /** Update the GPU parameter with the given value. */   
    void setGpuParameter(const Matrix4& val)  
    { 
        if (mParamsPtr != nullptr)
        {
            mParamsPtr->_writeRawConstant(mPhysicalIndex, val, 16);
        }
    }

    /** Update the GPU parameter with the given value. */   
    void setGpuParameter(const float *val, size_t count, size_t multiple = 4)  
    { 
        if (mParamsPtr != nullptr)
        {
            mParamsPtr->_writeRawConstants(mPhysicalIndex, val, count * multiple);
        }
    }

    /** Update the GPU parameter with the given value. */   
    void setGpuParameter(const double *val, size_t count, size_t multiple = 4)  
    { 
        if (mParamsPtr != nullptr)
        {
            mParamsPtr->_writeRawConstants(mPhysicalIndex, val, count * multiple);
        }
    }

    /** Update the GPU parameter with the given value. */   
    void setGpuParameter(const int *val, size_t count, size_t multiple = 4)  
    { 
        if (mParamsPtr != nullptr)
        {
            mParamsPtr->_writeRawConstants(mPhysicalIndex, val, count * multiple);
        }
    }

    /// light index or array size
    void updateExtraInfo(uint32 data)
    {
        if (!mParamsPtr)
            return;

        mParamsPtr->_setRawAutoConstant(mPhysicalIndex, mAutoConstantType, data, mVariability,
                                        mElementSize);
    }

private:
    GpuProgramParameters::AutoConstantType  mAutoConstantType;      // The auto constant type of this parameter.
    ::std::variant<::std::monostate, uint32, float> mAutoConstantData;
    // How this parameter varies (bitwise combination of GpuProgramVariability).
    GpuParamVariability mVariability;
    // The actual GPU parameters pointer.
    GpuProgramParameters* mParamsPtr;
    // The physical index of this parameter in the GPU program.
    size_t mPhysicalIndex;
    // The size of this parameter in the GPU program
    uint8 mElementSize;
};

using UniformParameterList = std::vector<UniformParameterPtr>;
using UniformParameterIterator = UniformParameterList::iterator;
using UniformParameterConstIterator = UniformParameterList::const_iterator;

/** Helper template which is the base for our ConstParameters
*/
template <class valueType>
class ConstParameter : public Parameter
{
public:

    ConstParameter( valueType val, 
        GpuConstantType type, 
        const Semantic& semantic,  
        const Content& content) 
        : Parameter(type, "Constant", semantic, 0, content)
    {
        mValue = val;
    }

                ~ConstParameter     () override = default;

    /** Returns the native value of this parameter. (for example a Vector3) */
    [[nodiscard]] auto getValue() const noexcept -> const valueType& { return mValue; }

    /** 
    @see Parameter::isConstParameter.
    */
    [[nodiscard]] auto isConstParameter() const noexcept -> bool override { return true; }

    /** 
    @see Parameter::toString.
    */
    [[nodiscard]] auto toString() const noexcept -> String override = 0;

protected:
    valueType mValue;
};

/** Helper utility class that creates common parameters.
*/
class ParameterFactory
{

    // Interface.
public:

    static auto createInPosition(int index, Parameter::Content content = Parameter::Content::POSITION_OBJECT_SPACE) -> ParameterPtr;
    static auto createOutPosition(int index) -> ParameterPtr;

    static auto createInNormal(int index) -> ParameterPtr;
    static auto createInWeights(int index) -> ParameterPtr;
    static auto createInIndices(int index) -> ParameterPtr;
    static auto createOutNormal(int index) -> ParameterPtr;
    static auto createInBiNormal(int index) -> ParameterPtr;
    static auto createOutBiNormal(int index) -> ParameterPtr;
    static auto createInTangent(int index) -> ParameterPtr;
    static auto createOutTangent(int index) -> ParameterPtr;
    static auto createInColor(int index) -> ParameterPtr;
    static auto createOutColor(int index) -> ParameterPtr;

    static auto createInTexcoord(GpuConstantType type, int index, Parameter::Content content) -> ParameterPtr;
    static auto createOutTexcoord(GpuConstantType type, int index, Parameter::Content content) -> ParameterPtr;

    static auto createConstParam(const Vector2& val) -> ParameterPtr;
    static auto createConstParam(const Vector3& val) -> ParameterPtr;
    static auto createConstParam(const Vector4& val) -> ParameterPtr;
    static auto createConstParam(float val) -> ParameterPtr;

    static auto createSampler(GpuConstantType type, int index) -> UniformParameterPtr;
    static auto createSampler1D(int index) -> UniformParameterPtr;
    static auto createSampler2D(int index) -> UniformParameterPtr;
    static auto createSampler2DArray(int index) -> UniformParameterPtr;
    static auto createSampler3D(int index) -> UniformParameterPtr;
    static auto createSamplerCUBE(int index) -> UniformParameterPtr;    

    static auto createUniform(GpuConstantType type, int index, GpuParamVariability variability, std::string_view suggestedName, size_t size) -> UniformParameterPtr;
};



/** @} */
/** @} */

}
}

#endif
