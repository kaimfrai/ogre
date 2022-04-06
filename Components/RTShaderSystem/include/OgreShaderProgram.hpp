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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_PROGRAM_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_PROGRAM_H

#include <cstddef>

#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreStringVector.hpp"

namespace Ogre {
namespace RTShader {
class Function;

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** A class that represents a shader based program.
*/
class Program : public RTShaderSystemAlloc
{

// Interface.
public: 
    /** Get the type of this program. */
    auto getType() const -> GpuProgramType;

    /** Resolve uniform auto constant parameter with associated real data of this program.
    @param autoType The auto type of the desired parameter. 
    @param data The data to associate with the auto parameter.
    @param size number of elements in the parameter.    
    @return parameter instance in case of that resolve operation succeeded.  
    */
    auto resolveAutoParameterReal(GpuProgramParameters::AutoConstantType autoType, Real data, size_t size = 0) -> UniformParameterPtr;
    
    /** Resolve uniform auto constant parameter with associated real data of this program.
    @param autoType The auto type of the desired parameter. 
    @param type The desired data type of the auto parameter.
    @param data The data to associate with the auto parameter.
    @param size number of elements in the parameter.    
    @return parameter instance in case of that resolve operation succeeded.  
    */
    auto resolveAutoParameterReal(GpuProgramParameters::AutoConstantType autoType, GpuConstantType type, float data, size_t size = 0) -> UniformParameterPtr;

    /** Resolve uniform auto constant parameter with associated int data of this program.
    @param autoType The auto type of the desired parameter.
    @param type The desired data type of the auto parameter.
    @param data The data to associate with the auto parameter.
    @param size number of elements in the parameter.
    @return parameter instance in case of that resolve operation succeeded.  
    */
    auto resolveAutoParameterInt(GpuProgramParameters::AutoConstantType autoType, GpuConstantType type, uint32 data, size_t size = 0) -> UniformParameterPtr;

    /** Resolve uniform parameter of this program.
    @param type The type of the desired parameter.
    @param index The index of the desired parameter.
    @param suggestedName The suggested name for the parameter in case new one should be create. 
    @param variability How this parameter varies (bitwise combination of GpuProgramVariability).
    @param size number of elements in the parameter.    
    @return parameter instance in case of that resolve operation succeeded.
    @remarks Pass -1 as index parameter to create a new parameter with the desired type and index.
    */
    auto resolveParameter(GpuConstantType type, int index, uint16 variability, const String& suggestedName, size_t size = 0) -> UniformParameterPtr;
    
    /// @overload
    auto resolveParameter(GpuConstantType type, const String& name, int index = -1) -> UniformParameterPtr
    {
        return resolveParameter(type, index, GPV_GLOBAL, name);
    }

    /** Resolve uniform auto constant parameter
    @param autoType The auto type of the desired parameter
    @param data The data to associate with the auto parameter. 
    @return parameter instance in case of that resolve operation succeeded.  
    */
    auto resolveParameter(GpuProgramParameters::AutoConstantType autoType, uint32 data = 0) -> UniformParameterPtr;

    /** Get parameter by a given name.  
    @param name The name of the parameter to search for.
    @remarks Return NULL if no matching parameter found.
    */
    auto getParameterByName(const String& name) -> UniformParameterPtr;

    /** Get parameter by a given auto constant type.    
    @param autoType The auto type of the parameter to search for.
    @remarks Return NULL if no matching parameter found.
    */
    auto getParameterByAutoType(GpuProgramParameters::AutoConstantType autoType) -> UniformParameterPtr;

    /** Get parameter by a given type and index.    
    @param type The type of the parameter to search for.
    @param index The index of the parameter to search for.
    @remarks Return NULL if no matching parameter found.
    */
    auto getParameterByType(GpuConstantType type, int index) -> UniformParameterPtr;

    /** Get the list of uniform parameters of this program.
    */
    auto getParameters() const -> const UniformParameterList& { return mParameters; };

    /// @deprecated use getMain()
    auto getEntryPointFunction() -> Function*                    { return mEntryPointFunction; }

    auto getMain() -> Function* { return mEntryPointFunction; }

    /** Add dependency for this program. Basically a filename that will be included in this
    program and provide predefined shader functions code.
    One should verify that the given library file he provides can be reached by the resource manager.
    This step can be achieved using the ResourceGroupManager::addResourceLocation method.
    */
    void addDependency(const String& libFileName);

    /** Get the number of external libs this program depends on */
    auto getDependencyCount() const -> size_t;

    /** Get the library name of the given index dependency.
    @param index The index of the dependecy.
    */
    auto getDependency(unsigned int index) const -> const String&;
    

    /** Sets whether a vertex program includes the required instructions
        to perform skeletal animation. 
    */
    void setSkeletalAnimationIncluded(bool value) { mSkeletalAnimation = value; }
 
    /** Returns whether a vertex program includes the required instructions
        to perform skeletal animation. 
    */
    auto getSkeletalAnimationIncluded() const -> bool { return mSkeletalAnimation; }

    /** Tells Ogre whether auto-bound matrices should be sent in column or row-major order.
    @remarks
        This method has the same effect as column_major_matrices option used when declaring manually written hlsl program.
        You want to use this method only when you use float3x4 type in a shader, e.g. for bone matrices.
        In mentioned case you should call this method with false as parameter.
    @par
        For more detail see OGRE Manual, section <b>3.1.6 DirectX9 HLSL</b>.
    @note
        This setting has any effect only when the target language is HLSL.
    @param value Should Ogre pass auto-bound matrices as column-major? The default is true.
    */
    void setUseColumnMajorMatrices(bool value) { mColumnMajorMatrices = value; }
    
    /** Returns whether Ogre will pass auto-bound matrices as column-major.
    @return
    true, when the matrices will be passed in column-major order, false, when they will be passed as row-major.
    */
    auto getUseColumnMajorMatrices() const -> bool { return mColumnMajorMatrices; }

    void addPreprocessorDefines(const String& defines);

    auto getPreprocessorDefines() const -> const String& { return mPreprocessorDefines; }

    /** Class destructor */
    ~Program();
// Protected methods.
private:

    /** Class constructor.
    @param type The type of this program.
    */
    Program(GpuProgramType type);

    /** Destroy all parameters of this program. */
    void destroyParameters();

    /** Add parameter to this program. */
    void addParameter(UniformParameterPtr parameter);
        
    /** Remove parameter from this program. */
    void removeParameter(UniformParameterPtr parameter);

    // Program type. (Vertex, Fragment, Geometry).
    GpuProgramType mType;
    // Program uniform parameters.  
    UniformParameterList mParameters;
    // Entry point function for this program.   
    Function* mEntryPointFunction;
    // Program dependencies.
    StringVector mDependencies;
    /// preprocessor definitions
    String mPreprocessorDefines;
    // Skeletal animation calculation
    bool mSkeletalAnimation;
    // Whether to pass matrices as column-major.
    bool mColumnMajorMatrices;
    friend class TargetRenderState;
};

/** @} */
/** @} */

}
}

#endif

