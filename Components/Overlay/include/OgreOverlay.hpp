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

#ifndef OGRE_COMPONENTS_OVERLAY_H
#define OGRE_COMPONENTS_OVERLAY_H

#include <list>
#include <memory>

#include "OgreMath.hpp"
#include "OgreMatrix4.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSceneNode.hpp"

namespace Ogre {
    class OverlayContainer;
    class OverlayElement;
    class Camera;
    class RenderQueue;
    class Viewport;

    /** \addtogroup Optional
    *  @{
    */
    /** \defgroup Overlays Overlay
    * Layers which is rendered on top of the 'normal' scene contents
    *  @{
    */
    /** Represents a layer which is rendered on top of the 'normal' scene contents.
    @remarks
        An overlay is a container for visual components (2D and 3D) which will be 
        rendered after the main scene in order to composite heads-up-displays, menus
        or other layers on top of the contents of the scene.
    @par
        An overlay always takes up the entire size of the viewport, although the 
        components attached to it do not have to. An overlay has no visual element
        in itself, it it merely a container for visual elements.
    @par
        Overlays are created by calling OverlayManager::create, or by defining them
        in special text scripts (.overlay files). As many overlays
        as you like can be defined; after creation an overlay is hidden i.e. not
        visible until you specifically enable it by calling 'show'. This allows you to have multiple
        overlays predefined (menus etc) which you make visible only when you want.
        It is possible to have multiple overlays enabled at once; in this case the
        relative 'zorder' parameter of the overlays determine which one is displayed
        on top.
    @par
        By default overlays are rendered into all viewports. This is fine when you only
        have fullscreen viewports, but if you have picture-in-picture views, you probably
        don't want the overlay displayed in the smaller viewports. You turn this off for 
        a specific viewport by calling the Viewport::setDisplayOverlays method.
    */
    class Overlay : public OverlayAlloc
    {

    public:
              using OverlayContainerList = std::list<OverlayContainer *>;
    protected:
        String mName;
        /// Internal root node, used as parent for 3D objects
        ::std::unique_ptr<SceneNode> mRootNode;
        /** 2D elements
            OverlayContainers, linked list for easy sorting by zorder later
            Not a map because sort can be saved since changes infrequent (unlike render queue) */
        OverlayContainerList m2DElements;

        /// Degrees of rotation around center
        Radian mRotate;
        /// Scroll values, offsets
        Real mScrollX{0.0f}, mScrollY{0.0f};
        /// Scale values
        Real mScaleX{1.0f}, mScaleY{1.0f};

        int mLastViewportWidth{0}, mLastViewportHeight{0};

        mutable Matrix4 mTransform;
        mutable bool mTransformOutOfDate{true};
        bool mTransformUpdated{true};
        ulong mZOrder{100};
        bool mVisible{false};
        bool mInitialised{false};
        String mOrigin;
        /** Internal lazy update method. */
        void updateTransform() const;
        /** Internal method for initialising an overlay */
        virtual void initialise();
        /** Internal method for updating container elements' Z-ordering */
        void assignZOrders();

    public:
        /// Constructor: do not call direct, use OverlayManager::create
        Overlay(std::string_view name);
        virtual ~Overlay();


        auto getChild(std::string_view name) -> OverlayContainer*;

        /** Gets the name of this overlay. */
        auto getName() const noexcept -> std::string_view ;
        
        /** Alters the Z-order of this overlay. 
        @remarks
            Values between 0 and 650 are valid here.
        */
        void setZOrder(ushort zorder);
        /** Gets the Z-order of this overlay. */
        auto getZOrder() const noexcept -> ushort;

        /** Gets whether the overlay is displayed or not. */
        auto isVisible() const noexcept -> bool;

        /** Gets whether the overlay is initialised or not. */
        auto isInitialised() const noexcept -> bool { return mInitialised; }

        /** Shows the overlay if it was hidden. */
        void show();

        /** Hides the overlay if it was visible. */
        void hide();

        /** Shows or hides this overlay. */
        void setVisible(bool visible);

        /** Adds a 2D 'container' to the overlay.
        @remarks
            Containers are created and managed using the OverlayManager. A container
            could be as simple as a square panel, or something more complex like
            a grid or tree view. Containers group collections of other elements,
            giving them a relative coordinate space and a common z-order.
            If you want to attach a GUI widget to an overlay, you have to do it via
            a container.
        @param cont Pointer to a container to add, created using OverlayManager.
        */
        void add2D(OverlayContainer* cont);


        /** Removes a 2D container from the overlay. 
        @remarks
            NOT FAST. Consider OverlayElement::hide.
        */
        void remove2D(OverlayContainer* cont);

        /** Adds a node capable of holding 3D objects to the overlay.
        @remarks    
            Although overlays are traditionally associated with 2D elements, there 
            are reasons why you might want to attach 3D elements to the overlay too.
            For example, if you wanted to have a 3D cockpit, which was overlaid with a
            HUD, then you would create 2 overlays, one with a 3D object attached for the
            cockpit, and one with the HUD elements attached (the zorder of the HUD 
            overlay would be higher than the cockpit to ensure it was always on top).
        @par    
            A SceneNode can have any number of 3D objects attached to it. SceneNodes
            are usually created using SceneManager::createSceneNode, but in this case
            you should create a standard SceneNode instance <b>manually</b>; this is
            because these scene nodes are not managed by the SceneManager and some custom
            SceneManager plugins will rely on specialist behaviour the overlay does not
            support. By attaching a SceneNode to an overlay, you indicate that:<OL>
            <LI>You want the contents of this node to only appear when the overlay is active</LI>
            <LI>You want the node to inherit a coordinate space relative to the camera,
                rather than relative to the root scene node</LI>
            <LI>You want these objects to be rendered after the contents of the main scene
                to ensure they are rendered on top</LI>
            </OL>
            One major consideration when using 3D objects in overlays is the behaviour of 
            the depth buffer. Overlays should use materials with depth checking off, to ensure
            that their contents are always displayed on top of the main scene (to do 
            otherwise would result in objects 'poking through' the overlay). The problem
            with using 3D objects is that if they are concave, or self-overlap, then you
            can get artefacts because of the lack of depth buffer checking. So you should 
            ensure that any 3D objects you us in the overlay are convex, and don't overlap
            each other. If they must overlap, split them up and put them in 2 overlays.
            Alternatively, use a 2D element underneath them which will clear the depth buffer
            values underneath ready for the 3D element to be rendered correctly.
        */
        void add3D(SceneNode* node);

        /** Removes a 3D element from the overlay. */
        void remove3D(SceneNode* node);

        /** Clears the overlay of all attached items. */
        void clear();

        /** Sets the scrolling factor of this overlay.
        @remarks
            You can use this to set an offset to be used to scroll an 
            overlay around the screen.
        @param x Horizontal scroll value, where 0 = normal, -0.5 = scroll so that only
            the right half the screen is visible etc
        @param y Vertical scroll value, where 0 = normal, 0.5 = scroll down by half 
            a screen etc.
        */
        void setScroll(Real x, Real y);

        /** Gets the current X scroll value */
        auto getScrollX() const -> Real;

        /** Gets the current Y scroll value */
        auto getScrollY() const -> Real;

        /** Scrolls the overlay by the offsets provided.
        @remarks
            This method moves the overlay by the amounts provided. As with
            other methods on this object, a full screen width / height is represented
            by the value 1.0.
        */
        void scroll(Real xoff, Real yoff);

        /** Sets the rotation applied to this overlay.*/
        void setRotate(const Radian& angle);

        /** Gets the rotation applied to this overlay, in degrees.*/
        auto getRotate() const noexcept -> const Radian & { return mRotate; }

        /** Adds the passed in angle to the rotation applied to this overlay. */
        void rotate(const Radian& angle);

        /** Sets the scaling factor of this overlay.
        @remarks
            You can use this to set an scale factor to be used to zoom an 
            overlay.
        @param x Horizontal scale value, where 1.0 = normal, 0.5 = half size etc
        @param y Vertical scale value, where 1.0 = normal, 0.5 = half size etc
        */
        void setScale(Real x, Real y);

        /** Gets the current X scale value */
        auto getScaleX() const -> Real;

        /** Gets the current Y scale value */
        auto getScaleY() const -> Real;

        /** Used to transform the overlay when scrolling, scaling etc. */
        void _getWorldTransforms(Matrix4* xform) const;

        /** Internal method to put the overlay contents onto the render queue. */
        virtual void _findVisibleObjects(Camera* cam, RenderQueue* queue, Viewport* vp);

        /** This returns a OverlayElement at position x,y. */
        virtual auto findElementAt(Real x, Real y) -> OverlayElement*;

        /** Returns all 2D elements in this manager.
        */
        auto get2DElements() const noexcept -> const OverlayContainerList& {
            return m2DElements;
        }

        /** Get the origin of this overlay, e.g. a script file name.
        @remarks
            This property will only contain something if the creator of
            this overlay chose to populate it. Script loaders are advised
            to populate it.
        */
        auto getOrigin() const noexcept -> std::string_view { return mOrigin; }
        /// Notify this overlay of it's origin
        void _notifyOrigin(std::string_view origin) { mOrigin = origin; }


    };
    /** @} */
    /** @} */

}


#endif

