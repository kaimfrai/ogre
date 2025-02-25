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
module Ogre.Core;

import :Camera;
import :Exception;
import :Frustum;
import :Light;
import :Math;
import :Matrix3;
import :Node;
import :Plane;
import :Ray;
import :ShadowCameraSetupFocused;
import :Vector;

import <algorithm>;
import <vector>;

namespace Ogre
{

    /** transform from normal to light space */
    const Matrix4 FocusedShadowCameraSetup::msNormalToLightSpace{
        1,  0,  0,  0,      // x
        0,  0, -1,  0,      // y
        0,  1,  0,  0,      // z
        0,  0,  0,  1}; // w
    /** transform  from light to normal space */
    const Matrix4 FocusedShadowCameraSetup::msLightSpaceToNormal{
        1,  0,  0,  0,      // x
        0,  0,  1,  0,      // y
        0, -1,  0,  0,      // z
        0,  0,  0,  1}; // w

    /// Builds a standard view matrix out of a given position, direction and up vector.
    static auto buildViewMatrix(const Vector3& pos, const Vector3& dir, const Vector3& up) -> Affine3
    {
        Matrix3 Rt = Math::lookRotation(-dir, up).transpose();
        Matrix4 m;
        m = Rt;
        m.setTrans(-Rt * pos);
        return Affine3::FromMatrix4(m); // make sure projective part is identity
    }

    FocusedShadowCameraSetup::FocusedShadowCameraSetup(bool useAggressiveRegion)
        : mTempFrustum(new Frustum())
        , mLightFrustumCameraNode(nullptr)
        , mLightFrustumCamera(new Camera("TEMP LIGHT INTERSECT CAM", nullptr))
        , mUseAggressiveRegion(useAggressiveRegion)
         
    {
        mLightFrustumCamera->_notifyAttached(&mLightFrustumCameraNode);
        mTempFrustum->setProjectionType(ProjectionType::PERSPECTIVE);
    }

    FocusedShadowCameraSetup::~FocusedShadowCameraSetup() = default;

    //-----------------------------------------------------------------------
    void FocusedShadowCameraSetup::calculateShadowMappingMatrix(const SceneManager& sm,
        const Camera& cam, const Light& light, Affine3 *out_view, Matrix4 *out_proj,
        Frustum *out_cam) const
    {
        // get the shadow far distance
        Real shadowDist = light.getShadowFarDistance();
        if (!shadowDist)
        {
            // need a shadow distance, make one up
            shadowDist = cam.getNearClipDistance() * 3000;
        }
        Real shadowOffset = shadowDist * sm.getShadowDirLightTextureOffset();

        if (out_cam)
        {
            // common options
            out_cam->setNearClipDistance(light._deriveShadowNearClipDistance(&cam));
            out_cam->setFarClipDistance(light._deriveShadowFarClipDistance());
        }

        if (light.getType() == Light::LightTypes::DIRECTIONAL)
        {
            // generate view matrix if requested
            if (out_view != nullptr)
            {
                Vector3 pos;
                if (sm.getCameraRelativeRendering())
                {
                    pos = Vector3::ZERO;
                }
                else
                {
                    pos = cam.getDerivedPosition();
                }
                *out_view = buildViewMatrix(pos, 
                    light.getDerivedDirection(), 
                    cam.getDerivedUp());
            }

            // generate projection matrix if requested
            if (out_proj != nullptr)
            {
                *out_proj = Affine3::getScale(1, 1, -1);
                //*out_proj = Matrix4::IDENTITY;
            }

            // set up camera if requested
            if (out_cam != nullptr)
            {
                out_cam->setProjectionType(ProjectionType::ORTHOGRAPHIC);
                out_cam->getParentSceneNode()->setDirection(light.getDerivedDirection(), Node::TransformSpace::WORLD);
                out_cam->getParentSceneNode()->setPosition(cam.getDerivedPosition());
                out_cam->setFOVy(Degree{90});
            }
        }
        else if (light.getType() == Light::LightTypes::POINT)
        {
            // target analogue to the default shadow textures
            // Calculate look at position
            // We want to look at a spot shadowOffset away from near plane
            // 0.5 is a little too close for angles
            Vector3 target = cam.getDerivedPosition() + 
                (cam.getDerivedDirection() * shadowOffset);
            Vector3 lightDir = target - light.getDerivedPosition();
            lightDir.normalise();

            // generate view matrix if requested
            if (out_view != nullptr)
            {
                *out_view = buildViewMatrix(light.getDerivedPosition(), 
                    lightDir, 
                    cam.getDerivedUp());
            }

            // generate projection matrix if requested
            if (out_proj != nullptr)
            {
                // set FOV to 120 degrees
                mTempFrustum->setFOVy(Degree{120});

                mTempFrustum->setNearClipDistance(light._deriveShadowNearClipDistance(&cam));
                mTempFrustum->setFarClipDistance(light._deriveShadowFarClipDistance());

                *out_proj = mTempFrustum->getProjectionMatrix();
            }

            // set up camera if requested
            if (out_cam != nullptr)
            {
                out_cam->setProjectionType(ProjectionType::PERSPECTIVE);
                out_cam->getParentSceneNode()->setDirection(lightDir, Node::TransformSpace::WORLD);
                out_cam->getParentSceneNode()->setPosition(light.getDerivedPosition());
                out_cam->setFOVy(Degree{120});
            }
        }
        else if (light.getType() == Light::LightTypes::SPOTLIGHT)
        {
            // generate view matrix if requested
            if (out_view != nullptr)
            {
                *out_view = buildViewMatrix(light.getDerivedPosition(), 
                    light.getDerivedDirection(), 
                    cam.getDerivedUp());
            }

            // generate projection matrix if requested
            if (out_proj != nullptr)
            {
                // set FOV slightly larger than spotlight range
                mTempFrustum->setFOVy(Ogre::Math::Clamp<Radian>(light.getSpotlightOuterAngle() * 1.2, Radian{0}, Radian{Math::PI/2.0f}));

                mTempFrustum->setNearClipDistance(light._deriveShadowNearClipDistance(&cam));
                mTempFrustum->setFarClipDistance(light._deriveShadowFarClipDistance());

                *out_proj = mTempFrustum->getProjectionMatrix();
            }

            // set up camera if requested
            if (out_cam != nullptr)
            {
                out_cam->setProjectionType(ProjectionType::PERSPECTIVE);
                out_cam->getParentSceneNode()->setDirection(light.getDerivedDirection(), Node::TransformSpace::WORLD);
                out_cam->getParentSceneNode()->setPosition(light.getDerivedPosition());
                out_cam->setFOVy(Ogre::Math::Clamp<Radian>(light.getSpotlightOuterAngle() * 1.2, Radian{0}, Radian{Math::PI/2.0f}));
            }
        }
    }
    //-----------------------------------------------------------------------
    void FocusedShadowCameraSetup::calculateB(const SceneManager& sm, const Camera& cam, 
        const Light& light, const AxisAlignedBox& sceneBB, const AxisAlignedBox& receiverBB, 
        PointListBody *out_bodyB) const
    {
        OgreAssert(out_bodyB != nullptr, "bodyB vertex list is NULL");

        /// perform convex intersection of the form B = ((V \cap S) + l) \cap S \cap L

        // get V
        mBodyB.define(cam);

        if (light.getType() != Light::LightTypes::DIRECTIONAL)
        {
            // clip bodyB with sceneBB
            /* Note, Matthias' original code states this:
            "The procedure ((V \cap S) + l) \cap S \cap L (Wimmer et al.) leads in some 
            cases to disappearing shadows. Valid parts of the scene are clipped, so shadows 
            are partly incomplete. The cause may be the transformation into light space that 
            is only done for the corner points which may not contain the whole scene afterwards 
            any more. So we fall back to the method of Stamminger et al. (V + l) \cap S \cap L 
            which does not show these anomalies."

            .. leading to the commenting out of the below clip. However, ift makes a major
            difference to the quality of the focus, and so far I haven't noticed
            the clipping issues described. Intuitively I would have thought that
            any clipping issue would be due to the findCastersForLight not being
            quite right, since if the sceneBB includes those there is no reason for
            this clip instruction to omit a genuine shadow caster.

            I have made this a user option since the quality effect is major and
            the clipping problem only seems to occur in some specific cases. 
            */
            if (mUseAggressiveRegion)
                mBodyB.clip(sceneBB);

            // form a convex hull of bodyB with the light position
            mBodyB.extend(light.getDerivedPosition());

            // clip bodyB with sceneBB
            mBodyB.clip(sceneBB);

            // clip with the light frustum
            // set up light camera to clip with the resulting frustum planes
            if (!mLightFrustumCameraCalculated)
            {
                calculateShadowMappingMatrix(sm, cam, light, nullptr, nullptr, mLightFrustumCamera.get());
                mLightFrustumCameraCalculated = true;
            }
            mBodyB.clip(*mLightFrustumCamera);

            // extract bodyB vertices
            out_bodyB->build(mBodyB);

        }
        else
        {
            // For directional lights, all we care about is projecting the receivers
            // backwards towards the light, clipped by the camera region
            mBodyB.clip(receiverBB.intersection(sceneBB));

            // Also clip based on shadow far distance if appropriate
            Real farDist = light.getShadowFarDistance();
            if (farDist)
            {
                Vector3 pointOnPlane = cam.getDerivedPosition() + 
                    (cam.getDerivedDirection() * farDist);
                Plane p = Plane::Redefine(cam.getDerivedDirection(), pointOnPlane);
                mBodyB.clip(p);
            }

            // Extrude the intersection bodyB into the inverted light direction and store 
            // the info in the point list.
            // Maximum extrusion extent is to the shadow far distance
            out_bodyB->buildAndIncludeDirection(mBodyB, 
                farDist ? farDist : cam.getNearClipDistance() * 3000, 
                -light.getDerivedDirection());
        }
    }

    //-----------------------------------------------------------------------
    void FocusedShadowCameraSetup::calculateLVS(const SceneManager& sm, const Camera& cam, 
        const Light& light, const AxisAlignedBox& sceneBB, PointListBody *out_LVS) const
    {
        ConvexBody bodyLVS;

        // init body with view frustum
        bodyLVS.define(cam);

        // clip the body with the light frustum (point + spot)
        // for a directional light the space of the intersected
        // view frustum and sceneBB is always lighted and in front
        // of the viewer.
        if (light.getType() != Light::LightTypes::DIRECTIONAL)
        {
            // clip with the light frustum
            // set up light camera to clip the resulting frustum
            if (!mLightFrustumCameraCalculated)
            {
                calculateShadowMappingMatrix(sm, cam, light, nullptr, nullptr, mLightFrustumCamera.get());
                mLightFrustumCameraCalculated = true;
            }
            bodyLVS.clip(*mLightFrustumCamera);
        }

        // clip the body with the scene bounding box
        bodyLVS.clip(sceneBB);

        // extract bodyLVS vertices
        out_LVS->build(bodyLVS);
    }
    //-----------------------------------------------------------------------
    auto FocusedShadowCameraSetup::getLSProjViewDir(const Matrix4& lightSpace, 
        const Camera& cam, const PointListBody& bodyLVS) const -> Vector3
    {
        // goal is to construct a view direction
        // because parallel lines are not parallel any more after perspective projection we have to transform
        // a ray to point us the viewing direction

        // fetch a point near the camera
        const Vector3 e_world = getNearCameraPoint_ws(cam.getViewMatrix(), bodyLVS);

        // plus the direction results in a second point
        const Vector3 b_world = e_world + cam.getDerivedDirection();

        // transformation into light space
        const Vector3 e_ls = lightSpace * e_world;
        const Vector3 b_ls = lightSpace * b_world;

        // calculate the projection direction, which is the subtraction of
        // b_ls from e_ls. The y component is set to 0 to project the view
        // direction into the shadow map plane.
        auto projectionDir = b_ls - e_ls;
        projectionDir.y = 0;

        // deal with Y-only vectors
        return Math::RealEqual(projectionDir.length(), 0.0) ? 
            Vector3::NEGATIVE_UNIT_Z : projectionDir.normalisedCopy();
    }
    //-----------------------------------------------------------------------
    auto FocusedShadowCameraSetup::getNearCameraPoint_ws(const Affine3& viewMatrix,
        const PointListBody& bodyLVS) const -> Vector3
    {
        if (bodyLVS.getPointCount() == 0)
            return {0,0,0};

        Vector3 nearEye = viewMatrix * bodyLVS.getPoint(0), // for comparison
            nearWorld = bodyLVS.getPoint(0);                // represents the final point

        // store the vertex with the highest z-value which is the nearest point
        for (size_t i = 1; i < bodyLVS.getPointCount(); ++i)
        {
            const Vector3& vWorld = bodyLVS.getPoint(i);

            // comparison is done from the viewer
            Vector3 vEye = viewMatrix * vWorld;

            if (vEye.z > nearEye.z)
            {
                nearEye     = vEye;
                nearWorld   = vWorld;
            }
        }

        return nearWorld;
    }
    //-----------------------------------------------------------------------
    auto FocusedShadowCameraSetup::transformToUnitCube(const Matrix4& m, 
        const PointListBody& body) const -> Matrix4
    {
        // map the transformed body AAB points to the unit cube (-1/-1/-1) / (+1/+1/+1) corners
        AxisAlignedBox aab_trans;

        for (size_t i = 0; i < body.getPointCount(); ++i)
        {
            aab_trans.merge(m * body.getPoint(i));
        }

        Vector3 vMin, vMax;

        vMin = aab_trans.getMinimum();
        vMax = aab_trans.getMaximum();

        const Vector3 trans{-(vMax.x + vMin.x) / (vMax.x - vMin.x),
            -(vMax.y + vMin.y) / (vMax.y - vMin.y),
            -(vMax.z + vMin.z) / (vMax.z - vMin.z)};

        const Vector3 scale{2 / (vMax.x - vMin.x),
            2 / (vMax.y - vMin.y),
            2 / (vMax.z - vMin.z)};

        Matrix4 mOut{Matrix4::IDENTITY};
        mOut.setTrans(trans);
        mOut.setScale(scale);

        return mOut;
    }
    //-----------------------------------------------------------------------
    void FocusedShadowCameraSetup::getShadowCamera (const SceneManager *sm, const Camera *cam, 
        const Viewport *vp, const Light *light, Camera *texCam, size_t iteration) const
    {
        // check availability - viewport not needed
        OgreAssert(sm != nullptr, "SceneManager is NULL");
        OgreAssert(cam != nullptr, "Camera (viewer) is NULL");
        OgreAssert(light != nullptr, "Light is NULL");
        OgreAssert(texCam != nullptr, "Camera (texture) is NULL");
        mLightFrustumCameraCalculated = false;

        texCam->setNearClipDistance(light->_deriveShadowNearClipDistance(cam));
        texCam->setFarClipDistance(light->_deriveShadowFarClipDistance());

        // calculate standard shadow mapping matrix
        Affine3 LView; Matrix4 LProj;
        calculateShadowMappingMatrix(*sm, *cam, *light, &LView, &LProj, nullptr);

        // build scene bounding box
        const VisibleObjectsBoundsInfo& visInfo = sm->getVisibleObjectsBoundsInfo(texCam);
        AxisAlignedBox sceneBB = visInfo.aabb;
        AxisAlignedBox receiverAABB = sm->getVisibleObjectsBoundsInfo(cam).receiverAabb;
        sceneBB.merge(cam->getDerivedPosition());

        // in case the sceneBB is empty (e.g. nothing visible to the cam) simply
        // return the standard shadow mapping matrix
        if (sceneBB.isNull())
        {
            texCam->setCustomViewMatrix(true, LView);
            texCam->setCustomProjectionMatrix(true, LProj);
            return;
        }

        // calculate the intersection body B
        mPointListBodyB.reset();
        calculateB(*sm, *cam, *light, sceneBB, receiverAABB, &mPointListBodyB);

        // in case the bodyB is empty (e.g. nothing visible to the light or the cam)
        // simply return the standard shadow mapping matrix
        if (mPointListBodyB.getPointCount() == 0)
        {
            texCam->setCustomViewMatrix(true, LView);
            texCam->setCustomProjectionMatrix(true, LProj);
            return;
        }

        // transform to light space: y -> -z, z -> y
        LProj = msNormalToLightSpace * LProj;

        // calculate LVS so it does not need to be calculated twice
        // calculate the body L \cap V \cap S to make sure all returned points are in 
        // front of the camera
        mPointListBodyLVS.reset();
        calculateLVS(*sm, *cam, *light, sceneBB, &mPointListBodyLVS);

        // fetch the viewing direction
        const Vector3 viewDir = getLSProjViewDir(LProj * LView, *cam, mPointListBodyLVS);

        // The light space will be rotated in such a way, that the projected light view 
        // always points upwards, so the up-vector is the y-axis (we already prepared the
        // light space for this usage).The transformation matrix is set up with the
        // following parameters:
        // - position is the origin
        // - the view direction is the calculated viewDir
        // - the up vector is the y-axis
        LProj = Matrix4::FromMatrix3(Math::lookRotation(-viewDir, Vector3::UNIT_Y).transpose()) * LProj;

        // map bodyB to unit cube
        LProj = transformToUnitCube(LProj * LView, mPointListBodyB) * LProj;

        // transform from light space to normal space: y -> z, z -> -y
        LProj = msLightSpaceToNormal * LProj;

        // set the two custom matrices
        texCam->setCustomViewMatrix(true, LView);
        texCam->setCustomProjectionMatrix(true, LProj);
    }

    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //-----------------------------------------------------------------------
    FocusedShadowCameraSetup::PointListBody::PointListBody()
    {
        // Preallocate some space
        mBodyPoints.reserve(12);
    }
    //-----------------------------------------------------------------------
    FocusedShadowCameraSetup::PointListBody::PointListBody(const ConvexBody& body)
    {
        build(body);
    }
    //-----------------------------------------------------------------------
    FocusedShadowCameraSetup::PointListBody::~PointListBody()
    = default;
    //-----------------------------------------------------------------------
    void FocusedShadowCameraSetup::PointListBody::merge(const PointListBody& plb)
    {
        size_t size = plb.getPointCount();
        for (size_t i = 0; i < size; ++i)
        {
            this->addPoint(plb.getPoint(i));
        }
    }
    //-----------------------------------------------------------------------
    void FocusedShadowCameraSetup::PointListBody::build(const ConvexBody& body, bool filterDuplicates)
    {
        // erase list
        mBodyPoints.clear();

        // Try to reserve a representative amount of memory
        mBodyPoints.reserve(body.getPolygonCount() * 6);

        // build new list
        for (size_t i = 0; i < body.getPolygonCount(); ++i)
        {
            for (size_t j = 0; j < body.getVertexCount(i); ++j)
            {
                const Vector3 &vInsert = body.getVertex(i, j);

                // duplicates allowed?
                if (filterDuplicates)
                {
                    bool bPresent = false;

                    for(auto & v : mBodyPoints)
                    {
                        if (vInsert.positionEquals(v))
                        {
                            bPresent = true;
                            break;
                        }
                    }

                    if (bPresent == false)
                    {
                        mBodyPoints.push_back(body.getVertex(i, j));
                    }
                }

                // else insert directly
                else
                {
                    mBodyPoints.push_back(body.getVertex(i, j));
                }
            }
        }

        // update AAB
        // no points altered, so take body AAB
        mAAB = body.getAABB();
    }
    //-----------------------------------------------------------------------
    void FocusedShadowCameraSetup::PointListBody::buildAndIncludeDirection(
        const ConvexBody& body, Real extrudeDist, const Vector3& dir)
    {
        // reset point list
        this->reset();

        // intersect the rays formed by the points in the list with the given direction and
        // insert them into the list

        const size_t polyCount = body.getPolygonCount();
        for (size_t iPoly = 0; iPoly < polyCount; ++iPoly)
        {

            // store the old inserted point and plane info
            // if the currently processed point hits a different plane than the previous point an 
            // intersection point is calculated that lies on the two planes' intersection edge

            // fetch current polygon
            const Polygon& p = body.getPolygon(iPoly);

            size_t pointCount = p.getVertexCount();
            for (size_t iPoint = 0; iPoint < pointCount ; ++iPoint)
            {
                // base point
                const Vector3& pt = p.getVertex(iPoint);

                // add the base point
                this->addPoint(pt);

                // intersection ray
                Ray ray{pt, dir};

                const Vector3 ptIntersect = ray.getPoint(extrudeDist);
                this->addPoint(ptIntersect);

            } // for: polygon point iteration

        } // for: polygon iteration
    }
    //-----------------------------------------------------------------------
    auto FocusedShadowCameraSetup::PointListBody::getAAB() const noexcept -> const AxisAlignedBox&
    {
        return mAAB;
    }
    //-----------------------------------------------------------------------   
    void FocusedShadowCameraSetup::PointListBody::addPoint(const Vector3& point)
    {
        // don't check for doubles, simply add
        mBodyPoints.push_back(point);

        // update AAB
        mAAB.merge(point);
    }
    //-----------------------------------------------------------------------
    void FocusedShadowCameraSetup::PointListBody::addAAB(const AxisAlignedBox& aab)
    {
        const Vector3& min = aab.getMinimum();
        const Vector3& max = aab.getMaximum();

        Vector3 currentVertex = min;
        // min min min
        addPoint(currentVertex);

        // min min max
        currentVertex.z = max.z;
        addPoint(currentVertex);

        // min max max
        currentVertex.y = max.y;
        addPoint(currentVertex);

        // min max min
        currentVertex.z = min.z;
        addPoint(currentVertex);

        // max max min
        currentVertex.x = max.x;
        addPoint(currentVertex);

        // max max max
        currentVertex.z = max.z;
        addPoint(currentVertex);

        // max min max
        currentVertex.y = min.y;
        addPoint(currentVertex);

        // max min min
        currentVertex.z = min.z;
        addPoint(currentVertex); 

    }
    //-----------------------------------------------------------------------   
    auto FocusedShadowCameraSetup::PointListBody::getPoint(size_t cnt) const -> const Vector3&
    {
        OgreAssertDbg(cnt < getPointCount(), "Search position out of range");

        return mBodyPoints[ cnt ];
    }
    //-----------------------------------------------------------------------   
    auto FocusedShadowCameraSetup::PointListBody::getPointCount() const -> size_t
    {
        return mBodyPoints.size();
    }
    //-----------------------------------------------------------------------   
    void FocusedShadowCameraSetup::PointListBody::reset()
    {
        mBodyPoints.clear();
        mAAB.setNull();
    }

}
