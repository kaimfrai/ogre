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
#ifndef OGRE_CORE_AUTOPARAMDATASOURCE_H
#define OGRE_CORE_AUTOPARAMDATASOURCE_H

#include <cstddef>

#include "OgreColourValue.hpp"
#include "OgreCommon.hpp"
#include "OgreConfig.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgreLight.hpp"
#include "OgreMatrix4.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSceneNode.hpp"
#include "OgreVector.hpp"

namespace Ogre {

    // forward decls
    struct VisibleObjectsBoundsInfo;
class Camera;
class Frustum;
class Pass;
class RenderTarget;
class Renderable;
class SceneManager;
class Viewport;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Materials
    *  @{
    */


    /** This utility class is used to hold the information used to generate the matrices
    and other information required to automatically populate GpuProgramParameters.
    @remarks
        This class exercises a lazy-update scheme in order to avoid having to update all
        the information a GpuProgramParameters class could possibly want all the time. 
        It relies on the SceneManager to update it when the base data has changed, and
        will calculate concatenated matrices etc only when required, passing back precalculated
        matrices when they are requested more than once when the underlying information has
        not altered.
    */
    class AutoParamDataSource : public SceneMgtAlloc
    {
    private:
        auto getLight(size_t index) const -> const Light&;
        mutable Affine3 mWorldMatrix[256];
        mutable size_t mWorldMatrixCount{0};
        mutable const Affine3* mWorldMatrixArray{nullptr};
        mutable Affine3 mWorldViewMatrix;
        mutable Matrix4 mViewProjMatrix;
        mutable Matrix4 mWorldViewProjMatrix;
        mutable Affine3 mInverseWorldMatrix;
        mutable Affine3 mInverseWorldViewMatrix;
        mutable Affine3 mInverseViewMatrix;
        mutable Matrix4 mInverseTransposeWorldMatrix;
        mutable Matrix4 mInverseTransposeWorldViewMatrix;
        mutable Vector4 mCameraPosition;
        mutable Vector4 mCameraPositionObjectSpace;
        mutable Matrix4 mTextureViewProjMatrix[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        mutable Matrix4 mTextureWorldViewProjMatrix[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        mutable Matrix4 mSpotlightViewProjMatrix[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        mutable Matrix4 mSpotlightWorldViewProjMatrix[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        mutable Vector4 mShadowCamDepthRanges[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        mutable Affine3 mViewMatrix;
        mutable Matrix4 mProjectionMatrix;
        mutable Real mDirLightExtrusionDistance;
        mutable Real mPointLightExtrusionDistance;
        mutable Vector4 mLodCameraPosition;
        mutable Vector4 mLodCameraPositionObjectSpace;

        mutable bool mWorldMatrixDirty{true};
        mutable bool mViewMatrixDirty{true};
        mutable bool mProjMatrixDirty{true};
        mutable bool mWorldViewMatrixDirty{true};
        mutable bool mViewProjMatrixDirty{true};
        mutable bool mWorldViewProjMatrixDirty{true};
        mutable bool mInverseWorldMatrixDirty{true};
        mutable bool mInverseWorldViewMatrixDirty{true};
        mutable bool mInverseViewMatrixDirty{true};
        mutable bool mInverseTransposeWorldMatrixDirty{true};
        mutable bool mInverseTransposeWorldViewMatrixDirty{true};
        mutable bool mCameraPositionDirty{true};
        mutable bool mCameraPositionObjectSpaceDirty{true};
        mutable bool mTextureViewProjMatrixDirty[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        mutable bool mTextureWorldViewProjMatrixDirty[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        mutable bool mSpotlightViewProjMatrixDirty[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        mutable bool mSpotlightWorldViewProjMatrixDirty[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        mutable bool mShadowCamDepthRangesDirty[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        ColourValue mAmbientLight;
        ColourValue mFogColour;
        Vector4f mFogParams;
        Vector4f mPointParams;
        int mPassNumber{0};
        mutable Vector4 mSceneDepthRange;
        mutable bool mSceneDepthRangeDirty{true};
        mutable bool mLodCameraPositionDirty{true};
        mutable bool mLodCameraPositionObjectSpaceDirty{true};

        const Renderable* mCurrentRenderable{nullptr};
        const Camera* mCurrentCamera{nullptr};
        bool mCameraRelativeRendering{false};
        Vector3 mCameraRelativePosition;
        const LightList* mCurrentLightList{nullptr};
        const Frustum* mCurrentTextureProjector[OGRE_MAX_SIMULTANEOUS_LIGHTS];
        const RenderTarget* mCurrentRenderTarget{nullptr};
        const Viewport* mCurrentViewport{nullptr};
        const SceneManager* mCurrentSceneManager{nullptr};
        const VisibleObjectsBoundsInfo* mMainCamBoundsInfo{nullptr};
        const Pass* mCurrentPass{nullptr};

        SceneNode mDummyNode;
        Light mBlankLight;
    public:
        AutoParamDataSource();
        /** Updates the current renderable */
        void setCurrentRenderable(const Renderable* rend);
        /** Sets the world matrices, avoid query from renderable again */
        void setWorldMatrices(const Affine3* m, size_t count);
        /** Updates the current camera */
        void setCurrentCamera(const Camera* cam, bool useCameraRelative);
        /** Sets the light list that should be used, and it's base index from the global list */
        void setCurrentLightList(const LightList* ll);
        /** Sets the current texture projector for a index */
        void setTextureProjector(const Frustum* frust, size_t index);
        /** Sets the current render target */
        void setCurrentRenderTarget(const RenderTarget* target);
        /** Sets the current viewport */
        void setCurrentViewport(const Viewport* viewport);
        /** Sets the shadow extrusion distance to be used for dir lights. */
        void setShadowDirLightExtrusionDistance(Real dist);
        /** Sets the shadow extrusion distance to be used for point lights. */
        void setShadowPointLightExtrusionDistance(Real dist);
        /** Sets the main camera's scene bounding information */
        void setMainCamBoundsInfo(VisibleObjectsBoundsInfo* info);
        /** Set the current scene manager for enquiring on demand */
        void setCurrentSceneManager(const SceneManager* sm);
        /** Sets the current pass */
        void setCurrentPass(const Pass* pass);

		/** Returns the current bounded camera */
		auto getCurrentCamera() const noexcept -> const Camera*;

        auto getWorldMatrix() const noexcept -> const Affine3&;
        auto getWorldMatrixArray() const noexcept -> const Affine3*;
        auto getWorldMatrixCount() const -> size_t;
        auto getViewMatrix() const noexcept -> const Affine3&;
        auto getViewProjectionMatrix() const noexcept -> const Matrix4&;
        auto getProjectionMatrix() const noexcept -> const Matrix4&;
        auto getWorldViewProjMatrix() const noexcept -> const Matrix4&;
        auto getWorldViewMatrix() const noexcept -> const Affine3&;
        auto getInverseWorldMatrix() const noexcept -> const Affine3&;
        auto getInverseWorldViewMatrix() const noexcept -> const Affine3&;
        auto getInverseViewMatrix() const noexcept -> const Affine3&;
        auto getInverseTransposeWorldMatrix() const noexcept -> const Matrix4&;
        auto getInverseTransposeWorldViewMatrix() const noexcept -> const Matrix4&;
        auto getCameraPosition() const noexcept -> const Vector4&;
        auto getCameraPositionObjectSpace() const noexcept -> const Vector4&;
        auto  getCameraRelativePosition() const -> const Vector4;
        auto getLodCameraPosition() const noexcept -> const Vector4&;
        auto getLodCameraPositionObjectSpace() const noexcept -> const Vector4&;
        auto hasLightList() const -> bool { return mCurrentLightList != nullptr; }
        /** Get the light which is 'index'th closest to the current object */        
        auto getLightNumber(size_t index) const -> float;
        auto getLightCount() const noexcept -> float;
        auto getLightCastsShadows(size_t index) const -> float;
        auto getLightDiffuseColour(size_t index) const -> const ColourValue&;
        auto getLightSpecularColour(size_t index) const -> const ColourValue&;
        auto getLightDiffuseColourWithPower(size_t index) const -> const ColourValue;
        auto getLightSpecularColourWithPower(size_t index) const -> const ColourValue;
        auto getLightPosition(size_t index) const -> Vector3;
        auto getLightAs4DVector(size_t index) const -> Vector4;
        auto getLightDirection(size_t index) const -> Vector3;
        auto getLightPowerScale(size_t index) const -> Real;
        auto getLightAttenuation(size_t index) const -> const Vector4f&;
        auto getSpotlightParams(size_t index) const -> Vector4f;
        void setAmbientLightColour(const ColourValue& ambient);
        auto getAmbientLightColour() const noexcept -> const ColourValue&;
        auto getSurfaceAmbientColour() const noexcept -> const ColourValue&;
        auto getSurfaceDiffuseColour() const noexcept -> const ColourValue&;
        auto getSurfaceSpecularColour() const noexcept -> const ColourValue&;
        auto getSurfaceEmissiveColour() const noexcept -> const ColourValue&;
        auto getSurfaceShininess() const -> Real;
        auto getSurfaceAlphaRejectionValue() const -> Real;
        auto getDerivedAmbientLightColour() const -> ColourValue;
        auto getDerivedSceneColour() const -> ColourValue;
        void setFog(FogMode mode, const ColourValue& colour, Real expDensity, Real linearStart, Real linearEnd);
        auto getFogColour() const noexcept -> const ColourValue&;
        auto getFogParams() const noexcept -> const Vector4f&;
        void setPointParameters(bool attenuation, const Vector4f& params);
        auto getPointParams() const noexcept -> const Vector4f&;
        auto getTextureViewProjMatrix(size_t index) const -> const Matrix4&;
        auto getTextureWorldViewProjMatrix(size_t index) const -> const Matrix4&;
        auto getSpotlightViewProjMatrix(size_t index) const -> const Matrix4&;
        auto getSpotlightWorldViewProjMatrix(size_t index) const -> const Matrix4&;
        auto getTextureTransformMatrix(size_t index) const -> const Matrix4&;
        auto getCurrentRenderTarget() const noexcept -> const RenderTarget*;
        auto getCurrentRenderable() const noexcept -> const Renderable*;
        auto getCurrentPass() const noexcept -> const Pass*;
        auto getTextureSize(size_t index) const -> Vector4f;
        auto getInverseTextureSize(size_t index) const -> Vector4f;
        auto getPackedTextureSize(size_t index) const -> Vector4f;
        auto getShadowExtrusionDistance() const -> Real;
        auto getSceneDepthRange() const noexcept -> const Vector4&;
        auto getShadowSceneDepthRange(size_t index) const -> const Vector4&;
        auto getShadowColour() const noexcept -> const ColourValue&;
        auto getInverseViewProjMatrix() const -> Matrix4;
        auto getInverseTransposeViewProjMatrix() const -> Matrix4;
        auto getTransposeViewProjMatrix() const -> Matrix4;
        auto getTransposeViewMatrix() const -> Matrix4;
        auto getInverseTransposeViewMatrix() const -> Matrix4;
        auto getTransposeProjectionMatrix() const -> Matrix4;
        auto getInverseProjectionMatrix() const -> Matrix4;
        auto getInverseTransposeProjectionMatrix() const -> Matrix4;
        auto getTransposeWorldViewProjMatrix() const -> Matrix4;
        auto getInverseWorldViewProjMatrix() const -> Matrix4;
        auto getInverseTransposeWorldViewProjMatrix() const -> Matrix4;
        auto getTransposeWorldViewMatrix() const -> Matrix4;
        auto getTransposeWorldMatrix() const -> Matrix4;
        auto getTime() const -> Real;
        auto getTime_0_X(Real x) const -> Real;
        auto getCosTime_0_X(Real x) const -> Real;
        auto getSinTime_0_X(Real x) const -> Real;
        auto getTanTime_0_X(Real x) const -> Real;
        auto getTime_0_X_packed(Real x) const -> Vector4f;
        auto getTime_0_1(Real x) const -> Real;
        auto getCosTime_0_1(Real x) const -> Real;
        auto getSinTime_0_1(Real x) const -> Real;
        auto getTanTime_0_1(Real x) const -> Real;
        auto getTime_0_1_packed(Real x) const -> Vector4f;
        auto getTime_0_2Pi(Real x) const -> Real;
        auto getCosTime_0_2Pi(Real x) const -> Real;
        auto getSinTime_0_2Pi(Real x) const -> Real;
        auto getTanTime_0_2Pi(Real x) const -> Real;
        auto getTime_0_2Pi_packed(Real x) const -> Vector4f;
        auto getFrameTime() const -> Real;
        auto getFPS() const -> Real;
        auto getViewportWidth() const -> Real;
        auto getViewportHeight() const -> Real;
        auto getInverseViewportWidth() const -> Real;
        auto getInverseViewportHeight() const -> Real;
        auto getViewDirection() const -> Vector3;
        auto getViewSideVector() const -> Vector3;
        auto getViewUpVector() const -> Vector3;
        auto getFOV() const -> Real;
        auto getNearClipDistance() const -> Real;
        auto getFarClipDistance() const -> Real;
        auto getPassNumber() const noexcept -> int;
        void setPassNumber(const int passNumber);
        void incPassNumber();
        void updateLightCustomGpuParameter(const GpuProgramParameters::AutoConstantEntry& constantEntry, GpuProgramParameters *params) const;
    };
    /** @} */
    /** @} */
}

#endif
