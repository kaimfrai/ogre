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
module;

#include <cassert>
#include <cstddef>

export module Ogre.Core:Light;

export import :AxisAlignedBox;
export import :Camera;
export import :ColourValue;
export import :Common;
export import :GpuProgramParams;
export import :Math;
export import :MovableObject;
export import :Node;
export import :PlaneBoundedVolume;
export import :Platform;
export import :Prerequisites;
export import :Quaternion;
export import :Renderable;
export import :SharedPtr;
export import :Vector;

export import <map>;

export
namespace Ogre {
class RenderQueue;
struct Sphere;


    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */
    /** Representation of a dynamic light source in the scene.

        Lights are added to the scene like any other object. They contain various
        parameters like type, attenuation (how light intensity fades with
        distance), colour etc.

        The light colour is computed based on the
        [Direct3D Light Model](https://docs.microsoft.com/en-us/windows/win32/direct3d9/mathematics-of-lighting) as:

        \f[ L_d = C_d \cdot p \cdot ( N \cdot L_{dir}) \cdot A \cdot S \f]
        \f[ L_s = C_s \cdot p \cdot ( N \cdot H)^s \cdot A \cdot S \f]

        where
        \f[ A = \frac{1}{c + l \cdot d + q \cdot d^2} \f]
        and only computed when attenuation is enabled,

        \f[ S = \left[ \frac{\rho - cos(0.5 \cdot \phi)}{cos(0.5 \cdot \theta) - cos(0.5 \cdot \phi)} \right]^f \f]
        and only computed with spotlights

        - \f$C_d\f$ is the light diffuse colour
        - \f$C_s\f$ is the light specular colour
        - \f$p\f$ is the light power scale factor
        - \f$s\f$ is the surface shininess
        - \f$N\f$ is the current surface normal
        - \f$L_{dir}\f$ is vector from the vertex position to the light (constant for directional lights)
        - \f$H = normalised(L_{dir} + V)\f$, where V is the vector from the vertex position to the camera
        - \f$c, l, q\f$ are the constant, linear and quadratic attenuation factors
        - \f$d = |L_{dir}|\f$
        - \f$\theta, \phi, f\f$ are the spotlight inner angle, outer angle and falloff
        - \f$\rho = \langle L_{dir} , L_{dcs} \rangle \f$ where \f$L_{dcs}\f$ is the light direction in camera space

        The defaults when a light is created is pure white diffuse light, with no
        attenuation (does not decrease with distance) and a range of 1000 world units.

        Lights are created by using the SceneManager::createLight method. They subsequently must be
        added to a SceneNode to orient them in the scene and to allow moving them.

        Remember also that dynamic lights rely on modifying the colour of vertices based on the position of
        the light compared to an object's vertex normals. Dynamic lighting will only look good if the
        object being lit has a fair level of tessellation and the normals are properly set. This is particularly
        true for the spotlight which will only look right on highly tessellated models. In the future OGRE may be
        extended for certain scene types so an alternative to the standard dynamic lighting may be used, such
            as dynamic lightmaps.
    */
    class Light : public MovableObject
    {
    public:
        /// Temp tag used for sorting
        Real tempSquareDist;
        /// internal method for calculating current squared distance from some world position
        void _calcTempSquareDist(const Vector3& worldPos);

        /// Defines the type of light
        enum class LightTypes : uint8
        {
            /// Point light sources give off light equally in all directions, so require only position not direction
            POINT = 0,
            /// Directional lights simulate parallel light beams from a distant source, hence have direction but no position
            DIRECTIONAL = 1,
            /// Spotlights simulate a cone of light from a source so require position and direction, plus extra values for falloff
            SPOTLIGHT = 2
        };

        /** Default constructor (for Python mainly).
        */
        Light();

        /** Normal constructor. Should not be called directly, but rather the SceneManager::createLight method should be used.
        */
        Light(std::string_view name);

        /** Standard destructor.
        */
        ~Light() override;

        /** Sets the type of light - see LightTypes for more info.
        */
        void setType(LightTypes type);

        /** Returns the light type.
        */
        auto getType() const -> LightTypes;

        /** Sets the colour of the diffuse light given off by this source.
        @remarks
            Material objects have ambient, diffuse and specular values which indicate how much of each type of
            light an object reflects. This value denotes the amount and colour of this type of light the light
            exudes into the scene. The actual appearance of objects is a combination of the two.
        @par
            Diffuse light simulates the typical light emanating from light sources and affects the base colour
            of objects together with ambient light.
        */
        void setDiffuseColour(float red, float green, float blue);

        /// @overload
        void setDiffuseColour(const ColourValue& colour);

        /** Returns the colour of the diffuse light given off by this light source (see setDiffuseColour for more info).
        */
        auto getDiffuseColour() const noexcept -> const ColourValue&;

        /** Sets the colour of the specular light given off by this source.
        @remarks
            Material objects have ambient, diffuse and specular values which indicate how much of each type of
            light an object reflects. This value denotes the amount and colour of this type of light the light
            exudes into the scene. The actual appearance of objects is a combination of the two.
        @par
            Specular light affects the appearance of shiny highlights on objects, and is also dependent on the
            'shininess' Material value.
        */
        void setSpecularColour(float red, float green, float blue);

        /// @overload
        void setSpecularColour(const ColourValue& colour);

        /** Returns the colour of specular light given off by this light source.
        */
        auto getSpecularColour() const noexcept -> const ColourValue&;

        /** Sets the attenuation parameters of the light source i.e. how it diminishes with distance.
        @remarks
            Lights normally get fainter the further they are away. Also, each light is given a maximum range
            beyond which it cannot affect any objects.
        @par
            Light attenuation is not applicable to directional lights since they have an infinite range and
            constant intensity.
        @par
            This follows a standard attenuation approach - see any good 3D text for the details of what they mean
            since i don't have room here!
        @param range
            The absolute upper range of the light in world units.
        @param constant
            The constant factor in the attenuation formula: 1.0 means never attenuate, 0.0 is complete attenuation.
        @param linear
            The linear factor in the attenuation formula: 1 means attenuate evenly over the distance.
        @param quadratic
            The quadratic factor in the attenuation formula: adds a curvature to the attenuation formula.
        */
        void setAttenuation(float range, float constant, float linear, float quadratic)
        {
            mAttenuation = {range, constant, linear, quadratic};
        }

        /** Returns the absolute upper range of the light.
        */
        auto getAttenuationRange() const noexcept -> float { return mAttenuation[0]; }

        /** Returns the constant factor in the attenuation formula.
        */
        auto getAttenuationConstant() const noexcept -> float { return mAttenuation[1]; }

        /** Returns the linear factor in the attenuation formula.
        */
        auto getAttenuationLinear() const noexcept -> float { return mAttenuation[2]; }

        /** Returns the quadric factor in the attenuation formula.
        */
        auto getAttenuationQuadric() const noexcept -> float { return mAttenuation[3]; }

        /// Returns all the attenuation params as (range, constant, linear, quadratic)
        auto getAttenuation() const noexcept -> const Vector4f& { return mAttenuation; }

        /** Sets the range of a spotlight, i.e. the angle of the inner and outer cones
            and the rate of falloff between them.
        @param innerAngle
            Angle covered by the bright inner cone
            @note
                The inner cone applicable only to Direct3D, it'll always treat as zero in OpenGL.
        @param outerAngle
            Angle covered by the outer cone
        @param falloff
            The rate of falloff between the inner and outer cones. 1.0 means a linear falloff,
            less means slower falloff, higher means faster falloff.
        */
        void setSpotlightRange(const Radian& innerAngle, const Radian& outerAngle, Real falloff = 1.0);

        /** Returns the angle covered by the spotlights inner cone.
        */
        auto getSpotlightInnerAngle() const noexcept -> const Radian&;

        /** Returns the angle covered by the spotlights outer cone.
        */
        auto getSpotlightOuterAngle() const noexcept -> const Radian&;

        /** Returns the falloff between the inner and outer cones of the spotlight.
        */
        auto getSpotlightFalloff() const -> Real;

        /** Sets the angle covered by the spotlights inner cone.
        */
        void setSpotlightInnerAngle(const Radian& val);

        /** Sets the angle covered by the spotlights outer cone.
        */
        void setSpotlightOuterAngle(const Radian& val);

        /** Sets the falloff between the inner and outer cones of the spotlight.
        */
        void setSpotlightFalloff(Real val);

        /** Set the near clip plane distance to be used by spotlights that use light
            clipping, allowing you to render spots as if they start from further
            down their frustum. 
        @param nearClip
            The near distance.
        */
        void setSpotlightNearClipDistance(Real nearClip) { mSpotNearClip = nearClip; }
        
        /** Returns the near clip plane distance to be used by spotlights that use light
            clipping.
        */
        auto getSpotlightNearClipDistance() const noexcept -> Real { return mSpotNearClip; }
        
        /** Set a scaling factor to indicate the relative power of a light.
        @remarks
            This factor is only useful in High Dynamic Range (HDR) rendering.
            You can bind it to a shader variable to take it into account,
            @see GpuProgramParameters
        @param power
            The power rating of this light, default is 1.0.
        */
        void setPowerScale(Real power);

        /** Returns the scaling factor which indicates the relative power of a
            light.
        */
        auto getPowerScale() const -> Real;

        auto getBoundingRadius() const noexcept -> Real override { return 0; }
        auto getBoundingBox() const noexcept -> const AxisAlignedBox& override;

        void _updateRenderQueue(RenderQueue* queue) override {} // No rendering

        /** @copydoc MovableObject::getMovableType */
        auto getMovableType() const noexcept -> std::string_view override;

        /** Retrieves the position of the light including any transform from nodes it is attached to. 
        @param cameraRelativeIfSet If set to true, returns data in camera-relative units if that's been set up (render use)
        */
        auto getDerivedPosition(bool cameraRelativeIfSet = false) const -> Vector3
        {
            assert(mParentNode && "Light must be attached to a SceneNode");
            auto ret = mParentNode->_getDerivedPosition();
            if (cameraRelativeIfSet && mCameraToBeRelativeTo)
                ret -= mCameraToBeRelativeTo->getDerivedPosition();
            return ret;
        }

        /** Retrieves the direction of the light including any transform from nodes it is attached to. */
        auto getDerivedDirection() const -> Vector3
        {
            assert(mParentNode && "Light must be attached to a SceneNode");
            return -mParentNode->_getDerivedOrientation().zAxis();
        }

        /** @copydoc MovableObject::setVisible
        @remarks
            Although lights themselves are not 'visible', setting a light to invisible
            means it no longer affects the scene.
        */
        void setVisible(bool visible) { MovableObject::setVisible(visible); }

        /** Returns the details of this light as a 4D vector.
        @remarks
            Getting details of a light as a 4D vector can be useful for
            doing general calculations between different light types; for
            example the vector can represent both position lights (w=1.0f)
            and directional lights (w=0.0f) and be used in the same 
            calculations.
        @param cameraRelativeIfSet
            If set to @c true, returns data in camera-relative units if that's been set up (render use).
        */
        auto getAs4DVector(bool cameraRelativeIfSet = false) const -> Vector4;

        /** Internal method for calculating the 'near clip volume', which is
            the volume formed between the near clip rectangle of the 
            camera and the light.
        @remarks
            This volume is a pyramid for a point/spot light and
            a cuboid for a directional light. It can used to detect whether
            an object could be casting a shadow on the viewport. Note that
            the reference returned is to a shared volume which will be 
            reused across calls to this method.
        */
        virtual auto _getNearClipVolume(const Camera* const cam) const -> const PlaneBoundedVolume&;

        /** Internal method for calculating the clip volumes outside of the 
            frustum which can be used to determine which objects are casting
            shadow on the frustum as a whole. 
        @remarks
            Each of the volumes is a pyramid for a point/spot light and
            a cuboid for a directional light. 
        */
        virtual auto _getFrustumClipVolumes(const Camera* const cam) const -> const PlaneBoundedVolumeList&;

        /// Override to return specific type flag
        auto getTypeFlags() const noexcept -> QueryTypeMask override;

        /// @copydoc AnimableObject::createAnimableValue
        auto createAnimableValue(std::string_view valueName) -> AnimableValuePtr override;

        /** Set this light to use a custom shadow camera when rendering texture shadows.
        @remarks
            This changes the shadow camera setup for just this light,  you can set
            the shadow camera setup globally using SceneManager::setShadowCameraSetup
        @see ShadowCameraSetup
        */
        void setCustomShadowCameraSetup(const ShadowCameraSetupPtr& customShadowSetup);

        /** Reset the shadow camera setup to the default. 
        @see ShadowCameraSetup
        */
        void resetCustomShadowCameraSetup();

        /** Return a pointer to the custom shadow camera setup (null means use SceneManager global version). */
        auto getCustomShadowCameraSetup() const noexcept -> const ShadowCameraSetupPtr&;

        void visitRenderables(Renderable::Visitor* visitor, 
            bool debugRenderables = false) override;

        /** Returns the index at which this light is in the current render.
        @remarks
            Lights will be present in the in a list for every renderable,
            detected and sorted appropriately, and sometimes it's useful to know 
            what position in that list a given light occupies. This can vary 
            from frame to frame (and object to object) so you should not use this
            value unless you're sure the context is correct.
        */
        auto _getIndexInFrame() const noexcept -> size_t { return mIndexInFrame; }
        void _notifyIndexInFrame(size_t i) { mIndexInFrame = i; }
        
        /** Sets the maximum distance away from the camera that shadows
            by this light will be visible.

            Shadow techniques can be expensive, therefore it is a good idea
            to limit them to being rendered close to the camera if possible,
            and to skip the expense of rendering shadows for distance objects.
            This method allows you to set the distance at which shadows casters
            will be culled.
        */
        void setShadowFarDistance(Real distance);
        /** Tells the light to use the shadow far distance of the SceneManager
        */
        void resetShadowFarDistance();
        /** Returns the maximum distance away from the camera that shadows
            by this light will be visible.
        */
        auto getShadowFarDistance() const -> Real;
        auto getShadowFarDistanceSquared() const -> Real;

        /** Set the near clip plane distance to be used by the shadow camera, if
            this light casts texture shadows.
        @param nearClip
            The distance, or -1 to use the main camera setting.
        */
        void setShadowNearClipDistance(Real nearClip) { mShadowNearClipDist = nearClip; }

        /** Returns the near clip plane distance to be used by the shadow camera, if
            this light casts texture shadows.
        @remarks
            May be zero if the light doesn't have it's own near distance set;
            use _deriveShadowNearDistance for a version guaranteed to give a result.
        */
        auto getShadowNearClipDistance() const noexcept -> Real { return mShadowNearClipDist; }

        /** Derive a shadow camera near distance from either the light, or
            from the main camera if the light doesn't have its own setting.
        */
        auto _deriveShadowNearClipDistance(const Camera* maincam) const -> Real;

        /** Set the far clip plane distance to be used by the shadow camera, if
            this light casts texture shadows.
        @remarks
            This is different from the 'shadow far distance', which is
            always measured from the main camera. This distance is the far clip plane
            of the light camera.
        @param farClip
            The distance, or -1 to use the main camera setting.
        */
        void setShadowFarClipDistance(Real farClip) { mShadowFarClipDist = farClip; }

        /** Returns the far clip plane distance to be used by the shadow camera, if
            this light casts texture shadows.
        @remarks
            May be zero if the light doesn't have it's own far distance set;
            use _deriveShadowfarDistance for a version guaranteed to give a result.
        */
        auto getShadowFarClipDistance() const noexcept -> Real { return mShadowFarClipDist; }

        /** Derive a shadow camera far distance
        */
        auto _deriveShadowFarClipDistance() const -> Real;

        /// Set the camera which this light should be relative to, for camera-relative rendering
        void _setCameraRelative(Camera* cam);

        /** Sets a custom parameter for this Light, which may be used to 
            drive calculations for this specific Renderable, like GPU program parameters.
        @remarks
            Calling this method simply associates a numeric index with a 4-dimensional
            value for this specific Light. This is most useful if the material
            which this Renderable uses a vertex or fragment program, and has an 
            AutoConstantType::LIGHT_CUSTOM parameter entry. This parameter entry can refer to the
            index you specify as part of this call, thereby mapping a custom
            parameter for this renderable to a program parameter.
        @param index
            The index with which to associate the value. Note that this
            does not have to start at 0, and can include gaps. It also has no direct
            correlation with a GPU program parameter index - the mapping between the
            two is performed by the AutoConstantType::LIGHT_CUSTOM entry, if that is used.
        @param value
            The value to associate.
        */
        void setCustomParameter(uint16 index, const Vector4& value);

        /** Returns the custom value associated with this Light at the given index.
        @param index Index of the parameter to retrieve
        @see setCustomParameter for full details.
        */
        auto getCustomParameter(uint16 index) const -> const Vector4&;

        /** Update a custom GpuProgramParameters constant which is derived from 
            information only this Light knows.
        @remarks
            This method allows a Light to map in a custom GPU program parameter
            based on it's own data. This is represented by a GPU auto parameter
            of AutoConstantType::LIGHT_CUSTOM, and to allow there to be more than one of these per
            Light, the 'data' field on the auto parameter will identify
            which parameter is being updated and on which light. The implementation 
            of this method must identify the parameter being updated, and call a 'setConstant' 
            method on the passed in GpuProgramParameters object.
        @par
            You do not need to override this method if you're using the standard
            sets of data associated with the Renderable as provided by setCustomParameter
            and getCustomParameter. By default, the implementation will map from the
            value indexed by the 'constantEntry.data' parameter to a value previously
            set by setCustomParameter. But custom Renderables are free to override
            this if they want, in any case.
        @param paramIndex
            The index of the constant being updated
        @param constantEntry
            The auto constant entry from the program parameters
        @param params
            The parameters object which this method should call to 
            set the updated parameters.
        */
        virtual void _updateCustomGpuParameter(uint16 paramIndex, 
            const GpuProgramParameters::AutoConstantEntry& constantEntry, 
            GpuProgramParameters* params) const;
                
        /** Check whether a sphere is included in the lighted area of the light 
        @note 
            The function trades accuracy for efficiency. As a result you may get
            false-positives (The function should not return any false-negatives).
        */
        auto isInLightRange(const Ogre::Sphere& sphere) const -> bool;
        
        /** Check whether a bounding box is included in the lighted area of the light
        @note 
            The function trades accuracy for efficiency. As a result you may get
            false-positives (The function should not return any false-negatives).
        */
        auto isInLightRange(const Ogre::AxisAlignedBox& container) const -> bool;
    
    private:
        ColourValue mDiffuse;
        ColourValue mSpecular;

        Radian mSpotOuter;
        Radian mSpotInner;
        Real mSpotFalloff;
        Real mSpotNearClip;
        // range, const, linear, quad coeffs
        Vector4f mAttenuation;
        Real mShadowFarDist;
        Real mShadowFarDistSquared;
        size_t mIndexInFrame;
        
        Real mShadowNearClipDist;
        Real mShadowFarClipDist;

        Camera* mCameraToBeRelativeTo;

        /// Shared class-level name for Movable type.
        static std::string_view const msMovableType;

        mutable PlaneBoundedVolume mNearClipVolume;
        mutable PlaneBoundedVolumeList mFrustumClipVolumes;

        /// Pointer to a custom shadow camera setup.
        mutable ShadowCameraSetupPtr mCustomShadowCameraSetup;

        using CustomParameterMap = std::map<uint16, Vector4>;
        /// Stores the custom parameters for the light.
        CustomParameterMap mCustomParameters;
        Real mPowerScale;
        LightTypes mLightType;
        bool mOwnShadowFarDist;
    };

    /** Factory object for creating Light instances. */
    class LightFactory : public MovableObjectFactory
    {
    private:
        auto createInstanceImpl( std::string_view name, const NameValuePairList* params) -> MovableObject* override;
    public:
        LightFactory() = default;
        ~LightFactory() override = default;

        static std::string_view const FACTORY_TYPE_NAME;

        [[nodiscard]] auto getType() const noexcept -> std::string_view override;
    };
    /** @} */
    /** @} */

} // namespace Ogre
