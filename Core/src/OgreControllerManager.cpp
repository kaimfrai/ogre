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
#include <cassert>

#include "OgreController.hpp"
#include "OgreControllerManager.hpp"
#include "OgrePredefinedControllers.hpp"
#include "OgreRoot.hpp"

namespace Ogre {
    //-----------------------------------------------------------------------
    template<> ControllerManager* Singleton<ControllerManager>::msSingleton = nullptr;
    auto ControllerManager::getSingletonPtr() -> ControllerManager*
    {
        return msSingleton;
    }
    auto ControllerManager::getSingleton() -> ControllerManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    ControllerManager::ControllerManager()
        : mFrameTimeController(new FrameTimeControllerValue())
        , mPassthroughFunction(new PassthroughControllerFunction())
         
    {

    }
    //-----------------------------------------------------------------------
    ControllerManager::~ControllerManager()
    {
        clearControllers();
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::createController(
        const ControllerValueRealPtr& src, const ControllerValueRealPtr& dest,
        const ControllerFunctionRealPtr& func) -> Controller<Real>*
    {
        auto* c = new Controller<Real>(src, dest, func);

        mControllers.insert(c);
        return c;
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::createFrameTimePassthroughController(
        const ControllerValueRealPtr& dest) -> Controller<Real>*
    {
        return createController(getFrameTimeSource(), dest, getPassthroughControllerFunction());
    }
    //-----------------------------------------------------------------------
    void ControllerManager::updateAllControllers()
    {
        // Only update once per frame
        unsigned long thisFrameNumber = Root::getSingleton().getNextFrameNumber();
        if (thisFrameNumber != mLastFrameNumber)
        {
            ControllerList::const_iterator ci;
            for (ci = mControllers.begin(); ci != mControllers.end(); ++ci)
            {
                (*ci)->update();
            }
            mLastFrameNumber = thisFrameNumber;
        }
    }
    //-----------------------------------------------------------------------
    void ControllerManager::clearControllers()
    {
        ControllerList::iterator ci;
        for (ci = mControllers.begin(); ci != mControllers.end(); ++ci)
        {
            delete *ci;
        }
        mControllers.clear();
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::getFrameTimeSource() const -> const ControllerValueRealPtr&
    {
        return mFrameTimeController;
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::getPassthroughControllerFunction() const -> const ControllerFunctionRealPtr&
    {
        return mPassthroughFunction;
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::createTextureAnimator(TextureUnitState* layer, Real sequenceTime) -> Controller<Real>*
    {
        return createController(mFrameTimeController, TextureFrameControllerValue::create(layer),
                                AnimationControllerFunction::create(sequenceTime));
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::createTextureUVScroller(TextureUnitState* layer, Real speed) -> Controller<Real>*
    {
        Controller<Real>* ret = nullptr;

        if (speed != 0)
        {
            // We do both scrolls with a single controller
            // Create function: use -speed since we're altering texture coords so they have reverse effect
            ret = createController(mFrameTimeController, TexCoordModifierControllerValue::create(layer, true, true),
                                   ScaleControllerFunction::create(-speed, true));
        }

        return ret;
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::createTextureUScroller(TextureUnitState* layer, Real uSpeed) -> Controller<Real>*
    {
        Controller<Real>* ret = nullptr;

        if (uSpeed != 0)
        {
            // Create function: use -speed since we're altering texture coords so they have reverse effect
            ret = createController(mFrameTimeController, TexCoordModifierControllerValue::create(layer, true),
                                   ScaleControllerFunction::create(-uSpeed, true));
        }

        return ret;
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::createTextureVScroller(TextureUnitState* layer, Real vSpeed) -> Controller<Real>*
    {
        Controller<Real>* ret = nullptr;

        if (vSpeed != 0)
        {
            // Set up a second controller for v scroll
            // Create function: use -speed since we're altering texture coords so they have reverse effect
            ret = createController(mFrameTimeController, TexCoordModifierControllerValue::create(layer, false, true),
                                   ScaleControllerFunction::create(-vSpeed, true));
        }

        return ret;
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::createTextureRotater(TextureUnitState* layer, Real speed) -> Controller<Real>*
    {
        // Target value is texture coord rotation
        // Function is simple scale (seconds * speed)
        // Use -speed since altering texture coords has the reverse visible effect
        return createController(mFrameTimeController,
                                TexCoordModifierControllerValue::create(layer, false, false, false, false, true),
                                ScaleControllerFunction::create(-speed, true));
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::createTextureWaveTransformer(TextureUnitState* layer,
        TextureUnitState::TextureTransformType ttype, WaveformType waveType, Real base, Real frequency, Real phase, Real amplitude) -> Controller<Real>*
    {
        ControllerValueRealPtr val;

        switch (ttype)
        {
        case TextureUnitState::TT_TRANSLATE_U:
            // Target value is a u scroll
            val = TexCoordModifierControllerValue::create(layer, true);
            break;
        case TextureUnitState::TT_TRANSLATE_V:
            // Target value is a v scroll
            val = TexCoordModifierControllerValue::create(layer, false, true);
            break;
        case TextureUnitState::TT_SCALE_U:
            // Target value is a u scale
            val = TexCoordModifierControllerValue::create(layer, false, false, true);
            break;
        case TextureUnitState::TT_SCALE_V:
            // Target value is a v scale
            val = TexCoordModifierControllerValue::create(layer, false, false, false, true);
            break;
        case TextureUnitState::TT_ROTATE:
            // Target value is texture coord rotation
            val = TexCoordModifierControllerValue::create(layer, false, false, false, false, true);
            break;
        }
        // Create new wave function for alterations
        return createController(mFrameTimeController, val,
                                WaveformControllerFunction::create(waveType, base, frequency, phase, amplitude, true));
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::createGpuProgramTimerParam(
        GpuProgramParametersSharedPtr params, size_t paramIndex, Real timeFactor) -> Controller<Real>*
    {
        return createController(mFrameTimeController, FloatGpuParameterControllerValue::create(params, paramIndex),
                                ScaleControllerFunction::create(timeFactor, true));
    }
    //-----------------------------------------------------------------------
    void ControllerManager::destroyController(Controller<Real>* controller)
    {
        auto i = mControllers.find(controller);
        if (i != mControllers.end())
        {
            mControllers.erase(i);
            delete controller;
        }
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::getTimeFactor() const -> Real {
        return static_cast<const FrameTimeControllerValue*>(mFrameTimeController.get())->getTimeFactor();
    }
    //-----------------------------------------------------------------------
    void ControllerManager::setTimeFactor(Real tf) {
        static_cast<FrameTimeControllerValue*>(mFrameTimeController.get())->setTimeFactor(tf);
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::getFrameDelay() const -> Real {
        return static_cast<const FrameTimeControllerValue*>(mFrameTimeController.get())->getFrameDelay();
    }
    //-----------------------------------------------------------------------
    void ControllerManager::setFrameDelay(Real fd) {
        static_cast<FrameTimeControllerValue*>(mFrameTimeController.get())->setFrameDelay(fd);
    }
    //-----------------------------------------------------------------------
    auto ControllerManager::getElapsedTime() const -> Real
    {
        return static_cast<const FrameTimeControllerValue*>(mFrameTimeController.get())->getElapsedTime();
    }
    //-----------------------------------------------------------------------
    void ControllerManager::setElapsedTime(Real elapsedTime)
    {
        static_cast<FrameTimeControllerValue*>(mFrameTimeController.get())->setElapsedTime(elapsedTime);
    }
}
