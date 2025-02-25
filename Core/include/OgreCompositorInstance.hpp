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

export module Ogre.Core:CompositorInstance;

export import :Common;
export import :CompositionTechnique;
export import :DepthBuffer;
export import :MaterialManager;
export import :MemoryAllocatorConfig;
export import :Platform;
export import :Prerequisites;
export import :RenderQueue;
export import :SharedPtr;

export import <algorithm>;
export import <bitset>;
export import <map>;
export import <string>;
export import <utility>;
export import <vector>;

export
namespace Ogre {
class Camera;
class CompositionTargetPass;
class Compositor;
class CompositorChain;
class MultiRenderTarget;
class RenderSystem;
class RenderTarget;
class SceneManager;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
            
    /** An instance of a Compositor object for one Viewport. It is part of the CompositorChain
        for a Viewport.
    */
    class CompositorInstance : public CompositorInstAlloc
    {
    public:
        CompositorInstance(CompositionTechnique *technique, CompositorChain *chain);
        virtual ~CompositorInstance();
        /** Provides an interface to "listen in" to to render system operations executed by this 
            CompositorInstance.
        */
        class Listener
        {
        public:
            virtual ~Listener();

            /** Notification of when a render target operation involving a material (like
                rendering a quad) is compiled, so that miscellaneous parameters that are different
                per Compositor instance can be set up.
            @param pass_id
                Pass identifier within Compositor instance, this is specified 
                by the user by CompositionPass::setIdentifier().
            @param mat
                Material, this may be changed at will and will only affect
                the current instance of the Compositor, not the global material
                it was cloned from.
             */
            virtual void notifyMaterialSetup(uint32 pass_id, MaterialPtr &mat);

            /** Notification before a render target operation involving a material (like
                rendering a quad), so that material parameters can be varied.
            @param pass_id
                Pass identifier within Compositor instance, this is specified 
                by the user by CompositionPass::setIdentifier().
            @param mat
                Material, this may be changed at will and will only affect
                the current instance of the Compositor, not the global material
                it was cloned from.
             */
            virtual void notifyMaterialRender(uint32 pass_id, MaterialPtr &mat);

            /** Notification after resources have been created (or recreated).
            @param forResizeOnly
                Was the creation because the viewport was resized?
            */
            virtual void notifyResourcesCreated(bool forResizeOnly);

            /** Notification before resources have been destructed.
              @param forResizeOnly Was the creation because the viewport was resized?
             */
            virtual void notifyResourcesReleased(bool forResizeOnly);
        };
        /** Specific render system operation. A render target operation does special operations
            between render queues like rendering a quad, clearing the frame buffer or 
            setting stencil state.
        */
        class RenderSystemOperation : public CompositorInstAlloc
        {
        public:
            virtual ~RenderSystemOperation();
            /// Set state to SceneManager and RenderSystem
            virtual void execute(SceneManager *sm, RenderSystem *rs) = 0;
        };
        using RenderSystemOpPair = std::pair<Ogre::RenderQueueGroupID, RenderSystemOperation *>;
        using RenderSystemOpPairs = std::vector<RenderSystemOpPair>;
        /** Operation setup for a RenderTarget (collected).
        */
        class TargetOperation
        {
        public:
            TargetOperation()
            = default;
            TargetOperation(RenderTarget* inTarget)
                : target(inTarget), currentQueueGroupID{0}, visibilityMask{0xFFFFFFFF}, lodBias(1.0f),
                  onlyInitial(false), hasBeenRendered(false), findVisibleObjects(false),
                  materialScheme(MaterialManager::DEFAULT_SCHEME_NAME), shadowsEnabled(true),
                  alignCameraToFace(-1)
            {
            }
            /// Target
            RenderTarget *target;

            /// Current group ID
            RenderQueueGroupID currentQueueGroupID;

            /// RenderSystem operations to queue into the scene manager, by
            /// uint8
            RenderSystemOpPairs renderSystemOperations;

            /// Scene visibility mask
            /// If this is 0, the scene is not rendered at all
            QueryTypeMask visibilityMask;
            
            /// LOD offset. This is multiplied with the camera LOD offset
            /// 1.0 is default, lower means lower detail, higher means higher detail
            float lodBias;
            
            /** A set of render queues to either include or exclude certain render queues.
            */
            using RenderQueueBitSet = std::bitset<std::to_underlying(RenderQueueGroupID::COUNT)>;

            /// Which renderqueues to render from scene
            RenderQueueBitSet renderQueues;
            
            /** @see CompositionTargetPass::mOnlyInitial
            */
            bool onlyInitial;
            /** "Has been rendered" flag; used in combination with
                onlyInitial to determine whether to skip this target operation.
            */
            bool hasBeenRendered;
            /** Whether this op needs to find visible scene objects or not 
            */
            bool findVisibleObjects;
            /** Which material scheme this op will use */
            String materialScheme;
            /** Whether shadows will be enabled */
            bool shadowsEnabled;

            String cameraOverride;
            int alignCameraToFace;
        };
        using CompiledState = std::vector<TargetOperation>;
        
        /** Set enabled flag. The compositor instance will only render if it is
            enabled, otherwise it is pass-through. Resources are only created if
            they weren't alive when enabling.
        */
        void setEnabled(bool value);
        
        /** Get enabled flag.
        */
        [[nodiscard]] auto getEnabled() const noexcept -> bool { return mEnabled; }

        /** Set alive/active flag. The compositor instance will create resources when alive,
            and destroy them when inactive.
        @remarks
            Killing an instance means also disabling it: setAlive(false) implies
            setEnabled(false)
        */
        void setAlive(bool value);

        /** Get alive flag.
        */
        [[nodiscard]] auto getAlive() const noexcept -> bool { return mAlive; }

        /** Get the instance name for a local texture.
        @note It is only valid to call this when local textures have been loaded, 
            which in practice means that the compositor instance is active. Calling
            it at other times will cause an exception. Note that since textures
            are cleaned up aggressively, this name is not guaranteed to stay the
            same if you disable and re-enable the compositor instance.
        @param name
            The name of the texture in the original compositor definition.
        @param mrtIndex
            If name identifies a MRT, which texture attachment to retrieve.
        @return
            The instance name for the texture, corresponds to a real texture.
        */
        auto getTextureInstanceName(std::string_view name, size_t mrtIndex) -> std::string_view ;

        /** Get the instance of a local texture.
        @note Textures are only valid when local textures have been loaded, 
            which in practice means that the compositor instance is active. Calling
            this method at other times will return null pointers. Note that since textures
            are cleaned up aggressively, this pointer is not guaranteed to stay the
            same if you disable and re-enable the compositor instance.
        @param name
            The name of the texture in the original compositor definition.
        @param mrtIndex
            If name identifies a MRT, which texture attachment to retrieve.
        @return
            The texture pointer, corresponds to a real texture.
        */
        auto getTextureInstance(std::string_view name, size_t mrtIndex) -> const TexturePtr&;

        /** Get the render target for a given render texture name. 
        @remarks
            You can use this to add listeners etc, but do not use it to update the
            targets manually or any other modifications, the compositor instance 
            is in charge of this.
        */
        auto getRenderTarget(std::string_view name, int slice = 0) -> RenderTarget*;

       
        /** Recursively collect target states (except for final Pass).
        @param compiledState
            This vector will contain a list of TargetOperation objects.
        */
        virtual void _compileTargetOperations(CompiledState &compiledState);
        
        /** Compile the final (output) operation. This is done separately because this
            is combined with the input in chained filters.
        */
        virtual void _compileOutputOperation(TargetOperation &finalState);
        
        /** Get Compositor of which this is an instance
        */
        [[nodiscard]] auto getCompositor() const noexcept -> Compositor * { return mCompositor; }
        
        /** Get CompositionTechnique used by this instance
        */
        [[nodiscard]] auto getTechnique() const noexcept -> CompositionTechnique * { return mTechnique; }

        /** Change the technique we're using to render this compositor. 
        @param tech
            The technique to use (must be supported and from the same Compositor)
        @param reuseTextures
            If textures have already been created for the current
            technique, whether to try to re-use them if sizes & formats match.
        */
        void setTechnique(CompositionTechnique* tech, bool reuseTextures = true);

        /** Pick a technique to use to render this compositor based on a scheme. 
        @remarks
            If there is no specific supported technique with this scheme name, 
            then the first supported technique with no specific scheme will be used.
        @see CompositionTechnique::setSchemeName
        @param schemeName
            The scheme to use 
        @param reuseTextures
            If textures have already been created for the current
            technique, whether to try to re-use them if sizes & formats match.
            Note that for this feature to be of benefit, the textures must have been created
            with the 'pooled' option enabled.
        */
        void setScheme(std::string_view schemeName, bool reuseTextures = true);

        /// Returns the name of the scheme this compositor is using.
        [[nodiscard]] auto getScheme() const noexcept -> std::string_view { return mTechnique ? mTechnique->getSchemeName() : BLANKSTRING; }

        /** Notify this instance that the primary surface has been resized. 
        @remarks
            This will allow the instance to recreate its resources that 
            are dependent on the size. 
        */
        void notifyResized();

        /** Get Chain that this instance is part of
        */
        auto getChain() -> CompositorChain *;

        /** Add a listener. Listeners provide an interface to "listen in" to to render system 
            operations executed by this CompositorInstance so that materials can be 
            programmatically set up.
        @see CompositorInstance::Listener
        */
        void addListener(Listener *l);

        /** Remove a listener.
        @see CompositorInstance::Listener
        */
        void removeListener(Listener *l);

        /** Notify listeners of a material compilation.
        */
        void _fireNotifyMaterialSetup(uint32 pass_id, MaterialPtr &mat);

        /** Notify listeners of a material render.
        */
        void _fireNotifyMaterialRender(uint32 pass_id, MaterialPtr &mat);

        /** Notify listeners of a material render.
        */
        void _fireNotifyResourcesCreated(bool forResizeOnly);
        
        /** Notify listeners ressources
        */
        void _fireNotifyResourcesReleased(bool forResizeOnly);
    private:
        /// Compositor of which this is an instance.
        Compositor *mCompositor;
        /// Composition technique used by this instance.
        CompositionTechnique *mTechnique;
        /// Composition chain of which this instance is part.
        CompositorChain *mChain;
        /// Is this instance enabled?
        bool mEnabled{false};
        /// Is this instance allocating resources?
        bool mAlive{false};
        /// Map from name->local texture.
        using LocalTextureMap = std::map<std::string_view, TexturePtr>;
        LocalTextureMap mLocalTextures;
        /// Store a list of MRTs we've created.
        using LocalMRTMap = std::map<std::string_view, MultiRenderTarget *>;
        LocalMRTMap mLocalMRTs;
        using ReserveTextureMap = std::map<CompositionTechnique::TextureDefinition *, TexturePtr>;
        /** Textures that are not currently in use, but that we want to keep for now,
            for example if we switch techniques but want to keep all textures available
            in case we switch back. 
        */
        ReserveTextureMap mReserveTextures;

        /// Vector of listeners.
        using Listeners = std::vector<Listener *>;
        Listeners mListeners;
        
        /// Previous instance (set by chain).
        CompositorInstance *mPreviousInstance;
        
        /** Collect rendering passes. Here, passes are converted into render target operations
            and queued with queueRenderSystemOp.
        */
        virtual void collectPasses(TargetOperation &finalState, const CompositionTargetPass *target);
        
        /** Create a local dummy material with one technique but no passes.
            The material is detached from the Material Manager to make sure it is destroyed
            when going out of scope.
        */
        auto createLocalMaterial(std::string_view srcName) -> MaterialPtr;
        
        /** Create local rendertextures and other resources. Builds mLocalTextures.
        */
        void createResources(bool forResizeOnly);

        void setupRenderTarget(RenderTarget* target, DepthBuffer::PoolId depthBufferId);
        
        /** Destroy local rendertextures and other resources.
        */
        void freeResources(bool forResizeOnly, bool clearReserveTextures);

        auto
        resolveTexReference(const CompositionTechnique::TextureDefinition* texDef) -> CompositionTechnique::TextureDefinition*;

        /** Get RenderTarget for a named local texture.
        */
        auto getTargetForTex(std::string_view name, int slice) -> RenderTarget *;
        
        /** Get source texture name for a named local texture.
        @param name
            The local name of the texture as given to it in the compositor.
        @param mrtIndex
            For MRTs, which attached surface to retrieve.
        */
        auto getSourceForTex(std::string_view name, size_t mrtIndex = 0) -> const TexturePtr &;

        /** Queue a render system operation.
        */
        void queueRenderSystemOp(TargetOperation &finalState, RenderSystemOperation *op);

        /// Util method for assigning a local texture name to a MRT attachment
        static auto getMRTTexLocalName(std::string_view baseName, size_t attachment) -> String;

        /** Search for options like AA and hardware gamma which we may want to 
            inherit from the main render target to which we're attached. 
        */
        void deriveTextureRenderTargetOptions(std::string_view texname, 
            bool *hwGammaWrite, uint *fsaa, String* fsaaHint);

        /// Notify this instance that the primary viewport's camera has changed.
        void notifyCameraChanged(Camera* camera);

        friend class CompositorChain;
        friend class Compositor;
    };
    /** @} */
    /** @} */

} // namespace Ogre
