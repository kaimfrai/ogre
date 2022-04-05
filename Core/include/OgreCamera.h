/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#include <algorithm>
#include <iosfwd>
#include <vector>

export module Ogre.Core:Camera;

// Default options
// Matrices & Vectors
export import :AxisAlignedBox;
export import :Common;
export import :Frustum;
export import :Plane;
export import :PlaneBoundedVolume;
export import :Prerequisites;
export import :Quaternion;
export import :Vector;

export
namespace Ogre {

    class Ray;
class Affine3;
class SceneManager;
class Sphere;
class Viewport;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */

    /** A viewpoint from which the scene will be rendered.

        OGRE renders scenes from a camera viewpoint into a buffer of
        some sort, normally a window or a texture (a subclass of
        RenderTarget). OGRE cameras support both perspective projection (the default,
        meaning objects get smaller the further away they are) and
        orthographic projection (blueprint-style, no decrease in size
        with distance). Each camera carries with it a style of rendering,
        e.g. full textured, flat shaded, wireframe), field of view,
        rendering distances etc, allowing you to use OGRE to create
        complex multi-window views if required. In addition, more than
        one camera can point at a single render target if required,
        each rendering to a subset of the target, allowing split screen
        and picture-in-picture views.

        At render time, all Scene Objects will be transformed in the camera space,
        which is defined as:
        - \f$+x\f$ is right
        - \f$+y\f$ is up
        - \f$-z\f$ is away

        Cameras maintain their own aspect ratios, field of view, and frustum,
        and project co-ordinates into normalised device coordinates measured from -1 to 1 in x and y,
        and 0 to 1 in z, where
        - \f$+x\f$ is right
        - \f$+y\f$ is up
        - \f$+z\f$ is away

        At render time, the camera will be rendering to a
        Viewport which will translate these parametric co-ordinates into real screen
        co-ordinates. Obviously it is advisable that the viewport has the same
        aspect ratio as the camera to avoid distortion (unless you want it!).
    */
    class Camera : public Frustum
    {
    public:
        /** Listener interface so you can be notified of Camera events. 
        */
        class Listener 
        {
        public:
            Listener() {}
            virtual ~Listener() {}

            /// Called prior to the scene being rendered with this camera
            virtual void cameraPreRenderScene(Camera* cam)
                        { (void)cam; }

            /// Called after the scene has been rendered with this camera
            virtual void cameraPostRenderScene(Camera* cam)
                        { (void)cam; }

            /// Called when the camera is being destroyed
            virtual void cameraDestroyed(Camera* cam)
                        { (void)cam; }

        };
    private:
        /// Is viewing window used.
        bool mWindowSet;
        /// Was viewing window changed.
        mutable bool mRecalcWindow;

        /** Whether aspect ratio will automatically be recalculated
            when a viewport changes its size
        */
        bool mAutoAspectRatio;
        /// Whether or not the rendering distance of objects should take effect for this camera
        bool mUseRenderingDistance;
        /// Whether or not the minimum display size of objects should take effect for this camera
        bool mUseMinPixelSize;

        /// Derived orientation/position of the camera, including reflection
        mutable Quaternion mDerivedOrientation;
        mutable Vector3 mDerivedPosition;

        /// Stored number of visible faces in the last render
        unsigned int mVisFacesLastRender;

        /// Stored number of visible batches in the last render
        unsigned int mVisBatchesLastRender;

        /// Shared class-level name for Movable type
        static String msMovableType;

        /// Scene LOD factor used to adjust overall LOD
        Real mSceneLodFactor;
        /// Inverted scene LOD factor, can be used by Renderables to adjust their LOD
        Real mSceneLodFactorInv;


        /** Viewing window. 
        @remarks
        Generalize camera class for the case, when viewing frustum doesn't cover all viewport.
        */
        Real mWLeft, mWTop, mWRight, mWBottom;
        /// Windowed viewport clip planes 
        mutable std::vector<Plane> mWindowClipPlanes;
        /// The last viewport to be added using this camera
        Viewport* mLastViewport;
        /// Custom culling frustum
        Frustum *mCullFrustum;
        /// Camera to use for LOD calculation
        const Camera* mLodCamera;

        typedef std::vector<Listener*> ListenerList;
        ListenerList mListeners;
        /// @see Camera::getPixelDisplayRatio
        Real mPixelDisplayRatio;

        SortMode mSortMode;
        /// Rendering type
        PolygonMode mSceneDetail;

        // Internal functions for calcs
        bool isViewOutOfDate(void) const;
        /// Signal to update frustum information.
        void invalidateFrustum(void) const;
        /// Signal to update view information.
        void invalidateView(void) const;


        /** Do actual window setting, using parameters set in SetWindow call
        @remarks
            The method will called on demand.
        */
        virtual void setWindowImpl(void) const;

        /** Helper function for forwardIntersect that intersects rays with canonical plane */
        virtual std::vector<Vector4> getRayForwardIntersect(const Vector3& anchor, const Vector3 *dir, Real planeOffset) const;

    public:
        /** Standard constructor.
        */
        Camera( const String& name, SceneManager* sm);

        /** Standard destructor.
        */
        virtual ~Camera();

        /// Add a listener to this camera
        virtual void addListener(Listener* l);
        /// Remove a listener to this camera
        virtual void removeListener(Listener* l);

        /** Returns a pointer to the SceneManager this camera is rendering through.
        */
        SceneManager* getSceneManager(void) const;

        /** Sets the level of rendering detail required from this camera.
        @remarks
            Each camera is set to render at full detail by default, that is
            with full texturing, lighting etc. This method lets you change
            that behaviour, allowing you to make the camera just render a
            wireframe view, for example.
        */
        void setPolygonMode(PolygonMode sd);

        /** Retrieves the level of detail that the camera will render.
        */
        PolygonMode getPolygonMode(void) const;

        /** Tells the Camera to contact the SceneManager to render from it's viewpoint.
        @param vp The viewport to render to
        */
        void _renderScene(Viewport *vp);

        /** Function for outputting to a stream.
        */
        friend std::ostream& operator<<(std::ostream& o, const Camera& c);

        /** Internal method to notify camera of the visible faces in the last render.
        */
        void _notifyRenderedFaces(unsigned int numfaces);

        /** Internal method to notify camera of the visible batches in the last render.
        */
        void _notifyRenderedBatches(unsigned int numbatches);

        /** Internal method to retrieve the number of visible faces in the last render.
        */
        unsigned int _getNumRenderedFaces(void) const;

        /** Internal method to retrieve the number of visible batches in the last render.
        */
        unsigned int _getNumRenderedBatches(void) const;

        /** Gets the derived orientation of the camera, including any
            rotation inherited from a node attachment and reflection matrix. */
        const Quaternion& getDerivedOrientation(void) const;
        /** Gets the derived position of the camera, including any
            translation inherited from a node attachment and reflection matrix. */
        const Vector3& getDerivedPosition(void) const;
        /** Gets the derived direction vector of the camera, including any
            rotation inherited from a node attachment and reflection matrix. */
        Vector3 getDerivedDirection(void) const;
        /** Gets the derived up vector of the camera, including any
            rotation inherited from a node attachment and reflection matrix. */
        Vector3 getDerivedUp(void) const;
        /** Gets the derived right vector of the camera, including any
            rotation inherited from a node attachment and reflection matrix. */
        Vector3 getDerivedRight(void) const;

        /** Gets the real world orientation of the camera, including any
            rotation inherited from a node attachment */
        const Quaternion& getRealOrientation(void) const;
        /** Gets the real world position of the camera, including any
            translation inherited from a node attachment. */
        const Vector3& getRealPosition(void) const;
        /** Gets the real world direction vector of the camera, including any
            rotation inherited from a node attachment. */
        Vector3 getRealDirection(void) const;
        /** Gets the real world up vector of the camera, including any
            rotation inherited from a node attachment. */
        Vector3 getRealUp(void) const;
        /** Gets the real world right vector of the camera, including any
            rotation inherited from a node attachment. */
        Vector3 getRealRight(void) const;

        const String& getMovableType(void) const override;

        /** Sets the level-of-detail factor for this Camera.
        @remarks
            This method can be used to influence the overall level of detail of the scenes 
            rendered using this camera. Various elements of the scene have level-of-detail
            reductions to improve rendering speed at distance; this method allows you 
            to hint to those elements that you would like to adjust the level of detail that
            they would normally use (up or down). 
        @par
            The most common use for this method is to reduce the overall level of detail used
            for a secondary camera used for sub viewports like rear-view mirrors etc.
            Note that scene elements are at liberty to ignore this setting if they choose,
            this is merely a hint.
        @param factor The factor to apply to the usual level of detail calculation. Higher
            values increase the detail, so 2.0 doubles the normal detail and 0.5 halves it.
        */
        void setLodBias(Real factor = 1.0);

        /** Returns the level-of-detail bias factor currently applied to this camera. 
        @remarks
            See Camera::setLodBias for more details.
        */
        Real getLodBias(void) const;

        /** Set a pointer to the camera which should be used to determine
            LOD settings. 
        @remarks
            Sometimes you don't want the LOD of a render to be based on the camera
            that's doing the rendering, you want it to be based on a different
            camera. A good example is when rendering shadow maps, since they will 
            be viewed from the perspective of another camera. Therefore this method
            lets you associate a different camera instance to use to determine the LOD.
        @par
            To revert the camera to determining LOD based on itself, call this method with 
            a pointer to itself. 
        */
        virtual void setLodCamera(const Camera* lodCam);

        /** Get a pointer to the camera which should be used to determine 
            LOD settings. 
        @remarks
            If setLodCamera hasn't been called with a different camera, this
            method will return 'this'. 
        */
        virtual const Camera* getLodCamera() const;


        /** Gets a world space ray as cast from the camera through a viewport position.
        @param screenx, screeny The x and y position at which the ray should intersect the viewport,
            in normalised screen coordinates [0,1]
        */
        Ray getCameraToViewportRay(Real screenx, Real screeny) const;
        /** Gets a world space ray as cast from the camera through a viewport position.
        @param screenx, screeny The x and y position at which the ray should intersect the viewport, 
            in normalised screen coordinates [0,1]
        @param outRay Ray instance to populate with result
        */
        void getCameraToViewportRay(Real screenx, Real screeny, Ray* outRay) const;

        /** Gets a world-space list of planes enclosing a volume based on a viewport
            rectangle. 
        @remarks
            Can be useful for populating a PlaneBoundedVolumeListSceneQuery, e.g. 
            for a rubber-band selection. 
        @param screenLeft, screenTop, screenRight, screenBottom The bounds of the
            on-screen rectangle, expressed in normalised screen coordinates [0,1]
        @param includeFarPlane If true, the volume is truncated by the camera far plane, 
            by default it is left open-ended
        */
        PlaneBoundedVolume getCameraToViewportBoxVolume(Real screenLeft, 
            Real screenTop, Real screenRight, Real screenBottom, bool includeFarPlane = false);

        /// @overload
        void getCameraToViewportBoxVolume(Real screenLeft, 
            Real screenTop, Real screenRight, Real screenBottom, 
            PlaneBoundedVolume* outVolume, bool includeFarPlane = false);

        /** Internal method for OGRE to use for LOD calculations. */
        Real _getLodBiasInverse(void) const;

        /** Sets the viewing window inside of viewport.
        @remarks
            This method can be used to set a subset of the viewport as the rendering
            target. 
        @param left Relative to Viewport - 0 corresponds to left edge, 1 - to right edge (default - 0).
        @param top Relative to Viewport - 0 corresponds to top edge, 1 - to bottom edge (default - 0).
        @param right Relative to Viewport - 0 corresponds to left edge, 1 - to right edge (default - 1).
        @param bottom Relative to Viewport - 0 corresponds to top edge, 1 - to bottom edge (default - 1).
        */
        virtual void setWindow (Real left, Real top, Real right, Real bottom);
        /// Cancel view window.
        virtual void resetWindow (void);
        /// Returns if a viewport window is being used
        virtual bool isWindowSet(void) const { return mWindowSet; }
        /// Gets the window clip planes, only applicable if isWindowSet == true
        const std::vector<Plane>& getWindowPlanes(void) const;

        Real getBoundingRadius(void) const override;
        
        /** Get the last viewport which was attached to this camera. 
        @note This is not guaranteed to be the only viewport which is
            using this camera, just the last once which was created referring
            to it.
        */
        Viewport* getViewport(void) const {return mLastViewport;}
        /** Notifies this camera that a viewport is using it.*/
        void _notifyViewport(Viewport* viewport) {mLastViewport = viewport;}

        /** If set to true a viewport that owns this frustum will be able to 
            recalculate the aspect ratio whenever the frustum is resized.
        @remarks
            You should set this to true only if the frustum / camera is used by 
            one viewport at the same time. Otherwise the aspect ratio for other 
            viewports may be wrong.
        */    
        void setAutoAspectRatio(bool autoratio);

        /** Retrieves if AutoAspectRatio is currently set or not
        */
        bool getAutoAspectRatio(void) const;

        /** Tells the camera to use a separate Frustum instance to perform culling.
        @remarks
            By calling this method, you can tell the camera to perform culling
            against a different frustum to it's own. This is mostly useful for
            debug cameras that allow you to show the culling behaviour of another
            camera, or a manual frustum instance. 
        @param frustum Pointer to a frustum to use; this can either be a manual
            Frustum instance (which you can attach to scene nodes like any other
            MovableObject), or another camera. If you pass 0 to this method it
            reverts the camera to normal behaviour.
        */
        void setCullingFrustum(Frustum* frustum) { mCullFrustum = frustum; }
        /** Returns the custom culling frustum in use. */
        Frustum* getCullingFrustum(void) const { return mCullFrustum; }

        /** Forward projects frustum rays to find forward intersection with plane.
        @remarks
            Forward projection may lead to intersections at infinity.
        */
        virtual void forwardIntersect(const Plane& worldPlane, std::vector<Vector4>* intersect3d) const;

        /// @copydoc Frustum::isVisible(const AxisAlignedBox&, FrustumPlane*) const
        bool isVisible(const AxisAlignedBox& bound, FrustumPlane* culledBy = 0) const;
        /// @copydoc Frustum::isVisible(const Sphere&, FrustumPlane*) const
        bool isVisible(const Sphere& bound, FrustumPlane* culledBy = 0) const;
        /// @copydoc Frustum::isVisible(const Vector3&, FrustumPlane*) const
        bool isVisible(const Vector3& vert, FrustumPlane* culledBy = 0) const;
        /// @copydoc Frustum::getWorldSpaceCorners
        const Corners& getWorldSpaceCorners(void) const;
        /// @copydoc Frustum::getFrustumPlane
        const Plane& getFrustumPlane( unsigned short plane ) const;
        /// @copydoc Frustum::projectSphere
        bool projectSphere(const Sphere& sphere, 
            Real* left, Real* top, Real* right, Real* bottom) const;
        /// @copydoc Frustum::getNearClipDistance
        Real getNearClipDistance(void) const;
        /// @copydoc Frustum::getFarClipDistance
        Real getFarClipDistance(void) const;
        /// @copydoc Frustum::getViewMatrix
        const Affine3& getViewMatrix(void) const;
        /** Specialised version of getViewMatrix allowing caller to differentiate
            whether the custom culling frustum should be allowed or not. 
        @remarks
            The default behaviour of the standard getViewMatrix is to delegate to 
            the alternate culling frustum, if it is set. This is expected when 
            performing CPU calculations, but the final rendering must be performed
            using the real view matrix in order to display the correct debug view.
        */
        const Affine3& getViewMatrix(bool ownFrustumOnly) const;
        /** Set whether this camera should use the 'rendering distance' on
            objects to exclude distant objects from the final image. The
            default behaviour is to use it.
        @param use True to use the rendering distance, false not to.
        */
        virtual void setUseRenderingDistance(bool use) { mUseRenderingDistance = use; }
        /** Get whether this camera should use the 'rendering distance' on
            objects to exclude distant objects from the final image.
        */
        virtual bool getUseRenderingDistance(void) const { return mUseRenderingDistance; }

        /** Synchronise core camera settings with another. 
        @remarks
            Copies the position, orientation, clip distances, projection type, 
            FOV, focal length and aspect ratio from another camera. Other settings like query flags, 
            reflection etc are preserved.
        */
        virtual void synchroniseBaseSettingsWith(const Camera* cam);

        /** @brief Sets whether to use min display size calculations.
            When active, objects that derive from MovableObject whose size on the screen is less then a MovableObject::mMinPixelSize will not
            be rendered.
        */
        void setUseMinPixelSize(bool enable) { mUseMinPixelSize = enable; }
        /** Returns whether to use min display size calculations 
        @see Camera::setUseMinDisplaySize
        */
        bool getUseMinPixelSize() const { return mUseMinPixelSize; }

        /** Returns an estimated ratio between a pixel and the display area it represents.
            For orthographic cameras this function returns the amount of meters covered by
            a single pixel along the vertical axis. For perspective cameras the value
            returned is the amount of meters covered by a single pixel per meter distance 
            from the camera.
        @note
            This parameter is calculated just before the camera is rendered
        @note
            This parameter is used in min display size calculations.
        */
        Real getPixelDisplayRatio() const { return mPixelDisplayRatio; }

        /// Set the function used to compute the camera-distance for sorting Renderables
        void setSortMode(SortMode sm) { mSortMode = sm; }
        /// get the currently used @ref SortMode
        SortMode getSortMode() const { return mSortMode; }
    };
    /** @} */
    /** @} */

} // namespace Ogre
