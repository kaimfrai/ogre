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
#ifndef OGRE_CORE_SIMPLERENDERABLE_H
#define OGRE_CORE_SIMPLERENDERABLE_H

#include "OgreAxisAlignedBox.hpp"
#include "OgreCommon.hpp"
#include "OgreMatrix4.hpp"
#include "OgreMovableObject.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderOperation.hpp"
#include "OgreRenderable.hpp"
#include "OgreSharedPtr.hpp"

namespace Ogre {
class Camera;
class RenderQueue;
class SceneManager;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */
    /** Simple implementation of MovableObject and Renderable for single-part custom objects. 
    @see ManualObject for a simpler interface with more flexibility
    */
    class SimpleRenderable : public MovableObject, public Renderable
    {
        auto getCastsShadows() const noexcept -> bool override { return getCastShadows(); }
    protected:
        RenderOperation mRenderOp;

        Affine3 mTransform;
        AxisAlignedBox mBox;

        MaterialPtr mMaterial;

        /// The scene manager for the current frame.
        SceneManager *mParentSceneManager;

        /// The camera for the current frame.
        Camera *mCamera;

        /// Static member used to automatically generate names for SimpleRendaerable objects.
        static uint msGenNameCount;

    public:
        /// Constructor
        SimpleRenderable();

        /// Named constructor
        SimpleRenderable(std::string_view name);

        virtual void setMaterial(const MaterialPtr& mat);
        auto getMaterial() const noexcept -> const MaterialPtr& override;

        virtual void setRenderOperation( const RenderOperation& rend );
        void getRenderOperation(RenderOperation& op) override;

        void setTransform( const Affine3& xform );
        void getWorldTransforms( Matrix4* xform ) const override;


        void _notifyCurrentCamera(Camera* cam) override;

        void setBoundingBox( const AxisAlignedBox& box );
        auto getBoundingBox() const noexcept -> const AxisAlignedBox& override;

        void _updateRenderQueue(RenderQueue* queue) override;

        void visitRenderables(Renderable::Visitor* visitor,
            bool debugRenderables = false) override;
        auto getMovableType() const noexcept -> std::string_view override;
        auto getLights() const noexcept -> const LightList& override;

    };
    /** @} */
    /** @} */
}

#endif
