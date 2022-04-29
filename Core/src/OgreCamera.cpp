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

#include <cassert>

module Ogre.Core;

import :AxisAlignedBox;
import :Camera;
import :Common;
import :Frustum;
import :Math;
import :Matrix4;
import :Plane;
import :PlaneBoundedVolume;
import :Prerequisites;
import :Quaternion;
import :Ray;
import :SceneManager;
import :Vector;
import :Viewport;

import <algorithm>;
import <memory>;
import <ostream>;
import <string>;

namespace Ogre {
class Sphere;

    String Camera::msMovableType = "Camera";
    //-----------------------------------------------------------------------
    Camera::Camera( const String& name, SceneManager* sm)
        : Frustum(name)
        
    {

        // Reasonable defaults to camera params
        mFOVy = Radian(Math::PI/4.0f);
        mNearDist = 100.0f;
        mFarDist = 100000.0f;
        mAspect = 1.33333333333333f;
        mProjType = PT_PERSPECTIVE;

        invalidateFrustum();
        invalidateView();

        // Init matrices
        mViewMatrix = Affine3::ZERO;
        mProjMatrixRS = Matrix4::ZERO;

        mParentNode = nullptr;

        // no reflection
        mReflect = false;

        mVisible = false;
        mManager = sm;
    }

    //-----------------------------------------------------------------------
    Camera::~Camera()
    {
        ListenerList listenersCopy = mListeners;
        for (auto & i : listenersCopy)
        {
            i->cameraDestroyed(this);
        }
    }
    //-----------------------------------------------------------------------
    auto Camera::getSceneManager() const -> SceneManager*
    {
        return mManager;
    }


    //-----------------------------------------------------------------------
    void Camera::setPolygonMode(PolygonMode sd)
    {
        mSceneDetail = sd;
    }

    //-----------------------------------------------------------------------
    auto Camera::getPolygonMode() const -> PolygonMode
    {
        return mSceneDetail;
    }

    //-----------------------------------------------------------------------
    auto Camera::isViewOutOfDate() const -> bool
    {
        if(Frustum::isViewOutOfDate())
            mRecalcWindow = true;

        // Deriving reflected orientation / position
        if (mRecalcView)
        {
            const auto& mRealOrientation = mLastParentOrientation;
            const auto& mRealPosition = mLastParentPosition;

            if (mReflect)
            {
                // Calculate reflected orientation, use up-vector as fallback axis.
                Vector3 dir = -mRealOrientation.zAxis();
                Vector3 rdir = dir.reflect(mReflectPlane.normal);
                Vector3 up = mRealOrientation.yAxis();
                mDerivedOrientation = dir.getRotationTo(rdir, up) * mRealOrientation;

                // Calculate reflected position.
                mDerivedPosition = mReflectMatrix * mRealPosition;
            }
            else
            {
                mDerivedOrientation = mRealOrientation;
                mDerivedPosition = mRealPosition;
            }
        }

        return mRecalcView;

    }

    // -------------------------------------------------------------------
    void Camera::invalidateView() const
    {
        mRecalcWindow = true;
        Frustum::invalidateView();
    }
    // -------------------------------------------------------------------
    void Camera::invalidateFrustum() const
    {
        mRecalcWindow = true;
        Frustum::invalidateFrustum();
    }
    //-----------------------------------------------------------------------
    void Camera::_renderScene(Viewport *vp)
    {
        //update the pixel display ratio
        if (mProjType == Ogre::PT_PERSPECTIVE)
        {
            mPixelDisplayRatio = (2 * Ogre::Math::Tan(mFOVy * 0.5f)) / vp->getActualHeight();
        }
        else
        {
            mPixelDisplayRatio = -mExtents.height() / vp->getActualHeight();
        }

        //notify prerender scene
        ListenerList listenersCopy = mListeners;
        for (auto & i : listenersCopy)
        {
            i->cameraPreRenderScene(this);
        }

        //render scene
        mManager->_renderScene(this, vp);

        // Listener list may have change
        listenersCopy = mListeners;

        //notify postrender scene
        for (auto & i : listenersCopy)
        {
            i->cameraPostRenderScene(this);
        }
    }
    //---------------------------------------------------------------------
    void Camera::addListener(Listener* l)
    {
        if (std::find(mListeners.begin(), mListeners.end(), l) == mListeners.end())
            mListeners.push_back(l);
    }
    //---------------------------------------------------------------------
    void Camera::removeListener(Listener* l)
    {
        auto i = std::find(mListeners.begin(), mListeners.end(), l);
        if (i != mListeners.end())
            mListeners.erase(i);
    }
    //-----------------------------------------------------------------------
    auto operator<<( std::ostream& o, const Camera& c ) -> std::ostream&
    {
        o << "Camera(Name='" << c.mName << "'";

        o << ", pos=" << c.mLastParentPosition << ", direction=" << -c.mLastParentOrientation.zAxis();

        o << ",near=" << c.mNearDist;
        o << ", far=" << c.mFarDist << ", FOVy=" << c.mFOVy.valueDegrees();
        o << ", aspect=" << c.mAspect << ", ";
        o << ", xoffset=" << c.mFrustumOffset.x << ", yoffset=" << c.mFrustumOffset.y;
        o << ", focalLength=" << c.mFocalLength << ", ";
        o << "NearFrustumPlane=" << c.mFrustumPlanes[FRUSTUM_PLANE_NEAR] << ", ";
        o << "FarFrustumPlane=" << c.mFrustumPlanes[FRUSTUM_PLANE_FAR] << ", ";
        o << "LeftFrustumPlane=" << c.mFrustumPlanes[FRUSTUM_PLANE_LEFT] << ", ";
        o << "RightFrustumPlane=" << c.mFrustumPlanes[FRUSTUM_PLANE_RIGHT] << ", ";
        o << "TopFrustumPlane=" << c.mFrustumPlanes[FRUSTUM_PLANE_TOP] << ", ";
        o << "BottomFrustumPlane=" << c.mFrustumPlanes[FRUSTUM_PLANE_BOTTOM];
        o << ")";

        return o;
    }
    //-----------------------------------------------------------------------
    void Camera::_notifyRenderedFaces(unsigned int numfaces)
    {
        mVisFacesLastRender = numfaces;
    }

    //-----------------------------------------------------------------------
    void Camera::_notifyRenderedBatches(unsigned int numbatches)
    {
        mVisBatchesLastRender = numbatches;
    }

    //-----------------------------------------------------------------------
    auto Camera::_getNumRenderedFaces() const -> unsigned int
    {
        return mVisFacesLastRender;
    }
    //-----------------------------------------------------------------------
    auto Camera::_getNumRenderedBatches() const -> unsigned int
    {
        return mVisBatchesLastRender;
    }
    //-----------------------------------------------------------------------
    auto Camera::getDerivedOrientation() const -> const Quaternion&
    {
        updateView();
        return mDerivedOrientation;
    }
    //-----------------------------------------------------------------------
    auto Camera::getDerivedPosition() const -> const Vector3&
    {
        updateView();
        return mDerivedPosition;
    }
    //-----------------------------------------------------------------------
    auto Camera::getDerivedDirection() const -> Vector3
    {
        // Direction points down -Z
        updateView();
        return -mDerivedOrientation.zAxis();
    }
    //-----------------------------------------------------------------------
    auto Camera::getDerivedUp() const -> Vector3
    {
        updateView();
        return mDerivedOrientation.yAxis();
    }
    //-----------------------------------------------------------------------
    auto Camera::getDerivedRight() const -> Vector3
    {
        updateView();
        return mDerivedOrientation.xAxis();
    }
    //-----------------------------------------------------------------------
    auto Camera::getRealOrientation() const -> const Quaternion&
    {
        updateView();
        return mLastParentOrientation;
    }
    //-----------------------------------------------------------------------
    auto Camera::getRealPosition() const -> const Vector3&
    {
        updateView();
        return mLastParentPosition;
    }
    //-----------------------------------------------------------------------
    auto Camera::getRealDirection() const -> Vector3
    {
        // Direction points down -Z
        updateView();
        return -mLastParentOrientation.zAxis();
    }
    //-----------------------------------------------------------------------
    auto Camera::getRealUp() const -> Vector3
    {
        updateView();
        return mLastParentOrientation.yAxis();
    }
    //-----------------------------------------------------------------------
    auto Camera::getRealRight() const -> Vector3
    {
        updateView();
        return mLastParentOrientation.xAxis();
    }
    //-----------------------------------------------------------------------
    auto Camera::getMovableType() const -> const String&
    {
        return msMovableType;
    }
    //-----------------------------------------------------------------------
    void Camera::setLodBias(Real factor)
    {
        assert(factor > 0.0f && "Bias factor must be > 0!");
        mSceneLodFactor = factor;
        mSceneLodFactorInv = 1.0f / factor;
    }
    //-----------------------------------------------------------------------
    auto Camera::getLodBias() const -> Real
    {
        return mSceneLodFactor;
    }
    //-----------------------------------------------------------------------
    auto Camera::_getLodBiasInverse() const -> Real
    {
        return mSceneLodFactorInv;
    }
    //-----------------------------------------------------------------------
    void Camera::setLodCamera(const Camera* lodCam)
    {
        if (lodCam == this)
            mLodCamera = nullptr;
        else
            mLodCamera = lodCam;
    }
    //---------------------------------------------------------------------
    auto Camera::getLodCamera() const -> const Camera*
    {
        return mLodCamera? mLodCamera : this;
    }
    //-----------------------------------------------------------------------
    auto Camera::getCameraToViewportRay(Real screenX, Real screenY) const -> Ray
    {
        Ray ret;
        getCameraToViewportRay(screenX, screenY, &ret);
        return ret;
    }
    //---------------------------------------------------------------------
    void Camera::getCameraToViewportRay(Real screenX, Real screenY, Ray* outRay) const
    {
        Matrix4 inverseVP = (getProjectionMatrix() * getViewMatrix(true)).inverse();

        Real nx = (2.0f * screenX) - 1.0f;
        Real ny = 1.0f - (2.0f * screenY);
        Vector3 nearPoint(nx, ny, -1.f);
        // Use midPoint rather than far point to avoid issues with infinite projection
        Vector3 midPoint (nx, ny,  0.0f);

        // Get ray origin and ray target on near plane in world space
        Vector3 rayOrigin, rayTarget;
        
        rayOrigin = inverseVP * nearPoint;
        rayTarget = inverseVP * midPoint;

        Vector3 rayDirection = rayTarget - rayOrigin;
        rayDirection.normalise();

        outRay->setOrigin(rayOrigin);
        outRay->setDirection(rayDirection);
    } 
    //---------------------------------------------------------------------
    auto Camera::getCameraToViewportBoxVolume(Real screenLeft, 
        Real screenTop, Real screenRight, Real screenBottom, bool includeFarPlane) -> PlaneBoundedVolume
    {
        PlaneBoundedVolume vol;
        getCameraToViewportBoxVolume(screenLeft, screenTop, screenRight, screenBottom, 
            &vol, includeFarPlane);
        return vol;

    }
    //---------------------------------------------------------------------()
    void Camera::getCameraToViewportBoxVolume(Real screenLeft, 
        Real screenTop, Real screenRight, Real screenBottom, 
        PlaneBoundedVolume* outVolume, bool includeFarPlane)
    {
        outVolume->planes.clear();

        if (mProjType == PT_PERSPECTIVE)
        {

            // Use the corner rays to generate planes
            Ray ul = getCameraToViewportRay(screenLeft, screenTop);
            Ray ur = getCameraToViewportRay(screenRight, screenTop);
            Ray bl = getCameraToViewportRay(screenLeft, screenBottom);
            Ray br = getCameraToViewportRay(screenRight, screenBottom);


            Vector3 normal;
            // top plane
            normal = ul.getDirection().crossProduct(ur.getDirection());
            normal.normalise();
            outVolume->planes.push_back(
                Plane(normal, getDerivedPosition()));

            // right plane
            normal = ur.getDirection().crossProduct(br.getDirection());
            normal.normalise();
            outVolume->planes.push_back(
                Plane(normal, getDerivedPosition()));

            // bottom plane
            normal = br.getDirection().crossProduct(bl.getDirection());
            normal.normalise();
            outVolume->planes.push_back(
                Plane(normal, getDerivedPosition()));

            // left plane
            normal = bl.getDirection().crossProduct(ul.getDirection());
            normal.normalise();
            outVolume->planes.push_back(
                Plane(normal, getDerivedPosition()));

        }
        else
        {
            // ortho planes are parallel to frustum planes

            Ray ul = getCameraToViewportRay(screenLeft, screenTop);
            Ray br = getCameraToViewportRay(screenRight, screenBottom);

            updateFrustumPlanes();
            outVolume->planes.push_back(
                Plane(mFrustumPlanes[FRUSTUM_PLANE_TOP].normal, ul.getOrigin()));
            outVolume->planes.push_back(
                Plane(mFrustumPlanes[FRUSTUM_PLANE_RIGHT].normal, br.getOrigin()));
            outVolume->planes.push_back(
                Plane(mFrustumPlanes[FRUSTUM_PLANE_BOTTOM].normal, br.getOrigin()));
            outVolume->planes.push_back(
                Plane(mFrustumPlanes[FRUSTUM_PLANE_LEFT].normal, ul.getOrigin()));
            

        }

        // near & far plane applicable to both projection types
        outVolume->planes.push_back(getFrustumPlane(FRUSTUM_PLANE_NEAR));
        if (includeFarPlane)
            outVolume->planes.push_back(getFrustumPlane(FRUSTUM_PLANE_FAR));
    }
    // -------------------------------------------------------------------
    void Camera::setWindow (Real Left, Real Top, Real Right, Real Bottom)
    {
        mWLeft = Left;
        mWTop = Top;
        mWRight = Right;
        mWBottom = Bottom;

        mWindowSet = true;
        mRecalcWindow = true;
    }
    // -------------------------------------------------------------------
    void Camera::resetWindow ()
    {
        mWindowSet = false;
    }
    // -------------------------------------------------------------------
    void Camera::setWindowImpl() const
    {
        if (!mWindowSet || !mRecalcWindow)
            return;

        // Calculate general projection parameters
        RealRect vp = calcProjectionParameters();

        Real vpWidth = vp.width();
        Real vpHeight = -vp.height();

        Real wvpLeft   = vp.left + mWLeft * vpWidth;
        Real wvpRight  = vp.left + mWRight * vpWidth;
        Real wvpTop    = vp.top - mWTop * vpHeight;
        Real wvpBottom = vp.top - mWBottom * vpHeight;

        Vector3 vp_ul (wvpLeft, wvpTop, -mNearDist);
        Vector3 vp_ur (wvpRight, wvpTop, -mNearDist);
        Vector3 vp_bl (wvpLeft, wvpBottom, -mNearDist);
        Vector3 vp_br (wvpRight, wvpBottom, -mNearDist);

        Affine3 inv = mViewMatrix.inverse();

        Vector3 vw_ul = inv * vp_ul;
        Vector3 vw_ur = inv * vp_ur;
        Vector3 vw_bl = inv * vp_bl;
        Vector3 vw_br = inv * vp_br;

        mWindowClipPlanes.clear();
        if (mProjType == PT_PERSPECTIVE)
        {
            Vector3 position = getPositionForViewUpdate();
            mWindowClipPlanes.emplace_back(position, vw_bl, vw_ul);
            mWindowClipPlanes.emplace_back(position, vw_ul, vw_ur);
            mWindowClipPlanes.emplace_back(position, vw_ur, vw_br);
            mWindowClipPlanes.emplace_back(position, vw_br, vw_bl);
        }
        else
        {
            Vector3 x_axis(inv[0][0], inv[0][1], inv[0][2]);
            Vector3 y_axis(inv[1][0], inv[1][1], inv[1][2]);
            x_axis.normalise();
            y_axis.normalise();
            mWindowClipPlanes.emplace_back( x_axis, vw_bl);
            mWindowClipPlanes.emplace_back(-x_axis, vw_ur);
            mWindowClipPlanes.emplace_back( y_axis, vw_bl);
            mWindowClipPlanes.emplace_back(-y_axis, vw_ur);
        }

        mRecalcWindow = false;

    }
    // -------------------------------------------------------------------
    auto Camera::getWindowPlanes() const -> const std::vector<Plane>&
    {
        updateView();
        setWindowImpl();
        return mWindowClipPlanes;
    }
    // -------------------------------------------------------------------
    auto Camera::getBoundingRadius() const -> Real
    {
        // return a little bigger than the near distance
        // just to keep things just outside
        return mNearDist * 1.5f;

    }
    //-----------------------------------------------------------------------
    auto Camera::getAutoAspectRatio() const -> bool
    {
        return mAutoAspectRatio;
    }
    //-----------------------------------------------------------------------
    void Camera::setAutoAspectRatio(bool autoratio)
    {
        mAutoAspectRatio = autoratio;
    }
    //-----------------------------------------------------------------------
    auto Camera::isVisible(const AxisAlignedBox& bound, FrustumPlane* culledBy) const -> bool
    {
        if (mCullFrustum)
        {
            return mCullFrustum->isVisible(bound, culledBy);
        }
        else
        {
            return Frustum::isVisible(bound, culledBy);
        }
    }
    //-----------------------------------------------------------------------
    auto Camera::isVisible(const Sphere& bound, FrustumPlane* culledBy) const -> bool
    {
        if (mCullFrustum)
        {
            return mCullFrustum->isVisible(bound, culledBy);
        }
        else
        {
            return Frustum::isVisible(bound, culledBy);
        }
    }
    //-----------------------------------------------------------------------
    auto Camera::isVisible(const Vector3& vert, FrustumPlane* culledBy) const -> bool
    {
        if (mCullFrustum)
        {
            return mCullFrustum->isVisible(vert, culledBy);
        }
        else
        {
            return Frustum::isVisible(vert, culledBy);
        }
    }
    //-----------------------------------------------------------------------
    auto Camera::getWorldSpaceCorners() const -> const Frustum::Corners&
    {
        if (mCullFrustum)
        {
            return mCullFrustum->getWorldSpaceCorners();
        }
        else
        {
            return Frustum::getWorldSpaceCorners();
        }
    }
    //-----------------------------------------------------------------------
    auto Camera::getFrustumPlane( unsigned short plane ) const -> const Plane&
    {
        if (mCullFrustum)
        {
            return mCullFrustum->getFrustumPlane(plane);
        }
        else
        {
            return Frustum::getFrustumPlane(plane);
        }
    }
    //-----------------------------------------------------------------------
    auto Camera::projectSphere(const Sphere& sphere, 
        Real* left, Real* top, Real* right, Real* bottom) const -> bool
    {
        if (mCullFrustum)
        {
            return mCullFrustum->projectSphere(sphere, left, top, right, bottom);
        }
        else
        {
            return Frustum::projectSphere(sphere, left, top, right, bottom);
        }
    }
    //-----------------------------------------------------------------------
    auto Camera::getNearClipDistance() const -> Real
    {
        if (mCullFrustum)
        {
            return mCullFrustum->getNearClipDistance();
        }
        else
        {
            return Frustum::getNearClipDistance();
        }
    }
    //-----------------------------------------------------------------------
    auto Camera::getFarClipDistance() const -> Real
    {
        if (mCullFrustum)
        {
            return mCullFrustum->getFarClipDistance();
        }
        else
        {
            return Frustum::getFarClipDistance();
        }
    }
    //-----------------------------------------------------------------------
    auto Camera::getViewMatrix() const -> const Affine3&
    {
        if (mCullFrustum)
        {
            return mCullFrustum->getViewMatrix();
        }
        else
        {
            return Frustum::getViewMatrix();
        }
    }
    //-----------------------------------------------------------------------
    auto Camera::getViewMatrix(bool ownFrustumOnly) const -> const Affine3&
    {
        if (ownFrustumOnly)
        {
            return Frustum::getViewMatrix();
        }
        else
        {
            return getViewMatrix();
        }
    }
    //-----------------------------------------------------------------------
    //_______________________________________________________
    //|                                                     |
    //| getRayForwardIntersect                              |
    //| -----------------------------                       |
    //| get the intersections of frustum rays with a plane  |
    //| of interest.  The plane is assumed to have constant |
    //| z.  If this is not the case, rays                   |
    //| should be rotated beforehand to work in a           |
    //| coordinate system in which this is true.            |
    //|_____________________________________________________|
    //
    auto Camera::getRayForwardIntersect(const Vector3& anchor, const Vector3 *dir, Real planeOffset) const -> std::vector<Vector4>
    {
        std::vector<Vector4> res;

        if(!dir)
            return res;

        int infpt[4] = {0, 0, 0, 0}; // 0=finite, 1=infinite, 2=straddles infinity
        Vector3 vec[4];

        // find how much the anchor point must be displaced in the plane's
        // constant variable
        Real delta = planeOffset - anchor.z;

        // now set the intersection point and note whether it is a 
        // point at infinity or straddles infinity
        unsigned int i;
        for (i=0; i<4; i++)
        {
            Real test = dir[i].z * delta;
            if (test == 0.0) {
                vec[i] = dir[i];
                infpt[i] = 1;
            }
            else {
                Real lambda = delta / dir[i].z;
                vec[i] = anchor + (lambda * dir[i]);
                if(test < 0.0)
                    infpt[i] = 2;
            }
        }

        for (i=0; i<4; i++)
        {
            // store the finite intersection points
            if (infpt[i] == 0)
                res.emplace_back(vec[i].x, vec[i].y, vec[i].z, 1.0);
            else
            {
                // handle the infinite points of intersection;
                // cases split up into the possible frustum planes 
                // pieces which may contain a finite intersection point
                int nextind = (i+1) % 4;
                int prevind = (i+3) % 4;
                if ((infpt[prevind] == 0) || (infpt[nextind] == 0))
                {
                    if (infpt[i] == 1)
                        res.emplace_back(vec[i].x, vec[i].y, vec[i].z, 0.0);
                    else
                    {
                        // handle the intersection points that straddle infinity (back-project)
                        if(infpt[prevind] == 0) 
                        {
                            Vector3 temp = vec[prevind] - vec[i];
                            res.emplace_back(temp.x, temp.y, temp.z, 0.0);
                        }
                        if(infpt[nextind] == 0)
                        {
                            Vector3 temp = vec[nextind] - vec[i];
                            res.emplace_back(temp.x, temp.y, temp.z, 0.0);
                        }
                    }
                } // end if we need to add an intersection point to the list
            } // end if infinite point needs to be considered
        } // end loop over frustun corners

        // we end up with either 0, 3, 4, or 5 intersection points

        return res;
    }

    //_______________________________________________________
    //|                                                     |
    //| forwardIntersect                                    |
    //| -----------------------------                       |
    //| Forward intersect the camera's frustum rays with    |
    //| a specified plane of interest.                      |
    //| Note that if the frustum rays shoot out and would   |
    //| back project onto the plane, this means the forward |
    //| intersection of the frustum would occur at the      |
    //| line at infinity.                                   |
    //|_____________________________________________________|
    //
    void Camera::forwardIntersect(const Plane& worldPlane, std::vector<Vector4>* intersect3d) const
    {
        if(!intersect3d)
            return;

        Vector3 trCorner = getWorldSpaceCorners()[0];
        Vector3 tlCorner = getWorldSpaceCorners()[1];
        Vector3 blCorner = getWorldSpaceCorners()[2];
        Vector3 brCorner = getWorldSpaceCorners()[3];

        // need some sort of rotation that will bring the plane normal to the z axis
        Plane pval = worldPlane;
        if(pval.normal.z < 0.0)
        {
            pval.normal *= -1.0;
            pval.d *= -1.0;
        }
        Quaternion invPlaneRot = pval.normal.getRotationTo(Vector3::UNIT_Z);

        // get rotated light
        Vector3 lPos = invPlaneRot * getDerivedPosition();
        Vector3 vec[4];
        vec[0] = invPlaneRot * trCorner - lPos;
        vec[1] = invPlaneRot * tlCorner - lPos; 
        vec[2] = invPlaneRot * blCorner - lPos; 
        vec[3] = invPlaneRot * brCorner - lPos; 

        // compute intersection points on plane
        std::vector<Vector4> iPnt = getRayForwardIntersect(lPos, vec, -pval.d);


        // return wanted data
        if(intersect3d) 
        {
            Quaternion planeRot = invPlaneRot.Inverse();
            (*intersect3d).clear();
            for(auto & i : iPnt)
            {
                Vector3 intersection = planeRot * Vector3(i.x, i.y, i.z);
                (*intersect3d).emplace_back(intersection.x, intersection.y, intersection.z, i.w);
            }
        }
    }
    //-----------------------------------------------------------------------
    void Camera::synchroniseBaseSettingsWith(const Camera* cam)
    {
        this->setProjectionType(cam->getProjectionType());
        invalidateView();
        this->setAspectRatio(cam->getAspectRatio());
        this->setNearClipDistance(cam->getNearClipDistance());
        this->setFarClipDistance(cam->getFarClipDistance());
        this->setUseRenderingDistance(cam->getUseRenderingDistance());
        this->setFOVy(cam->getFOVy());
        this->setFocalLength(cam->getFocalLength());

        // Don't do these, they're not base settings and can cause referencing issues
        //this->setLodCamera(cam->getLodCamera());
        //this->setCullingFrustum(cam->getCullingFrustum());

    }


} // namespace Ogre
