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
#ifndef OGRE_CORE_PREDEFINEDCONTROLLERS_H
#define OGRE_CORE_PREDEFINEDCONTROLLERS_H

#include <cstddef>
#include <memory>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreController.hpp"
#include "OgreControllerManager.hpp"
#include "OgreFrameListener.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSharedPtr.hpp"

namespace Ogre {
class TextureUnitState;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Animation
    *  @{
    */
    //-----------------------------------------------------------------------
    // Controller Values
    //-----------------------------------------------------------------------
    /** Predefined controller value for getting the latest frame time.
    */
    class FrameTimeControllerValue : public ControllerValue<Real>, public FrameListener
    {
    private:
        Real mFrameTime;
        Real mTimeFactor;
        Real mElapsedTime;
        Real mFrameDelay;

    public:
        /// @deprecated use create()
        FrameTimeControllerValue();

        static auto create() -> ControllerValueRealPtr { return std::make_shared<FrameTimeControllerValue>(); }

        auto frameEnded(const FrameEvent &evt) -> bool override;
        auto frameStarted(const FrameEvent &evt) -> bool override;
        [[nodiscard]]
        auto getValue() const -> Real override;
        void setValue(Real value) override;
        [[nodiscard]]
        auto getTimeFactor() const -> Real;
        void setTimeFactor(Real tf);
        [[nodiscard]]
        auto getFrameDelay() const -> Real;
        void setFrameDelay(Real fd);
        [[nodiscard]]
        auto getElapsedTime() const -> Real;
        void setElapsedTime(Real elapsedTime);
    };

    //-----------------------------------------------------------------------
    /** Predefined controller value for getting / setting the frame number of a texture layer
    */
    class TextureFrameControllerValue : public ControllerValue<Real>
    {
    private:
        TextureUnitState* mTextureLayer;
    public:
        /// @deprecated use create()
        TextureFrameControllerValue(TextureUnitState* t);

        static auto create(TextureUnitState* t) -> ControllerValueRealPtr
        {
            return std::make_shared<TextureFrameControllerValue>(t);
        }

        /** Gets the frame number as a parametric value in the range [0,1]
        */
        [[nodiscard]]
        auto getValue() const -> Real override;
        /** Sets the frame number as a parametric value in the range [0,1]; the actual frame number is (value * numFrames) % numFrames).
        */
        void setValue(Real value) override;

    };
    //-----------------------------------------------------------------------
    /** Predefined controller value for getting / setting a texture coordinate modifications (scales and translates).
        @remarks
            Effects can be applied to the scale or the offset of the u or v coordinates, or both. If separate
            modifications are required to u and v then 2 instances are required to control both independently, or 4
            if you want separate u and v scales as well as separate u and v offsets.
        @par
            Because of the nature of this value, it can accept values outside the 0..1 parametric range.
    */
    class TexCoordModifierControllerValue : public ControllerValue<Real>
    {
    private:
        bool mTransU, mTransV;
        bool mScaleU, mScaleV;
        bool mRotate;
        TextureUnitState* mTextureLayer;
    public:
        /// @deprecated use create
        TexCoordModifierControllerValue(TextureUnitState* t, bool translateU = false, bool translateV = false,
            bool scaleU = false, bool scaleV = false, bool rotate = false );

        /** Constructor.
            @param
                t TextureUnitState to apply the modification to.
            @param
                translateU If true, the u coordinates will be translated by the modification.
            @param
                translateV If true, the v coordinates will be translated by the modification.
            @param
                scaleU If true, the u coordinates will be scaled by the modification.
            @param
                scaleV If true, the v coordinates will be scaled by the modification.
            @param
                rotate If true, the texture will be rotated by the modification.
        */
        static auto create(TextureUnitState* t, bool translateU = false, bool translateV = false,
                                             bool scaleU = false, bool scaleV = false, bool rotate = false) -> ControllerValueRealPtr
        {
            return std::make_shared<TexCoordModifierControllerValue>(t, translateU, translateV, scaleU, scaleV, rotate);
        }

        [[nodiscard]]
        auto getValue() const -> Real override;
        void setValue(Real value) override;

    };

    //-----------------------------------------------------------------------
    /** Predefined controller value for setting a single floating-
        point value in a constant parameter of a vertex or fragment program.
    @remarks
        Any value is accepted, it is propagated into the 'x'
        component of the constant register identified by the index. If you
        need to use named parameters, retrieve the index from the param
        object before setting this controller up.
    @note
        Retrieving a value from the program parameters is not currently
        supported, therefore do not use this controller value as a source,
        only as a target.
    */
    class FloatGpuParameterControllerValue : public ControllerValue<Real>
    {
    private:
        /// The parameters to access
        GpuProgramParametersSharedPtr mParams;
        /// The index of the parameter to be read or set
        size_t mParamIndex;
    public:
        /// @deprecated use create()
        FloatGpuParameterControllerValue(GpuProgramParametersSharedPtr params, size_t index);

        /** Constructor.
            @param
                params The parameters object to access
            @param
                index The index of the parameter to be set
        */
        static auto create(GpuProgramParametersSharedPtr params, size_t index) -> ControllerValueRealPtr
        {
            return std::make_shared<FloatGpuParameterControllerValue>(params, index);
        }

        [[nodiscard]]
        auto getValue() const -> Real override;
        void setValue(Real value) override;

    };
    //-----------------------------------------------------------------------
    // Controller functions
    //-----------------------------------------------------------------------

    /** Predefined controller function which just passes through the original source
    directly to dest.
    */
    class PassthroughControllerFunction : public ControllerFunction<Real>
    {
    public:
        /// @deprecated use create()
        PassthroughControllerFunction(bool deltaInput = false);

        /// @copydoc ControllerFunction::ControllerFunction
        static auto create(bool deltaInput = false) -> ControllerFunctionRealPtr
        {
            return std::make_shared<PassthroughControllerFunction>(deltaInput);
        }

        auto calculate(Real source) -> Real override;
    };

    /** Predefined controller function for dealing with animation.
    */
    class AnimationControllerFunction : public ControllerFunction<Real>
    {
    private:
        Real mSeqTime;
        Real mTime;
    public:
        /// @deprecated use create()
        AnimationControllerFunction(Real sequenceTime, Real timeOffset = 0.0f);

        /** Constructor.
            @param
                sequenceTime The amount of time in seconds it takes to loop through the whole animation sequence.
            @param
                timeOffset The offset in seconds at which to start (default is start at 0)
        */
        static auto create(Real sequenceTime, Real timeOffset = 0.0f) -> ControllerFunctionRealPtr
        {
            return std::make_shared<AnimationControllerFunction>(sequenceTime, timeOffset);
        }

        auto calculate(Real source) -> Real override;

        /** Set the time value manually. */
        void setTime(Real timeVal);
        /** Set the sequence duration value manually. */
        void setSequenceTime(Real seqVal);
    };

    //-----------------------------------------------------------------------
    /** Predefined controller function which simply scales an input to an output value.
    */
    class ScaleControllerFunction : public ControllerFunction<Real>
    {
    private:
        Real mScale;
    public:
        /// @deprecated use create()
        ScaleControllerFunction(Real scalefactor, bool deltaInput);

        /** Constructor, requires a scale factor.
            @param
                scalefactor The multiplier applied to the input to produce the output.
            @param
                deltaInput If true, signifies that the input will be a delta value such that the function should
                 add it to an internal counter before calculating the output.
        */
        static auto create(Real scalefactor, bool deltaInput = false) -> ControllerFunctionRealPtr
        {
            return std::make_shared<ScaleControllerFunction>(scalefactor, deltaInput);
        }

        auto calculate(Real source) -> Real override;
    };

    //-----------------------------------------------------------------------
    /** Predefined controller function based on a waveform.
        @remarks
            A waveform function translates parametric input to parametric output based on a wave.
        @par
            Note that for simplicity of integration with the rest of the controller insfrastructure, the output of
            the wave is parametric i.e. 0..1, rather than the typical wave output of [-1,1]. To compensate for this, the
            traditional output of the wave is scaled by the following function before output:
        @par
            output = (waveoutput + 1) * 0.5
        @par
            Hence a wave output of -1 becomes 0, a wave output of 1 becomes 1, and a wave output of 0 becomes 0.5.
    */
    class WaveformControllerFunction : public ControllerFunction<Real>
    {
    private:
        WaveformType mWaveType;
        Real mBase;
        Real mFrequency;
        Real mPhase;
        Real mAmplitude;
        Real mDutyCycle;

        /** Overridden from ControllerFunction. */
        auto getAdjustedInput(Real input) -> Real;

    public:
        /// @deprecated use create()
        WaveformControllerFunction(WaveformType wType, Real base = 0, Real frequency = 1, Real phase = 0, Real amplitude = 1, bool deltaInput = true, Real dutyCycle = 0.5);

        /** Default constructor, requires at least a wave type, other parameters can be defaulted unless required.
            @param wType the shape of the wave
            @param base the base value of the output from the wave
            @param frequency the speed of the wave in cycles per second
            @param phase the offset of the start of the wave, e.g. 0.5 to start half-way through the wave
            @param amplitude scales the output so that instead of lying within [0,1] it lies within [0,1] * amplitude
            @param
                deltaInput If true, signifies that the input will be a delta value such that the function should
                add it to an internal counter before calculating the output.
            @param
                dutyCycle Used in PWM mode to specify the pulse width.
        */
        static auto create(WaveformType wType, Real base = 0, Real frequency = 1, Real phase = 0, Real amplitude = 1, bool deltaInput = true, Real dutyCycle = 0.5) -> ControllerFunctionRealPtr
        {
            return std::make_shared<WaveformControllerFunction>(wType, base, frequency, phase, amplitude, deltaInput, dutyCycle);
        }

        auto calculate(Real source) -> Real override;
    };

    //-----------------------------------------------------------------------
    /** Predefined controller function based on linear function interpolation.
    */
    class LinearControllerFunction : public ControllerFunction<Real> {
        Real mFrequency;
        std::vector<Real> mKeys;
        std::vector<Real> mValues;
    public:
        /// @deprecated use create()
        LinearControllerFunction(std::vector<Real>  keys, std::vector<Real>  values, Real frequency = 1, bool deltaInput = true);

        /** Constructor, requires keys and values of the function to interpolate

            For simplicity and compatibility with the predefined ControllerValue classes the function domain must be [0,1].
            However, you can use the frequency parameter to rescale the domain to a different range.
            @param
                keys the x-values of the function sampling points. Value range is [0,1]. Must include at least the keys 0 and 1.
            @param
                values the function values f(x) of the function. order must match keys
            @param frequency the speed of the evaluation in cycles per second
            @param
                deltaInput If true, signifies that the input will be a delta value such that the function should
                 add it to an internal counter before calculating the output.
            @note
                there must be the same amount of keys and values
        */
        static auto create(const std::vector<Real>& keys, const std::vector<Real>& values, Real frequency = 1, bool deltaInput = true) -> ControllerFunctionRealPtr
        {
            return std::make_shared<LinearControllerFunction>(keys, values, frequency, deltaInput);
        }

        auto calculate(Real source) -> Real override;
    };
    //-----------------------------------------------------------------------
    /** @} */
    /** @} */

}

#endif
