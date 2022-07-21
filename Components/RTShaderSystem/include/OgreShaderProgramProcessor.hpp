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
module;

#include <cstddef>

export module Ogre.Components.RTShaderSystem:ShaderProgramProcessor;

export import :ShaderFunctionAtom;
export import :ShaderPrerequisites;

export import Ogre.Core;

export import <map>;
export import <vector>;

export
namespace Ogre::RTShader {
class Function;
class Program;
class ProgramSet;

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** A class that provides extra processing services on CPU based programs.
The base class perform only the generic processing. In order to provide target language specific services and 
optimization one should derive from this class and register its factory via the ProgramManager instance.
*/
class ProgramProcessor : public RTShaderSystemAlloc
{

// Interface.
public: 

    /** Class constructor.
    */
    ProgramProcessor();

    /** Class destructor */
    virtual ~ProgramProcessor();
    
    /** Called before creation of the GPU programs.
    Do several preparation operation such as validation, register compaction and specific target language optimizations.
    @param programSet The program set container.
    Return true on success.
    */
    virtual auto preCreateGpuPrograms(ProgramSet* programSet) -> bool = 0;

    /** Called after creation of the GPU programs.
    @param programSet The program set container.
    Return true on success.
    */
    virtual auto postCreateGpuPrograms(ProgramSet* programSet) -> bool = 0;
 
// Protected types.
protected:
    
    //-----------------------------------------------------------------------------
    // Class that holds merge parameter information.
    class MergeParameter 
    {
    // Interface.
    public:
        /** Class constructor. */
        MergeParameter();


        /** Clear the state of this merge parameter. */
        void clear();
        
        /** Add source parameter to this merged */
        void addSourceParameter(ParameterPtr srcParam, Operand::OpMask mask);

        /** Return the source parameter count. */
        [[nodiscard]] auto getSourceParameterCount() const noexcept -> size_t { return mSrcParameterCount; }

        /** Return source parameter by index. */
        auto getSourceParameter(unsigned int index) -> ParameterPtr { return mSrcParameter[index]; }

        /** Return source parameter mask by index. */
        [[nodiscard]] auto getSourceParameterMask(unsigned int index) const -> Operand::OpMask { return mSrcParameterMask[index]; }

        /** Return destination parameter mask by index. */
        [[nodiscard]] auto getDestinationParameterMask(unsigned int index) const -> Operand::OpMask { return mDstParameterMask[index]; }

        /** Return the number of used floats. */ 
        auto getUsedFloatCount() noexcept -> int;
        
        /** Return the destination parameter. */
        auto getDestinationParameter(Operand::OpSemantic usage, int index) -> ParameterPtr;

    protected:
    
        /** Creates the destination parameter by a given class and index. */
        void createDestinationParameter(Operand::OpSemantic usage, int index);


    protected:
        // Destination merged parameter.
        ParameterPtr mDstParameter;
        // Source parameters - 4 source at max 1,1,1,1 -> 4.
        ParameterPtr mSrcParameter[4];
        // Source parameters mask. OpMask::ALL means all fields used, otherwise it is split source parameter.
        Operand::OpMask mSrcParameterMask[4];
        // Destination parameters mask. OpMask::ALL means all fields used, otherwise it is split source parameter.
        Operand::OpMask mDstParameterMask[4];
        // The actual source parameters count.
        size_t mSrcParameterCount;
        // The number of used floats.
        int mUsedFloatCount;
    };
    using MergeParameterList = std::vector<MergeParameter>;

    
    //-----------------------------------------------------------------------------
    // A struct that defines merge parameters combination.
    struct MergeCombination
    {       
        // The count of each source type. I.E (1 FLOAT1, 0 FLOAT2, 1 FLOAT3, 0 FLOAT4).
        size_t srcParameterTypeCount[4];
        // Source parameters mask. OpMask::ALL means all fields used, otherwise it is split source parameter.
        using enum Operand::OpMask;
        Operand::OpMask srcParameterMask[4]{ALL, ALL, ALL, ALL};
    };
    using MergeCombinationList = std::vector<MergeCombination>;

    //-----------------------------------------------------------------------------
    using OperandPtrVector = std::vector<Operand *>;
    using ParameterOperandMap = std::map<Parameter *, OperandPtrVector>;
    using LocalParameterMap = std::map<Parameter *, ParameterPtr>;

protected:

    /** Build parameter merging combinations. */
    void buildMergeCombinations();

    /** Compact the vertex shader output registers.
    @param vsMain The vertex shader entry function.
    @param fsMain The fragment shader entry function.
    Return true on success.
    */
    virtual auto compactVsOutputs(Function* vsMain, Function* fsMain) -> bool;

    /** Internal method that counts vertex shader texcoord output slots and output floats.
    @param vsMain The vertex shader entry function.
    @param outTexCoordSlots Will hold the number of used output texcoord slots.
    @param outTexCoordFloats Will hold the total number of floats used by output texcoord slots.
    */
    void countVsTexcoordOutputs(Function* vsMain, int& outTexCoordSlots, int& outTexCoordFloats);

    /** Internal function that builds parameters table.
    @param paramList The parameter list.
    @param outParamsTable Will hold the texcoord params sorted by types in each row.
    */
    void buildTexcoordTable(const ShaderParameterList& paramList, ShaderParameterList outParamsTable[4]);


    /** Merge the parameters from the given table. 
    @param paramsTable Source parameters table.
    @param mergedParams Will hold the merged parameters list.
    */
    void mergeParameters(ShaderParameterList paramsTable[4], MergeParameterList& mergedParams, ShaderParameterList& splitParams);


    /** Internal function that creates merged parameter using pre defined combinations. 
    @param paramsTable Source parameters table.
    @param mergedParams The merged parameters list.
    */
    void mergeParametersByPredefinedCombinations(ShaderParameterList paramsTable[4], MergeParameterList& mergedParams);

    /** Internal function that creates merged parameter from given combination.
    @param combination The merge combination to try.
    @param paramsTable The params table sorted by types in each row.    
    @param mergedParameter Will hold the merged parameter.
    */
    auto mergeParametersByCombination(const MergeCombination& combination, ShaderParameterList paramsTable[4], 
                                                                 MergeParameter* mergedParameter) -> bool;

    /** Merge reminders parameters that could not be merged into one slot using the predefined combinations.
    @param paramsTable The params table sorted by types in each row.    
    @param mergedParams The merged parameters list.
    @param splitParams The split parameters list.
    */
    void mergeParametersReminders(ShaderParameterList paramsTable[4], MergeParameterList& mergedParams, ShaderParameterList& splitParams);


    /** Generates local parameters for the split parameters and perform packing/unpacking operation using them. */
    void generateLocalSplitParameters(Function* func, GpuProgramType progType, MergeParameterList& mergedParams, ShaderParameterList& splitParams, LocalParameterMap& localParamsMap);
    
    /** Rebuild the given parameters list using the merged parameters.  
    */
    void rebuildParameterList(Function* func, Operand::OpSemantic paramsUsage, MergeParameterList& mergedParams);

    /** Rebuild function invocations by replacing references to old source parameters with the matching merged parameters components. */
    void rebuildFunctionInvocations(const FunctionAtomInstanceList& funcAtomList, MergeParameterList& mergedParams, LocalParameterMap& localParamsMap);

    /** Builds a map between parameter and all the references to it. */
    void buildParameterReferenceMap(const FunctionAtomInstanceList& funcAtomList, ParameterOperandMap& paramsRefMap);

    /** Replace references to old parameters with the new merged parameters. */
    void replaceParametersReferences(MergeParameterList& mergedParams, ParameterOperandMap& paramsRefMap);

    /** Replace references to old parameters that have been split with the new local parameters that represents them. */
    void replaceSplitParametersReferences(LocalParameterMap& localParamsMap, ParameterOperandMap& paramsRefMap);

    /** Return number of floats needed by the given type. */
    static auto getParameterFloatCount(GpuConstantType type) -> int;        

    /** Return the parameter mask of by the given parameter type (I.E: X|Y for FLOAT2 etc..) */
    static auto getParameterMaskByType(GpuConstantType type) -> Operand::OpMask;
    
    /** Return the parameter mask of by the float count type (I.E: X|Y for 2 etc..) */
    static auto getParameterMaskByFloatCount(int floatCount) -> Operand::OpMask;
    
    /** Bind the auto parameters for a given CPU and GPU program set. */
    void bindAutoParameters(Program* pCpuProgram, GpuProgramPtr pGpuProgram);

protected:
    // Merging combinations defs.
    MergeCombinationList mParamMergeCombinations;
    // Maximum texcoord slots.
    int mMaxTexCoordSlots;
    // Maximum texcoord floats count.
    int mMaxTexCoordFloats;
    std::map<Function *, String *>  mFunctionMap;           // Map between function signatures and source code

};


/** @} */
/** @} */

}
