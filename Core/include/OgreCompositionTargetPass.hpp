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
#ifndef OGRE_CORE_COMPOSITIONTARGETPASS_H
#define OGRE_CORE_COMPOSITIONTARGETPASS_H

#include <cstddef>
#include <vector>

#include "OgreCompositionPass.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"

namespace Ogre {
    template <typename T> class VectorIterator;
class CompositionTechnique;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
    /** Object representing one render to a RenderTarget or Viewport in the Ogre Composition
        framework.
     */
    class CompositionTargetPass : public CompositorInstAlloc
    {
    public:
        CompositionTargetPass(CompositionTechnique *parent);
        ~CompositionTargetPass();
        
        /** Input mode of a TargetPass
        */
        enum InputMode
        {
            IM_NONE,        /// No input
            IM_PREVIOUS     /// Output of previous Composition in chain
        };
        using Passes = std::vector<CompositionPass *>;
        using PassIterator = VectorIterator<Passes>;
        
        /** Set input mode of this TargetPass
        */
        void setInputMode(InputMode mode);
        /** Get input mode */
        [[nodiscard]] auto getInputMode() const -> InputMode;
        
        /** Set output local texture name */
        void setOutputName(StringView out);
        /** Get output local texture name */
        [[nodiscard]] auto getOutputName() const -> StringView ;

        /// sets the slice of output texture
        void setOutputSlice(int slice) { mOutputSlice = slice; }
        [[nodiscard]] auto getOutputSlice() const noexcept -> int { return mOutputSlice; }

        /** Set "only initial" flag. This makes that this target pass is only executed initially 
            after the effect has been enabled.
        */
        void setOnlyInitial(bool value);
        /** Get "only initial" flag.
        */
        auto getOnlyInitial() noexcept -> bool;
        
        /** Set the scene visibility mask used by this pass 
        */
        void setVisibilityMask(uint32 mask);
        /** Get the scene visibility mask used by this pass 
        */
        auto getVisibilityMask() noexcept -> uint32;

        /** Set the material scheme used by this target pass.
        @remarks
            Only applicable to targets that render the scene as
            one of their passes.
            @see Technique::setScheme.
        */
        void setMaterialScheme(StringView schemeName);
        /** Get the material scheme used by this target pass.
        @remarks
            Only applicable to targets that render the scene as
            one of their passes.
            @see Technique::setScheme.
        */
        [[nodiscard]] auto getMaterialScheme() const noexcept -> StringView ;
        
        /** Set whether shadows are enabled in this target pass.
        @remarks
            Only applicable to targets that render the scene as
            one of their passes.
        */
        void setShadowsEnabled(bool enabled);
        /** Get whether shadows are enabled in this target pass.
        @remarks
            Only applicable to targets that render the scene as
            one of their passes.
        */
        [[nodiscard]] auto getShadowsEnabled() const noexcept -> bool;
        /** Set the scene LOD bias used by this pass. The default is 1.0,
            everything below that means lower quality, higher means higher quality.
        */
        void setLodBias(float bias);
        /** Get the scene LOD bias used by this pass 
        */
        auto getLodBias() noexcept -> float;

        /** Create a new pass, and return a pointer to it.
        */
        auto createPass(CompositionPass::PassType type = CompositionPass::PT_RENDERQUAD) -> CompositionPass *;
        /** Remove a pass. It will also be destroyed.
        */
        void removePass(size_t idx);
        /** Get a pass.*/
        [[nodiscard]] auto getPass(size_t idx) const -> CompositionPass * { return mPasses.at(idx); }
        /** Get the number of passes.
        */
        [[nodiscard]] auto getNumPasses() const -> size_t { return mPasses.size(); }
        
        /// Get the Passes in this TargetPass
        [[nodiscard]] auto getPasses() const noexcept -> const Passes& {
            return mPasses;
        }

        /** Remove all passes
        */
        void removeAllPasses();
        
        /** Get parent object */
        auto getParent() -> CompositionTechnique *;

        /** Determine if this target pass is supported on the current rendering device. 
         */
        auto _isSupported() -> bool;

    private:
        /// Parent technique
        CompositionTechnique *mParent;
        /// Input mode
        InputMode mInputMode{IM_NONE};
        /// (local) output texture
        String mOutputName;
        /// Passes
        Passes mPasses;
        /// This target pass is only executed initially after the effect
        /// has been enabled.
        bool mOnlyInitial{false};
        /// Visibility mask for this render
        uint32 mVisibilityMask{0xFFFFFFFF};
        /// LOD bias of this render
        float mLodBias{1.0f};
        /// Material scheme name
        String mMaterialScheme;
        /// Shadows option
        bool mShadowsEnabled{true};
        /// Output Slice
        int mOutputSlice{0};
    };

    /** @} */
    /** @} */
}

#endif
