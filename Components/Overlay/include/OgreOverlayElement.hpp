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
export module Ogre.Components.Overlay:Element;

export import Ogre.Core;

export
namespace Ogre {
    class Overlay;
    class OverlayContainer;

    /** \addtogroup Optional
    *  @{
    */
    /** \addtogroup Overlays
    *  @{
    */

    using DisplayString = String;

    /** Enum describing how the position / size of an element is to be recorded. 
    */
    enum class GuiMetricsMode
    {
        /// 'left', 'top', 'height' and 'width' are parametrics from 0.0 to 1.0
        RELATIVE,
        /// Positions & sizes are in absolute pixels
        PIXELS,
        /// Positions & sizes are in virtual pixels
        RELATIVE_ASPECT_ADJUSTED
    };

    /** Enum describing where '0' is in relation to the parent in the horizontal dimension.
    @remarks Affects how 'left' is interpreted.
    */
    enum class GuiHorizontalAlignment
    {
        LEFT,
        CENTER,
        RIGHT
    };
    /** Enum describing where '0' is in relation to the parent in the vertical dimension.
    @remarks Affects how 'top' is interpreted.
    */
    enum class GuiVerticalAlignment
    {
        TOP,
        CENTER,
        BOTTOM
    };

    /** Abstract definition of a 2D element to be displayed in an Overlay.
    @remarks
    This class abstracts all the details of a 2D element which will appear in
    an overlay. In fact, not all OverlayElement instances can be directly added to an
    Overlay, only those which are OverlayContainer instances (a subclass of this class).
    OverlayContainer objects can contain any OverlayElement however. This is just to 
    enforce some level of grouping on widgets.
    @par
    OverlayElements should be managed using OverlayManager. This class is responsible for
    instantiating / deleting elements, and also for accepting new types of element
    from plugins etc.
    @par
    Note that positions / dimensions of 2D screen elements are expressed as parametric
    values (0.0 - 1.0) because this makes them resolution-independent. However, most
    screen resolutions have an aspect ratio of 1.3333:1 (width : height) so note that
    in physical pixels 0.5 is wider than it is tall, so a 0.5x0.5 panel will not be
    square on the screen (but it will take up exactly half the screen in both dimensions).
    @par
    Because this class is designed to be extensible, it subclasses from StringInterface
    so its parameters can be set in a generic way.
    */
    class OverlayElement : public StringInterface, public Renderable, public OverlayAlloc
    {
    public:
        static std::string_view const DEFAULT_RESOURCE_GROUP;
    protected:
        String mName;
        bool mVisible{true};
        bool mCloneable{true};
        Real mLeft{0.0f};
        Real mTop{0.0f};
        Real mWidth{1.0f};
        Real mHeight{1.0f};
        MaterialPtr mMaterial;
        DisplayString mCaption;
        ColourValue mColour;
        RealRect mClippingRegion;

        GuiMetricsMode mMetricsMode{GuiMetricsMode::RELATIVE};
        GuiHorizontalAlignment mHorzAlign{GuiHorizontalAlignment::LEFT};
        GuiVerticalAlignment mVertAlign{GuiVerticalAlignment::TOP};

        // metric-mode positions, used in GuiMetricsMode::PIXELS & GuiMetricsMode::RELATIVE_ASPECT_ADJUSTED mode.
        Real mPixelTop{0.0};
        Real mPixelLeft{0.0};
        Real mPixelWidth{1.0};
        Real mPixelHeight{1.0};
        Real mPixelScaleX{1.0};
        Real mPixelScaleY{1.0};

        /// Parent pointer
        OverlayContainer* mParent{nullptr};
        /// Overlay attached to
        Overlay* mOverlay{nullptr};

        // Derived positions from parent
        Real mDerivedLeft{0};
        Real mDerivedTop{0};
        bool mDerivedOutOfDate{true};

        /// Flag indicating if the vertex positions need recalculating
        bool mGeomPositionsOutOfDate{true};
        /// Flag indicating if the vertex uvs need recalculating
        bool mGeomUVsOutOfDate{true};

        /** Zorder for when sending to render queue.
            Derived from parent */
        ushort mZOrder{0};

        /// World transforms
        Matrix4 mXForm;

        /// Is element enabled?
        bool mEnabled{true};

        /// Is element initialised?
        bool mInitialised{false};

        /** Internal method which is triggered when the positions of the element get updated,
        meaning the element should be rebuilding it's mesh positions. Abstract since
        subclasses must implement this.
        */
        virtual void updatePositionGeometry() = 0;
        /** Internal method which is triggered when the UVs of the element get updated,
        meaning the element should be rebuilding it's mesh UVs. Abstract since
        subclasses must implement this.
        */
        virtual void updateTextureGeometry() = 0;

        /** Internal method for setting up the basic parameter definitions for a subclass. 
        @remarks
        Because StringInterface holds a dictionary of parameters per class, subclasses need to
        call this to ask the base class to add it's parameters to their dictionary as well.
        Can't do this in the constructor because that runs in a non-virtual context.
        @par
        The subclass must have called it's own createParamDictionary before calling this method.
        */
        virtual void addBaseParameters();

    public:
        /// Constructor: do not call direct, use OverlayManager::createElement
        OverlayElement(std::string_view name);
        ~OverlayElement() override;

        /** Initialise gui element */
        virtual void initialise() = 0;

        /** Notifies that hardware resources were lost */
        virtual void _releaseManualHardwareResources() {}
        /** Notifies that hardware resources should be restored */
        virtual void _restoreManualHardwareResources() {}

        /** Gets the name of this overlay. */
        [[nodiscard]] auto getName() const noexcept -> std::string_view { return mName; }


        /** Shows this element if it was hidden. */
        void show() { setVisible(true); }

        /** Hides this element if it was visible. */
        void hide() { setVisible(false); }

        /** Shows or hides this element. */
        void setVisible(bool visible) { mVisible = visible; }

        /** Returns whether or not the element is visible. */
        [[nodiscard]] auto isVisible() const noexcept -> bool { return mVisible; }

        [[nodiscard]] auto isEnabled() const noexcept -> bool { return mEnabled; }
        void setEnabled(bool b) { mEnabled = b; }


        /** Sets the dimensions of this element in relation to the current #GuiMetricsMode. */
        void setDimensions(Real width, Real height);

        /** Sets the position of the top-left corner in relation to the current #GuiMetricsMode (where 0 = top). */
        void setPosition(Real left, Real top);

        /** Sets the width of this element in relation to the current #GuiMetricsMode. */
        void setWidth(Real width);
        /** Gets the width of this element in relation to the current #GuiMetricsMode. */
        [[nodiscard]] auto getWidth() const -> Real;

        /** Sets the height of this element in relation to the current #GuiMetricsMode. */
        void setHeight(Real height);
        /** Gets the height of this element in relation to the current #GuiMetricsMode. */
        [[nodiscard]] auto getHeight() const -> Real;

        /** Sets the left of this element in relation to the current #GuiMetricsMode. */
        void setLeft(Real left);
        /** Gets the left of this element in relation to the current #GuiMetricsMode. */
        [[nodiscard]] auto getLeft() const -> Real;

        /** Sets the top of this element in relation to the current #GuiMetricsMode (where 0 = top). */
        void setTop(Real Top);
        /** Gets the top of this element in relation to the current #GuiMetricsMode (where 0 = top). */
        [[nodiscard]] auto getTop() const -> Real;

        /** Gets the left of this element in relation to the screen (where 0 = far left, 1.0 = far right)  */
        [[nodiscard]] auto _getLeft() const noexcept -> Real { return mLeft; }
        /** Gets the top of this element in relation to the screen (where 0 = far top, 1.0 = far bottom)  */
        [[nodiscard]] auto _getTop() const noexcept -> Real { return mTop; }
        /** Gets the width of this element in relation to the screen (where 1.0 = screen width)  */
        [[nodiscard]] auto _getWidth() const noexcept -> Real { return mWidth; }
        /** Gets the height of this element in relation to the screen (where 1.0 = screen height)  */
        [[nodiscard]] auto _getHeight() const noexcept -> Real { return mHeight; }
        /** Sets the left of this element in relation to the screen (where 1.0 = screen width) */
        void _setLeft(Real left);
        /** Sets the top of this element in relation to the screen (where 1.0 = screen height) */
        void _setTop(Real top);
        /** Sets the width of this element in relation to the screen (where 1.0 = screen width) */
        void _setWidth(Real width);
        /** Sets the height of this element in relation to the screen (where 1.0 = screen height) */
        void _setHeight(Real height);
        /** Sets the left and top of this element in relation to the screen (where 1.0 = screen width/height) */
        void _setPosition(Real left, Real top);
        /** Sets the width and height of this element in relation to the screen (where 1.0 = screen width/height) */
        void _setDimensions(Real width, Real height);

        /** Gets the name of the material this element uses. */
        [[nodiscard]] virtual auto getMaterialName() const noexcept -> std::string_view ;

        /** Sets the the material this element will use.
        @remarks
        Different elements will use different materials. One constant about them
        all though is that a Material used for a OverlayElement must have it's depth
        checking set to 'off', which means it always gets rendered on top. OGRE
        will set this flag for you if necessary. What it does mean though is that 
        you should not use the same Material for rendering OverlayElements as standard 
        scene objects. It's fine to use the same textures, just not the same
        Material.
        */

        void setMaterial(const MaterialPtr& mat);

        /// @overload
        void setMaterialName(std::string_view matName, std::string_view group = DEFAULT_RESOURCE_GROUP );


        // --- Renderable Overrides ---
        [[nodiscard]] auto getMaterial() const noexcept -> const MaterialPtr& override;

        // NB getRenderOperation not implemented, still abstract here

        void getWorldTransforms(Matrix4* xform) const override;

        /** Tell the object to recalculate */
        virtual void _positionsOutOfDate();

        /** Internal method to update the element based on transforms applied. */
        virtual void _update();

        /** Updates this elements transform based on it's parent. */
        virtual void _updateFromParent();

        /** Internal method for notifying the GUI element of it's parent and ultimate overlay. */
        virtual void _notifyParent(OverlayContainer* parent, Overlay* overlay);

        /** Gets the 'left' position as derived from own left and that of parents. */
        virtual auto _getDerivedLeft() -> Real;

        /** Gets the 'top' position as derived from own left and that of parents. */
        virtual auto _getDerivedTop() -> Real;

        /** Gets the 'width' as derived from own width and metrics mode. */
        virtual auto _getRelativeWidth() -> Real;
        /** Gets the 'height' as derived from own height and metrics mode. */
        virtual auto _getRelativeHeight() -> Real;


        /** Gets the clipping region of the element */
        virtual void _getClippingRegion(RealRect &clippingRegion);

        /** Internal method to notify the element when Z-order of parent overlay
        has changed.
        @remarks
        Overlays have explicit Z-orders. OverlayElements do not, they inherit the 
        Z-order of the overlay, and the Z-order is incremented for every container
        nested within this to ensure that containers are displayed behind contained
        items. This method is used internally to notify the element of a change in
        final Z-order which is used to render the element.
        @return Return the next Z-ordering number available. For single elements, this
        is simply 'newZOrder + 1', except for containers. They increment it once for each
        child (or even more if those children are also containers with their own elements).
        */
        virtual auto _notifyZOrder(ushort newZOrder) -> ushort;

        /** Internal method to notify the element when it's world transform
         of parent overlay has changed.
        */
        virtual void _notifyWorldTransforms(const Matrix4& xform);

        /** Internal method to notify the element when the viewport
         of parent overlay has changed.
        */
        virtual void _notifyViewport();

        /** Internal method to put the contents onto the render queue. */
        virtual void _updateRenderQueue(RenderQueue* queue);

        /// @copydoc MovableObject::visitRenderables
        void visitRenderables(Renderable::Visitor* visitor, 
            bool debugRenderables = false);

        /** Gets the type name of the element. All concrete subclasses must implement this. */
        [[nodiscard]] virtual auto getTypeName() const noexcept -> std::string_view = 0;

        /** Sets the caption on elements that support it. 
        @remarks
        This property doesn't do something on all elements, just those that support it.
        However, being a common requirement it is in the top-level interface to avoid
        having to set it via the StringInterface all the time.
        */
        virtual void setCaption(std::string_view text);
        /** Gets the caption for this element. */
        [[nodiscard]] auto getCaption() const noexcept -> std::string_view { return mCaption; }
        /** Sets the colour on elements that support it. 
        @remarks
        This property doesn't do something on all elements, just those that support it.
        However, being a common requirement it is in the top-level interface to avoid
        having to set it via the StringInterface all the time.
        */
        virtual void setColour(const ColourValue& col);

        /** Gets the colour for this element. */
        [[nodiscard]] virtual auto getColour() const noexcept -> const ColourValue&;

        /** Tells this element how to interpret the position and dimension values it is given.
        @remarks
        By default, OverlayElements are positioned and sized according to relative dimensions
        of the screen. This is to ensure portability between different resolutions when you
        want things to be positioned and sized the same way across all resolutions. However, 
        sometimes you want things to be sized according to fixed pixels. In order to do this,
        you can call this method with the parameter GuiMetricsMode::PIXELS. Note that if you then want
        to place your element relative to the center, right or bottom of it's parent, you will
        need to use the setHorizontalAlignment and setVerticalAlignment methods.
        */
        virtual void setMetricsMode(GuiMetricsMode gmm);
        /** Retrieves the current settings of how the element metrics are interpreted. */
        [[nodiscard]] auto getMetricsMode() const noexcept -> GuiMetricsMode { return mMetricsMode; }
        /** Sets the horizontal origin for this element.
        @remarks
        By default, the horizontal origin for a OverlayElement is the left edge of the parent container
        (or the screen if this is a root element). You can alter this by calling this method, which is
        especially useful when you want to use pixel-based metrics (see setMetricsMode) since in this
        mode you can't use relative positioning.
        @par
        For example, if you were using GuiMetricsMode::PIXELS metrics mode, and you wanted to place a 30x30 pixel
        crosshair in the center of the screen, you would use GuiHorizontalAlignment::CENTER with a 'left' property of -15.
        @par
        Note that neither GuiHorizontalAlignment::CENTER or GuiHorizontalAlignment::RIGHT alter the position of the element based
        on it's width, you have to alter the 'left' to a negative number to do that; all this
        does is establish the origin. This is because this way you can align multiple things
        in the center and right with different 'left' offsets for maximum flexibility.
        */
        virtual void setHorizontalAlignment(GuiHorizontalAlignment gha);
        /** Gets the horizontal alignment for this element. */
        [[nodiscard]] auto getHorizontalAlignment() const noexcept -> GuiHorizontalAlignment { return mHorzAlign; }
        /** Sets the vertical origin for this element. 
        @remarks
        By default, the vertical origin for a OverlayElement is the top edge of the parent container
        (or the screen if this is a root element). You can alter this by calling this method, which is
        especially useful when you want to use pixel-based metrics (see setMetricsMode) since in this
        mode you can't use relative positioning.
        @par
        For example, if you were using GuiMetricsMode::PIXELS metrics mode, and you wanted to place a 30x30 pixel
        crosshair in the center of the screen, you would use GuiHorizontalAlignment::CENTER with a 'top' property of -15.
        @par
        Note that neither GuiVerticalAlignment::CENTER or GuiVerticalAlignment::BOTTOM alter the position of the element based
        on it's height, you have to alter the 'top' to a negative number to do that; all this
        does is establish the origin. This is because this way you can align multiple things
        in the center and bottom with different 'top' offsets for maximum flexibility.
        */
        virtual void setVerticalAlignment(GuiVerticalAlignment gva);
        /** Gets the vertical alignment for this element. */
        [[nodiscard]] auto getVerticalAlignment() const noexcept -> GuiVerticalAlignment { return mVertAlign; }




        /** Returns true if xy is within the constraints of the component */
        [[nodiscard]] virtual auto contains(Real x, Real y) const -> bool;

        /** Returns true if xy is within the constraints of the component */
        virtual auto findElementAt(Real x, Real y) -> OverlayElement*;      // relative to parent

        /**
        * returns false as this class is not a container type 
        */
        [[nodiscard]] virtual auto isContainer() const noexcept -> bool { return false; }
        [[nodiscard]] virtual auto isKeyEnabled() const noexcept -> bool { return false; }
        [[nodiscard]] auto isCloneable() const noexcept -> bool { return mCloneable; }
        void setCloneable(bool c) { mCloneable = c; }

        /**
        * Returns the parent container.
        */
        auto getParent() noexcept -> OverlayContainer* { return mParent; }
        void _setParent(OverlayContainer* parent) { mParent = parent; }

        /**
        * Returns the zOrder of the element
        */
        [[nodiscard]] auto getZOrder() const noexcept -> ushort { return mZOrder; }

        /** Overridden from Renderable */
        auto getSquaredViewDepth(const Camera* cam) const -> Real override 
        { 
            (void)cam;
            return 10000.0f - (Real)getZOrder(); 
        }

        /** @copydoc Renderable::getLights */
        [[nodiscard]] auto getLights() const noexcept -> const LightList& override
        {
            // Overlayelements should not be lit by the scene, this will not get called
            static LightList ll;
            return ll;
        }

        virtual void copyFromTemplate(OverlayElement* templateOverlay) { templateOverlay->copyParametersTo(this); }
        virtual auto clone(std::string_view instanceName) -> OverlayElement*;
    };


    /** @} */
    /** @} */

}
