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

#ifndef OGRE_TESTS_VISUALTESTS_TESTCONTEXT_H
#define OGRE_TESTS_VISUALTESTS_TESTCONTEXT_H

#include <deque>
#include <map>
#include <string>

#include "OgreIteratorWrapper.hpp"
#include "OgrePrerequisites.hpp"
#include "SampleContext.hpp"

namespace Ogre{
    struct FrameEvent;
}

namespace OgreBites{
    class Sample;
    class SamplePlugin;
}

class TestBatch;

using namespace Ogre;

/** The common environment that all of the tests run in */
class TestContext : public OgreBites::SampleContext
{
 public:

    TestContext(int argc = 0, char** argv = 0);
    virtual ~TestContext();

    /** Does basic setup for the context */
    virtual void setup();

    bool frameRenderingQueued(const Ogre::FrameEvent& evt);

    /** Frame listener callback, handles updating of the tests at the start of frames
     *        @param evt The frame event (passed in for the framelistener) */
    virtual bool frameStarted(const FrameEvent& evt);

    /** Frame listener callback, handles updating of the tests at the end of frames
     *        @param evt The frame event (passed in for the framelistener) */
    virtual bool frameEnded(const FrameEvent& evt);

    /** Runs a given test or sample
     *        @param s The OgreBites::Sample to run
     *        @remarks If s is a VisualTest, then timing and rand will be setup for
     *            determinism. */
    virtual void runSample(OgreBites::Sample* s);

    /** Loads test plugins
     *        @return The initial tets or sample to run */
    OgreBites::Sample* loadTests();

    /** Setup the Root */
    virtual void createRoot();

    /** Start it up */
    virtual void go(OgreBites::Sample* initialSample = 0);

    /** Handles the config dialog */
    virtual bool oneTimeConfig();

    /** Set up directories for the tests to output to */
    virtual void setupDirectories(String batchName);

    /** Called after tests successfully complete, generates output */
    virtual void finishedTests();

    /** Sets the timstep value
     *        @param timestep The time to simulate elapsed between each frame
     *        @remarks Use with care! Screenshots produced at different timesteps
     *            will almost certainly turn out different. */
    void setTimestep(Real timestep);

    /** Gets the current timestep value */
    Real getTimestep();

    /// Returns whether the entire test was successful or not.
    bool wasSuccessful() const {
        return mSuccess;
    }

 private:
    typedef std::map<String, OgreBites::SamplePlugin *> PluginMap;
    bool mSuccess;

    /// The timestep
    Real mTimestep;

    /// The tests to be run
    std::deque<OgreBites::Sample*> mTests;

    /// Path to the output directory for the running test
    String mOutputDir;

    /// Path to the reference set location
    String mReferenceSetPath;

    /// The current frame of a running test
    int mCurrentFrame;

    /// Info about the running batch of tests
    TestBatch* mBatch;

    // A structure to map plugin names to class types
    PluginMap mPluginNameMap;

    // command line options
    // Is a reference set being generated?
    bool mReferenceSet;
    // Should html output be created?
    bool mGenerateHtml;
    // Force the config dialog
    bool mForceConfig;
    // Do not confine mouse to window
    bool mNoGrabMouse;
    // Show usage details
    bool mHelp;
    // Render system to use
    String mRenderSystemName;
    // Optional name for this batch
    String mBatchName;
    // Set to compare against
    String mCompareWith;
    // Optional comment
    String mComment;
    // Name of the test set to use
    String mTestSetName;
    // Location to output a test summary (used for CTest)
    String mSummaryOutputDir;
};

#endif
