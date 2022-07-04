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

#ifndef OGRE_CORE_TIMER_H
#define OGRE_CORE_TIMER_H

#include <sys/types.h>

#include <chrono>
#include <cstdint>

#include "OgreMemoryAllocatorConfig.hpp"

namespace Ogre
{
    /** Timer class */
    class Timer : public TimerAlloc
    {
    private:
        std::chrono::steady_clock::time_point start;
        clock_t zeroClock;
    public:
        static auto clocksToMilliseconds(long double clocks) -> long double;
        static auto clocksToMicroseconds(long double clocks) -> long double;

        Timer();

        /** Resets timer */
        void reset();

        /** Returns milliseconds since initialisation or last reset */
        auto getMilliseconds() -> uint64_t;

        /** Returns microseconds since initialisation or last reset */
        auto getMicroseconds() -> uint64_t;

        [[nodiscard]] auto getCPUClocks() const -> uint64_t;

        /** Returns milliseconds since initialisation or last reset, only CPU time measured */  
        auto getMillisecondsCPU() -> uint64_t;

        /** Returns microseconds since initialisation or last reset, only CPU time measured */  
        auto getMicrosecondsCPU() -> uint64_t;
    };
}
#endif
