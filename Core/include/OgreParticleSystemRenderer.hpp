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

#include <cassert>

export module Ogre.Core:ParticleSystemRenderer;

export import :Common;
export import :FactoryObj;
export import :Prerequisites;
export import :RenderQueue;
export import :Renderable;
export import :StringInterface;

export
namespace Ogre {

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
    /** Abstract class defining the interface required to be implemented
        by classes which provide rendering capability to ParticleSystem instances.
    */
    class ParticleSystemRenderer : public StringInterface, public FXAlloc
    {
    public:
        /// Constructor
        ParticleSystemRenderer() = default;
        /// Destructor
        ~ParticleSystemRenderer() override = default;

        /** Gets the type of this renderer - must be implemented by subclasses */
        [[nodiscard]] virtual auto getType() const noexcept -> std::string_view = 0;

        /** Delegated to by ParticleSystem::_updateRenderQueue
        @remarks
            The subclass must update the render queue using whichever Renderable
            instance(s) it wishes.
        */
        virtual void _updateRenderQueue(RenderQueue* queue, 
            std::vector<Particle*>& currentParticles, bool cullIndividually) = 0;

        /** Sets the material this renderer must use; called by ParticleSystem. */
        virtual void _setMaterial(MaterialPtr& mat) = 0;
        /** Delegated to by ParticleSystem::_notifyCurrentCamera */
        virtual void _notifyCurrentCamera(Camera* cam) = 0;
        /** Delegated to by ParticleSystem::_notifyAttached */
        virtual void _notifyAttached(Node* parent, bool isTagPoint = false) = 0;
        /** Tells the renderer that the particle quota has changed */
        virtual void _notifyParticleQuota(size_t quota) = 0;
        /** Tells the renderer that the particle default size has changed */
        virtual void _notifyDefaultDimensions(Real width, Real height) = 0;
        /** Optional callback notified when particle emitted */
        virtual void _notifyParticleEmitted(Particle* particle) {}
        /** Optional callback notified when particle expired */
        virtual void _notifyParticleExpired(Particle* particle) {}
        /** Optional callback notified when particles moved */
        virtual void _notifyParticleMoved(std::vector<Particle*>& currentParticles) {}
        /** Optional callback notified when particles cleared */
        virtual void _notifyParticleCleared(std::vector<Particle*>& currentParticles) {}
        /** Create a new ParticleVisualData instance for attachment to a particle.
        @remarks
            If this renderer needs additional data in each particle, then this should
            be held in an instance of a subclass of ParticleVisualData, and this method
            should be overridden to return a new instance of it. The default
            behaviour is to return null.
        */
        virtual auto _createVisualData() noexcept -> ParticleVisualData* { return nullptr; }
        /** Destroy a ParticleVisualData instance.
        @remarks
            If this renderer needs additional data in each particle, then this should
            be held in an instance of a subclass of ParticleVisualData, and this method
            should be overridden to destroy an instance of it. The default
            behaviour is to do nothing.
        */
        virtual void _destroyVisualData(ParticleVisualData* vis) { assert (vis == nullptr); }

        /** Sets which render queue group this renderer should target with it's
            output.
        */
        virtual void setRenderQueueGroup(RenderQueueGroupID queueID) = 0;
        /** Sets which render queue group and priority this renderer should target with it's
            output.
        */
        virtual void setRenderQueueGroupAndPriority(RenderQueueGroupID queueID, ushort priority) = 0;

        /** Setting carried over from ParticleSystem.
        */
        virtual void setKeepParticlesInLocalSpace(bool keepLocal) = 0;

        /** Gets the desired particles sort mode of this renderer */
        [[nodiscard]] virtual auto _getSortMode() const -> SortMode = 0;

        /** Required method to allow the renderer to communicate the Renderables
            it will be using to render the system to a visitor.
        @see MovableObject::visitRenderables
        */
        virtual void visitRenderables(Renderable::Visitor* visitor, 
            bool debugRenderables = false) = 0;

        /// Tells the Renderer about the ParticleSystem bounds
        virtual void _notifyBoundingBox(const AxisAlignedBox& aabb) {}

        /// Tells the Renderer whether to cast shadows
        virtual void _notifyCastShadows(bool enabled) {}
    };

    /** @} */
    /** @} */

}
