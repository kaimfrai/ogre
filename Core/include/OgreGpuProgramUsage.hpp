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

export module Ogre.Core:GpuProgramUsage;

export import :GpuProgram;
export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :Resource;
export import :SharedPtr;

export
namespace Ogre 
{
    class Pass;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Materials
    *  @{
    */
    /** This class makes the usage of a vertex and fragment programs (low-level or high-level), 
        with a given set of parameters, explicit.
    @remarks
        Using a vertex or fragment program can get fairly complex; besides the fairly rudimentary
        process of binding a program to the GPU for rendering, managing usage has few
        complications, such as:
        <ul>
        <li>Programs can be high level (e.g. Cg, RenderMonkey) or low level (assembler). Using
        either should be relatively seamless, although high-level programs give you the advantage
        of being able to use named parameters, instead of just indexed registers</li>
        <li>Programs and parameters can be shared between multiple usages, in order to save
        memory</li>
        <li>When you define a user of a program, such as a material, you often want to be able to
        set up the definition but not load / compile / assemble the program at that stage, because
        it is not needed just yet. The program should be loaded when it is first needed, or
        earlier if specifically requested. The program may not be defined at this time, you
        may want to have scripts that can set up the definitions independent of the order in which
        those scripts are loaded.</li>
        </ul>
        This class packages up those details so you don't have to worry about them. For example,
        this class lets you define a high-level program and set up the parameters for it, without
        having loaded the program (which you normally could not do). When the program is loaded and
        compiled, this class will then validate the parameters you supplied earlier and turn them
        into runtime parameters.
    @par
        Just incase it wasn't clear from the above, this class provides linkage to both 
        GpuProgram and HighLevelGpuProgram, despite its name.
    */
    class GpuProgramUsage : public Resource::Listener, public PassAlloc
    {
    private:
        Pass* mParent;
        /// The program link
        GpuProgramPtr mProgram;

        /// Program parameters
        GpuProgramParametersSharedPtr mParameters;
        
        /// Whether to recreate parameters next load
        bool mRecreateParams;
        GpuProgramType mType;

        void recreateParameters();

    public:
        /** Default constructor.
        @param gptype The type of program to link to
        @param parent
        */
        GpuProgramUsage(GpuProgramType gptype, Pass* parent);

        /** Copy constructor */
        GpuProgramUsage(const GpuProgramUsage& rhs, Pass* newparent);

        ~GpuProgramUsage() override;

        /** Gets the type of program we're trying to link to. */
        [[nodiscard]] auto getType() const noexcept -> GpuProgramType { return mType; }

        /** Sets the name of the program to use. 
        @param name The name of the program to use
        @param resetParams
            If true, this will create a fresh set of parameters from the
            new program being linked, so if you had previously set parameters
            you will have to set them again. If you set this to false, you must
            be absolutely sure that the parameters match perfectly, and in the
            case of named parameters refers to the indexes underlying them, 
            not just the names.
        */
        void setProgramName(std::string_view name, bool resetParams = true);
        /** Sets the program to use.
        @remarks
            Note that this will create a fresh set of parameters from the
            new program being linked, so if you had previously set parameters
            you will have to set them again.
        */
        void setProgram(const GpuProgramPtr& prog, bool resetParams = true);
        /** Gets the program being used. */
        [[nodiscard]] auto getProgram() const noexcept -> const GpuProgramPtr& { return mProgram; }
        /** Gets the program being used. */
        [[nodiscard]] auto getProgramName() const noexcept -> std::string_view { return mProgram->getName(); }

        /** Sets the program parameters that should be used; because parameters can be
            shared between multiple usages for efficiency, this method is here for you
            to register externally created parameter objects. Otherwise, the parameters
            will be created for you when a program is linked.
        */
        void setParameters(const GpuProgramParametersSharedPtr& params);
        /** Gets the parameters being used here. 
        */
        [[nodiscard]] auto getParameters() const noexcept -> const GpuProgramParametersSharedPtr&;

        /// Load this usage (and ensure program is loaded)
        void _load();
        /// Unload this usage 
        void _unload();

        [[nodiscard]] auto calculateSize() const -> size_t;

        // Resource Listener
        void unloadingComplete(Resource* prog) override;
        void loadingComplete(Resource* prog) override;

        static auto _getProgramByName(std::string_view name, std::string_view group,
                                               GpuProgramType type) -> GpuProgramPtr;
    };
    /** @} */
    /** @} */
}
