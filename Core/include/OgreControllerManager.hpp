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

#include <cstddef>

export module Ogre.Core:ControllerManager;

export import :Common;
export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :SharedPtr;
export import :Singleton;
export import :TextureUnitState;

export import <set>;

export
namespace Ogre {
template <typename T> class Controller;
template <typename T> class ControllerFunction;
template <typename T> class ControllerValue;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Animation
    *  @{
    */

    using ControllerValueRealPtr = SharedPtr<ControllerValue<Real>>;
    using ControllerFunctionRealPtr = SharedPtr<ControllerFunction<Real>>;

    /** Class for managing Controller instances.
    @remarks
        This class is responsible to keeping tabs on all the Controller instances registered
        and updating them when requested. It also provides a number of convenience methods
        for creating commonly used controllers (such as texture animators).
    */
    class ControllerManager : public Singleton<ControllerManager>, public ControllerAlloc
    {
    private:
        using ControllerList = std::set<Controller<Real> *>;
        ControllerList mControllers;

        /// Global predefined controller
        ControllerValueRealPtr mFrameTimeController;
        
        /// Global predefined controller
        ControllerFunctionRealPtr mPassthroughFunction;

        /// Last frame number updated
        unsigned long mLastFrameNumber{0};

    public:
        ControllerManager();
        ~ControllerManager();

        /** Creates a new controller and registers it with the manager.
        */
        auto createController(const ControllerValueRealPtr& src,
            const ControllerValueRealPtr& dest, const ControllerFunctionRealPtr& func) -> Controller<Real>*;

        /** Creates a new controller use frame time source and passthrough controller function.
        */
        auto createFrameTimePassthroughController(
            const ControllerValueRealPtr& dest) -> Controller<Real>*;

        /** Destroys all the controllers in existence.
        */
        void clearControllers();

        /** Updates all the registered controllers.
        */
        void updateAllControllers();


        /** Returns a ControllerValue which provides the time since the last frame as a control value source.
        @remarks
            A common source value to use to feed into a controller is the time since the last frame. This method
            returns a pointer to a common source value which provides this information.
        @par
            Remember the value will only be up to date after the RenderSystem::beginFrame method is called.
        @see
            RenderSystem::beginFrame
        */
        [[nodiscard]] auto getFrameTimeSource() const noexcept -> const ControllerValueRealPtr&;

        /** Retrieve a simple passthrough controller function. */
        [[nodiscard]] auto getPassthroughControllerFunction() const noexcept -> const ControllerFunctionRealPtr&;

        /** Creates a texture layer animator controller.
        @remarks
            This helper method creates the Controller, ControllerValue and ControllerFunction classes required
            to animate a texture.
        @param layer
            TextureUnitState object to animate
        @param sequenceTime
            The amount of time in seconds it will take to loop through all the frames.
        */
        auto createTextureAnimator(TextureUnitState* layer, Real sequenceTime) -> Controller<Real>*;

        /** Creates a basic time-based texture uv coordinate modifier designed for creating scrolling textures.
        @remarks
            This simple method allows you to easily create constant-speed uv scrolling textures. If you want to 
            specify different speed values for horizontal and vertical scroll, use the specific methods
            ControllerManager::createTextureUScroller and ControllerManager::createTextureVScroller.
            If you want more control, look up the ControllerManager::createTextureWaveTransformer 
            for more complex wave-based scrollers / stretchers / rotators.
        @param layer
            The texture layer to animate.
        @param speed
            Speed of horizontal (u-coord) and vertical (v-coord) scroll, in complete wraps per second.
        */
        auto createTextureUVScroller(TextureUnitState* layer, Real speed) -> Controller<Real>*;

        /** Creates a basic time-based texture u coordinate modifier designed for creating scrolling textures.
        @remarks
            This simple method allows you to easily create constant-speed u scrolling textures. If you want more
            control, look up the ControllerManager::createTextureWaveTransformer for more complex wave-based
            scrollers / stretchers / rotators.
        @param layer
            The texture layer to animate.
        @param uSpeed
            Speed of horizontal (u-coord) scroll, in complete wraps per second.
        */
        auto createTextureUScroller(TextureUnitState* layer, Real uSpeed) -> Controller<Real>*;

        /** Creates a basic time-based texture v coordinate modifier designed for creating scrolling textures.
        @remarks
            This simple method allows you to easily create constant-speed v scrolling textures. If you want more
            control, look up the ControllerManager::createTextureWaveTransformer for more complex wave-based
            scrollers / stretchers / rotators.
        @param layer
            The texture layer to animate.
        @param vSpeed
            Speed of vertical (v-coord) scroll, in complete wraps per second.
        */
        auto createTextureVScroller(TextureUnitState* layer, Real vSpeed) -> Controller<Real>*;

        /** Creates a basic time-based texture coordinate modifier designed for creating rotating textures.
        @return
            This simple method allows you to easily create constant-speed rotating textures. If you want more
            control, look up the ControllerManager::createTextureWaveTransformer for more complex wave-based
            scrollers / stretchers / rotators.
        @param layer
            The texture layer to rotate.
        @param speed
            Speed of rotation, in complete anticlockwise revolutions per second.
        */
        auto createTextureRotater(TextureUnitState* layer, Real speed) -> Controller<Real>*;

        /** Creates a very flexible time-based texture transformation which can alter the scale, position or
            rotation of a texture based on a wave function.
        @param layer
            The texture layer to affect.
        @param ttype
            The type of transform, either translate (scroll), scale (stretch) or rotate (spin).
        @param waveType
            The shape of the wave, see WaveformType enum class for details.
        @param base
            The base value of the output.
        @param frequency
            The speed of the wave in cycles per second.
        @param phase
            The offset of the start of the wave, e.g. 0.5 to start half-way through the wave.
        @param amplitude
            Scales the output so that instead of lying within 0..1 it lies within 0..1*amplitude for exaggerated effects.
        */
        auto createTextureWaveTransformer(TextureUnitState* layer, TextureUnitState::TextureTransformType ttype,
            WaveformType waveType, Real base = 0, Real frequency = 1, Real phase = 0, Real amplitude = 1) -> Controller<Real>*;

        /** Creates a controller for passing a frame time value through to a vertex / fragment program parameter.
        @remarks
            The destination parameter is expected to be a float, and the '.x' attribute will be populated
            with the appropriately scaled time value.
        @param params
            The parameters to update.
        @param paramIndex
            The index of the parameter to update; if you want a named parameter, then
            retrieve the index beforehand using GpuProgramParameters::getParamIndex.
        @param timeFactor
            The factor by which to adjust the time elapsed by before passing it to the program.
        */
        auto createGpuProgramTimerParam(GpuProgramParametersSharedPtr params, size_t paramIndex,
            Real timeFactor = 1.0f) -> Controller<Real>*;

        /** Removes & destroys the controller passed in as a pointer.
        */
        void destroyController(Controller<Real>* controller);

        /** Return relative speed of time as perceived by time based controllers.
        @remarks
            See setTimeFactor for full information on the meaning of this value.
        */
        [[nodiscard]] auto getTimeFactor() const -> Real;

        /** Set the relative speed to update frame time based controllers.
        @remarks
            Normally any controllers which use time as an input (FrameTimeController) are updated
            automatically in line with the real passage of time. This method allows you to change
            that, so that controllers are told that the time is passing slower or faster than it
            actually is. Use this to globally speed up / slow down the effect of time-based controllers.
        @param tf
            The virtual speed of time (1.0 is real time).
        */
        void setTimeFactor(Real tf);

        /** Gets the constant that is added to time lapsed between each frame.
        @remarks
            See setFrameDelay for full information on the meaning of this value.
        */
        [[nodiscard]] auto getFrameDelay() const -> Real;

        /** Sets a constant frame rate.
        @remarks
            This function is useful when rendering a sequence to
            files that should create a film clip with constant frame
            rate.
            It will ensure that scrolling textures and animations
            move at a constant frame rate.
        @param fd
            The delay in seconds wanted between each frame 
            (1.0f / 25.0f means a seconds worth of animation is done 
            in 25 frames).
        */
        void setFrameDelay(Real fd);

        /** Return the elapsed time.
        @remarks
            See setElapsedTime for full information on the meaning of this value.
        */
        [[nodiscard]] auto getElapsedTime() const -> Real;

        /** Set the elapsed time.
        @remarks
            Normally elapsed time accumulated all frames time (which speed relative to time
            factor) since the rendering loop started. This method allows your to change that to
            special time, so some elapsed-time-based globally effect is repeatable.
        @param elapsedTime
            The new elapsed time.
        */
        void setElapsedTime(Real elapsedTime);

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> ControllerManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> ControllerManager*;
    };

    /** @} */
    /** @} */

} // namespace Ogre
