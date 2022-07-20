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
export module Ogre.Core:PlatformInformation;

export import :Prerequisites;

export
namespace Ogre {
    class Log;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */


    /** Class which provides the run-time platform information Ogre runs on.
        @remarks
            Ogre is designed to be platform-independent, but some platform
            and run-time environment specific optimised functions are built-in
            to maximise performance, and those special optimised routines are
            need to determine run-time environment for select variant executing
            path.
        @par
            This class manages that provides a couple of functions to determine
            platform information of the run-time environment.
        @note
            This class is supposed to use by advanced user only.
    */
    class PlatformInformation
    {
    public:

        /// Enum describing the different CPU features we want to check for, platform-dependent
        enum class CpuFeatures : uint
        {
            SSE             = 1 << 0,
            SSE2            = 1 << 1,
            SSE3            = 1 << 2,
            SSE41           = 1 << 3,
            SSE42           = 1 << 4,
            MMX             = 1 << 5,
            MMXEXT          = 1 << 6,
            _3DNOW           = 1 << 7,
            _3DNOWEXT        = 1 << 8,
            CMOV            = 1 << 9,
            TSC             = 1 << 10,
            INVARIANT_TSC   = 1 << 11,
            FPU             = 1 << 12,
            PRO             = 1 << 13,
            HTT             = 1 << 14,

            NONE            = 0
        };

        friend auto constexpr operator not (CpuFeatures mask) -> bool
        {
            return not std::to_underlying(mask);
        }

        friend auto constexpr operator bitor(CpuFeatures left, CpuFeatures right) -> CpuFeatures
        {
            return static_cast<CpuFeatures>
            (   std::to_underlying(left)
            bitor
                std::to_underlying(right)
            );
        }

        friend auto constexpr operator |=(CpuFeatures& left, CpuFeatures right) -> CpuFeatures&
        {
            return left = left bitor right;
        }

        friend auto constexpr operator bitand(CpuFeatures left, CpuFeatures right) -> CpuFeatures
        {
            return static_cast<CpuFeatures>
            (   std::to_underlying(left)
            bitand
                std::to_underlying(right)
            );
        }

        friend auto constexpr operator &=(CpuFeatures& left, CpuFeatures right) -> CpuFeatures&
        {
            return left = left bitand right;
        }

        friend auto constexpr operator compl(CpuFeatures mask) -> CpuFeatures
        {
            return static_cast<CpuFeatures>(compl std::to_underlying(mask));
        }

        /** Gets a string of the CPU identifier.
        @note
            Actual detecting are performs in the first time call to this function,
            and then all future calls with return internal cached value.
        */
        static auto getCpuIdentifier() noexcept -> std::string_view ;

        /** Gets a or-masked of enum class CpuFeatures that are supported by the CPU.
        @note
            Actual detecting are performs in the first time call to this function,
            and then all future calls with return internal cached value.
        */
        static auto getCpuFeatures() noexcept -> CpuFeatures;

        /** Gets whether a specific feature is supported by the CPU.
        @note
            Actual detecting are performs in the first time call to this function,
            and then all future calls with return internal cached value.
        */
        static auto hasCpuFeature(CpuFeatures feature) -> bool;


        /** Write the CPU information to the passed in Log */
        static void log(Log* pLog);

    };
    /** @} */
    /** @} */

}
