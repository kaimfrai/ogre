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
#ifndef __Platform_H_
#define __Platform_H_

#include "OgreConfig.h"
#include "OgreExports.h"

/* Initial platform/compiler-related stuff to set.
*/
    
#define OGRE_COMPILER_MSVC 1
#define OGRE_COMPILER_GNUC 2
#define OGRE_COMPILER_CLANG 3

#define OGRE_ENDIAN_LITTLE 1
#define OGRE_ENDIAN_BIG 2

#define OGRE_ARCHITECTURE_32 1
#define OGRE_ARCHITECTURE_64 2

#define OGRE_CPU_UNKNOWN    0
#define OGRE_CPU_X86        1
#define OGRE_CPU_PPC        2
#define OGRE_CPU_ARM        3
#define OGRE_CPU_MIPS       4

/* Find CPU type */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64)
#   define OGRE_CPU OGRE_CPU_X86
#elif defined(__ppc__) || defined(__ppc64__) || defined(_M_PPC)
#   define OGRE_CPU OGRE_CPU_PPC
#elif defined(__arm__) || defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#   define OGRE_CPU OGRE_CPU_ARM
#elif defined(__mips__) || defined(__mips64) || defined(__mips64_) || defined(_M_MIPS)
#   define OGRE_CPU OGRE_CPU_MIPS
#else
#   define OGRE_CPU OGRE_CPU_UNKNOWN
#endif

/* Determine CPU endian.
   We were once in situation when XCode could produce mixed endian fat binary with x86 and ppc archs inside, so it's safer to sniff compiler macros too
 */
#if defined(OGRE_CONFIG_BIG_ENDIAN) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define OGRE_ENDIAN OGRE_ENDIAN_BIG
#else
#    define OGRE_ENDIAN OGRE_ENDIAN_LITTLE
#endif


/* Finds the compiler type and version.
*/
#if (defined( __WIN32__ ) || defined( _WIN32 )) && defined(__ANDROID__) // We are using NVTegra
#   define OGRE_COMPILER OGRE_COMPILER_GNUC
#   define OGRE_COMP_VER 470
#elif defined( _MSC_VER )
#   define OGRE_COMPILER OGRE_COMPILER_MSVC
#   define OGRE_COMP_VER _MSC_VER
#elif defined( __clang__ )
#   define OGRE_COMPILER OGRE_COMPILER_CLANG
#   define OGRE_COMP_VER (((__clang_major__)*100) + \
        (__clang_minor__*10) + \
        __clang_patchlevel__)
#elif defined( __GNUC__ )
#   define OGRE_COMPILER OGRE_COMPILER_GNUC
#   define OGRE_COMP_VER (((__GNUC__)*100) + \
        (__GNUC_MINOR__*10) + \
        __GNUC_PATCHLEVEL__)
#else
#   pragma error "No known compiler. Abort! Abort!"

#endif

#define OGRE_COMPILER_MIN_VERSION(COMPILER, VERSION) (OGRE_COMPILER == (COMPILER) && OGRE_COMP_VER >= (VERSION))

/* See if we can use __forceinline or if we need to use __inline instead */
#if OGRE_COMPILER_MIN_VERSION(OGRE_COMPILER_MSVC, 1200)
    #define OGRE_FORCE_INLINE __forceinline
#elif OGRE_COMPILER_MIN_VERSION(OGRE_COMPILER_GNUC, 340)
    #define OGRE_FORCE_INLINE inline __attribute__((always_inline))
#else
        #define OGRE_FORCE_INLINE __inline
#endif

/* fallthrough attribute */
#if OGRE_COMPILER_MIN_VERSION(OGRE_COMPILER_GNUC, 700)
#define OGRE_FALLTHROUGH __attribute__((fallthrough))
#else
#define OGRE_FALLTHROUGH
#endif

#if OGRE_COMPILER == OGRE_COMPILER_GNUC || OGRE_COMPILER == OGRE_COMPILER_CLANG
#define OGRE_NODISCARD __attribute__((__warn_unused_result__))
#else
#define OGRE_NODISCARD
#endif

/* define OGRE_NORETURN macro */
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#	define OGRE_NORETURN __declspec(noreturn)
#elif OGRE_COMPILER == OGRE_COMPILER_GNUC || OGRE_COMPILER == OGRE_COMPILER_CLANG
#	define OGRE_NORETURN __attribute__((noreturn))
#else
#	define OGRE_NORETURN
#endif

/* Find the arch type */
#if defined(__LP64__) || defined(_WIN64)
#   define OGRE_ARCH_TYPE OGRE_ARCHITECTURE_64
#else
#   define OGRE_ARCH_TYPE OGRE_ARCHITECTURE_32
#endif

/* Find how to declare aligned variable. */
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#   define OGRE_ALIGNED_DECL(type, var, alignment)  __declspec(align(alignment)) type var
#elif (OGRE_COMPILER == OGRE_COMPILER_GNUC) || (OGRE_COMPILER == OGRE_COMPILER_CLANG)
#   define OGRE_ALIGNED_DECL(type, var, alignment)  type var __attribute__((__aligned__(alignment)))
#else
#   define OGRE_ALIGNED_DECL(type, var, alignment)  type var
#endif

/** Find perfect alignment (should supports SIMD alignment if SIMD available) */
#define OGRE_SIMD_ALIGNMENT 16

/* Declare variable aligned to SIMD alignment. */
#define OGRE_SIMD_ALIGNED_DECL(type, var)   OGRE_ALIGNED_DECL(type, var, OGRE_SIMD_ALIGNMENT)


// For generating compiler warnings - should work on any compiler
// As a side note, if you start your message with 'Warning: ', the MSVC
// IDE actually does catch a warning :)
#define OGRE_QUOTE_INPLACE(x) # x
#define OGRE_QUOTE(x) OGRE_QUOTE_INPLACE(x)
#define OGRE_WARN( x )  message( __FILE__ "(" QUOTE( __LINE__ ) ") : " x "\n" )


//----------------------------------------------------------------------------
// Endian Settings
// check for BIG_ENDIAN config flag, set OGRE_ENDIAN correctly
#ifdef OGRE_CONFIG_BIG_ENDIAN
#    define OGRE_ENDIAN OGRE_ENDIAN_BIG
#else
#    define OGRE_ENDIAN OGRE_ENDIAN_LITTLE
#endif


#define OGRE_DEFAULT_LOCALE "C"

//----------------------------------------------------------------------------
// Library suffixes
// "_d" for debug builds, nothing otherwise

#define OGRE_BUILD_SUFFIX ""

#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#define DECL_MALLOC __declspec(restrict) __declspec(noalias)
#else
#define DECL_MALLOC __attribute__ ((malloc))
#endif

#include <stdint.h>

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
