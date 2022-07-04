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
#ifndef OGRE_CORE_HIGHLEVELGPUPROGRAM_H
#define OGRE_CORE_HIGHLEVELGPUPROGRAM_H

#include <cstddef>
#include <utility>
#include <vector>

#include "OgreGpuProgram.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResource.hpp"
#include "OgreSharedPtr.hpp"

namespace Ogre {
class ResourceManager;
struct GpuNamedConstants;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    /** Abstract base class representing a high-level program (a vertex or
        fragment program).
    @remarks
        High-level programs are vertex and fragment programs written in a high-level
        language such as Cg or HLSL, and as such do not require you to write assembler code
        like GpuProgram does. However, the high-level program does eventually 
        get converted (compiled) into assembler and then eventually microcode which is
        what runs on the GPU. As well as the convenience, some high-level languages like Cg allow
        you to write a program which will operate under both Direct3D and OpenGL, something
        which you cannot do with just GpuProgram (which requires you to write 2 programs and
        use each in a Technique to provide cross-API compatibility). Ogre will be creating
        a GpuProgram for you based on the high-level program, which is compiled specifically 
        for the API being used at the time, but this process is transparent.
    @par
        You cannot create high-level programs direct - use HighLevelGpuProgramManager instead.
        Plugins can register new implementations of HighLevelGpuProgramFactory in order to add
        support for new languages without requiring changes to the core Ogre API. To allow 
        custom parameters to be set, this class extends StringInterface - the application
        can query on the available custom parameters and get/set them without having to 
        link specifically with it.
    */
    class HighLevelGpuProgram : public GpuProgram
    {
    protected:
        /// Whether the high-level program (and it's parameter defs) is loaded
        bool mHighLevelLoaded{false};
        /// Have we built the name->index parameter map yet?
        bool mConstantDefsBuilt{false};
        /// The underlying assembler program
        GpuProgramPtr mAssemblerProgram;
        /// Preprocessor options
        String mPreprocessorDefines;
        /// Entry point for this program
        String mEntryPoint;

        /// in-situ parsing of defines
        static auto parseDefines(String& defines) -> std::vector<std::pair<const char*, const char*>>;

        auto appendBuiltinDefines(String defines) -> String;

        /// Internal load high-level portion if not loaded
        virtual void loadHighLevel();
        /// Internal unload high-level portion if loaded
        virtual void unloadHighLevel();
        /** Internal method for creating an appropriate low-level program from this
        high-level program, must be implemented by subclasses. */
        virtual void createLowLevelImpl() = 0;
        /// Internal unload implementation, must be implemented by subclasses
        virtual void unloadHighLevelImpl() = 0;
        /// Populate the passed parameters with name->index map
        void populateParameterNames(GpuProgramParametersSharedPtr params);
        /** Build the constant definition map, must be overridden.
        @note The implementation must fill in the (inherited) mConstantDefs field at a minimum, 
            and if the program requires that parameters are bound using logical 
            parameter indexes then the mLogicalToPhysical and mIntLogicalToPhysical
            maps must also be populated.
        */
        virtual void buildConstantDefinitions() = 0;

        /** @copydoc Resource::loadImpl */
        void loadImpl() override;
        /** @copydoc Resource::unloadImpl */
        void unloadImpl() override;

        void setupBaseParamDictionary() override;
    public:
        /** Constructor, should be used only by factory classes. */
        HighLevelGpuProgram(ResourceManager* creator, const String& name, ResourceHandle handle,
            const String& group, bool isManual = false, ManualResourceLoader* loader = nullptr);
        ~HighLevelGpuProgram() override = default;


        /** Creates a new parameters object compatible with this program definition. 
        @remarks
            Unlike low-level assembly programs, parameters objects are specific to the
            program and therefore must be created from it rather than by the 
            HighLevelGpuProgramManager. This method creates a new instance of a parameters
            object containing the definition of the parameters this program understands.
        */
        auto createParameters() -> GpuProgramParametersSharedPtr override;
        /** @copydoc GpuProgram::_getBindingDelegate */
        auto _getBindingDelegate() noexcept -> GpuProgram* override { return mAssemblerProgram.get(); }

        /** Get the full list of GpuConstantDefinition instances.
        @note
        Only available if this parameters object has named parameters.
        */
        auto getConstantDefinitions() noexcept -> const GpuNamedConstants& override;

        auto calculateSize() const -> size_t override;

        /** Sets the preprocessor defines used to compile the program. */
        void setPreprocessorDefines(const String& defines) { mPreprocessorDefines = defines; }
        /** Gets the preprocessor defines used to compile the program. */
        auto getPreprocessorDefines() const noexcept -> const String& { return mPreprocessorDefines; }

        /** Sets the entry point for this program i.e, the first method called. */
        void setEntryPoint(const String& entryPoint) { mEntryPoint = entryPoint; }
        /** Gets the entry point defined for this program. */
        auto getEntryPoint() const noexcept -> const String& { return mEntryPoint; }

        /// Scan the source for \#include and replace with contents from OGRE resources
        static auto _resolveIncludes(const String& source, Resource* resourceBeingLoaded, const String& fileName, bool supportsFilename = false) -> String;
    };
    /** @} */
    /** @} */

}

#endif
