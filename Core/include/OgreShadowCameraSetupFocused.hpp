/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd
Copyright (c) 2006 Matthias Fink, netAllied GmbH <matthias.fink@web.de>

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

#include <cstddef>

export module Ogre.Core:ShadowCameraSetupFocused;

export import :AxisAlignedBox;
export import :ConvexBody;
export import :Light;
export import :Matrix4;
export import :Polygon;
export import :Prerequisites;
export import :SceneManager;
export import :SceneNode;
export import :ShadowCameraSetup;
export import :SharedPtr;

export import <memory>;

export
namespace Ogre {
class Camera;
class Frustum;
class Viewport;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */
    /** Implements the uniform shadow mapping algorithm in focused mode.
    @remarks
        Differs from the default shadow mapping projection in that it focuses the
        shadow map on the visible areas of the scene. This results in better
        shadow map texel usage, at the expense of some 'swimming' of the shadow
        texture on receivers as the basis is constantly being reevaluated.
    @note
        Original implementation by Matthias Fink <matthias.fink@web.de>, 2006.
    */
    class FocusedShadowCameraSetup : public ShadowCameraSetup
    {
        /** Temporary preallocated frustum to set up a projection matrix in
            calculateShadowMappingMatrix().
        */
        std::unique_ptr<Frustum> mTempFrustum;
        /** Temporary preallocated camera to set up a light frustum for clipping in FocusedShadowCameraSetup::calculateB.
        */
        SceneNode mLightFrustumCameraNode;
        std::unique_ptr<Camera> mLightFrustumCamera;
        // Persistent calculations to prevent reallocation
        mutable ConvexBody mBodyB;
        /// Use tighter focus region?
        bool mUseAggressiveRegion;
    protected:
        /** Transform to or from light space as defined by Wimmer et al.
        @remarks
            Point and spot lights need to be converted to directional lights to enable a 1:1 
            light mapping. Otherwise a directional light may become a point light or a point 
            sink (opposite of a light source) or point/spot lights may become directional lights
            or light sinks. The light direction is always -y.
        */
        static const Matrix4 msNormalToLightSpace;
        static const Matrix4 msLightSpaceToNormal;

        mutable bool mLightFrustumCameraCalculated{false};

        /** Internal class holding a point list representation of a convex body.
        */
        class PointListBody
        {
            Polygon::VertexList mBodyPoints;
            AxisAlignedBox      mAAB;

        public:
            PointListBody();
            PointListBody(const ConvexBody& body);
            ~PointListBody();

            /** Merges a second PointListBody into this one.
            */
            void merge(const PointListBody& plb);

            /** Builds a point list body from a 'real' body.
            @remarks
                Inserts all vertices from a body into the point list with or without adding duplicate vertices.
            */
            void build(const ConvexBody& body, bool filterDuplicates = true);

            /** Builds a PointListBody from a Body and includes all the space in a given direction.
            @remarks
                Intersects the bounding box with a ray from each available point of the body with the given
                direction. Base and intersection points are stored in a PointListBody structure.
            @note
                Duplicate vertices are not filtered.
            @note
                Body is not checked for correctness.
            */
            void buildAndIncludeDirection(const ConvexBody& body, 
                Real extrudeDist, const Vector3& dir);

            /** Returns the bounding box representation.
            */
            [[nodiscard]] auto getAAB() const noexcept -> const AxisAlignedBox&;   

            /** Adds a specific point to the body list.
            */
            void addPoint(const Vector3& point);

            /** Adds all points of an AAB.
            */
            void addAAB(const AxisAlignedBox& aab);

            /** Returns a point.
            */
            [[nodiscard]] auto getPoint(size_t cnt) const -> const Vector3&;

            /** Returns the point count.
            */
            [[nodiscard]] auto getPointCount() const -> size_t;

            /** Resets the body.
            */
            void reset();

        };

        // Persistent calculations to prevent reallocation
        mutable PointListBody mPointListBodyB;
        mutable PointListBody mPointListBodyLVS;

    protected:
        /** Calculates the standard shadow mapping matrix.
        @remarks
            Provides the view and projection matrix for standard shadow mapping.
        @note
            You can choose which things you want to have: view matrix and/or projection 
            matrix and/or shadow camera. Passing a NULL value as parameter ignores the
            generation of this specific value.
        @param sm
            Scene manager.
        @param cam
            Currently active camera.
        @param light
            Currently active light.
        @param out_view
            Calculated uniform view shadow mapping matrix (may be @c NULL).
        @param out_proj
            Calculated uniform projection shadow mapping matrix (may be @c NULL).
        @param out_cam
            Calculated uniform shadow camera (may be @c NULL).
        */
        void calculateShadowMappingMatrix(const SceneManager& sm, const Camera& cam, 
            const Light& light, Affine3 *out_view,
            Matrix4 *out_proj, Frustum *out_cam) const;

        /** Calculates the intersection bodyB.
        @remarks
            The intersection bodyB consists of the concatenation the cam frustum clipped 
            by the scene bounding box followed by a convex hullification with the light's 
            position and the clipping with the scene bounding box and the light frustum:
            ((V \cap S) + l) \cap S \cap L (\cap: convex intersection, +: convex hull 
            operation).
            For directional lights the bodyB is assembled out of the camera frustum 
            clipped by the scene bounding box followed by the extrusion of all available 
            bodyB points towards the negative light direction. The rays are intersected 
            by a maximum bounding box and added to the bodyB points to form the final 
            intersection bodyB point list.
        @param sm
            Scene manager.
        @param cam
            Currently active camera.
        @param light
            Currently active light.
        @param sceneBB
            Scene bounding box for clipping operations.
        @param receiverBB
            Bounding information for just the receivers.
        @param out_bodyB
            Final intersection bodyB point list.
        */
        void calculateB(const SceneManager& sm, const Camera& cam, const Light& light, 
            const AxisAlignedBox& sceneBB, const AxisAlignedBox& receiverBB, PointListBody *out_bodyB) const;

        /** Calculates the bodyLVS.
        @remarks
            Calculates the bodyLVS which consists of the convex intersection operation 
            affecting the light frustum, the view frustum, and the current scene bounding
            box is used to find suitable positions in the viewer's frustum to build the 
            rotation matrix L_r. This matrix is applied after the projection matrix L_p to 
            avoid an accidental flip of the frustum orientation for views tilted with 
            respect to the shadow map.
        @param cam
            Current viewer camera.
        @param light
            Current light.
        @param sceneBB
            Holds all potential occluders / receivers as one single bounding box
            of the currently active scene node.
        @param out_LVS
            Intersection body LVS (world coordinates).
        */
        void calculateLVS(const SceneManager& sm, const Camera& cam, const Light& light,
            const AxisAlignedBox& sceneBB, PointListBody *out_LVS) const;

        /** Returns the projection view direction.
        @remarks
            After the matrix L_p is applied the orientation of the light space may tilt for
            non-identity projections. To prevent a false shadow cast the real view direction
            is evaluated and applied to the light matrix L.
        @param lightSpace
            Matrix of the light space transformation.
        @param cam
            Current viewer camera.
        @param bodyLVS
            Intersection body LVS (relevant space in front of the camera).
        */
        auto getLSProjViewDir(const Matrix4& lightSpace, const Camera& cam, 
            const PointListBody& bodyLVS) const -> Vector3;

        /** Returns a valid near-point seen by the camera.
        @remarks
            Returns a point that is situated near the camera by analyzing the bodyLVS that
            contains all the relevant scene space in front of the light and the camera in
            a point list array. The view matrix is relevant because the nearest point in
            front of the camera should be determined.
        @param viewMatrix
            View matrix of the current camera.
        @param bodyLVS
            Intersection body LVS (relevant space in front of the camera).
        */
        auto getNearCameraPoint_ws(const Affine3& viewMatrix,
            const PointListBody& bodyLVS) const -> Vector3;

        /** Transforms a given body to the unit cube (-1,-1,-1) / (+1,+1,+1) with a specific 
            shadow matrix enabled.
        @remarks
            Transforms a given point list body object with the matrix m and then maps its
            extends to a (-1,-1,-1) / (+1,+1,+1) unit cube.
        @param m
            Transformation matrix applied on the point list body.
        @param body 
            Contains the points of the extends of all valid scene elements which 
            are mapped to the unit cube.
        */
        auto transformToUnitCube(const Matrix4& m, const PointListBody& body) const -> Matrix4;
    public:
        /// @deprecated use create()
        FocusedShadowCameraSetup(bool useAggressiveRegion = true);

        ~FocusedShadowCameraSetup() override;

        /** Create an instance

            There are 2 approaches that can  be used to define the focus region,
            the more aggressive way introduced by Wimmer et al, or the original
            way as described in Stamminger et al. Wimmer et al's way tends to
            come up with a tighter focus region but in rare cases (mostly highly
            glancing angles) can cause some shadow casters to be clipped
            incorrectly. By default the more aggressive approach is used since it
            leads to significantly better results in most cases, but if you experience
            clipping issues, you can use the less aggressive version.
        @param useAggressiveRegion
            True to use the more aggressive approach, false otherwise.
         */
        static auto create(bool useAggressiveRegion = true) -> ShadowCameraSetupPtr
        {
            return std::make_shared<FocusedShadowCameraSetup>(useAggressiveRegion);
        }

        /** Returns a uniform shadow camera with a focused view.
        */
        void getShadowCamera(const SceneManager *sm, const Camera *cam,
            const Viewport *vp, const Light *light, Camera *texCam, size_t iteration) const override;

        /** Sets whether or not to use the more aggressive approach to deciding on
            the focus region or not.
        */
        void setUseAggressiveFocusRegion(bool aggressive) { mUseAggressiveRegion = aggressive; }

        auto getUseAggressiveFocusRegion() const noexcept -> bool { return mUseAggressiveRegion; }

    };

    /** @} */
    /** @} */

} // namespace Ogre
