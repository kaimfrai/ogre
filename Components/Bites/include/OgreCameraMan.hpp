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
#ifndef OGRE_COMPONENTS_BITES_CAMERAMAN_H
#define OGRE_COMPONENTS_BITES_CAMERAMAN_H

#include "OgreInput.hpp"
#include "OgreMath.hpp"
#include "OgreNode.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreVector.hpp"

namespace Ogre {
class SceneNode;
struct FrameEvent;
}  // namespace Ogre

namespace OgreBites
{
    /** \addtogroup Optional
    *  @{
    */
    /** \addtogroup Bites
    *  @{
    */
    enum class CameraStyle   /// enumerator values for different styles of camera movement
    {
        FREELOOK,
        ORBIT,
        MANUAL
    };

    /**
    Utility class for controlling the camera in samples.
    */
    class CameraMan : public InputListener
    {
    public:
        CameraMan(Ogre::SceneNode* cam);

        /**
        Swaps the camera on our camera man for another camera.
        */
        void setCamera(Ogre::SceneNode* cam);

        auto getCamera() noexcept -> Ogre::SceneNode*
        {
            return mCamera;
        }

        /**
        Sets the target we will revolve around. Only applies for orbit style.
        */
        virtual void setTarget(Ogre::SceneNode* target);

        auto getTarget() noexcept -> Ogre::SceneNode*
        {
            return mTarget;
        }

        /**
        Sets the spatial offset from the target. Only applies for orbit style.
        */
        void setYawPitchDist(const Ogre::Radian& yaw, const Ogre::Radian& pitch, Ogre::Real dist);

        /**
        Sets the camera's top speed. Only applies for free-look style.
        */
        void setTopSpeed(Ogre::Real topSpeed)
        {
            mTopSpeed = topSpeed;
        }

        auto getTopSpeed() -> Ogre::Real
        {
            return mTopSpeed;
        }

        /**
        Sets the movement style of our camera man.
        */
        virtual void setStyle(CameraStyle style);

        auto getStyle() -> CameraStyle
        {
            return mStyle;
        }

        /**
        Manually stops the camera when in free-look mode.
        */
        void manualStop();

        void frameRendered(const Ogre::FrameEvent& evt) override;

        /**
        Processes key presses for free-look style movement.
        */
        auto keyPressed(const KeyDownEvent& evt) noexcept -> bool override;

        /**
        Processes key releases for free-look style movement.
        */
        auto keyReleased(const KeyUpEvent& evt) noexcept -> bool override;

        /**
        Processes mouse movement differently for each style.
        */
        auto mouseMoved(const MouseMotionEvent& evt) noexcept -> bool override;

        auto mouseWheelRolled(const MouseWheelEvent& evt) noexcept -> bool override;

        /**
        Processes mouse presses. Only applies for orbit style.
        Left button is for orbiting, and right button is for zooming.
        */
        auto mousePressed(const MouseButtonDownEvent& evt) noexcept -> bool override;

        /**
        Processes mouse releases. Only applies for orbit style.
        Left button is for orbiting, and right button is for zooming.
        */
        auto mouseReleased(const MouseButtonUpEvent& evt) noexcept -> bool override;

        /**
         * fix the yaw axis to be Vector3::UNIT_Y of the parent node (tabletop mode)
         * 
         * otherwise the yaw axis can change freely
         */
        void setFixedYaw(bool fixed)
        {
            using enum Ogre::Node::TransformSpace;
            mYawSpace = fixed ? PARENT : LOCAL;
        }

        void setPivotOffset(const Ogre::Vector3& offset);
    protected:
        auto getDistToTarget() -> Ogre::Real;
        Ogre::Node::TransformSpace mYawSpace{Ogre::Node::TransformSpace::PARENT};
        Ogre::SceneNode* mCamera{nullptr};
        CameraStyle mStyle{CameraStyle::MANUAL};
        Ogre::SceneNode* mTarget{nullptr};
        bool mOrbiting{false};
        bool mMoving{false};
        Ogre::Real mTopSpeed{150};
        Ogre::Vector3 mVelocity;
        bool mGoingForward{false};
        bool mGoingBack{false};
        bool mGoingLeft{false};
        bool mGoingRight{false};
        bool mGoingUp{false};
        bool mGoingDown{false};
        bool mFastMove{false};
        Ogre::Vector3 mOffset;
    };
    /** @} */
    /** @} */
}
#endif
