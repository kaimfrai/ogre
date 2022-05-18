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
#ifndef OGRE_CORE_COMPOSITORCHAIN_H
#define OGRE_CORE_COMPOSITORCHAIN_H

#include <algorithm>
#include <cstddef>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreCompositorInstance.hpp"
#include "OgreIteratorWrapper.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderQueueListener.hpp"
#include "OgreRenderTargetListener.hpp"
#include "OgreViewport.hpp"

namespace Ogre {
class Camera;
class RenderSystem;
class SceneManager;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
    /** Chain of compositor effects applying to one viewport.
    */
    class CompositorChain : public RenderTargetListener, public Viewport::Listener, public CompositorInstAlloc
    {
    public:
        CompositorChain(Viewport *vp);
        /** Another gcc warning here, which is no problem because RenderTargetListener is never used
            to delete an object.
        */
        ~CompositorChain() override;
        
        /// Data types
        using Instances = std::vector<CompositorInstance *>;
        using InstanceIterator = VectorIterator<Instances>;
        
        enum {
            /// Identifier for best technique.
            BEST = 0,
            /// Identifier for "last" compositor in chain.
            LAST = (size_t)-1,
            NPOS = LAST
        };
        
        /** Apply a compositor. Initially, the filter is enabled.
        @param filter
            Filter to apply.
        @param addPosition
            Position in filter chain to insert this filter at; defaults to the end (last applied filter).
        @param scheme
            Scheme to use (blank means default).
        */
        CompositorInstance* addCompositor(CompositorPtr filter, size_t addPosition=LAST, const String& scheme = BLANKSTRING);

        /** Remove a compositor.
        @param position
            Position in filter chain of filter to remove; defaults to the end (last applied filter)
        */
        void removeCompositor(size_t position=LAST);

        /** Remove all compositors.
        */
        void removeAllCompositors();
        
        /** Get compositor instance by name. Returns null if not found.
        */
        [[nodiscard]] CompositorInstance* getCompositor(const String& name) const;

        /// @overload
        [[nodiscard]] CompositorInstance *getCompositor(size_t index) const { return mInstances.at(index); }

        /// Get compositor position by name. Returns #NPOS if not found.
        [[nodiscard]] size_t getCompositorPosition(const String& name) const;

        /** Get the original scene compositor instance for this chain (internal use). 
        */
        CompositorInstance* _getOriginalSceneCompositor() noexcept { return mOriginalScene; }

        /** The compositor instances. The first compositor in this list is applied first, the last one is applied last.
        */
        [[nodiscard]] const Instances& getCompositorInstances() const noexcept { return mInstances; }
    
        /** Enable or disable a compositor, by position. Disabling a compositor stops it from rendering
            but does not free any resources. This can be more efficient than using removeCompositor and 
            addCompositor in cases the filter is switched on and off a lot.
        @param position
            Position in filter chain of filter
        @param state enabled flag
        */
        void setCompositorEnabled(size_t position, bool state);

        void preRenderTargetUpdate(const RenderTargetEvent& evt) override;
        void postRenderTargetUpdate(const RenderTargetEvent& evt) override;
        void preViewportUpdate(const RenderTargetViewportEvent& evt) override;
        void postViewportUpdate(const RenderTargetViewportEvent& evt) override;
        void viewportCameraChanged(Viewport* viewport) override;
        void viewportDimensionsChanged(Viewport* viewport) override;
        void viewportDestroyed(Viewport* viewport) override;

        /** Mark state as dirty, and to be recompiled next frame.
        */
        void _markDirty();
        
        /** Get viewport that is the target of this chain
        */
        Viewport *getViewport();
        /** Set viewport that is the target of this chain
        */
        void _notifyViewport(Viewport* vp);

        /** Remove a compositor by pointer. This is internally used by CompositionTechnique to
            "weak" remove any instanced of a deleted technique.
        */
        void _removeInstance(CompositorInstance *i);

        /** Internal method for registering a queued operation for deletion later **/
        void _queuedOperation(CompositorInstance::RenderSystemOperation* op);

        /** Compile this Composition chain into a series of RenderTarget operations.
        */
        void _compile();

        /** Get the previous instance in this chain to the one specified. 
        */
        CompositorInstance* getPreviousInstance(CompositorInstance* curr, bool activeOnly = true);
        /** Get the next instance in this chain to the one specified. 
        */
        CompositorInstance* getNextInstance(CompositorInstance* curr, bool activeOnly = true);

    private:
        /// Viewport affected by this CompositorChain
        Viewport *mViewport;
        
        /** Plainly renders the scene; implicit first compositor in the chain.
        */
        CompositorInstance *mOriginalScene;
        
        /// Postfilter instances in this chain
        Instances mInstances;
        
        /// State needs recompile
        bool mDirty;
        /// Any compositors enabled?
        bool mAnyCompositorsEnabled;

        String mOriginalSceneScheme;

        /// Compiled state (updated with _compile)
        CompositorInstance::CompiledState mCompiledState;
        CompositorInstance::TargetOperation mOutputOperation;
        /// Render System operations queued by last compile, these are created by this
        /// instance thus managed and deleted by it. The list is cleared with 
        /// clearCompilationState()
        using RenderSystemOperations = std::vector<CompositorInstance::RenderSystemOperation *>;
        RenderSystemOperations mRenderSystemOperations;

        /** Clear compiled state */
        void clearCompiledState();
        
        /** Prepare a viewport, the camera and the scene for a rendering operation
        */
        void preTargetOperation(CompositorInstance::TargetOperation &op, Viewport *vp, Camera *cam);
        
        /** Restore a viewport, the camera and the scene after a rendering operation
        */
        void postTargetOperation(CompositorInstance::TargetOperation &op, Viewport *vp, Camera *cam);

        void createOriginalScene();
        void destroyOriginalScene();

        /// destroy internal resources
        void destroyResources();
        
        /** Internal method to get a unique name of a compositor
        */
        [[nodiscard]] const String getCompositorName() const;

        /** Render queue listener used to set up rendering events. */
        class RQListener: public RenderQueueListener
        {
        public:
            RQListener() : mOperation(nullptr), mSceneManager(nullptr), mRenderSystem(nullptr), mViewport(nullptr) {}

            void renderQueueStarted(uint8 queueGroupId, const String& invocation, bool& skipThisInvocation) override;
            void renderQueueEnded(uint8 queueGroupId, const String& invocation, bool& repeatThisInvocation) override;

            /** Set current operation and target. */
            void setOperation(CompositorInstance::TargetOperation *op,SceneManager *sm,RenderSystem *rs);

            /** Notify current destination viewport. */
            void notifyViewport(Viewport* vp) { mViewport = vp; }

            /** Flush remaining render system operations. */
            void flushUpTo(uint8 id);
        private:
            CompositorInstance::TargetOperation *mOperation;
            SceneManager *mSceneManager;
            RenderSystem *mRenderSystem;
            Viewport* mViewport;
            CompositorInstance::RenderSystemOpPairs::iterator currentOp, lastOp;
        };
        RQListener mOurListener;
        /// Old viewport settings
        unsigned int mOldClearEveryFrameBuffers;
        /// Store old scene visibility mask
        uint32 mOldVisibilityMask;
        /// Store old find visible objects
        bool mOldFindVisibleObjects;
        /// Store old camera LOD bias
        float mOldLodBias;
        /// Store old viewport material scheme
        String mOldMaterialScheme;
        /// Store old shadows enabled flag
        bool mOldShadowsEnabled;

    };
    /** @} */
    /** @} */
} // namespace Ogre

#endif // OGRE_CORE_COMPOSITORCHAIN_H
