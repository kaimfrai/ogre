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
export module Ogre.Core:Bone;

export import :Node;
export import :Prerequisites;
export import :Quaternion;
export import :Vector;

export import <string_view>;

export
namespace Ogre 
{
struct Affine3;
class Skeleton;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Animation
    *  @{
    */

    /** A bone in a skeleton.
    @remarks
        See Skeleton for more information about the principles behind skeletal animation.
        This class is a node in the joint hierarchy. Mesh vertices also have assignments
        to bones to define how they move in relation to the skeleton.
    */
    class Bone : public Node
    {
    public:
        /** Constructor, not to be used directly (use Bone::createChild or Skeleton::createBone) */
        Bone(unsigned short handle, Skeleton* creator);
        /** Constructor, not to be used directly (use Bone::createChild or Skeleton::createBone) */
        Bone(std::string_view name, unsigned short handle, Skeleton* creator);
        ~Bone() override;

    private:
        // Intentionally hide base implementations of createChild methods. This will also suppress
        // warnings like 'Ogre::Bone::createChild' hides overloaded virtual functions
        using Node::createChild;
    public:
        /** Creates a new Bone as a child of this bone.
        @remarks
            This method creates a new bone which will inherit the transforms of this
            bone, with the handle specified.
            @param 
                handle The numeric handle to give the new bone; must be unique within the Skeleton.
            @param
                translate Initial translation offset of child relative to parent
            @param
                rotate Initial rotation relative to parent
        */
        auto createChild(unsigned short handle, 
            const Vector3& translate = Vector3::ZERO, const Quaternion& rotate = Quaternion::IDENTITY) -> Bone*;

        /** Gets the numeric handle for this bone (unique within the skeleton). */
        auto getHandle() const noexcept -> unsigned short;

        /** Sets the current position / orientation to be the 'binding pose' ie the layout in which 
            bones were originally bound to a mesh.
        */
        void setBindingPose();

        /** Resets the position and orientation of this Bone to the original binding position.
        @remarks
            Bones are bound to the mesh in a binding pose. They are then modified from this
            position during animation. This method returns the bone to it's original position and
            orientation.
        */
        void reset();

        /** Sets whether or not this bone is manually controlled. 
        @remarks
            Manually controlled bones can be altered by the application at runtime, 
            and their positions will not be reset by the animation routines. Note 
            that you should also make sure that there are no AnimationTrack objects
            referencing this bone, or if there are, you should disable them using
            pAnimation->destroyTrack(pBone->getHandle());
        @par
            You can also use AnimationState::setBlendMask to mask out animation from 
            chosen tracks if you want to prevent application of a scripted animation 
            to a bone without altering the Animation definition.
        */
        void setManuallyControlled(bool manuallyControlled);

        /** Getter for mManuallyControlled Flag */
        auto isManuallyControlled() const noexcept -> bool;

        
        /** Gets the transform which takes bone space to current from the binding pose. 
        @remarks
            Internal use only.
        */
        void _getOffsetTransform(Affine3& m) const;

        /** Gets the inverted binding pose scale. */
        auto _getBindingPoseInverseScale() const noexcept -> const Vector3& { return mBindDerivedInverseScale; }
        /** Gets the inverted binding pose position. */
        auto _getBindingPoseInversePosition() const noexcept -> const Vector3& { return mBindDerivedInversePosition; }
        /** Gets the inverted binding pose orientation. */
        auto _getBindingPoseInverseOrientation() const noexcept -> const Quaternion& { return mBindDerivedInverseOrientation; }

        /// @see Node::needUpdate
        void needUpdate(bool forceParentUpdate = false) override;


    private:
        /** See Node. */
        auto createChildImpl() -> Node* override;
        /** See Node. */
        auto createChildImpl(std::string_view name) -> Node* override;

        /// Pointer back to creator, for child creation (not smart ptr so child does not preserve parent)
        Skeleton* mCreator;

        /// The inversed derived scale of the bone in the binding pose
        Vector3 mBindDerivedInverseScale;
        /// The inversed derived orientation of the bone in the binding pose
        Quaternion mBindDerivedInverseOrientation;
        /// The inversed derived position of the bone in the binding pose
        Vector3 mBindDerivedInversePosition;
        /// The numeric handle of this bone
        unsigned short mHandle;
        /** Bones set as manuallyControlled are not reseted in Skeleton::reset() */
        bool mManuallyControlled;
    };

    /** @} */
    /** @} */

}
