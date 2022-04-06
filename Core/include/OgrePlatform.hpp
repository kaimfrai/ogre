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
#ifndef OGRE_CORE_PLATFORM_H
#define OGRE_CORE_PLATFORM_H

#include "OgreConfig.hpp"

/* See if we can use __forceinline or if we need to use __inline instead */

#define OGRE_FORCE_INLINE inline __attribute__((always_inline))

/* fallthrough attribute */
#define OGRE_FALLTHROUGH __attribute__((fallthrough))

/* define OGRE_NORETURN macro */
#define OGRE_NORETURN __attribute__((noreturn))

/* Find how to declare aligned variable. */
#define OGRE_ALIGNED_DECL(type, var, alignment)  type var __attribute__((__aligned__(alignment)))

/** Find perfect alignment (should supports SIMD alignment if SIMD available) */
#define OGRE_SIMD_ALIGNMENT 16

/* Declare variable aligned to SIMD alignment. */
#define OGRE_SIMD_ALIGNED_DECL(type, var)   OGRE_ALIGNED_DECL(type, var, OGRE_SIMD_ALIGNMENT)

#define OGRE_DEFAULT_LOCALE "C"

#define DECL_MALLOC __attribute__ ((malloc))

#include <cstdint>

namespace Ogre {
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef uint64_t uint64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef int64_t int64;
}

#endif
