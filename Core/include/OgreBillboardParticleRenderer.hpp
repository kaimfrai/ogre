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
#ifndef OGRE_CORE_BILLBOARDPARTICLERENDERER_H
#define OGRE_CORE_BILLBOARDPARTICLERENDERER_H

#include <cstddef>
#include <vector>

#include "OgreAxisAlignedBox.hpp"
#include "OgreBillboardSet.hpp"
#include "OgreCommon.hpp"
#include "OgreFactoryObj.hpp"
#include "OgreParticleSystemRenderer.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderable.hpp"
#include "OgreVector.hpp"

namespace Ogre {
class Camera;
class Node;
class Particle;
class RenderQueue;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */

    /** Specialisation of ParticleSystemRenderer to render particles using 
        a BillboardSet. 
    @remarks
        This renderer has a few more options than the standard particle system,
        which will be passed to it automatically when the particle system itself
        does not understand them.
    */
    class BillboardParticleRenderer : public ParticleSystemRenderer
    {
        /// The billboard set that's doing the rendering
        BillboardSet* mBillboardSet;
        Vector2 mStacksSlices;
    public:
        BillboardParticleRenderer();
        ~BillboardParticleRenderer() override;

        /// @copydoc BillboardSet::setTextureStacksAndSlices
        void setTextureStacksAndSlices(uchar stacks, uchar slices)
        {
            mStacksSlices = Vector2(stacks, slices); // cache for get call
            mBillboardSet->setTextureStacksAndSlices(stacks, slices);
        }

        [[nodiscard]] const Vector2& getTextureStacksAndSlices() const { return mStacksSlices; }

        /// @copydoc BillboardSet::setBillboardType
        void setBillboardType(BillboardType bbt) { mBillboardSet->setBillboardType(bbt); }
        /// @copydoc BillboardSet::getBillboardType
        [[nodiscard]] BillboardType getBillboardType() const { return mBillboardSet->getBillboardType(); }
        /// @copydoc BillboardSet::setUseAccurateFacing
        void setUseAccurateFacing(bool acc) { mBillboardSet->setUseAccurateFacing(acc); }
        /// @copydoc BillboardSet::getUseAccurateFacing
        [[nodiscard]] bool getUseAccurateFacing() const { return mBillboardSet->getUseAccurateFacing(); }
        /// @copydoc BillboardSet::setBillboardOrigin
        void setBillboardOrigin(BillboardOrigin origin) { mBillboardSet->setBillboardOrigin(origin); }
        /// @copydoc BillboardSet::getBillboardOrigin
        [[nodiscard]] BillboardOrigin getBillboardOrigin() const { return mBillboardSet->getBillboardOrigin(); }
        /// @copydoc BillboardSet::setBillboardRotationType
        void setBillboardRotationType(BillboardRotationType rotationType)
        {
            mBillboardSet->setBillboardRotationType(rotationType);
        }
        /// @copydoc BillboardSet::getBillboardRotationType
        [[nodiscard]] BillboardRotationType getBillboardRotationType() const
        {
            return mBillboardSet->getBillboardRotationType();
        }
        /// @copydoc BillboardSet::setCommonDirection
        void setCommonDirection(const Vector3& vec) { mBillboardSet->setCommonDirection(vec); }
        /// @copydoc BillboardSet::getCommonDirection
        [[nodiscard]] const Vector3& getCommonDirection() const { return mBillboardSet->getCommonDirection(); }
        /// @copydoc BillboardSet::setCommonUpVector
        void setCommonUpVector(const Vector3& vec) { mBillboardSet->setCommonUpVector(vec); }
        /// @copydoc BillboardSet::getCommonUpVector
        [[nodiscard]] const Vector3& getCommonUpVector() const { return mBillboardSet->getCommonUpVector(); }
        /// @copydoc BillboardSet::setPointRenderingEnabled
        void setPointRenderingEnabled(bool enabled) { mBillboardSet->setPointRenderingEnabled(enabled); }
        /// @copydoc BillboardSet::isPointRenderingEnabled
        [[nodiscard]] bool isPointRenderingEnabled() const { return mBillboardSet->isPointRenderingEnabled(); }

        /// @copydoc ParticleSystemRenderer::getType
        [[nodiscard]] const String& getType() const override;
        /// @copydoc ParticleSystemRenderer::_updateRenderQueue
        void _updateRenderQueue(RenderQueue* queue, 
            std::vector<Particle*>& currentParticles, bool cullIndividually) override;
        /// @copydoc ParticleSystemRenderer::visitRenderables
        void visitRenderables(Renderable::Visitor* visitor, bool debugRenderables = false) override
        {
            mBillboardSet->visitRenderables(visitor, debugRenderables);
        }
        void _setMaterial(MaterialPtr& mat) override { mBillboardSet->setMaterial(mat); }
        /// @copydoc ParticleSystemRenderer::_notifyCurrentCamera
        void _notifyCurrentCamera(Camera* cam) override { mBillboardSet->_notifyCurrentCamera(cam); }
        /// @copydoc ParticleSystemRenderer::_notifyParticleQuota
        void _notifyParticleQuota(size_t quota) override { mBillboardSet->setPoolSize(quota); }
        /// @copydoc ParticleSystemRenderer::_notifyAttached
        void _notifyAttached(Node* parent, bool isTagPoint = false) override
        {
            mBillboardSet->_notifyAttached(parent, isTagPoint);
        }
        /// @copydoc ParticleSystemRenderer::_notifyDefaultDimensions
        void _notifyDefaultDimensions(Real width, Real height) override
        {
            mBillboardSet->setDefaultDimensions(width, height);
        }
        /// @copydoc ParticleSystemRenderer::setRenderQueueGroup
        void setRenderQueueGroup(uint8 queueID) override { mBillboardSet->setRenderQueueGroup(queueID); }
        /// @copydoc MovableObject::setRenderQueueGroupAndPriority
        void setRenderQueueGroupAndPriority(uint8 queueID, ushort priority) override
        {
            mBillboardSet->setRenderQueueGroupAndPriority(queueID, priority);
        }
        /// @copydoc ParticleSystemRenderer::setKeepParticlesInLocalSpace
        void setKeepParticlesInLocalSpace(bool keepLocal) override
        {
            mBillboardSet->setBillboardsInWorldSpace(!keepLocal);
        }
        /// @copydoc ParticleSystemRenderer::_getSortMode
        [[nodiscard]] SortMode _getSortMode() const override { return mBillboardSet->_getSortMode(); }

        /// Access BillboardSet in use
        [[nodiscard]] BillboardSet* getBillboardSet() const { return mBillboardSet; }

        void _notifyBoundingBox(const AxisAlignedBox& aabb) override;

        void _notifyCastShadows(bool enabled) override { mBillboardSet->setCastShadows(enabled); }
    };

    /** Factory class for BillboardParticleRenderer */
    class BillboardParticleRendererFactory : public ParticleSystemRendererFactory
    {
    public:
        /// @copydoc FactoryObj::getType
        [[nodiscard]] const String& getType() const override;
        /// @copydoc FactoryObj::createInstance
        ParticleSystemRenderer* createInstance( const String& name ) override;
    };
    /** @} */
    /** @} */

} // namespace Ogre

#endif // OGRE_CORE_BILLBOARDPARTICLERENDERER_H
