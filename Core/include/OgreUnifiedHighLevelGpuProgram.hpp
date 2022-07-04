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
#ifndef OGRE_CORE_UNIFIEDHIGHLEVELGPUPROGRAM_H
#define OGRE_CORE_UNIFIEDHIGHLEVELGPUPROGRAM_H

#include <cstddef>

#include "OgreCommon.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramManager.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResource.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreStringVector.hpp"

namespace Ogre {
class ResourceManager;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    /** Specialisation of HighLevelGpuProgram which just delegates its implementation
        to one other GpuProgram, allowing a single program definition
        to represent one supported program from a number of options
    @remarks
        Whilst you can use Technique to implement several ways to render an object
        depending on hardware support, if the only reason to need multiple paths is
        because of the shader language supported, this can be
        cumbersome. For example you might want to implement the same shader 
        in HLSL and GLSL for portability but apart from the implementation detail,
        the shaders do the same thing and take the same parameters. If the materials
        in question are complex, duplicating the techniques just to switch language
        is not optimal, so instead you can define high-level programs with a 
        syntax of 'unified', and list the actual implementations in order of
        preference via repeated use of the 'delegate' parameter, which just points
        at another program name. The first one which has a supported syntax 
        will be used.
    */
    class UnifiedHighLevelGpuProgram final : public GpuProgram
    {
        /// Ordered list of potential delegates
        StringVector mDelegateNames;
        /// The chosen delegate
        mutable GpuProgramPtr mChosenDelegate;

        /// Choose the delegate to use
        void chooseDelegate() const;

        void createLowLevelImpl();
        void unloadHighLevelImpl();
        void loadFromSource() override;

        void unloadImpl() override { resetCompileError(); }
    public:
        /** Constructor, should be used only by factory classes. */
        UnifiedHighLevelGpuProgram(ResourceManager* creator, const String& name, ResourceHandle handle,
            const String& group, bool isManual = false, ManualResourceLoader* loader = nullptr);
        ~UnifiedHighLevelGpuProgram() override;

        auto calculateSize() const -> size_t override;

        /** Adds a new delegate program to the list.
        @remarks
            Delegates are tested in order so earlier ones are preferred.
        */
        void addDelegateProgram(const String& name);

        /// Remove all delegate programs
        void clearDelegatePrograms();

        /// Get the chosen delegate
        auto _getDelegate() const -> const GpuProgramPtr&;

        /** @copydoc GpuProgram::getLanguage */
        auto getLanguage() const noexcept -> const String& override;

        /** Creates a new parameters object compatible with this program definition. 
        @remarks
        Unlike low-level assembly programs, parameters objects are specific to the
        program and therefore must be created from it rather than by the 
        HighLevelGpuProgramManager. This method creates a new instance of a parameters
        object containing the definition of the parameters this program understands.
        */
        auto createParameters() -> GpuProgramParametersSharedPtr override;
        /** @copydoc GpuProgram::_getBindingDelegate */
        auto _getBindingDelegate() noexcept -> GpuProgram* override;

        // All the following methods must delegate to the implementation

        /** @copydoc GpuProgram::isSupported */
        auto isSupported() const noexcept -> bool override;

        auto getSource() const noexcept -> const String& override
        {
            return _getDelegate() ? _getDelegate()->getSource() : BLANKSTRING;
        }

        /** @copydoc GpuProgram::isSkeletalAnimationIncluded */
        auto isSkeletalAnimationIncluded() const noexcept -> bool override;

        auto isMorphAnimationIncluded() const noexcept -> bool override;

        auto isPoseAnimationIncluded() const noexcept -> bool override;
        auto getNumberOfPosesIncluded() const noexcept -> ushort override;

        auto isVertexTextureFetchRequired() const noexcept -> bool override;
        auto getDefaultParameters() noexcept -> const GpuProgramParametersPtr& override;
        auto hasDefaultParameters() const -> bool override;
        auto getPassSurfaceAndLightStates() const noexcept -> bool override;
        auto getPassFogStates() const noexcept -> bool override;
        auto getPassTransformStates() const noexcept -> bool override;
        auto hasCompileError() const noexcept -> bool override;
        void resetCompileError() override;

        void load(bool backgroundThread = false) override;
        void reload(LoadingFlags flags = LF_DEFAULT) override;
        auto isReloadable() const noexcept -> bool;
        auto isLoaded() const noexcept -> bool;
        auto isLoading() const noexcept -> bool;
        auto getLoadingState() const -> LoadingState;
        void unload() override;
        auto getSize() const -> size_t;
        void touch() override;
        auto isBackgroundLoaded() const noexcept -> bool;
        void setBackgroundLoaded(bool bl);
        void escalateLoading() override;
        void addListener(Listener* lis) override;
        void removeListener(Listener* lis) override;

    };

    /** Factory class for Unified programs. */
    class UnifiedHighLevelGpuProgramFactory : public GpuProgramFactory
    {
    public:
        UnifiedHighLevelGpuProgramFactory();
        ~UnifiedHighLevelGpuProgramFactory() override;
        /// Get the name of the language this factory creates programs for
        [[nodiscard]] auto getLanguage() const noexcept -> const String& override;
        auto create(ResourceManager* creator,
            const String& name, ResourceHandle handle,
            const String& group, bool isManual, ManualResourceLoader* loader) -> GpuProgram* override;
    };

    /** @} */
    /** @} */

}

#endif
