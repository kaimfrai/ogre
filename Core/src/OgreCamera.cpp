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
#include <algorithm>
#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "OgreAxisAlignedBox.hpp"
#include "OgreCamera.hpp"
#include "OgreCommon.hpp"
#include "OgreFrustum.hpp"
#include "OgreMath.hpp"
#include "OgreMatrix4.hpp"
#include "OgrePlane.hpp"
#include "OgrePlaneBoundedVolume.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreQuaternion.hpp"
#include "OgreRay.hpp"
#include "OgreSceneManager.hpp"
#include "OgreVector.hpp"
#include "OgreViewport.hpp"

namespace Ogre {
class Sphere;

    String Camera::msMovableType = "Camera";
    //-----------------------------------------------------------------------
    Camera::Camera( const String& name, SceneManager* sm)
        : Frustum(name),
        mWindowSet(false),
        mAutoAspectRatio(false),
        mUseRenderingDistance(true),
        mUseMinPixelSize(false),
        mSceneLodFactor(1.0f),
        mSceneLodFactorInv(1.0f),
        mLastViewport(0),
        mCullFrustum(0),
        mLodCamera(0),
        mPixelDisplayRatio(0),
        mSortMode(SM_DISTANCE),
        mSceneDetail(PM_SOLID)
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

        mParentNode = 0;

        // no reflection
        mReflect = false;

        mVisible = false;
        mManager = sm;
    }

    //-----------------------------------------------------------------------
    Camera::~Camera()
    {
        ListenerList listenersCopy = mListeners;
        for (ListenerList::iterator i = listenersCopy.begin(); i != listenersCopy.end(); ++i)
        {
            (*i)->cameraDestroyed(this);
        }
    }
    //-----------------------------------------------------------------------
    SceneManager* Camera::getSceneManager() const
    {
        return mManager;
    }


    //-----------------------------------------------------------------------
    void Camera::setPolygonMode(PolygonMode sd)
    {
        mSceneDetail = sd;
    }

    //-----------------------------------------------------------------------
    PolygonMode Camera::getPolygonMode() const
    {
        return mSceneDetail;
    }

    //-----------------------------------------------------------------------
    bool Camera::isViewOutOfDate() const
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
        for (ListenerList::iterator i = listenersCopy.begin(); i != listenersCopy.end(); ++i)
        {
            (*i)->cameraPreRenderScene(this);
        }

        //render scene
        mManager->_renderScene(this, vp);

        // Listener list may have change
        listenersCopy = mListeners;

        //notify postrender scene
        for (ListenerList::iterator i = listenersCopy.begin(); i != listenersCopy.end(); ++i)
        {
            (*i)->cameraPostRenderScene(this);
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
        ListenerList::iterator i = std::find(mListeners.begin(), mListeners.end(), l);
        if (i != mListeners.end())
            mListeners.erase(i);
    }
    //-----------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& o, const Camera& c )
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
    unsigned int Camera::_getNumRenderedFaces() const
    {
        return mVisFacesLastRender;
    }
    //-----------------------------------------------------------------------
    unsigned int Camera::_getNumRenderedBatches() const
    {
        return mVisBatchesLastRender;
    }
    //-----------------------------------------------------------------------
    const Quaternion& Camera::getDerivedOrientation() const
    {
        updateView();
        return mDerivedOrientation;
    }
    //-----------------------------------------------------------------------
    const Vector3& Camera::getDerivedPosition() const
    {
        updateView();
        return mDerivedPosition;
    }
    //-----------------------------------------------------------------------
    Vector3 Camera::getDerivedDirection() const
    {
        // Direction points down -Z
        updateView();
        return -mDerivedOrientation.zAxis();
    }
    //-----------------------------------------------------------------------
    Vector3 Camera::getDerivedUp() const
    {
        updateView();
        return mDerivedOrientation.yAxis();
    }
    //-----------------------------------------------------------------------
    Vector3 Camera::getDerivedRight() const
    {
        updateView();
        return mDerivedOrientation.xAxis();
    }
    //-----------------------------------------------------------------------
    const Quaternion& Camera::getRealOrientation() const
    {
        updateView();
        return mLastParentOrientation;
    }
    //-----------------------------------------------------------------------
    const Vector3& Camera::getRealPosition() const
    {
        updateView();
        return mLastParentPosition;
    }
    //-----------------------------------------------------------------------
    Vector3 Camera::getRealDirection() const
    {
        // Direction points down -Z
        updateView();
        return -mLastParentOrientation.zAxis();
    }
    //-----------------------------------------------------------------------
    Vector3 Camera::getRealUp() const
    {
        updateView();
        return mLastParentOrientation.yAxis();
    }
    //-----------------------------------------------------------------------
    Vector3 Camera::getRealRight() const
    {
        updateView();
        return mLastParentOrientation.xAxis();
    }
    //-----------------------------------------------------------------------
    const String& Camera::getMovableType() const
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
    Real Camera::getLodBias() const
    {
        return mSceneLodFactor;
    }
    //-----------------------------------------------------------------------
    Real Camera::_getLodBiasInverse() const
    {
        return mSceneLodFactorInv;
    }
    //-----------------------------------------------------------------------
    void Camera::setLodCamera(const Camera* lodCam)
    {
        if (lodCam == this)
            mLodCamera = 0;
        else
            mLodCamera = lodCam;
    }
    //---------------------------------------------------------------------
    const Camera* Camera::getLodCamera() const
    {
        return mLodCamera? mLodCamera : this;
    }
    //-----------------------------------------------------------------------
    Ray Camera::getCameraToViewportRay(Real screenX, Real screenY) const
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
    PlaneBoundedVolume Camera::getCameraToViewportBoxVolume(Real screenLeft, 
        Real screenTop, Real screenRight, Real screenBottom, bool includeFarPlane)
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
            mWindowClipPlanes.push_back(Plane(position, vw_bl, vw_ul));
            mWindowClipPlanes.push_back(Plane(position, vw_ul, vw_ur));
            mWindowClipPlanes.push_back(Plane(position, vw_ur, vw_br));
            mWindowClipPlanes.push_back(Plane(position, vw_br, vw_bl));
        }
        else
        {
            Vector3 x_axis(inv[0][0], inv[0][1], inv[0][2]);
            Vector3 y_axis(inv[1][0], inv[1][1], inv[1][2]);
            x_axis.normalise();
            y_axis.normalise();
            mWindowClipPlanes.push_back(Plane( x_axis, vw_bl));
            mWindowClipPlanes.push_back(Plane(-x_axis, vw_ur));
            mWindowClipPlanes.push_back(Plane( y_axis, vw_bl));
            mWindowClipPlanes.push_back(Plane(-y_axis, vw_ur));
        }

        mRecalcWindow = false;

    }
    // -------------------------------------------------------------------
    const std::vector<Plane>& Camera::getWindowPlanes() const
    {
        updateView();
        setWindowImpl();
        return mWindowClipPlanes;
    }
    // -------------------------------------------------------------------
    Real Camera::getBoundingRadius() const
    {
        // return a little bigger than the near distance
        // just to keep things just outside
        return mNearDist * 1.5f;

    }
    //-----------------------------------------------------------------------
    bool Camera::getAutoAspectRatio() const
    {
        return mAutoAspectRatio;
    }
    //-----------------------------------------------------------------------
    void Camera::setAutoAspectRatio(bool autoratio)
    {
        mAutoAspectRatio = autoratio;
    }
    //-----------------------------------------------------------------------
    bool Camera::isVisible(const AxisAlignedBox& bound, FrustumPlane* culledBy) const
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
    bool Camera::isVisible(const Sphere& bound, FrustumPlane* culledBy) const
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
    bool Camera::isVisible(const Vector3& vert, FrustumPlane* culledBy) const
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
    const Frustum::Corners& Camera::getWorldSpaceCorners() const
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
    const Plane& Camera::getFrustumPlane( unsigned short plane ) const
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
    bool Camera::projectSphere(const Sphere& sphere, 
        Real* left, Real* top, Real* right, Real* bottom) const
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
    Real Camera::getNearClipDistance() const
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
    Real Camera::getFarClipDistance() const
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
    const Affine3& Camera::getViewMatrix() const
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
    const Affine3& Camera::getViewMatrix(bool ownFrustumOnly) const
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
    std::vector<Vector4> Camera::getRayForwardIntersect(const Vector3& anchor, const Vector3 *dir, Real planeOffset) const
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
                res.push_back(Vector4(vec[i].x, vec[i].y, vec[i].z, 1.0));
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
                        res.push_back(Vector4(vec[i].x, vec[i].y, vec[i].z, 0.0));
                    else
                    {
                        // handle the intersection points that straddle infinity (back-project)
                        if(infpt[prevind] == 0) 
                        {
                            Vector3 temp = vec[prevind] - vec[i];
                            res.push_back(Vector4(temp.x, temp.y, temp.z, 0.0));
                        }
                        if(infpt[nextind] == 0)
                        {
                            Vector3 temp = vec[nextind] - vec[i];
                            res.push_back(Vector4(temp.x, temp.y, temp.z, 0.0));
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
            for(unsigned int i=0; i<iPnt.size(); i++)
            {
                Vector3 intersection = planeRot * Vector3(iPnt[i].x, iPnt[i].y, iPnt[i].z);
                (*intersect3d).push_back(Vector4(intersection.x, intersection.y, intersection.z, iPnt[i].w));
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
