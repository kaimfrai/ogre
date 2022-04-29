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

module Ogre.Core;

/*

    Although the code is original, many of the ideas for the profiler were borrowed from 
"Real-Time In-Game Profiling" by Steve Rabin which can be found in Game Programming
Gems 1.

    This code can easily be adapted to your own non-Ogre project. The only code that is 
Ogre-dependent is in the visualization/logging routines and the use of the Timer class.

    Enjoy!

*/

import :LogManager;
import :Platform;
import :Prerequisites;
import :Profiler;
import :RenderSystem;
import :Root;
import :Singleton;
import :StringConverter;
import :Timer;

import <algorithm>;
import <cmath>;
import <cstddef>;
import <map>;
import <set>;
import <string>;
import <utility>;
import <vector>;

namespace Ogre {
    //-----------------------------------------------------------------------
    // PROFILE DEFINITIONS
    //-----------------------------------------------------------------------
    template<> Profiler* Singleton<Profiler>::msSingleton = nullptr;
    auto Profiler::getSingletonPtr() -> Profiler*
    {
        return msSingleton;
    }
    auto Profiler::getSingleton() -> Profiler&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }

    //-----------------------------------------------------------------------
    // PROFILER DEFINITIONS
    //-----------------------------------------------------------------------
    Profiler::Profiler() 
        : mCurrent(&mRoot)
        , 
         mRoot()
         
    {
        mRoot.hierarchicalLvl = 0 - 1;
    }
    //-----------------------------------------------------------------------
    ProfileInstance::ProfileInstance()
         
    {
        history.numCallsThisFrame = 0;
        history.totalClocksPercent = 0;
        history.totalClocks = 0;
        history.totalCalls = 0;
        history.maxClocksPercent = 0;
        history.maxClocks = 0;
        history.minClocksPercent = 1;
        history.minClocks = static_cast<ulong>(-1l);
        history.currentClocksPercent = 0;
        history.currentClocks = 0;
        history.sumOfSquareClocks = 0;

        frame.frameClocks = 0;
        frame.calls = 0;
    }
    ProfileInstance::~ProfileInstance()
    {                                        
        for(auto & it : children)
        {
            ProfileInstance* instance = it.second;
            delete instance;
        }
        children.clear();
    }
    //-----------------------------------------------------------------------
    Profiler::~Profiler()
    {
        if (!mRoot.children.empty()) 
        {
            // log the results of our profiling before we quit
            logResults();
        }

        // clear all our lists
        mDisabledProfiles.clear();
    }
    //-----------------------------------------------------------------------
    void Profiler::setTimer(Timer* t)
    {
        mTimer = t;
    }
    //-----------------------------------------------------------------------
    auto Profiler::getTimer() -> Timer*
    {
        assert(mTimer && "Timer not set!");
        return mTimer;
    }
    //-----------------------------------------------------------------------
    void Profiler::setEnabled(bool enabled) 
    {
        if (!mInitialized && enabled) 
        {
            for(auto & mListener : mListeners)
                mListener->initializeSession();

            mInitialized = true;
        }
        else if (mInitialized && !enabled)
        {
            for(auto & mListener : mListeners)
                mListener->finializeSession();

            mInitialized = false;
            mEnabled = false;
        }
        // We store this enable/disable request until the frame ends
        // (don't want to screw up any open profiles!)
        mNewEnableState = enabled;
    }
    //-----------------------------------------------------------------------
    auto Profiler::getEnabled() const -> bool
    {
        return mEnabled;
    }
    //-----------------------------------------------------------------------
    void Profiler::changeEnableState() 
    {
        for(auto & mListener : mListeners)
            mListener->changeEnableState(mNewEnableState);

        mEnabled = mNewEnableState;
    }
    //-----------------------------------------------------------------------
    void Profiler::disableProfile(const String& profileName)
    {
        // even if we are in the middle of this profile, endProfile() will still end it.
        mDisabledProfiles.insert(profileName);
    }
    //-----------------------------------------------------------------------
    void Profiler::enableProfile(const String& profileName) 
    {
        mDisabledProfiles.erase(profileName);
    }
    //-----------------------------------------------------------------------
    void Profiler::beginProfile(const String& profileName, uint32 groupID)
    {
        // if the profiler is enabled
        if (!mEnabled)
            return;

        // mask groups
        if ((groupID & mProfileMask) == 0)
            return;

        // empty string is reserved for the root
        // not really fatal anymore, however one shouldn't name one's profile as an empty string anyway.
        assert ((profileName != "") && ("Profile name can't be an empty string"));

        // we only process this profile if isn't disabled
        if (mDisabledProfiles.find(profileName) != mDisabledProfiles.end())
            return;

        // regardless of whether or not we are enabled, we need the application's root profile (ie the first profile started each frame)
        // we need this so bogus profiles don't show up when users enable profiling mid frame
        // so we check

        // this would be an internal error.
        assert (mCurrent);

        // need a timer to profile!
        assert (mTimer && "Timer not set!");

        ProfileInstance*& instance = mCurrent->children[profileName];
        if(instance)
        {   // found existing child.

            // Sanity check.
            assert(instance->name == profileName);

            if(instance->frameNumber != mCurrentFrame)
            {   // new frame, reset stats
                instance->frame.calls = 0;
                instance->frame.frameClocks = 0;
                instance->frameNumber = mCurrentFrame;
            }
        }
        else
        {   // new child!
            instance = new ProfileInstance();
            instance->name = profileName;
            instance->parent = mCurrent;
            instance->hierarchicalLvl = mCurrent->hierarchicalLvl + 1;
        }

        instance->frameNumber = mCurrentFrame;

        mCurrent = instance;

        // we do this at the very end of the function to get the most
        // accurate timing results
        mCurrent->currentClock = mTimer->getCPUClocks();
    }
    //-----------------------------------------------------------------------
    void Profiler::endProfile(const String& profileName, uint32 groupID) 
    {
        if(!mEnabled) 
        {
            // if the profiler received a request to be enabled or disabled
            if(mNewEnableState != mEnabled) 
            {   // note mNewEnableState == true to reach this.
                changeEnableState();

                // NOTE we will be in an 'error' state until the next begin. ie endProfile will likely get invoked using a profileName that was never started.
                // even then, we can't be sure that the next beginProfile will be the true start of a new frame
            }

            return;
        }
        else
        {
            if(mNewEnableState != mEnabled) 
            {   // note mNewEnableState == false to reach this.
                changeEnableState();

                // unwind the hierarchy, should be easy enough
                mCurrent = &mRoot;
                mLast = nullptr;
            }

            if(&mRoot == mCurrent && mLast)
            {   // profiler was enabled this frame, but the first subsequent beginProfile was NOT the beinging of a new frame as we had hoped.
                // we have bogus ProfileInstance in our hierarchy, we will need to remove it, then update the overlays so as not to confuse ze user

                for(auto& e : mRoot.children)
                {
                    delete e.second;
                }
                mRoot.children.clear();

                mLast = nullptr;

                processFrameStats();
                displayResults();
            }
        }

        if(&mRoot == mCurrent)
            return;

        // mask groups
        if ((groupID & mProfileMask) == 0)
            return;

        // need a timer to profile!
        assert (mTimer && "Timer not set!");

        // get the end time of this profile
        // we do this as close the beginning of this function as possible
        // to get more accurate timing results
        const ulong endClock = mTimer->getCPUClocks();

        // empty string is reserved for designating an empty parent
        assert ((profileName != "") && ("Profile name can't be an empty string"));

        // we only process this profile if isn't disabled
        // we check the current instance name against the provided profileName as a guard against disabling a profile name /after/ said profile began
        if(mCurrent->name != profileName && mDisabledProfiles.find(profileName) != mDisabledProfiles.end()) 
            return;

        // calculate the elapsed time of this profile
        const ulong clocksElapsed = endClock - mCurrent->currentClock;

        // update parent's accumulator if it isn't the root
        if (&mRoot != mCurrent->parent) 
        {
            // add this profile's time to the parent's accumlator
            mCurrent->parent->accumClocks += clocksElapsed;
        }

        mCurrent->frame.frameClocks += clocksElapsed;
        ++mCurrent->frame.calls;

        mLast = mCurrent;
        mCurrent = mCurrent->parent;

        if (&mRoot == mCurrent) 
        {
            // the stack is empty and all the profiles have been completed
            // we have reached the end of the frame so process the frame statistics

            // we know that the time elapsed of the main loop is the total time the frame took
            mTotalFrameClocks = clocksElapsed;

            if(clocksElapsed > mMaxTotalFrameClocks)
                mMaxTotalFrameClocks = clocksElapsed;

            // we got all the information we need, so process the profiles
            // for this frame
            processFrameStats();

            // we display everything to the screen
            displayResults();

            if  (   mLast->history.totalCalls
                //  5% margin of error, 99% confidence level
                >=  666
                )
            {
                Ogre::Root::getSingleton().queueEndRendering();
            }
        }
    }
    //-----------------------------------------------------------------------
    void Profiler::beginGPUEvent(const String& event)
    {
        Root::getSingleton().getRenderSystem()->beginProfileEvent(event);
    }
    //-----------------------------------------------------------------------
    void Profiler::endGPUEvent(const String& event)
    {
        Root::getSingleton().getRenderSystem()->endProfileEvent();
    }
    //-----------------------------------------------------------------------
    void Profiler::markGPUEvent(const String& event)
    {
        Root::getSingleton().getRenderSystem()->markProfileEvent(event);
    }
    //-----------------------------------------------------------------------
    void Profiler::processFrameStats(ProfileInstance* instance, ulong& maxFrameClocks)
    {
        // calculate what percentage of frame time this profile took
        const long double framePercentage = (long double) instance->frame.frameClocks / (long double) mTotalFrameClocks;

        const ulong frameClocks = instance->frame.frameClocks;

        // update the profile stats
        instance->history.currentClocksPercent = framePercentage;
        instance->history.currentClocks = frameClocks;
        if(mResetExtents)
        {
            instance->history.totalClocksPercent = framePercentage;
            instance->history.totalClocks = frameClocks;
            instance->history.sumOfSquareClocks = frameClocks * frameClocks;
            instance->history.totalCalls = 1;
        }
        else
        {
            instance->history.totalClocksPercent += framePercentage;
            instance->history.totalClocks += frameClocks;
            instance->history.sumOfSquareClocks += frameClocks * frameClocks;
            instance->history.totalCalls++;
        }
        instance->history.numCallsThisFrame = instance->frame.calls;

        // if we find a new minimum for this profile, update it
        if (frameClocks < instance->history.minClocks || mResetExtents)
        {
            instance->history.minClocksPercent = framePercentage;
            instance->history.minClocks = frameClocks;
        }

        // if we find a new maximum for this profile, update it
        if (frameClocks > instance->history.maxClocks || mResetExtents)
        {
            instance->history.maxClocksPercent = framePercentage;
            instance->history.maxClocks = frameClocks;
        }

        if(frameClocks > maxFrameClocks)
            maxFrameClocks = frameClocks;

        auto it = instance->children.begin(), endit = instance->children.end();
        for(;it != endit; ++it)
        {
            ProfileInstance* child = it->second;

            // we set the number of times each profile was called per frame to 0
            // because not all profiles are called every frame
            child->history.numCallsThisFrame = 0;

            if(child->frame.calls > 0)
            {
                processFrameStats(child, maxFrameClocks);
            }
        }
    }
    //-----------------------------------------------------------------------
    void Profiler::processFrameStats() 
    {
        ulong maxFrameClocks = 0;

        auto it = mRoot.children.begin(), endit = mRoot.children.end();
        for(;it != endit; ++it)
        {
            ProfileInstance* child = it->second;

            // we set the number of times each profile was called per frame to 0
            // because not all profiles are called every frame
            child->history.numCallsThisFrame = 0;

            if(child->frame.calls > 0)
            {
                processFrameStats(child, maxFrameClocks);
            }
        }

        // Calculate whether the extents are now so out of date they need regenerating
        if (mCurrentFrame == 0)
            mAverageFrameClocks = maxFrameClocks;
        else
            mAverageFrameClocks = (mAverageFrameClocks + maxFrameClocks) * 0.5l;

        if ((long double)mMaxTotalFrameClocks > mAverageFrameClocks * 4)
        {
            mResetExtents = true;
            mMaxTotalFrameClocks= (ulong)mAverageFrameClocks;
        }
        else
            mResetExtents = false;
    }
    //-----------------------------------------------------------------------
    void Profiler::displayResults() 
    {
        // if its time to update the display
        if (!(mCurrentFrame % mUpdateDisplayFrequency)) 
        {
            // ensure the root won't be culled
            mRoot.frame.calls = 1;

            for(auto & mListener : mListeners)
                mListener->displayResults(mRoot, mMaxTotalFrameClocks);
        }
        ++mCurrentFrame;
    }
    //-----------------------------------------------------------------------
    auto Profiler::watchForMax(const String& profileName) -> bool 
    {
        assert ((profileName != "") && ("Profile name can't be an empty string"));

        return mRoot.watchForMax(profileName);
    }
    //-----------------------------------------------------------------------
    auto ProfileInstance::watchForMax(const String& profileName) -> bool 
    {
        auto it = children.begin(), endit = children.end();
        for(;it != endit; ++it)
        {
            ProfileInstance* child = it->second;
            if( (child->name == profileName && child->watchForMax()) || child->watchForMax(profileName))
                return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    auto Profiler::watchForMin(const String& profileName) -> bool 
    {
        assert ((profileName != "") && ("Profile name can't be an empty string"));
        return mRoot.watchForMin(profileName);
    }
    //-----------------------------------------------------------------------
    auto ProfileInstance::watchForMin(const String& profileName) -> bool 
    {
        auto it = children.begin(), endit = children.end();
        for(;it != endit; ++it)
        {
            ProfileInstance* child = it->second;
            if( (child->name == profileName && child->watchForMin()) || child->watchForMin(profileName))
                return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    auto Profiler::watchForLimit(const String& profileName, long double limit, bool greaterThan) -> bool
    {
        assert ((profileName != "") && ("Profile name can't be an empty string"));
        return mRoot.watchForLimit(profileName, limit, greaterThan);
    }
    //-----------------------------------------------------------------------
    auto ProfileInstance::watchForLimit(const String& profileName, long double limit, bool greaterThan) -> bool
    {
        auto it = children.begin(), endit = children.end();
        for(;it != endit; ++it)
        {
            ProfileInstance* child = it->second;
            if( (child->name == profileName && child->watchForLimit(limit, greaterThan)) || child->watchForLimit(profileName, limit, greaterThan))
                return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    void Profiler::logResults() 
    {
        LogManager::getSingleton().logMessage("----------------------Profiler Results----------------------");

        for(auto & it : mRoot.children)
        {
            it.second->logResults();
        }

        LogManager::getSingleton().logMessage("------------------------------------------------------------");
    }

    auto ProfileHistory::StandardDeviationMilliseconds() const -> long double
    {
        if (totalCalls == 0ul)
            return 0.0l;

        ulong const sumSquare = totalClocks * totalClocks;
        ulong const numerator = totalCalls * sumOfSquareClocks - sumSquare;
        ulong const denominator = totalCalls * (totalCalls - 1ul);
        return Timer::clocksToMilliseconds(::std::sqrt((long double)numerator / (long double)denominator));
    }

    //-----------------------------------------------------------------------
    void ProfileInstance::logResults() 
    {
        // create an indent that represents the hierarchical order of the profile
        String indent = "";
        for (uint i = 0; i < hierarchicalLvl; ++i) 
        {
            indent = ::std::format("{}  ", indent);
        }

        LogManager::getSingleton().logMessage
        (   ::std::format("{}{} | Min {} | Max {} | Avg {} | StdDev {} | Calls {}",
            indent,
            name,
            Timer::clocksToMilliseconds(history.minClocks),
            Timer::clocksToMilliseconds(history.maxClocks),
            (   Timer::clocksToMilliseconds(history.totalClocks)
            /   (long double)(history.totalCalls)
            ),
            history.StandardDeviationMilliseconds(),
            history.totalCalls
            )
        );

        for(auto & it : children)
        {
            it.second->logResults();
        }
    }
    //-----------------------------------------------------------------------
    void Profiler::reset() 
    {
        mRoot.reset();
        mMaxTotalFrameClocks = 0;
    }
    //-----------------------------------------------------------------------
    void ProfileInstance::reset()
    {
        history.currentClocksPercent = history.maxClocksPercent = history.totalClocksPercent = 0;
        history.currentClocks = history.maxClocks = history.totalClocks = 0;
        history.numCallsThisFrame = history.totalCalls = 0;

        history.minClocksPercent = 1;
        history.minClocks = static_cast<ulong>(-1l);
        for(auto & it : children)
        {
            it.second->reset();
        }
    }
    //-----------------------------------------------------------------------
    void Profiler::setUpdateDisplayFrequency(uint freq)
    {
        mUpdateDisplayFrequency = freq;
    }
    //-----------------------------------------------------------------------
    auto Profiler::getUpdateDisplayFrequency() const -> uint
    {
        return mUpdateDisplayFrequency;
    }
    //-----------------------------------------------------------------------
    void Profiler::addListener(ProfileSessionListener* listener)
    {
        if (std::find(mListeners.begin(), mListeners.end(), listener) == mListeners.end())
            mListeners.push_back(listener);
    }
    //-----------------------------------------------------------------------
    void Profiler::removeListener(ProfileSessionListener* listener)
    {
        auto i = std::find(mListeners.begin(), mListeners.end(), listener);
        if (i != mListeners.end())
            mListeners.erase(i);
    }
    //-----------------------------------------------------------------------
}
