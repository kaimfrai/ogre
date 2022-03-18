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
#include "OgreTimer.h"

#include <ctime>
#include <type_traits>

static_assert(std::is_integral<std::clock_t>::value, "clock_t assumed to be an integral!");
static_assert(sizeof(std::clock_t) == sizeof(ulong), "clock_t assumed to match ulong!");
static_assert(alignof(std::clock_t) == alignof(ulong), "clock_t assumed to match ulong!");

using namespace std::chrono;

namespace Ogre {

long double Timer::clocksToMilliseconds(long double clocks)
{
    return 1000.0l * (clocks / (long double)CLOCKS_PER_SEC);
}

long double Timer::clocksToMicroseconds(long double clocks)
{
    return clocksToMilliseconds(clocks * 1000.0l);
}

//--------------------------------------------------------------------------------//
Timer::Timer()
{
    reset();
}

//--------------------------------------------------------------------------------//
void Timer::reset()
{
    zeroClock = clock();
    start = steady_clock::now();
}

//--------------------------------------------------------------------------------//
uint64_t Timer::getMilliseconds()
{
    auto now = steady_clock::now();
    return duration_cast<milliseconds>(now - start).count();
}

//--------------------------------------------------------------------------------//
uint64_t Timer::getMicroseconds()
{
    auto now = steady_clock::now();
    return duration_cast<microseconds>(now - start).count();
}

uint64_t Timer::getCPUClocks() const
{
    clock_t newClock = clock();
    return static_cast<uint64_t>(newClock - zeroClock);
}

//-- Common Across All Timers ----------------------------------------------------//
uint64_t Timer::getMillisecondsCPU()
{
    return clocksToMilliseconds(getCPUClocks());
}

//-- Common Across All Timers ----------------------------------------------------//
uint64_t Timer::getMicrosecondsCPU()
{
    return clocksToMicroseconds(getCPUClocks());
}

}

