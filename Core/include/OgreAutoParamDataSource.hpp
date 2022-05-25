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
        const Light& getLight(size_t index) const;
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
		const Camera* getCurrentCamera() const noexcept;

        const Affine3& getWorldMatrix() const noexcept;
        const Affine3* getWorldMatrixArray() const noexcept;
        size_t getWorldMatrixCount() const;
        const Affine3& getViewMatrix() const noexcept;
        const Matrix4& getViewProjectionMatrix() const noexcept;
        const Matrix4& getProjectionMatrix() const noexcept;
        const Matrix4& getWorldViewProjMatrix() const noexcept;
        const Affine3& getWorldViewMatrix() const noexcept;
        const Affine3& getInverseWorldMatrix() const noexcept;
        const Affine3& getInverseWorldViewMatrix() const noexcept;
        const Affine3& getInverseViewMatrix() const noexcept;
        const Matrix4& getInverseTransposeWorldMatrix() const noexcept;
        const Matrix4& getInverseTransposeWorldViewMatrix() const noexcept;
        const Vector4& getCameraPosition() const noexcept;
        const Vector4& getCameraPositionObjectSpace() const noexcept;
        const Vector4  getCameraRelativePosition() const;
        const Vector4& getLodCameraPosition() const noexcept;
        const Vector4& getLodCameraPositionObjectSpace() const noexcept;
        bool hasLightList() const { return mCurrentLightList != nullptr; }
        /** Get the light which is 'index'th closest to the current object */        
        float getLightNumber(size_t index) const;
        float getLightCount() const noexcept;
        float getLightCastsShadows(size_t index) const;
        const ColourValue& getLightDiffuseColour(size_t index) const;
        const ColourValue& getLightSpecularColour(size_t index) const;
        const ColourValue getLightDiffuseColourWithPower(size_t index) const;
        const ColourValue getLightSpecularColourWithPower(size_t index) const;
        Vector3 getLightPosition(size_t index) const;
        Vector4 getLightAs4DVector(size_t index) const;
        Vector3 getLightDirection(size_t index) const;
        Real getLightPowerScale(size_t index) const;
        const Vector4f& getLightAttenuation(size_t index) const;
        Vector4f getSpotlightParams(size_t index) const;
        void setAmbientLightColour(const ColourValue& ambient);
        const ColourValue& getAmbientLightColour() const noexcept;
        const ColourValue& getSurfaceAmbientColour() const noexcept;
        const ColourValue& getSurfaceDiffuseColour() const noexcept;
        const ColourValue& getSurfaceSpecularColour() const noexcept;
        const ColourValue& getSurfaceEmissiveColour() const noexcept;
        Real getSurfaceShininess() const;
        Real getSurfaceAlphaRejectionValue() const;
        ColourValue getDerivedAmbientLightColour() const;
        ColourValue getDerivedSceneColour() const;
        void setFog(FogMode mode, const ColourValue& colour, Real expDensity, Real linearStart, Real linearEnd);
        const ColourValue& getFogColour() const noexcept;
        const Vector4f& getFogParams() const noexcept;
        void setPointParameters(bool attenuation, const Vector4f& params);
        const Vector4f& getPointParams() const noexcept;
        const Matrix4& getTextureViewProjMatrix(size_t index) const;
        const Matrix4& getTextureWorldViewProjMatrix(size_t index) const;
        const Matrix4& getSpotlightViewProjMatrix(size_t index) const;
        const Matrix4& getSpotlightWorldViewProjMatrix(size_t index) const;
        const Matrix4& getTextureTransformMatrix(size_t index) const;
        const RenderTarget* getCurrentRenderTarget() const noexcept;
        const Renderable* getCurrentRenderable() const noexcept;
        const Pass* getCurrentPass() const noexcept;
        Vector4f getTextureSize(size_t index) const;
        Vector4f getInverseTextureSize(size_t index) const;
        Vector4f getPackedTextureSize(size_t index) const;
        Real getShadowExtrusionDistance() const;
        const Vector4& getSceneDepthRange() const noexcept;
        const Vector4& getShadowSceneDepthRange(size_t index) const;
        const ColourValue& getShadowColour() const noexcept;
        Matrix4 getInverseViewProjMatrix() const;
        Matrix4 getInverseTransposeViewProjMatrix() const;
        Matrix4 getTransposeViewProjMatrix() const;
        Matrix4 getTransposeViewMatrix() const;
        Matrix4 getInverseTransposeViewMatrix() const;
        Matrix4 getTransposeProjectionMatrix() const;
        Matrix4 getInverseProjectionMatrix() const;
        Matrix4 getInverseTransposeProjectionMatrix() const;
        Matrix4 getTransposeWorldViewProjMatrix() const;
        Matrix4 getInverseWorldViewProjMatrix() const;
        Matrix4 getInverseTransposeWorldViewProjMatrix() const;
        Matrix4 getTransposeWorldViewMatrix() const;
        Matrix4 getTransposeWorldMatrix() const;
        Real getTime() const;
        Real getTime_0_X(Real x) const;
        Real getCosTime_0_X(Real x) const;
        Real getSinTime_0_X(Real x) const;
        Real getTanTime_0_X(Real x) const;
        Vector4f getTime_0_X_packed(Real x) const;
        Real getTime_0_1(Real x) const;
        Real getCosTime_0_1(Real x) const;
        Real getSinTime_0_1(Real x) const;
        Real getTanTime_0_1(Real x) const;
        Vector4f getTime_0_1_packed(Real x) const;
        Real getTime_0_2Pi(Real x) const;
        Real getCosTime_0_2Pi(Real x) const;
        Real getSinTime_0_2Pi(Real x) const;
        Real getTanTime_0_2Pi(Real x) const;
        Vector4f getTime_0_2Pi_packed(Real x) const;
        Real getFrameTime() const;
        Real getFPS() const;
        Real getViewportWidth() const;
        Real getViewportHeight() const;
        Real getInverseViewportWidth() const;
        Real getInverseViewportHeight() const;
        Vector3 getViewDirection() const;
        Vector3 getViewSideVector() const;
        Vector3 getViewUpVector() const;
        Real getFOV() const;
        Real getNearClipDistance() const;
        Real getFarClipDistance() const;
        int getPassNumber() const noexcept;
        void setPassNumber(const int passNumber);
        void incPassNumber();
        void updateLightCustomGpuParameter(const GpuProgramParameters::AutoConstantEntry& constantEntry, GpuProgramParameters *params) const;
    };
    /** @} */
    /** @} */
}

#endif
