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
#ifndef OGRE_CORE_COMPOSITIONPASS_H
#define OGRE_CORE_COMPOSITIONPASS_H

#include <cstddef>
#include <string>
#include <utility>

#include "OgreColourValue.hpp"
#include "OgreCommon.hpp"
#include "OgreConfig.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderQueue.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreVector.hpp"

namespace Ogre {
class CompositionTargetPass;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
    /** Object representing one pass or operation in a composition sequence. This provides a 
        method to conveniently interleave RenderSystem commands between Render Queues.
     */
    class CompositionPass : public CompositorInstAlloc
    {
    public:
        CompositionPass(CompositionTargetPass *parent);
        ~CompositionPass();
        
        /** Enumeration that enumerates the various composition pass types.
        */
        enum class PassType
        {
            CLEAR,           //!< Clear target to one colour
            STENCIL,         //!< Set stencil operation
            RENDERSCENE,     //!< Render the scene or part of it
            RENDERQUAD,      //!< Render a full screen quad
            RENDERCUSTOM,    //!< Render a custom sequence
            COMPUTE          //!< dispatch a compute shader
        };
        
        /** Set the type of composition pass */
        void setType(PassType type);
        /** Get the type of composition pass */
        [[nodiscard]] auto getType() const -> PassType;
        
        /** Set an identifier for this pass. This identifier can be used to
            "listen in" on this pass with an CompositorInstance::Listener. 
        */
        void setIdentifier(uint32 id);
        /** Get the identifier for this pass */
        [[nodiscard]] auto getIdentifier() const noexcept -> uint32;

        /** Set the material used by this pass
            @note applies when PassType is RENDERQUAD 
        */
        void setMaterial(const MaterialPtr& mat);
        /** Set the material used by this pass 
            @note applies when PassType is RENDERQUAD 
        */
        void setMaterialName(std::string_view name);
        /** Get the material used by this pass 
            @note applies when PassType is RENDERQUAD 
        */
        [[nodiscard]] auto getMaterial() const noexcept -> const MaterialPtr&;
        /** Set the first render queue to be rendered in this pass (inclusive) 
            @note applies when PassType is RENDERSCENE
        */
        void setFirstRenderQueue(RenderQueueGroupID id);
        /** Get the first render queue to be rendered in this pass (inclusive) 
            @note applies when PassType is RENDERSCENE
        */
        [[nodiscard]] auto getFirstRenderQueue() const noexcept -> RenderQueueGroupID;
        /** Set the last render queue to be rendered in this pass (inclusive) 
            @note applies when PassType is RENDERSCENE
        */
        void setLastRenderQueue(RenderQueueGroupID id);
        /** Get the last render queue to be rendered in this pass (inclusive) 
            @note applies when PassType is RENDERSCENE
        */
        [[nodiscard]] auto getLastRenderQueue() const noexcept -> RenderQueueGroupID;

        /** Set the material scheme used by this pass.
        @remarks
            Only applicable to passes that render the scene.
            @see Technique::setScheme.
        */
        void setMaterialScheme(std::string_view schemeName);
        /** Get the material scheme used by this pass.
        @remarks
            Only applicable to passes that render the scene.
            @see Technique::setScheme.
        */
        [[nodiscard]] auto getMaterialScheme() const noexcept -> std::string_view ;

        /** Would be nice to have for RENDERSCENE:
            flags to:
                exclude transparents
                override material (at least -- color)
        */

        /** Set the viewport clear buffers  (defaults to FrameBufferType::COLOUR|FrameBufferType::DEPTH)
            @param val is a combination of FrameBufferType::COLOUR, FrameBufferType::DEPTH, FrameBufferType::STENCIL.
            @note applies when PassType is CLEAR
        */
        void setClearBuffers(FrameBufferType val);
        /** Get the viewport clear buffers.
            @note applies when PassType is CLEAR
        */
        [[nodiscard]] auto getClearBuffers() const noexcept -> FrameBufferType;
        /** Set the viewport clear colour (defaults to 0,0,0,0) 
            @note applies when PassType is CLEAR
         */
        void setClearColour(const ColourValue &val);
        /** Get the viewport clear colour (defaults to 0,0,0,0) 
            @note applies when PassType is CLEAR
         */
        [[nodiscard]] auto getClearColour() const -> const ColourValue &;
        /** Set the clear colour to be the background colour of the original viewport
        @note applies when PassType is CLEAR
        */
        void setAutomaticColour(bool val);
        /** Retrieves if the clear colour is automatically setted to the background colour of the original viewport
        @note applies when PassType is CLEAR
        */
        [[nodiscard]] auto getAutomaticColour() const noexcept -> bool;
        /** Set the viewport clear depth (defaults to 1.0) 
            @note applies when PassType is CLEAR
        */
        void setClearDepth(float depth);
        /** Get the viewport clear depth (defaults to 1.0) 
            @note applies when PassType is CLEAR
        */
        [[nodiscard]] auto getClearDepth() const noexcept -> float;
        /** Set the viewport clear stencil value (defaults to 0) 
            @note applies when PassType is CLEAR
        */
        void setClearStencil(uint16 value);
        /** Get the viewport clear stencil value (defaults to 0) 
            @note applies when PassType is CLEAR
        */
        [[nodiscard]] auto getClearStencil() const noexcept -> uint16;

        /** Set stencil check on or off.
            @note applies when PassType is STENCIL
        */
        void setStencilCheck(bool value);
        /** Get stencil check enable.
            @note applies when PassType is STENCIL
        */
        [[nodiscard]] auto getStencilCheck() const noexcept -> bool;
        /** Set stencil compare function.
            @note applies when PassType is STENCIL
        */
        void setStencilFunc(CompareFunction value); 
        /** Get stencil compare function.
            @note applies when PassType is STENCIL
        */
        [[nodiscard]] auto getStencilFunc() const -> CompareFunction; 
        /** Set stencil reference value.
            @note applies when PassType is STENCIL
        */
        void setStencilRefValue(uint32 value);
        /** Get stencil reference value.
            @note applies when PassType is STENCIL
        */
        [[nodiscard]] auto getStencilRefValue() const noexcept -> uint32;
        /** Set stencil mask.
            @note applies when PassType is STENCIL
        */
        void setStencilMask(uint32 value);
        /** Get stencil mask.
            @note applies when PassType is STENCIL
        */
        [[nodiscard]] auto getStencilMask() const noexcept -> uint32;
        /** Set stencil fail operation.
            @note applies when PassType is STENCIL
        */
        void setStencilFailOp(StencilOperation value);
        /** Get stencil fail operation.
            @note applies when PassType is STENCIL
        */
        [[nodiscard]] auto getStencilFailOp() const -> StencilOperation;
        /** Set stencil depth fail operation.
            @note applies when PassType is STENCIL
        */
        void setStencilDepthFailOp(StencilOperation value);
        /** Get stencil depth fail operation.
            @note applies when PassType is STENCIL
        */
        [[nodiscard]] auto getStencilDepthFailOp() const -> StencilOperation;
        /** Set stencil pass operation.
            @note applies when PassType is STENCIL
        */
        void setStencilPassOp(StencilOperation value);
        /** Get stencil pass operation.
            @note applies when PassType is STENCIL
        */
        [[nodiscard]] auto getStencilPassOp() const -> StencilOperation;
        /** Set two sided stencil operation.
            @note applies when PassType is STENCIL
        */
        void setStencilTwoSidedOperation(bool value);
        /** Get two sided stencil operation.
            @note applies when PassType is STENCIL
        */
        [[nodiscard]] auto getStencilTwoSidedOperation() const noexcept -> bool;

        [[nodiscard]] auto getStencilState() const noexcept -> const StencilState& { return mStencilState; }

        /// Inputs (for material used for rendering the quad)
        struct InputTex
        {
            /// Name (local) of the input texture (empty == no input)
            std::string_view name;
            /// MRT surface index if applicable
            size_t mrtIndex{0};
        };

        /** Set an input local texture. An empty string clears the input.
            @param id    Input to set. Must be in 0..OGRE_MAX_TEXTURE_LAYERS-1
            @param input Which texture to bind to this input. An empty string clears the input.
            @param mrtIndex Which surface of an MRT to retrieve
            @note applies when PassType is RENDERQUAD 
        */
        void setInput(size_t id, std::string_view input=BLANKSTRING, size_t mrtIndex=0);
        
        /** Get the value of an input.
            @param id    Input to get. Must be in 0..OGRE_MAX_TEXTURE_LAYERS-1.
            @note applies when PassType is RENDERQUAD 
        */
        [[nodiscard]] auto getInput(size_t id) const -> const InputTex &;
        
        /** Get the number of inputs used.
            @note applies when PassType is RENDERQUAD 
        */
        [[nodiscard]] auto getNumInputs() const -> size_t;
        
        /** Clear all inputs.
            @note applies when PassType is RENDERQUAD 
        */
        void clearAllInputs();
        
        /** Get parent object 
            @note applies when PassType is RENDERQUAD 
        */
        auto getParent() -> CompositionTargetPass *;

        /** Determine if this target pass is supported on the current rendering device. 
         */
        auto _isSupported() -> bool;

        /** Set quad normalised positions [-1;1]x[-1;1]
            @note applies when PassType is RENDERQUAD
         */
        void setQuadCorners(const FloatRect& quad) { mQuad.rect = quad; mQuad.cornerModified = true; }

        /** Get quad normalised positions [-1;1]x[-1;1]
            @note applies when PassType is RENDERQUAD 
         */
        auto getQuadCorners(FloatRect& quad) const -> bool { quad = mQuad.rect; return mQuad.cornerModified; }
            
        /** Sets the use of camera frustum far corners provided in the quad's normals
            @note applies when PassType is RENDERQUAD 
        */
        void setQuadFarCorners(bool farCorners, bool farCornersViewSpace);

        /** Returns true if camera frustum far corners are provided in the quad.
            @note applies when PassType is RENDERQUAD 
        */
        [[nodiscard]] auto getQuadFarCorners() const noexcept -> bool;

        /** Returns true if the far corners provided in the quad are in view space
            @note applies when PassType is RENDERQUAD 
        */
        [[nodiscard]] auto getQuadFarCornersViewSpace() const noexcept -> bool;

        /** Set the type name of this custom composition pass.
            @note applies when PassType is RENDERCUSTOM
            @see CompositorManager::registerCustomCompositionPass
        */
        void setCustomType(std::string_view customType);

        /** Get the type name of this custom composition pass.
            @note applies when PassType is RENDERCUSTOM
            @see CompositorManager::registerCustomCompositionPass
        */
        [[nodiscard]] auto getCustomType() const noexcept -> std::string_view ;

        void setThreadGroups(const Vector3i& g) { mThreadGroups = g; }
        [[nodiscard]] auto getThreadGroups() const noexcept -> const Vector3i& { return mThreadGroups; }

        void setCameraName(std::string_view name) { mRenderScene.cameraName = name; }
        [[nodiscard]] auto getCameraName() const noexcept -> std::string_view { return mRenderScene.cameraName; }

        void setAlignCameraToFace(bool val) { mRenderScene.alignCameraToFace = val; }
        [[nodiscard]] auto getAlignCameraToFace() const noexcept -> bool { return mRenderScene.alignCameraToFace; }
    private:
        /// Parent technique
        CompositionTargetPass *mParent;
        /// Type of composition pass
        PassType mType{PassType::RENDERQUAD};

        // in case of PassType::RENDERQUAD, PassType::COMPUTE, PassType::CUSTOM
        struct MaterialData
        {
            /// Identifier for this pass
            uint32 identifier{0};
            /// Material used for rendering
            MaterialPtr material;
            /** Inputs (for material used for rendering the quad).
                An empty string signifies that no input is used */
            InputTex inputs[OGRE_MAX_TEXTURE_LAYERS];

        } mMaterial;

        // in case of PassType::RENDERSCENE
        struct RenderSceneData
        {
            /// [first,last] render queue to render this pass (in case of PassType::RENDERSCENE)
            RenderQueueGroupID firstRenderQueue{RenderQueueGroupID::BACKGROUND};
            RenderQueueGroupID lastRenderQueue{RenderQueueGroupID::SKIES_LATE};

            /// Material scheme name
            std::string_view materialScheme;

            /// name of camera to use instead of default
            std::string_view cameraName;
            bool alignCameraToFace{false};

        } mRenderScene;

        // in case of PassType::CLEAR
        struct ClearData
        {
            /// Clear buffers
            FrameBufferType buffers{FrameBufferType::COLOUR | FrameBufferType::DEPTH};
            /// Clear colour
            ColourValue colour{ColourValue::ZERO};
            /// Clear colour with the colour of the original viewport. Overrides mClearColour
            bool automaticColour{false};
            /// Clear depth
            float depth{1.0f};
            /// Clear stencil value
            uint16 stencil{0};

        } mClear;

        /// in case of PassType::COMPUTE
        Vector3i mThreadGroups;

        /// in case of PassType::STENCIL
        StencilState mStencilState;

        // in case of PassType::RENDERQUAD
        struct QuadData
        {
            /// True if quad should not cover whole screen
            bool cornerModified{false};
            /// quad positions in normalised coordinates [-1;1]x[-1;1]
            FloatRect rect{-1, 1, 1, -1};
            bool farCorners{false}, farCornersViewSpace{false};

        } mQuad;

        /// in case of PassType::RENDERCUSTOM
        String mCustomType;
    };
    /** @} */
    /** @} */

}

#endif
