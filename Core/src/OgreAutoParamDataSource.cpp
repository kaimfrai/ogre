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

module Ogre.Core:AutoParamDataSource.Obj;

import :AutoParamDataSource;
import :Camera;
import :Controller;
import :ControllerManager;
import :Frustum;
import :Math;
import :Pass;
import :Platform;
import :Quaternion;
import :RenderSystem;
import :RenderTarget;
import :Renderable;
import :Root;
import :SceneManager;
import :SharedPtr;
import :Texture;
import :TextureUnitState;
import :Viewport;

import <cmath>;
import <limits>;

namespace Ogre {
    //-----------------------------------------------------------------------------
    AutoParamDataSource::AutoParamDataSource()
        : 
         mAmbientLight(ColourValue::Black),
         
         mDummyNode(nullptr)
    {
        mBlankLight.setDiffuseColour(ColourValue::Black);
        mBlankLight.setSpecularColour(ColourValue::Black);
        mBlankLight.setAttenuation(0,1,0,0);
        mBlankLight._notifyAttached(&mDummyNode);
        for(size_t i = 0; i < OGRE_MAX_SIMULTANEOUS_LIGHTS; ++i)
        {
            mTextureViewProjMatrixDirty[i] = true;
            mTextureWorldViewProjMatrixDirty[i] = true;
            mSpotlightViewProjMatrixDirty[i] = true;
            mSpotlightWorldViewProjMatrixDirty[i] = true;
            mCurrentTextureProjector[i] = nullptr;
            mShadowCamDepthRangesDirty[i] = false;
        }

    }
    //-----------------------------------------------------------------------------
	auto AutoParamDataSource::getCurrentCamera() const -> const Camera*
	{
		return mCurrentCamera;
	}
	//-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLight(size_t index) const -> const Light&
    {
        // If outside light range, return a blank light to ensure zeroised for program
        if (mCurrentLightList && index < mCurrentLightList->size())
        {
            return *((*mCurrentLightList)[index]);
        }
        else
        {
            return mBlankLight;
        }        
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setCurrentRenderable(const Renderable* rend)
    {
        mCurrentRenderable = rend;
        mWorldMatrixDirty = true;
        mViewMatrixDirty = true;
        mProjMatrixDirty = true;
        mWorldViewMatrixDirty = true;
        mViewProjMatrixDirty = true;
        mWorldViewProjMatrixDirty = true;
        mInverseWorldMatrixDirty = true;
        mInverseViewMatrixDirty = true;
        mInverseWorldViewMatrixDirty = true;
        mInverseTransposeWorldMatrixDirty = true;
        mInverseTransposeWorldViewMatrixDirty = true;
        mCameraPositionObjectSpaceDirty = true;
        mLodCameraPositionObjectSpaceDirty = true;
        for(size_t i = 0; i < OGRE_MAX_SIMULTANEOUS_LIGHTS; ++i)
        {
            mTextureWorldViewProjMatrixDirty[i] = true;
            mSpotlightWorldViewProjMatrixDirty[i] = true;
        }

    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setCurrentCamera(const Camera* cam, bool useCameraRelative)
    {
        mCurrentCamera = cam;
        mCameraRelativeRendering = useCameraRelative;
        mCameraRelativePosition = cam->getDerivedPosition();
        mViewMatrixDirty = true;
        mProjMatrixDirty = true;
        mWorldViewMatrixDirty = true;
        mViewProjMatrixDirty = true;
        mWorldViewProjMatrixDirty = true;
        mInverseViewMatrixDirty = true;
        mInverseWorldViewMatrixDirty = true;
        mInverseTransposeWorldViewMatrixDirty = true;
        mCameraPositionObjectSpaceDirty = true;
        mCameraPositionDirty = true;
        mLodCameraPositionObjectSpaceDirty = true;
        mLodCameraPositionDirty = true;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setCurrentLightList(const LightList* ll)
    {
        mCurrentLightList = ll;
        for(size_t i = 0; i < ll->size() && i < OGRE_MAX_SIMULTANEOUS_LIGHTS; ++i)
        {
            mSpotlightViewProjMatrixDirty[i] = true;
            mSpotlightWorldViewProjMatrixDirty[i] = true;
        }

    }
    //---------------------------------------------------------------------
    auto AutoParamDataSource::getLightNumber(size_t index) const -> float
    {
        return static_cast<float>(getLight(index)._getIndexInFrame());
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLightDiffuseColour(size_t index) const -> const ColourValue&
    {
        return getLight(index).getDiffuseColour();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLightSpecularColour(size_t index) const -> const ColourValue&
    {
        return getLight(index).getSpecularColour();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLightDiffuseColourWithPower(size_t index) const -> const ColourValue
    {
        const Light& l = getLight(index);
        ColourValue scaled(l.getDiffuseColour());
        Real power = l.getPowerScale();
        // scale, but not alpha
        scaled.r *= power;
        scaled.g *= power;
        scaled.b *= power;
        return scaled;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLightSpecularColourWithPower(size_t index) const -> const ColourValue
    {
        const Light& l = getLight(index);
        ColourValue scaled(l.getSpecularColour());
        Real power = l.getPowerScale();
        // scale, but not alpha
        scaled.r *= power;
        scaled.g *= power;
        scaled.b *= power;
        return scaled;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLightPosition(size_t index) const -> Vector3
    {
        return getLight(index).getDerivedPosition(true);
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLightAs4DVector(size_t index) const -> Vector4
    {
        return getLight(index).getAs4DVector(true);
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLightDirection(size_t index) const -> Vector3
    {
        return getLight(index).getDerivedDirection();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLightPowerScale(size_t index) const -> Real
    {
        return getLight(index).getPowerScale();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLightAttenuation(size_t index) const -> const Vector4f&
    {
        // range, const, linear, quad
        return getLight(index).getAttenuation();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSpotlightParams(size_t index) const -> Vector4f
    {
        // inner, outer, fallof, isSpot
        const Light& l = getLight(index);
        if (l.getType() == Light::LT_SPOTLIGHT)
        {
            return {Math::Cos(l.getSpotlightInnerAngle().valueRadians() * 0.5f),
                           Math::Cos(l.getSpotlightOuterAngle().valueRadians() * 0.5f),
                           l.getSpotlightFalloff(),
                           1.0};
        }
        else
        {
            // Use safe values which result in no change to point & dir light calcs
            // The spot factor applied to the usual lighting calc is 
            // pow((dot(spotDir, lightDir) - y) / (x - y), z)
            // Therefore if we set z to 0.0f then the factor will always be 1
            // since pow(anything, 0) == 1
            // However we also need to ensure we don't overflow because of the division
            // therefore set x = 1 and y = 0 so divisor doesn't change scale
            return {1.0, 0.0, 0.0, 0.0}; // since the main op is pow(.., vec4.z), this will result in 1.0
        }
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setMainCamBoundsInfo(VisibleObjectsBoundsInfo* info)
    {
        mMainCamBoundsInfo = info;
        mSceneDepthRangeDirty = true;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setCurrentSceneManager(const SceneManager* sm)
    {
        mCurrentSceneManager = sm;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setWorldMatrices(const Affine3* m, size_t count)
    {
        mWorldMatrixArray = m;
        mWorldMatrixCount = count;
        mWorldMatrixDirty = false;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getWorldMatrix() const -> const Affine3&
    {
        if (mWorldMatrixDirty)
        {
            mWorldMatrixArray = mWorldMatrix;
            mCurrentRenderable->getWorldTransforms(reinterpret_cast<Matrix4*>(mWorldMatrix));
            mWorldMatrixCount = mCurrentRenderable->getNumWorldTransforms();
            if (mCameraRelativeRendering && !mCurrentRenderable->getUseIdentityView())
            {
                for (size_t i = 0; i < mWorldMatrixCount; ++i)
                {
                    mWorldMatrix[i].setTrans(mWorldMatrix[i].getTrans() - mCameraRelativePosition);
                }
            }
            mWorldMatrixDirty = false;
        }
        return mWorldMatrixArray[0];
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getWorldMatrixCount() const -> size_t
    {
        // trigger derivation
        getWorldMatrix();
        return mWorldMatrixCount;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getWorldMatrixArray() const -> const Affine3*
    {
        // trigger derivation
        getWorldMatrix();
        return mWorldMatrixArray;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getViewMatrix() const -> const Affine3&
    {
        if (mViewMatrixDirty)
        {
            if (mCurrentRenderable && mCurrentRenderable->getUseIdentityView())
                mViewMatrix = Affine3::IDENTITY;
            else
            {
                mViewMatrix = mCurrentCamera->getViewMatrix(true);
                if (mCameraRelativeRendering)
                {
                    mViewMatrix.setTrans(Vector3::ZERO);
                }

            }
            mViewMatrixDirty = false;
        }
        return mViewMatrix;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getViewProjectionMatrix() const -> const Matrix4&
    {
        if (mViewProjMatrixDirty)
        {
            mViewProjMatrix = getProjectionMatrix() * getViewMatrix();
            mViewProjMatrixDirty = false;
        }
        return mViewProjMatrix;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getProjectionMatrix() const -> const Matrix4&
    {
        if (mProjMatrixDirty)
        {
            // NB use API-independent projection matrix since GPU programs
            // bypass the API-specific handedness and use right-handed coords
            if (mCurrentRenderable && mCurrentRenderable->getUseIdentityProjection())
            {
                // Use identity projection matrix, still need to take RS depth into account.
                RenderSystem* rs = Root::getSingleton().getRenderSystem();
                rs->_convertProjectionMatrix(Matrix4::IDENTITY, mProjectionMatrix, true);
            }
            else
            {
                mProjectionMatrix = mCurrentCamera->getProjectionMatrixWithRSDepth();
            }
            if (mCurrentRenderTarget && mCurrentRenderTarget->requiresTextureFlipping())
            {
                // Because we're not using setProjectionMatrix, this needs to be done here
                // Invert transformed y
                mProjectionMatrix[1][0] = -mProjectionMatrix[1][0];
                mProjectionMatrix[1][1] = -mProjectionMatrix[1][1];
                mProjectionMatrix[1][2] = -mProjectionMatrix[1][2];
                mProjectionMatrix[1][3] = -mProjectionMatrix[1][3];
            }
            mProjMatrixDirty = false;
        }
        return mProjectionMatrix;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getWorldViewMatrix() const -> const Affine3&
    {
        if (mWorldViewMatrixDirty)
        {
            mWorldViewMatrix = getViewMatrix() * getWorldMatrix();
            mWorldViewMatrixDirty = false;
        }
        return mWorldViewMatrix;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getWorldViewProjMatrix() const -> const Matrix4&
    {
        if (mWorldViewProjMatrixDirty)
        {
            mWorldViewProjMatrix = getProjectionMatrix() * getWorldViewMatrix();
            mWorldViewProjMatrixDirty = false;
        }
        return mWorldViewProjMatrix;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseWorldMatrix() const -> const Affine3&
    {
        if (mInverseWorldMatrixDirty)
        {
            mInverseWorldMatrix = getWorldMatrix().inverse();
            mInverseWorldMatrixDirty = false;
        }
        return mInverseWorldMatrix;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseWorldViewMatrix() const -> const Affine3&
    {
        if (mInverseWorldViewMatrixDirty)
        {
            mInverseWorldViewMatrix = getWorldViewMatrix().inverse();
            mInverseWorldViewMatrixDirty = false;
        }
        return mInverseWorldViewMatrix;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseViewMatrix() const -> const Affine3&
    {
        if (mInverseViewMatrixDirty)
        {
            mInverseViewMatrix = getViewMatrix().inverse();
            mInverseViewMatrixDirty = false;
        }
        return mInverseViewMatrix;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseTransposeWorldMatrix() const -> const Matrix4&
    {
        if (mInverseTransposeWorldMatrixDirty)
        {
            mInverseTransposeWorldMatrix = getInverseWorldMatrix().transpose();
            mInverseTransposeWorldMatrixDirty = false;
        }
        return mInverseTransposeWorldMatrix;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseTransposeWorldViewMatrix() const -> const Matrix4&
    {
        if (mInverseTransposeWorldViewMatrixDirty)
        {
            mInverseTransposeWorldViewMatrix = getInverseWorldViewMatrix().transpose();
            mInverseTransposeWorldViewMatrixDirty = false;
        }
        return mInverseTransposeWorldViewMatrix;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getCameraPosition() const -> const Vector4&
    {
        if(mCameraPositionDirty)
        {
            Vector3 vec3 = mCurrentCamera->getDerivedPosition();
            if (mCameraRelativeRendering)
            {
                vec3 -= mCameraRelativePosition;
            }
            mCameraPosition[0] = vec3[0];
            mCameraPosition[1] = vec3[1];
            mCameraPosition[2] = vec3[2];
            mCameraPosition[3] = 1.0;
            mCameraPositionDirty = false;
        }
        return mCameraPosition;
    }    
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getCameraPositionObjectSpace() const -> const Vector4&
    {
        if (mCameraPositionObjectSpaceDirty)
        {
            if (mCameraRelativeRendering)
            {
                mCameraPositionObjectSpace = Vector4(getInverseWorldMatrix() * Vector3::ZERO);
            }
            else
            {
                mCameraPositionObjectSpace =
                    Vector4(getInverseWorldMatrix() * mCurrentCamera->getDerivedPosition());
            }
            mCameraPositionObjectSpaceDirty = false;
        }
        return mCameraPositionObjectSpace;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getCameraRelativePosition () const -> const Vector4
    {
        return Ogre::Vector4 (mCameraRelativePosition.x, mCameraRelativePosition.y, mCameraRelativePosition.z, 1);
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLodCameraPosition() const -> const Vector4&
    {
        if(mLodCameraPositionDirty)
        {
            Vector3 vec3 = mCurrentCamera->getLodCamera()->getDerivedPosition();
            if (mCameraRelativeRendering)
            {
                vec3 -= mCameraRelativePosition;
            }
            mLodCameraPosition[0] = vec3[0];
            mLodCameraPosition[1] = vec3[1];
            mLodCameraPosition[2] = vec3[2];
            mLodCameraPosition[3] = 1.0;
            mLodCameraPositionDirty = false;
        }
        return mLodCameraPosition;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getLodCameraPositionObjectSpace() const -> const Vector4&
    {
        if (mLodCameraPositionObjectSpaceDirty)
        {
            if (mCameraRelativeRendering)
            {
                mLodCameraPositionObjectSpace =
                    Vector4(getInverseWorldMatrix() *
                            (mCurrentCamera->getLodCamera()->getDerivedPosition() -
                             mCameraRelativePosition));
            }
            else
            {
                mLodCameraPositionObjectSpace =
                    Vector4(getInverseWorldMatrix() *
                            (mCurrentCamera->getLodCamera()->getDerivedPosition()));
            }
            mLodCameraPositionObjectSpaceDirty = false;
        }
        return mLodCameraPositionObjectSpace;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setAmbientLightColour(const ColourValue& ambient)
    {
        mAmbientLight = ambient;
    }
    //---------------------------------------------------------------------
    auto AutoParamDataSource::getLightCount() const -> float
    {
        return static_cast<float>(mCurrentLightList->size());
    }
    //---------------------------------------------------------------------
    auto AutoParamDataSource::getLightCastsShadows(size_t index) const -> float
    {
        return getLight(index).getCastShadows() ? 1.0f : 0.0f;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getAmbientLightColour() const -> const ColourValue&
    {
        return mAmbientLight;
        
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setCurrentPass(const Pass* pass)
    {
        mCurrentPass = pass;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getCurrentPass() const -> const Pass*
    {
        return mCurrentPass;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTextureSize(size_t index) const -> Vector4f
    {
        Vector4f size = Vector4f(1,1,1,1);

        if (index < mCurrentPass->getNumTextureUnitStates())
        {
            const TexturePtr& tex = mCurrentPass->getTextureUnitState(
                static_cast<unsigned short>(index))->_getTexturePtr();
            if (tex)
            {
                size[0] = static_cast<Real>(tex->getWidth());
                size[1] = static_cast<Real>(tex->getHeight());
                size[2] = static_cast<Real>(tex->getDepth());
            }
        }

        return size;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseTextureSize(size_t index) const -> Vector4f
    {
        Vector4f size = getTextureSize(index);
        return 1 / size;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getPackedTextureSize(size_t index) const -> Vector4f
    {
        Vector4f size = getTextureSize(index);
        return {size[0], size[1], 1 / size[0], 1 / size[1]};
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSurfaceAmbientColour() const -> const ColourValue&
    {
        return mCurrentPass->getAmbient();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSurfaceDiffuseColour() const -> const ColourValue&
    {
        return mCurrentPass->getDiffuse();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSurfaceSpecularColour() const -> const ColourValue&
    {
        return mCurrentPass->getSpecular();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSurfaceEmissiveColour() const -> const ColourValue&
    {
        return mCurrentPass->getSelfIllumination();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSurfaceShininess() const -> Real
    {
        return mCurrentPass->getShininess();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSurfaceAlphaRejectionValue() const -> Real
    {
        return static_cast<Real>(static_cast<unsigned int>(mCurrentPass->getAlphaRejectValue())) / 255.0f;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getDerivedAmbientLightColour() const -> ColourValue
    {
        return getAmbientLightColour() * getSurfaceAmbientColour();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getDerivedSceneColour() const -> ColourValue
    {
        ColourValue result = getDerivedAmbientLightColour() + getSurfaceEmissiveColour();
        result.a = getSurfaceDiffuseColour().a;
        return result;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setFog(FogMode mode, const ColourValue& colour,
        Real expDensity, Real linearStart, Real linearEnd)
    {
        (void)mode; // ignored
        mFogColour = colour;
        mFogParams[0] = expDensity;
        mFogParams[1] = linearStart;
        mFogParams[2] = linearEnd;
        mFogParams[3] = linearEnd != linearStart ? 1 / (linearEnd - linearStart) : 0;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getFogColour() const -> const ColourValue&
    {
        return mFogColour;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getFogParams() const -> const Vector4f&
    {
        return mFogParams;
    }

    void AutoParamDataSource::setPointParameters(bool attenuation, const Vector4f& params)
    {
        mPointParams = params;
        if(attenuation)
            mPointParams[0] *= getViewportHeight();
    }

    auto AutoParamDataSource::getPointParams() const -> const Vector4f&
    {
        return mPointParams;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setTextureProjector(const Frustum* frust, size_t index = 0)
    {
        if (index < OGRE_MAX_SIMULTANEOUS_LIGHTS)
        {
            mCurrentTextureProjector[index] = frust;
            mTextureViewProjMatrixDirty[index] = true;
            mTextureWorldViewProjMatrixDirty[index] = true;
            mShadowCamDepthRangesDirty[index] = true;
        }

    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTextureViewProjMatrix(size_t index) const -> const Matrix4&
    {
        if (index < OGRE_MAX_SIMULTANEOUS_LIGHTS)
        {
            if (mTextureViewProjMatrixDirty[index] && mCurrentTextureProjector[index])
            {
                if (mCameraRelativeRendering)
                {
                    // World positions are now going to be relative to the camera position
                    // so we need to alter the projector view matrix to compensate
                    Matrix4 viewMatrix;
                    mCurrentTextureProjector[index]->calcViewMatrixRelative(
                        mCurrentCamera->getDerivedPosition(), viewMatrix);
                    mTextureViewProjMatrix[index] = 
                        Matrix4::CLIPSPACE2DTOIMAGESPACE *
                        mCurrentTextureProjector[index]->getProjectionMatrixWithRSDepth() * 
                        viewMatrix;
                }
                else
                {
                    mTextureViewProjMatrix[index] = 
                        Matrix4::CLIPSPACE2DTOIMAGESPACE *
                        mCurrentTextureProjector[index]->getProjectionMatrixWithRSDepth() * 
                        mCurrentTextureProjector[index]->getViewMatrix();
                }
                mTextureViewProjMatrixDirty[index] = false;
            }
            return mTextureViewProjMatrix[index];
        }
        else
            return Matrix4::IDENTITY;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTextureWorldViewProjMatrix(size_t index) const -> const Matrix4&
    {
        if (index < OGRE_MAX_SIMULTANEOUS_LIGHTS)
        {
            if (mTextureWorldViewProjMatrixDirty[index] && mCurrentTextureProjector[index])
            {
                mTextureWorldViewProjMatrix[index] = 
                    getTextureViewProjMatrix(index) * getWorldMatrix();
                mTextureWorldViewProjMatrixDirty[index] = false;
            }
            return mTextureWorldViewProjMatrix[index];
        }
        else
            return Matrix4::IDENTITY;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSpotlightViewProjMatrix(size_t index) const -> const Matrix4&
    {
        if (index < OGRE_MAX_SIMULTANEOUS_LIGHTS)
        {
            const Light& l = getLight(index);

            if (&l != &mBlankLight && 
                l.getType() == Light::LT_SPOTLIGHT &&
                mSpotlightViewProjMatrixDirty[index])
            {
                Frustum frust;
                SceneNode dummyNode(nullptr);
                dummyNode.attachObject(&frust);

                frust.setProjectionType(PT_PERSPECTIVE);
                frust.setFOVy(l.getSpotlightOuterAngle());
                frust.setAspectRatio(1.0f);
                // set near clip the same as main camera, since they are likely
                // to both reflect the nature of the scene
                frust.setNearClipDistance(mCurrentCamera->getNearClipDistance());
                // Calculate position, which same as spotlight position, in camera-relative coords if required
                dummyNode.setPosition(l.getDerivedPosition(true));
                // Calculate direction, which same as spotlight direction
                Vector3 dir = - l.getDerivedDirection(); // backwards since point down -z
                dir.normalise();
                Vector3 up = Vector3::UNIT_Y;
                // Check it's not coincident with dir
                if (Math::Abs(up.dotProduct(dir)) >= 1.0f)
                {
                    // Use camera up
                    up = Vector3::UNIT_Z;
                }
                // cross twice to rederive, only direction is unaltered
                Vector3 left = dir.crossProduct(up);
                left.normalise();
                up = dir.crossProduct(left);
                up.normalise();
                // Derive quaternion from axes
                Quaternion q;
                q.FromAxes(left, up, dir);
                dummyNode.setOrientation(q);

                // The view matrix here already includes camera-relative changes if necessary
                // since they are built into the frustum position
                mSpotlightViewProjMatrix[index] = 
                    Matrix4::CLIPSPACE2DTOIMAGESPACE *
                    frust.getProjectionMatrixWithRSDepth() * 
                    frust.getViewMatrix();

                mSpotlightViewProjMatrixDirty[index] = false;
            }
            return mSpotlightViewProjMatrix[index];
        }
        else
            return Matrix4::IDENTITY;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSpotlightWorldViewProjMatrix(size_t index) const -> const Matrix4&
    {
        if (index < OGRE_MAX_SIMULTANEOUS_LIGHTS)
        {
            const Light& l = getLight(index);

            if (&l != &mBlankLight && 
                l.getType() == Light::LT_SPOTLIGHT &&
                mSpotlightWorldViewProjMatrixDirty[index])
            {
                mSpotlightWorldViewProjMatrix[index] = 
                    getSpotlightViewProjMatrix(index) * getWorldMatrix();
                mSpotlightWorldViewProjMatrixDirty[index] = false;
            }
            return mSpotlightWorldViewProjMatrix[index];
        }
        else
            return Matrix4::IDENTITY;
    }
//-----------------------------------------------------------------------------
  auto AutoParamDataSource::getTextureTransformMatrix(size_t index) const -> const Matrix4&
  {
    // make sure the current pass is set
    assert(mCurrentPass && "current pass is NULL!");
    // check if there is a texture unit with the given index in the current pass
    if(index < mCurrentPass->getNumTextureUnitStates())
    {
      // texture unit existent, return its currently set transform
      return mCurrentPass->getTextureUnitState(static_cast<unsigned short>(index))->getTextureTransform();
    }
    else
    {
      // no such texture unit, return unity
      return Matrix4::IDENTITY;
    }
  }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setCurrentRenderTarget(const RenderTarget* target)
    {
        mCurrentRenderTarget = target;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getCurrentRenderTarget() const -> const RenderTarget*
    {
        return mCurrentRenderTarget;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setCurrentViewport(const Viewport* viewport)
    {
        mCurrentViewport = viewport;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setShadowDirLightExtrusionDistance(Real dist)
    {
        mDirLightExtrusionDistance = dist;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setShadowPointLightExtrusionDistance(Real dist)
    {
        mPointLightExtrusionDistance = dist;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getShadowExtrusionDistance() const -> Real
    {
        const Light& l = getLight(0); // only ever applies to one light at once
        return (l.getType() == Light::LT_DIRECTIONAL) ?
            mDirLightExtrusionDistance : mPointLightExtrusionDistance;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getCurrentRenderable() const -> const Renderable*
    {
        return mCurrentRenderable;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseViewProjMatrix() const -> Matrix4
    {
        return this->getViewProjectionMatrix().inverse();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseTransposeViewProjMatrix() const -> Matrix4
    {
        return this->getInverseViewProjMatrix().transpose();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTransposeViewProjMatrix() const -> Matrix4
    {
        return this->getViewProjectionMatrix().transpose();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTransposeViewMatrix() const -> Matrix4
    {
        return this->getViewMatrix().transpose();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseTransposeViewMatrix() const -> Matrix4
    {
        return this->getInverseViewMatrix().transpose();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTransposeProjectionMatrix() const -> Matrix4
    {
        return this->getProjectionMatrix().transpose();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseProjectionMatrix() const -> Matrix4 
    {
        return this->getProjectionMatrix().inverse();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseTransposeProjectionMatrix() const -> Matrix4
    {
        return this->getInverseProjectionMatrix().transpose();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTransposeWorldViewProjMatrix() const -> Matrix4
    {
        return this->getWorldViewProjMatrix().transpose();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseWorldViewProjMatrix() const -> Matrix4
    {
        return this->getWorldViewProjMatrix().inverse();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseTransposeWorldViewProjMatrix() const -> Matrix4
    {
        return this->getInverseWorldViewProjMatrix().transpose();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTransposeWorldViewMatrix() const -> Matrix4
    {
        return this->getWorldViewMatrix().transpose();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTransposeWorldMatrix() const -> Matrix4
    {
        return this->getWorldMatrix().transpose();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTime() const -> Real
    {
        return ControllerManager::getSingleton().getElapsedTime();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTime_0_X(Real x) const -> Real
    {
        return std::fmod(this->getTime(), x);
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getCosTime_0_X(Real x) const -> Real
    { 
        return std::cos(this->getTime_0_X(x));
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSinTime_0_X(Real x) const -> Real
    { 
        return std::sin(this->getTime_0_X(x));
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTanTime_0_X(Real x) const -> Real
    { 
        return std::tan(this->getTime_0_X(x));
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTime_0_X_packed(Real x) const -> Vector4f
    {
        float t = this->getTime_0_X(x);
        return {t, std::sin(t), std::cos(t), std::tan(t)};
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTime_0_1(Real x) const -> Real
    { 
        return this->getTime_0_X(x)/x; 
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getCosTime_0_1(Real x) const -> Real
    { 
        return std::cos(this->getTime_0_1(x));
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSinTime_0_1(Real x) const -> Real
    { 
        return std::sin(this->getTime_0_1(x));
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTanTime_0_1(Real x) const -> Real
    { 
        return std::tan(this->getTime_0_1(x));
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTime_0_1_packed(Real x) const -> Vector4f
    {
        float t = this->getTime_0_1(x);
        return {t, std::sin(t), std::cos(t), std::tan(t)};
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTime_0_2Pi(Real x) const -> Real
    { 
        return this->getTime_0_X(x)/x*2*Math::PI; 
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getCosTime_0_2Pi(Real x) const -> Real
    { 
        return std::cos(this->getTime_0_2Pi(x));
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSinTime_0_2Pi(Real x) const -> Real
    { 
        return std::sin(this->getTime_0_2Pi(x));
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTanTime_0_2Pi(Real x) const -> Real
    { 
        return std::tan(this->getTime_0_2Pi(x));
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getTime_0_2Pi_packed(Real x) const -> Vector4f
    {
        float t = this->getTime_0_2Pi(x);
        return {t, std::sin(t), std::cos(t), std::tan(t)};
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getFrameTime() const -> Real
    {
        return ControllerManager::getSingleton().getFrameTimeSource()->getValue();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getFPS() const -> Real
    {
        return mCurrentRenderTarget->getStatistics().lastFPS;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getViewportWidth() const -> Real
    { 
        return static_cast<Real>(mCurrentViewport->getActualWidth()); 
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getViewportHeight() const -> Real
    { 
        return static_cast<Real>(mCurrentViewport->getActualHeight()); 
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseViewportWidth() const -> Real
    { 
        return 1.0f/mCurrentViewport->getActualWidth(); 
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getInverseViewportHeight() const -> Real
    { 
        return 1.0f/mCurrentViewport->getActualHeight(); 
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getViewDirection() const -> Vector3
    {
        return mCurrentCamera->getDerivedDirection();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getViewSideVector() const -> Vector3
    { 
        return mCurrentCamera->getDerivedRight();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getViewUpVector() const -> Vector3
    { 
        return mCurrentCamera->getDerivedUp();
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getFOV() const -> Real
    { 
        return mCurrentCamera->getFOVy().valueRadians(); 
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getNearClipDistance() const -> Real
    { 
        return mCurrentCamera->getNearClipDistance(); 
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getFarClipDistance() const -> Real
    { 
        return mCurrentCamera->getFarClipDistance(); 
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getPassNumber() const -> int
    {
        return mPassNumber;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::setPassNumber(const int passNumber)
    {
        mPassNumber = passNumber;
    }
    //-----------------------------------------------------------------------------
    void AutoParamDataSource::incPassNumber()
    {
        ++mPassNumber;
    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getSceneDepthRange() const -> const Vector4&
    {
        static Vector4 dummy(0, 100000, 100000, 1.f/100000);

        if (mSceneDepthRangeDirty)
        {
            // calculate depth information
            Real depthRange = mMainCamBoundsInfo->maxDistanceInFrustum - mMainCamBoundsInfo->minDistanceInFrustum;
            if (depthRange > std::numeric_limits<Real>::epsilon())
            {
                mSceneDepthRange = Vector4(
                    mMainCamBoundsInfo->minDistanceInFrustum,
                    mMainCamBoundsInfo->maxDistanceInFrustum,
                    depthRange,
                    1.0f / depthRange);
            }
            else
            {
                mSceneDepthRange = dummy;
            }
            mSceneDepthRangeDirty = false;
        }

        return mSceneDepthRange;

    }
    //-----------------------------------------------------------------------------
    auto AutoParamDataSource::getShadowSceneDepthRange(size_t index) const -> const Vector4&
    {
        static Vector4 dummy(0, 100000, 100000, 1/100000);

        if (!mCurrentSceneManager->isShadowTechniqueTextureBased())
            return dummy;

        if (index < OGRE_MAX_SIMULTANEOUS_LIGHTS)
        {
            if (mShadowCamDepthRangesDirty[index] && mCurrentTextureProjector[index])
            {
                const VisibleObjectsBoundsInfo& info = 
                    mCurrentSceneManager->getVisibleObjectsBoundsInfo(
                        (const Camera*)mCurrentTextureProjector[index]);

                Real depthRange = info.maxDistanceInFrustum - info.minDistanceInFrustum;
                if (depthRange > std::numeric_limits<Real>::epsilon())
                {
                    mShadowCamDepthRanges[index] = Vector4(
                        info.minDistanceInFrustum,
                        info.maxDistanceInFrustum,
                        depthRange,
                        1.0f / depthRange);
                }
                else
                {
                    mShadowCamDepthRanges[index] = dummy;
                }

                mShadowCamDepthRangesDirty[index] = false;
            }
            return mShadowCamDepthRanges[index];
        }
        else
            return dummy;
    }
    //---------------------------------------------------------------------
    auto AutoParamDataSource::getShadowColour() const -> const ColourValue&
    {
        return mCurrentSceneManager->getShadowColour();
    }
    //-------------------------------------------------------------------------
    void AutoParamDataSource::updateLightCustomGpuParameter(const GpuProgramParameters::AutoConstantEntry& constantEntry, GpuProgramParameters *params) const
    {
        auto lightIndex = static_cast<uint16>(constantEntry.data & 0xFFFF),
            paramIndex = static_cast<uint16>((constantEntry.data >> 16) & 0xFFFF);
        if(mCurrentLightList && lightIndex < mCurrentLightList->size())
        {
            const Light &light = getLight(lightIndex);
            light._updateCustomGpuParameter(paramIndex, constantEntry, params);
        }
    }

}
