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
export module Ogre.Samples:SampleContext;

export import :Sample;

export import Ogre.Components.Bites;
export import Ogre.Core;

export
namespace OgreBites
{
    /*=============================================================================
    | Base class responsible for setting up a common context for samples.
    | May be subclassed for specific sample types (not specific samples).
    | Allows one sample to run at a time, while maintaining a sample queue.
    =============================================================================*/
    class SampleContext : public ApplicationContext, public InputListener
    {
    public:
        Ogre::RenderWindow* mWindow{nullptr};

        SampleContext(const Ogre::String& appName = /*OGRE_VERSION_NAME*/"Tsathoggua")
        : ApplicationContext(appName) 
        {
            mCurrentSample = nullptr;
            mSamplePaused = false;
            mLastRun = false;
            mLastSample = nullptr;
        }

        virtual auto getCurrentSample() -> Sample*
        {
            return mCurrentSample;
        }

        /*-----------------------------------------------------------------------------
        | Quits the current sample and starts a new one.
        -----------------------------------------------------------------------------*/
        virtual void runSample(Sample* s)
        {
            Ogre::Profiler* prof = Ogre::Profiler::getSingletonPtr();
            if (prof)
                prof->setEnabled(false);

            if (mCurrentSample)
            {
                mCurrentSample->_shutdown();    // quit current sample
                mSamplePaused = false;          // don't pause the next sample
            }

            mWindow->removeAllViewports();                  // wipe viewports
            mWindow->resetStatistics();

            if (s)
            {
                // retrieve sample's required plugins and currently installed plugins
                Ogre::Root::PluginInstanceList ip = mRoot->getInstalledPlugins();
                Ogre::StringVector rp = s->getRequiredPlugins();

                for (auto & j : rp)
                {
                    bool found = false;
                    // try to find the required plugin in the current installed plugins
                    for (auto & k : ip)
                    {
                        if (k->getName() == j)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)  // throw an exception if a plugin is not found
                    {
                        OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED, "Sample requires plugin: " + j);
                    }
                }

                // test system capabilities against sample requirements
                s->testCapabilities(mRoot->getRenderSystem()->getCapabilities());
                s->setShaderGenerator(mShaderGenerator);
                s->_setup(mWindow, mFSLayer, mOverlaySystem);   // start new sample
            }

            if (prof)
                prof->setEnabled(true);

            mCurrentSample = s;
        }

        /*-----------------------------------------------------------------------------
        | This function encapsulates the entire lifetime of the context.
        -----------------------------------------------------------------------------*/
        virtual void go(Sample* initialSample = nullptr)
        {
            while (!mLastRun)
            {
                mLastRun = true;  // assume this is our last run

                initApp();

                // restore the last sample if there was one or, if not, start initial sample
                if (!mFirstRun) recoverLastSample();
                else if (initialSample) runSample(initialSample);

                loadStartUpSample();
        
                if (mRoot->getRenderSystem() != nullptr)
                {
                    mRoot->startRendering();    // start the render loop
                }

                closeApp();

                mFirstRun = false;
            }
        }

        virtual void loadStartUpSample() {}

        auto isCurrentSamplePaused() -> bool
        {
            return !mCurrentSample || mSamplePaused;
        }

        virtual void pauseCurrentSample()
        {
            if (!isCurrentSamplePaused())
            {
                mSamplePaused = true;
                mCurrentSample->paused();
            }
        }

        virtual void unpauseCurrentSample()
        {
            if (mCurrentSample && mSamplePaused)
            {
                mSamplePaused = false;
                mCurrentSample->unpaused();
            }
        }
            
        /*-----------------------------------------------------------------------------
        | Processes frame started events.
        -----------------------------------------------------------------------------*/
        auto frameStarted(const Ogre::FrameEvent& evt) -> bool override
        {
            pollEvents();

            // manually call sample callback to ensure correct order
            return !isCurrentSamplePaused() ? mCurrentSample->frameStarted(evt) : true;
        }
            
        /*-----------------------------------------------------------------------------
        | Processes rendering queued events.
        -----------------------------------------------------------------------------*/
        auto frameRenderingQueued(const Ogre::FrameEvent& evt) -> bool override
        {
            // manually call sample callback to ensure correct order
            return !isCurrentSamplePaused() ? mCurrentSample->frameRenderingQueued(evt) : true;
        }
            
        /*-----------------------------------------------------------------------------
        | Processes frame ended events.
        -----------------------------------------------------------------------------*/
        auto frameEnded(const Ogre::FrameEvent& evt) -> bool override
        {
            // manually call sample callback to ensure correct order
            if (mCurrentSample && !mSamplePaused && !mCurrentSample->frameEnded(evt)) return false;
            // quit if window was closed
            if (mWindow->isClosed()) return false;
            // go into idle mode if current sample has ended
            if (mCurrentSample && mCurrentSample->isDone()) runSample(nullptr);

            return true;
        }

        /*-----------------------------------------------------------------------------
        | Processes window size change event. Adjusts mouse's region to match that
        | of the window. You could also override this method to prevent resizing.
        -----------------------------------------------------------------------------*/
        void windowResized(Ogre::RenderWindow* rw) override
        {
            // manually call sample callback to ensure correct order
            if (!isCurrentSamplePaused()) mCurrentSample->windowResized(rw);
        }

        // window event callbacks which manually call their respective sample callbacks to ensure correct order

        void windowMoved(Ogre::RenderWindow* rw) override
        {
            if (!isCurrentSamplePaused()) mCurrentSample->windowMoved(rw);
        }

        auto windowClosing(Ogre::RenderWindow* rw) -> bool override
        {
            if (!isCurrentSamplePaused()) return mCurrentSample->windowClosing(rw);
            return true;
        }

        void windowClosed(Ogre::RenderWindow* rw) override
        {
            if (!isCurrentSamplePaused()) mCurrentSample->windowClosed(rw);
        }

        void windowFocusChange(Ogre::RenderWindow* rw) override
        {
            if (!isCurrentSamplePaused()) mCurrentSample->windowFocusChange(rw);
        }

        // keyboard and mouse callbacks which manually call their respective sample callbacks to ensure correct order

        auto keyPressed(const KeyboardEvent& evt) -> bool override
        {
            // Ignore repeated signals from key being held down.
            if (evt.repeat) return true;

            if (!isCurrentSamplePaused()) return mCurrentSample->keyPressed(evt);
            return true;
        }

        auto keyReleased(const KeyboardEvent& evt) -> bool override
        {
            if (!isCurrentSamplePaused()) return mCurrentSample->keyReleased(evt);
            return true;
        }

        auto touchMoved(const TouchFingerEvent& evt) -> bool override
        {
            if (!isCurrentSamplePaused())
                return mCurrentSample->touchMoved(evt);
            return true;
        }

        auto mouseMoved(const MouseMotionEvent& evt) -> bool override
        {
            if (!isCurrentSamplePaused())
                return mCurrentSample->mouseMoved(evt);
            return true;
        }

        auto touchPressed(const TouchFingerEvent& evt) -> bool override
        {
            if (!isCurrentSamplePaused())
                return mCurrentSample->touchPressed(evt);
            return true;
        }

        auto mousePressed(const MouseButtonEvent& evt) -> bool override
        {
            if (!isCurrentSamplePaused())
                return mCurrentSample->mousePressed(evt);
            return true;
        }

        auto touchReleased(const TouchFingerEvent& evt) -> bool override
        {
            if (!isCurrentSamplePaused())
                return mCurrentSample->touchReleased(evt);
            return true;
        }

        auto mouseReleased(const MouseButtonEvent& evt) -> bool override
        {
            if (!isCurrentSamplePaused())
                return mCurrentSample->mouseReleased(evt);
            return true;
        }

        auto mouseWheelRolled(const MouseWheelEvent& evt) -> bool override
        {
            if (!isCurrentSamplePaused())
                return mCurrentSample->mouseWheelRolled(evt);
            return true;
        }

        auto textInput (const TextInputEvent& evt) -> bool override
        {
            if (!isCurrentSamplePaused ())
                return mCurrentSample->textInput (evt);
            return true;
        }

        auto isFirstRun() -> bool { return mFirstRun; }
        void setFirstRun(bool flag) { mFirstRun = flag; }
        auto isLastRun() -> bool { return mLastRun; }
        void setLastRun(bool flag) { mLastRun = flag; }
    protected:
        /*-----------------------------------------------------------------------------
        | Reconfigures the context. Attempts to preserve the current sample state.
        -----------------------------------------------------------------------------*/
        void reconfigure(const Ogre::String& renderer, Ogre::NameValuePairList& options) override
        {
            // save current sample state
            mLastSample = mCurrentSample;
            if (mCurrentSample) mCurrentSample->saveState(mLastSampleState);

            mLastRun = false;             // we want to go again with the new settings
            ApplicationContext::reconfigure(renderer, options);
        }

        /*-----------------------------------------------------------------------------
        | Recovers the last sample after a reset. You can override in the case that
        | the last sample is destroyed in the process of resetting, and you have to
        | recover it through another means.
        -----------------------------------------------------------------------------*/
        virtual void recoverLastSample()
        {
            runSample(mLastSample);
            mLastSample->restoreState(mLastSampleState);
            mLastSample = nullptr;
            mLastSampleState.clear();
        }

        /*-----------------------------------------------------------------------------
        | Cleans up and shuts down the context.
        -----------------------------------------------------------------------------*/
        void shutdown() override
        {
            if (mCurrentSample)
            {
                mCurrentSample->_shutdown();
                mCurrentSample = nullptr;
            }

            ApplicationContext::shutdown();
        }
        
        Sample* mCurrentSample;         // The active sample (0 if none is active)
        bool mSamplePaused;             // whether current sample is paused
        bool mLastRun;                  // whether or not this is the final run
        Sample* mLastSample;            // last sample run before reconfiguration
        Ogre::NameValuePairList mLastSampleState;     // state of last sample
    };
}
