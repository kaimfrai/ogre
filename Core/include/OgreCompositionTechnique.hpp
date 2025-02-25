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

export module Ogre.Core:CompositionTechnique;

export import :CompositionTargetPass;
export import :DepthBuffer;
export import :MemoryAllocatorConfig;
export import :PixelFormat;
export import :Platform;
export import :Prerequisites;
export import :Texture;

export import <memory>;
export import <vector>;

export
namespace Ogre {
    template <typename T> class VectorIterator;
    class Compositor;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
    /** Base composition technique, can be subclassed in plugins.
     */
    class CompositionTechnique : public CompositorInstAlloc
    {
    public:
        CompositionTechnique(Compositor *parent);
        virtual ~CompositionTechnique();
    
        //The scope of a texture defined by the compositor
        enum class TextureScope { 
            //Local texture - only available to the compositor passes in this technique
            LOCAL,
            //Chain texture - available to the other compositors in the chain
            CHAIN,
            //Global texture - available to everyone in every scope
            GLOBAL
        };

        /// Local texture definition
        class TextureDefinition : public CompositorInstAlloc
        {
        public:
            String name;
            //Texture definition being a reference is determined by these two fields not being empty.
            String refCompName; //If a reference, the name of the compositor being referenced
            String refTexName;  //If a reference, the name of the texture in the compositor being referenced
            uint32 width{0};       // 0 means adapt to target width
            uint32 height{0};      // 0 means adapt to target height
            TextureType type{TextureType::_2D};   // either 2d or cubic
            float widthFactor{1.0f};  // multiple of target width to use (if width = 0)
            float heightFactor{1.0f}; // multiple of target height to use (if height = 0)
            PixelFormatList formatList; // more than one means MRT
            bool fsaa{true};          // FSAA enabled; true = determine from main target (if render_scene), false = disable
            bool hwGammaWrite{false};  // Do sRGB gamma correction on write (only 8-bit per channel formats) 
            DepthBuffer::PoolId depthBufferId{1};//Depth Buffer's pool ID. (unrelated to "pool" variable below)
            bool pooled{false};        // whether to use pooled textures for this one
            TextureScope scope{TextureScope::LOCAL}; // Which scope has access to this texture

            TextureDefinition()  = default;
        };
        /// Typedefs for several iterators
        using TargetPasses = std::vector<CompositionTargetPass *>;
        using TargetPassIterator = VectorIterator<TargetPasses>;
        using TextureDefinitions = std::vector<TextureDefinition *>;
        using TextureDefinitionIterator = VectorIterator<TextureDefinitions>;
        
        /** Create a new local texture definition, and return a pointer to it.
            @param name     Name of the local texture
        */
        auto createTextureDefinition(std::string_view name) -> TextureDefinition *;
        
        /** Remove and destroy a local texture definition.
        */
        void removeTextureDefinition(size_t idx);
        
        /** Get a local texture definition.
        */
        [[nodiscard]] auto getTextureDefinition(size_t idx) const -> TextureDefinition * { return mTextureDefinitions.at(idx); }
        
        /** Get a local texture definition with a specific name.
        */
        [[nodiscard]] auto getTextureDefinition(std::string_view name) const -> TextureDefinition *;

        /** Get the number of local texture definitions.*/
        [[nodiscard]] auto getNumTextureDefinitions() const -> size_t { return mTextureDefinitions.size(); }
        
        /** Remove all Texture Definitions
        */
        void removeAllTextureDefinitions();
        
        /** Get the TextureDefinitions in this Technique. */
        [[nodiscard]] auto getTextureDefinitions() const noexcept -> const TextureDefinitions& { return mTextureDefinitions; }
        
        /** Create a new target pass, and return a pointer to it.
        */
        auto createTargetPass() -> CompositionTargetPass *;
        
        /** Remove a target pass. It will also be destroyed.
        */
        void removeTargetPass(size_t idx);
        
        /** Get a target pass.
        */
        [[nodiscard]] auto getTargetPass(size_t idx) const -> CompositionTargetPass* { return mTargetPasses.at(idx); }
        
        /** Get the number of target passes. */
        [[nodiscard]] auto getNumTargetPasses() const -> size_t { return mTargetPasses.size(); }
        
        /** Remove all target passes.
        */
        void removeAllTargetPasses();
        
        /** Get the TargetPasses in this Technique. */
        [[nodiscard]] auto getTargetPasses() const noexcept -> const TargetPasses& { return mTargetPasses; }
        
        /** Get output (final) target pass
         */
        [[nodiscard]] auto getOutputTargetPass() const noexcept -> CompositionTargetPass * { return mOutputTarget.get(); }
        
        /** Determine if this technique is supported on the current rendering device. 
        @param allowTextureDegradation True to accept a reduction in texture depth
         */
        virtual auto isSupported(bool allowTextureDegradation) -> bool;
        
        /** Assign a scheme name to this technique, used to switch between 
            multiple techniques by choice rather than for hardware compatibility.
        */
        virtual void setSchemeName(std::string_view schemeName);
        /** Get the scheme name assigned to this technique. */
        [[nodiscard]] auto getSchemeName() const noexcept -> std::string_view { return mSchemeName; }
        
        /** Set the name of the compositor logic assigned to this technique.
            Instances of this technique will be auto-coupled with the matching logic.
        */
        void setCompositorLogicName(std::string_view compositorLogicName) 
            { mCompositorLogicName = compositorLogicName; }
        /** Get the compositor logic name assigned to this technique */
        [[nodiscard]] auto getCompositorLogicName() const noexcept -> std::string_view { return mCompositorLogicName; }

        /** Get parent object */
        auto getParent() -> Compositor *;
    private:
        /// Parent compositor
        Compositor *mParent;
        /// Local texture definitions
        TextureDefinitions mTextureDefinitions;
        
        /// Intermediate target passes
        TargetPasses mTargetPasses;
        /// Output target pass (can be only one)
        ::std::unique_ptr<CompositionTargetPass> mOutputTarget;

        /// Optional scheme name
        String mSchemeName;
        
        /// Optional compositor logic name
        String mCompositorLogicName;

    };
    /** @} */
    /** @} */

}
