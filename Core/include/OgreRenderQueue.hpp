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
export module Ogre.Core:RenderQueue;

export import :MemoryAllocatorConfig;
export import :Platform;
export import :Prerequisites;

export import <memory>;

export
namespace Ogre {

    class Camera;
    class MovableObject;
    struct VisibleObjectsBoundsInfo;
class RenderQueueGroup;
class Renderable;
class Technique;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */
    /** Enumeration of queue groups, by which the application may group queued renderables
        so that they are rendered together with events in between
    @remarks
        When passed into methods these are actually passed as a uint8 to allow you
        to use values in between if you want to.
    */
    enum class RenderQueueGroupID : uint8
    {
        /// Use this queue for objects which must be rendered first e.g. backgrounds
        BACKGROUND = 0,
        /// First queue (after backgrounds), used for skyboxes if rendered first
        SKIES_EARLY = 5,
        _1 = 10,
        _2 = 20,
        WORLD_GEOMETRY_1 = 25,
        _3 = 30,
        _4 = 40,
        /// The default render queue
        MAIN = 50,
        _6 = 60,
        _7 = 70,
        WORLD_GEOMETRY_2 = 75,
        _8 = 80,
        _9 = 90,
        /// Penultimate queue(before overlays), used for skyboxes if rendered last
        SKIES_LATE = 95,
        /// Use this queue for objects which must be rendered last e.g. overlays
        OVERLAY = 100,
        /// Final possible render queue, don't exceed this
        MAX = 105,
        COUNT
    };

    auto constexpr operator + (RenderQueueGroupID id, decltype(0z) add)
    {
        return static_cast<RenderQueueGroupID>
        (   std::to_underlying(id)
        +   add
        );
    }

    auto constexpr operator - (RenderQueueGroupID id, decltype(0z) subtract)
    {
        return static_cast<RenderQueueGroupID>
        (   std::to_underlying(id)
        -   subtract
        );
    }

    /** Class to manage the scene object rendering queue.
        @remarks
            Objects are grouped by material to minimise rendering state changes. The map from
            material to renderable object is wrapped in a class for ease of use.
        @par
            This class now includes the concept of 'queue groups' which allows the application
            adding the renderable to specifically schedule it so that it is included in 
            a discrete group. Good for separating renderables into the main scene,
            backgrounds and overlays, and also could be used in the future for more
            complex multipass routines like stenciling.
    */
    class RenderQueue : public RenderQueueAlloc
    {
    public:

        using RenderQueueGroupMap = std::unique_ptr<RenderQueueGroup>[std::to_underlying(RenderQueueGroupID::COUNT)];

        /** Class to listen in on items being added to the render queue. 
        @remarks
            Use RenderQueue::setRenderableListener to get callbacks when an item
            is added to the render queue.
        */
        class RenderableListener
        {
        public:
            RenderableListener() = default;
            virtual ~RenderableListener() = default;

            /** Method called when a Renderable is added to the queue.
            @remarks
                You can use this event hook to alter the Technique used to
                render a Renderable as the item is added to the queue. This is
                a low-level way to override the material settings for a given
                Renderable on the fly.
            @param rend The Renderable being added to the queue
            @param groupID The render queue group this Renderable is being added to
            @param priority The priority the Renderable has been given
            @param ppTech A pointer to the pointer to the Technique that is 
                intended to be used; you can alter this to an alternate Technique
                if you so wish (the Technique doesn't have to be from the same
                Material either).
            @param pQueue Pointer to the render queue that this object is being
                added to. You can for example call this back to duplicate the 
                object with a different technique
            @return true to allow the Renderable to be added to the queue, 
                false if you want to prevent it being added
            */
            virtual auto renderableQueued(Renderable* rend, RenderQueueGroupID groupID,
                ushort priority, Technique** ppTech, RenderQueue* pQueue) -> bool = 0;
        };
    private:
        RenderQueueGroupMap mGroups;
        /// The current default queue group
        RenderQueueGroupID mDefaultQueueGroup;
        /// The default priority
        ushort mDefaultRenderablePriority;

        bool mSplitPassesByLightingType{false};
        bool mSplitNoShadowPasses{false};
        bool mShadowCastersCannotBeReceivers{false};

        RenderableListener* mRenderableListener{nullptr};
    public:
        RenderQueue();
        virtual ~RenderQueue();

        /** Empty the queue - should only be called by SceneManagers.
        @param destroyPassMaps Set to true to destroy all pass maps so that
            the queue is completely clean (useful when switching scene managers)
        */
        void clear(bool destroyPassMaps = false);

        /** Get a render queue group.
        @remarks
            OGRE registers new queue groups as they are requested, 
            therefore this method will always return a valid group.
        */
        auto getQueueGroup(RenderQueueGroupID qid) -> RenderQueueGroup*;

        /** Add a renderable object to the queue.
        @remarks
            This methods adds a Renderable to the queue, which will be rendered later by 
            the SceneManager. This is the advanced version of the call which allows the renderable
            to be added to any queue.
        @note
            Called by implementation of MovableObject::_updateRenderQueue.
        @param
            pRend Pointer to the Renderable to be added to the queue
        @param
            groupID The group the renderable is to be added to. This
            can be used to schedule renderable objects in separate groups such that the SceneManager
            respects the divisions between the groupings and does not reorder them outside these
            boundaries. This can be handy for overlays where no matter what you want the overlay to 
            be rendered last.
        @param
            priority Controls the priority of the renderable within the queue group. If this number
            is raised, the renderable will be rendered later in the group compared to it's peers.
            Don't use this unless you really need to, manually ordering renderables prevents OGRE
            from sorting them for best efficiency. However this could be useful for ordering 2D
            elements manually for example.
        */
        void addRenderable(Renderable* pRend, RenderQueueGroupID groupID, ushort priority);

        /** Add a renderable object to the queue.
        @remarks
            This methods adds a Renderable to the queue, which will be rendered later by 
            the SceneManager. This is the simplified version of the call which does not 
            require a priority to be specified. The queue priority is take from the
            current default (see setDefaultRenderablePriority).
        @note
            Called by implementation of MovableObject::_updateRenderQueue.
        @param pRend
            Pointer to the Renderable to be added to the queue
        @param groupId
            The group the renderable is to be added to. This
            can be used to schedule renderable objects in separate groups such that the SceneManager
            respects the divisions between the groupings and does not reorder them outside these
            boundaries. This can be handy for overlays where no matter what you want the overlay to 
            be rendered last.
        */
        void addRenderable(Renderable* pRend, RenderQueueGroupID groupId);

        /** Add a renderable object to the queue.
        @remarks
            This methods adds a Renderable to the queue, which will be rendered later by 
            the SceneManager. This is the simplified version of the call which does not 
            require a queue or priority to be specified. The queue group is taken from the
            current default (see setDefaultQueueGroup).  The queue priority is take from the
            current default (see setDefaultRenderablePriority).
        @note
            Called by implementation of MovableObject::_updateRenderQueue.
        @param
            pRend Pointer to the Renderable to be added to the queue
        */
        void addRenderable(Renderable* pRend);
        
        /** Gets the current default queue group, which will be used for all renderable which do not
            specify which group they wish to be on.
        */
        [[nodiscard]] auto getDefaultQueueGroup() const noexcept -> RenderQueueGroupID;

        /** Sets the current default renderable priority, 
            which will be used for all renderables which do not
            specify which priority they wish to use.
        */
        void setDefaultRenderablePriority(ushort priority);

        /** Gets the current default renderable priority, which will be used for all renderables which do not
            specify which priority they wish to use.
        */
        [[nodiscard]] auto getDefaultRenderablePriority() const noexcept -> ushort;

        /** Sets the current default queue group, which will be used for all renderable which do not
            specify which group they wish to be on. See the enum class RenderQueueGroupID for what kind of
            values can be used here.
        */
        void setDefaultQueueGroup(RenderQueueGroupID grp);
        
        /** Internal method, returns the queue groups. */
        [[nodiscard]] auto _getQueueGroups() const -> const RenderQueueGroupMap& {
            return mGroups;
        }

        /** Sets whether or not the queue will split passes by their lighting type,
            ie ambient, per-light and decal. 
        */
        void setSplitPassesByLightingType(bool split);

        /** Gets whether or not the queue will split passes by their lighting type,
            ie ambient, per-light and decal. 
        */
        [[nodiscard]] auto getSplitPassesByLightingType() const noexcept -> bool;

        /** Sets whether or not the queue will split passes which have shadow receive
        turned off (in their parent material), which is needed when certain shadow
        techniques are used.
        */
        void setSplitNoShadowPasses(bool split);

        /** Gets whether or not the queue will split passes which have shadow receive
        turned off (in their parent material), which is needed when certain shadow
        techniques are used.
        */
        [[nodiscard]] auto getSplitNoShadowPasses() const noexcept -> bool;

        /** Sets whether or not objects which cast shadows should be treated as
        never receiving shadows. 
        */
        void setShadowCastersCannotBeReceivers(bool ind);

        /** Gets whether or not objects which cast shadows should be treated as
        never receiving shadows. 
        */
        [[nodiscard]] auto getShadowCastersCannotBeReceivers() const noexcept -> bool;

        /** Set a renderable listener on the queue.
        @remarks
            There can only be a single renderable listener on the queue, since
            that listener has complete control over the techniques in use.
        */
        void setRenderableListener(RenderableListener* listener)
        { mRenderableListener = listener; }

        [[nodiscard]] auto getRenderableListener() const noexcept -> RenderableListener*
        { return mRenderableListener; }

        /** Merge render queue.
        */
        void merge( const RenderQueue* rhs );
        /** Utility method to perform the standard actions associated with 
            getting a visible object to add itself to the queue. This is 
            a replacement for SceneManager implementations of the associated
            tasks related to calling MovableObject::_updateRenderQueue.
        */
        void processVisibleObject(MovableObject* mo, 
            Camera* cam, 
            bool onlyShadowCasters, 
            VisibleObjectsBoundsInfo* visibleBounds);

    };

    /** @} */
    /** @} */

}
