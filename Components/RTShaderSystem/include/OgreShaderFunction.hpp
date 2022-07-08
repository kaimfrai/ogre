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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_FUNCTION_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_FUNCTION_H

#include <cstddef>
#include <map>
#include <memory>
#include <vector>

#include "OgreException.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderFunctionAtom.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrerequisites.hpp"

namespace Ogre::RTShader {
class Function;
class ProgramManager;

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/// represents a @ref FFPShaderStage, part of a Function
class FunctionStageRef
{
    friend class Function;
public:
    /** call a library function
     * @param name the function name
     * @param inout function argument
     */
    void callFunction(const char* name, const InOut& inout) const;

    /// @overload
    void callFunction(const char* name, const std::vector<Operand>& params) const;
    /// @overload
    void callFunction(const char* name, const In& arg, const Out& ret) const { callFunction(name, {arg, ret}); }
    /// @overload
    void callFunction(const char* name, const In& arg0, const In& arg1, const Out& ret) const
    {
        callFunction(name, {arg0, arg1, ret});
    }

    /// dst = texture(sampler, texcoord);
    void sampleTexture(const In& sampler, const In& texcoord, const Out& dst) const
    {
        sampleTexture({sampler, texcoord, dst});
    }
    /// @overload
    void sampleTexture(const std::vector<Operand>& params) const;

    /// to = from;
    void assign(const In& from, const Out& to) const { assign({from, to}); }
    /// @overload
    void assign(const std::vector<Operand>& params) const;

    /// dst = arg0 * arg1;
    void mul(const In& arg0, const In& arg1, const Out& dst) const { binaryOp('*', {arg0, arg1, dst}); }

    /// dst = arg0 / arg1;
    void div(const In& arg0, const In& arg1, const Out& dst) const { binaryOp('/', {arg0, arg1, dst}); }

    /// dst = arg0 - arg1;
    void sub(const In& arg0, const In& arg1, const Out& dst) const { binaryOp('-', {arg0, arg1, dst}); }

    /// dst = arg0 + arg1;
    void add(const In& arg0, const In& arg1, const Out& dst) const { binaryOp('+', {arg0, arg1, dst}); }

    /// dst = arg0 OP arg1;
    void binaryOp(char op, const std::vector<Operand>& params) const;

private:
    uint32 mStage;
    Function* mParent;
    FunctionStageRef(uint32 stage, Function* parent) : mStage(stage), mParent(parent) {}
};

/** A class that represents a shader based program function.
*/
class Function : public RTShaderSystemAlloc
{
    friend ProgramManager;
    friend ::std::default_delete<Function>;
// Interface.
public:
    /// @deprecated
    auto resolveInputParameter(Parameter::Semantic semantic, int index,  const Parameter::Content content, GpuConstantType type) -> ParameterPtr;

    /** Resolve input parameter of this function
    @param content The content of the parameter.
    @param type The type of the desired parameter.
    @return parameter instance in case of that resolve operation succeeded.
    */
    auto resolveInputParameter(Parameter::Content content, GpuConstantType type = GCT_UNKNOWN) -> ParameterPtr
    {
        return resolveInputParameter(Parameter::SPS_UNKNOWN, 0, content, type);
    }

    /// resolve input parameter from previous output
    auto resolveInputParameter(const ParameterPtr& out) -> ParameterPtr
    {
        OgreAssert(out, "parameter must not be NULL");
        return resolveInputParameter(out->getSemantic(), out->getIndex(), out->getContent(), out->getType());
    }

    /**
     * get input parameter by content
     * @param content
     * @param type The type of the desired parameter.
     * @return parameter or NULL if not found
     */
    auto getInputParameter(Parameter::Content content, GpuConstantType type = GCT_UNKNOWN) -> ParameterPtr
    {
        return _getParameterByContent(mInputParameters, content, type);
    }

    /// @deprecated
    auto resolveOutputParameter(Parameter::Semantic semantic, int index,  const Parameter::Content content, GpuConstantType type) -> ParameterPtr;

    /** Resolve output parameter of this function
    @param content The content of the parameter.
    @param type The type of the desired parameter.
    @return parameter instance in case of that resolve operation succeeded.
    */
    auto resolveOutputParameter(Parameter::Content content, GpuConstantType type = GCT_UNKNOWN) -> ParameterPtr
    {
        return resolveOutputParameter(Parameter::SPS_UNKNOWN, 0, content, type);
    }

    /**
     * get output parameter by content
     * @param content
     * @param type The type of the desired parameter.
     * @return parameter or NULL if not found
     */
    auto getOutputParameter(Parameter::Content content, GpuConstantType type = GCT_UNKNOWN) -> ParameterPtr
    {
        return _getParameterByContent(mOutputParameters, content, type);
    }

    /** Resolve local parameter of this function

    local parameters do not have index or semantic.
    @param name The name of the parameter.
    @param type The type of the desired parameter.  
    @return parameter instance in case of that resolve operation succeeded.
    */
    auto resolveLocalParameter(GpuConstantType type, StringView name) -> ParameterPtr;

    /** Resolve local parameter of this function

    local parameters do not have index or semantic.
    @param content The content of the parameter.
    @param type The type of the desired parameter.
    @return parameter instance in case of that resolve operation succeeded.
    */
    auto resolveLocalParameter(Parameter::Content content, GpuConstantType type = GCT_UNKNOWN) -> ParameterPtr;

    /**
     * get local parameter by content
     * @param content
     * @return parameter or NULL if not found
     */
    auto getLocalParameter(Parameter::Content content) -> ParameterPtr
    {
        return _getParameterByContent(mLocalParameters, content, GCT_UNKNOWN);
    }
    /// @overload
    auto getLocalParameter(StringView name) -> ParameterPtr
    {
        return _getParameterByName(mLocalParameters, name);
    }

    /** Return a list of input parameters. */
    [[nodiscard]] auto getInputParameters() const noexcept -> const ShaderParameterList& { return mInputParameters; }  

    /** Return a list of output parameters. */
    [[nodiscard]] auto getOutputParameters() const noexcept -> const ShaderParameterList& { return mOutputParameters; }

    /** Return a list of local parameters. */
    [[nodiscard]] auto getLocalParameters() const noexcept -> const ShaderParameterList& { return mLocalParameters; }  
    
    /** Add a function atom instance to this function. 
    @param atomInstance The atom instance to add.
    */
    void addAtomInstance(FunctionAtom* atomInstance);

    /// get a @ref FFPShaderStage of this function
    auto getStage(uint32 s) -> FunctionStageRef
    {
        return {s, this};
    }

    /** Delete a function atom instance from this function. 
    @param atomInstance The atom instance to delete.
    */
    auto deleteAtomInstance(FunctionAtom* atomInstance) -> bool;

    /** Return list of atom instances composing this function. (Const version) */
    auto getAtomInstances() noexcept -> const FunctionAtomInstanceList&;

    /** Add input parameter to this function. */
    void addInputParameter(ParameterPtr parameter);

    /** Add output parameter to this function. */
    void addOutputParameter(ParameterPtr parameter);

    /** Delete input parameter from this function. */
    void deleteInputParameter(ParameterPtr parameter);

    /** Delete output parameter from this function. */
    void deleteOutputParameter(ParameterPtr parameter);

    /** Delete all input parameters from this function. */
    void deleteAllInputParameters();

    /** Delete all output parameters from this function. */
    void deleteAllOutputParameters();

private:

    static auto _getParameterByName(const ShaderParameterList& parameterList, StringView name) -> ParameterPtr;
    static auto _getParameterBySemantic(const ShaderParameterList& parameterList, const Parameter::Semantic semantic, int index) -> ParameterPtr;
    static auto _getParameterByContent(const ShaderParameterList& parameterList, const Parameter::Content content, GpuConstantType type) -> ParameterPtr;

    /** Class destructor */
    ~Function();

    /** Add parameter to given list */
    void addParameter(ShaderParameterList& parameterList, ParameterPtr parameter);

    /** Delete parameter from a given list */
    void deleteParameter(ShaderParameterList& parameterList, ParameterPtr parameter);

    // Input parameters.
    ShaderParameterList mInputParameters;
    // Output parameters.
    ShaderParameterList mOutputParameters;
    // Local parameters.
    ShaderParameterList mLocalParameters;
    // Atom instances composing this function.
    std::map<size_t, std::vector<::std::unique_ptr<FunctionAtom>>> mAtomInstances;
    FunctionAtomInstanceList mSortedAtomInstances;
private:
    friend class Program;
};

using ShaderFunctionList = std::vector<Function *>;
using ShaderFunctionIterator = ShaderFunctionList::iterator;
using ShaderFunctionConstIterator = ShaderFunctionList::const_iterator;

/** @} */
/** @} */

}

#endif
