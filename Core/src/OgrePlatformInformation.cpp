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
#include <cstring>
#include <ostream>
#include <string>

#include "OgreLog.hpp"
#include "OgrePlatformInformation.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreString.hpp"
#include "OgreStringConverter.hpp"


// Yes, I know, this file looks very ugly, but there aren't other ways to do it better.

namespace Ogre {

    //---------------------------------------------------------------------
    // Struct for store CPUID instruction result, compiler-independent
    //---------------------------------------------------------------------
    struct CpuidResult
    {
        // Note: DO NOT CHANGE THE ORDER, some code based on that.
        uint _eax;
        uint _ebx;
        uint _edx;
        uint _ecx;
    };

    //---------------------------------------------------------------------
    // Compiler-dependent routines
    //---------------------------------------------------------------------

    //---------------------------------------------------------------------
    // Detect whether CPU supports CPUID instruction, returns non-zero if supported.
    static int _isSupportCpuid()
    {
        return true;
    }

    //---------------------------------------------------------------------
    // Performs CPUID instruction with 'query', fill the results, and return value of eax.
    static uint _performCpuid(int query, CpuidResult& result)
    {
        __asm__
        (
            "cpuid": "=a" (result._eax), "=b" (result._ebx), "=c" (result._ecx), "=d" (result._edx) : "a" (query)
        );
        return result._eax;
    }

    //---------------------------------------------------------------------
    // Detect whether or not os support Streaming SIMD Extension.

    static bool _checkOperatingSystemSupportSSE()
    {
        return true;
    }

    //---------------------------------------------------------------------
    // Compiler-independent routines
    //---------------------------------------------------------------------

    static uint queryCpuFeatures()
    {

#define CPUID_FUNC_VENDOR_ID                 0x0
#define CPUID_FUNC_STANDARD_FEATURES         0x1
#define CPUID_FUNC_EXTENSION_QUERY           0x80000000
#define CPUID_FUNC_EXTENDED_FEATURES         0x80000001
#define CPUID_FUNC_ADVANCED_POWER_MANAGEMENT 0x80000007

#define CPUID_STD_FPU               (1<<0)
#define CPUID_STD_TSC               (1<<4)
#define CPUID_STD_CMOV              (1<<15)
#define CPUID_STD_MMX               (1<<23)
#define CPUID_STD_SSE               (1<<25)
#define CPUID_STD_SSE2              (1<<26)
#define CPUID_STD_HTT               (1<<28)     // EDX[28] - Bit 28 set indicates  Hyper-Threading Technology is supported in hardware.

#define CPUID_STD_SSE3              (1<<0)      // ECX[0]  - Bit 0 of standard function 1 indicate SSE3 supported
#define CPUID_STD_SSE41             (1<<19)     // ECX[19] - Bit 0 of standard function 1 indicate SSE41 supported
#define CPUID_STD_SSE42             (1<<20)     // ECX[20] - Bit 0 of standard function 1 indicate SSE42 supported

#define CPUID_FAMILY_ID_MASK        0x0F00      // EAX[11:8] - Bit 11 thru 8 contains family  processor id
#define CPUID_EXT_FAMILY_ID_MASK    0x0F00000   // EAX[23:20] - Bit 23 thru 20 contains extended family processor id
#define CPUID_PENTIUM4_ID           0x0F00      // Pentium 4 family processor id

#define CPUID_EXT_3DNOW             (1<<31)
#define CPUID_EXT_AMD_3DNOWEXT      (1<<30)
#define CPUID_EXT_AMD_MMXEXT        (1<<22)


#define CPUID_APM_INVARIANT_TSC     (1<<8)      // EDX[8] - Bit 8 of function 0x80000007 indicates support for invariant TSC.

        uint features = 0;

        // Supports CPUID instruction ?
        if (_isSupportCpuid())
        {
            CpuidResult result;

            // Has standard feature ?
            if (_performCpuid(CPUID_FUNC_VENDOR_ID, result))
            {
                // Check vendor strings
                if (memcmp(&result._ebx, "GenuineIntel", 12) == 0)
                {
                    if (result._eax > 2)
                        features |= PlatformInformation::CPU_FEATURE_PRO;

                    // Check standard feature
                    _performCpuid(CPUID_FUNC_STANDARD_FEATURES, result);

                    if (result._edx & CPUID_STD_FPU)
                        features |= PlatformInformation::CPU_FEATURE_FPU;
                    if (result._edx & CPUID_STD_TSC)
                        features |= PlatformInformation::CPU_FEATURE_TSC;
                    if (result._edx & CPUID_STD_CMOV)
                        features |= PlatformInformation::CPU_FEATURE_CMOV;
                    if (result._edx & CPUID_STD_MMX)
                        features |= PlatformInformation::CPU_FEATURE_MMX;
                    if (result._edx & CPUID_STD_SSE)
                        features |= PlatformInformation::CPU_FEATURE_MMXEXT | PlatformInformation::CPU_FEATURE_SSE;
                    if (result._edx & CPUID_STD_SSE2)
                        features |= PlatformInformation::CPU_FEATURE_SSE2;
                    if (result._ecx & CPUID_STD_SSE3)
                        features |= PlatformInformation::CPU_FEATURE_SSE3;
                    if (result._ecx & CPUID_STD_SSE41)
                        features |= PlatformInformation::CPU_FEATURE_SSE41;
                    if (result._ecx & CPUID_STD_SSE42)
                        features |= PlatformInformation::CPU_FEATURE_SSE42;

                    // Check to see if this is a Pentium 4 or later processor
                    if ((result._eax & CPUID_EXT_FAMILY_ID_MASK) ||
                        (result._eax & CPUID_FAMILY_ID_MASK) == CPUID_PENTIUM4_ID)
                    {
                        // Check hyper-threading technology
                        if (result._edx & CPUID_STD_HTT)
                            features |= PlatformInformation::CPU_FEATURE_HTT;
                    }


                    const uint maxExtensionFunctionSupport = _performCpuid(CPUID_FUNC_EXTENSION_QUERY, result);
                    if (maxExtensionFunctionSupport >= CPUID_FUNC_ADVANCED_POWER_MANAGEMENT)
                    {
                        _performCpuid(CPUID_FUNC_ADVANCED_POWER_MANAGEMENT, result);

                        if (result._edx & CPUID_APM_INVARIANT_TSC)
                            features |= PlatformInformation::CPU_FEATURE_INVARIANT_TSC;
                    }
                }
                else if (memcmp(&result._ebx, "AuthenticAMD", 12) == 0)
                {
                    features |= PlatformInformation::CPU_FEATURE_PRO;

                    // Check standard feature
                    _performCpuid(CPUID_FUNC_STANDARD_FEATURES, result);

                    if (result._edx & CPUID_STD_FPU)
                        features |= PlatformInformation::CPU_FEATURE_FPU;
                    if (result._edx & CPUID_STD_TSC)
                        features |= PlatformInformation::CPU_FEATURE_TSC;
                    if (result._edx & CPUID_STD_CMOV)
                        features |= PlatformInformation::CPU_FEATURE_CMOV;
                    if (result._edx & CPUID_STD_MMX)
                        features |= PlatformInformation::CPU_FEATURE_MMX;
                    if (result._edx & CPUID_STD_SSE)
                        features |= PlatformInformation::CPU_FEATURE_SSE;
                    if (result._edx & CPUID_STD_SSE2)
                        features |= PlatformInformation::CPU_FEATURE_SSE2;

                    if (result._ecx & CPUID_STD_SSE3)
                        features |= PlatformInformation::CPU_FEATURE_SSE3;

                    // Has extended feature ?
                    const uint maxExtensionFunctionSupport = _performCpuid(CPUID_FUNC_EXTENSION_QUERY, result);
                    if (maxExtensionFunctionSupport >= CPUID_FUNC_EXTENDED_FEATURES)
                    {
                        // Check extended feature
                        _performCpuid(CPUID_FUNC_EXTENDED_FEATURES, result);

                        if (result._edx & CPUID_EXT_3DNOW)
                            features |= PlatformInformation::CPU_FEATURE_3DNOW;
                        if (result._edx & CPUID_EXT_AMD_3DNOWEXT)
                            features |= PlatformInformation::CPU_FEATURE_3DNOWEXT;
                        if (result._edx & CPUID_EXT_AMD_MMXEXT)
                            features |= PlatformInformation::CPU_FEATURE_MMXEXT;
                    }


                    if (maxExtensionFunctionSupport >= CPUID_FUNC_ADVANCED_POWER_MANAGEMENT)
                    {
                        _performCpuid(CPUID_FUNC_ADVANCED_POWER_MANAGEMENT, result);

                        if (result._edx & CPUID_APM_INVARIANT_TSC)
                            features |= PlatformInformation::CPU_FEATURE_INVARIANT_TSC;
                    }
                }
            }
        }

        return features;
    }
    //---------------------------------------------------------------------
    static uint _detectCpuFeatures()
    {
        uint features = queryCpuFeatures();

        const uint sse_features = 0
            | PlatformInformation::CPU_FEATURE_SSE
            | PlatformInformation::CPU_FEATURE_SSE2
            | PlatformInformation::CPU_FEATURE_SSE3
            | PlatformInformation::CPU_FEATURE_SSE41
            | PlatformInformation::CPU_FEATURE_SSE42;

        if ((features & sse_features) && !_checkOperatingSystemSupportSSE())
        {
            features &= ~sse_features;
        }

        return features;
    }
    //---------------------------------------------------------------------
    static String _detectCpuIdentifier()
    {
        // Supports CPUID instruction ?
        if (_isSupportCpuid())
        {
            CpuidResult result;
            uint nExIds;
            char CPUString[0x20];
            char CPUBrandString[0x40];

            StringStream detailedIdentStr;


            // Has standard feature ?
            if (_performCpuid(0, result))
            {
                memset(CPUString, 0, sizeof(CPUString));
                memset(CPUBrandString, 0, sizeof(CPUBrandString));

                //*((int*)CPUString) = result._ebx;
                //*((int*)(CPUString+4)) = result._edx;
                //*((int*)(CPUString+8)) = result._ecx;
                memcpy(CPUString, &result._ebx, sizeof(int));
                memcpy(CPUString+4, &result._edx, sizeof(int));
                memcpy(CPUString+8, &result._ecx, sizeof(int));

                detailedIdentStr << CPUString;

                // Calling _performCpuid with 0x80000000 as the query argument
                // gets the number of valid extended IDs.
                nExIds = _performCpuid(0x80000000, result);

                for (uint i=0x80000000; i<=nExIds; ++i)
                {
                    _performCpuid(i, result);

                    // Interpret CPU brand string and cache information.
                    if  (i == 0x80000002)
                    {
                        memcpy(CPUBrandString + 0, &result._eax, sizeof(result._eax));
                        memcpy(CPUBrandString + 4, &result._ebx, sizeof(result._ebx));
                        memcpy(CPUBrandString + 8, &result._ecx, sizeof(result._ecx));
                        memcpy(CPUBrandString + 12, &result._edx, sizeof(result._edx));
                    }
                    else if  (i == 0x80000003)
                    {
                        memcpy(CPUBrandString + 16 + 0, &result._eax, sizeof(result._eax));
                        memcpy(CPUBrandString + 16 + 4, &result._ebx, sizeof(result._ebx));
                        memcpy(CPUBrandString + 16 + 8, &result._ecx, sizeof(result._ecx));
                        memcpy(CPUBrandString + 16 + 12, &result._edx, sizeof(result._edx));
                    }
                    else if  (i == 0x80000004)
                    {
                        memcpy(CPUBrandString + 32 + 0, &result._eax, sizeof(result._eax));
                        memcpy(CPUBrandString + 32 + 4, &result._ebx, sizeof(result._ebx));
                        memcpy(CPUBrandString + 32 + 8, &result._ecx, sizeof(result._ecx));
                        memcpy(CPUBrandString + 32 + 12, &result._edx, sizeof(result._edx));
                    }
                }

                String brand(CPUBrandString);
                StringUtil::trim(brand);
                if (!brand.empty())
                    detailedIdentStr << ": " << brand;

                return detailedIdentStr.str();
            }
        }

        return "X86";
    }

    //---------------------------------------------------------------------
    // Platform-independent routines, but the returns value are platform-dependent
    //---------------------------------------------------------------------

    const String& PlatformInformation::getCpuIdentifier() noexcept
    {
        static const String sIdentifier = _detectCpuIdentifier();
        return sIdentifier;
    }
    //---------------------------------------------------------------------
    uint PlatformInformation::getCpuFeatures() noexcept
    {
        static const uint sFeatures = _detectCpuFeatures();
        return sFeatures;
    }
    //---------------------------------------------------------------------
    bool PlatformInformation::hasCpuFeature(CpuFeatures feature)
    {
        return (getCpuFeatures() & feature) != 0;
    }
    //---------------------------------------------------------------------
    void PlatformInformation::log(Log* pLog)
    {
        pLog->logMessage("CPU Identifier & Features");
        pLog->logMessage("-------------------------");
        pLog->logMessage(
            ::std::format(" *   CPU ID: {}", getCpuIdentifier()));

        if(_isSupportCpuid())
        {
            pLog->logMessage(
                ::std::format(" *          SSE: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_SSE)), true));
            pLog->logMessage(
                ::std::format(" *         SSE2: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_SSE2)), true));
            pLog->logMessage(
                ::std::format(" *         SSE3: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_SSE3)), true));
            pLog->logMessage(
                ::std::format(" *        SSE41: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_SSE41)), true));
            pLog->logMessage(
                ::std::format(" *        SSE42: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_SSE42)), true));
            pLog->logMessage(
                ::std::format(" *          MMX: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_MMX)), true));
            pLog->logMessage(
                ::std::format(" *       MMXEXT: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_MMXEXT)), true));
            pLog->logMessage(
                ::std::format(" *        3DNOW: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_3DNOW)), true));
            pLog->logMessage(
                ::std::format(" *     3DNOWEXT: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_3DNOWEXT)), true));
            pLog->logMessage(
                ::std::format(" *         CMOV: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_CMOV)), true));
            pLog->logMessage(
                ::std::format(" *          TSC: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_TSC)), true));
            pLog->logMessage(
                ::std::format(" *INVARIANT TSC: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_INVARIANT_TSC)), true));
            pLog->logMessage(
                ::std::format(" *          FPU: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_FPU)), true));
            pLog->logMessage(
                ::std::format(" *          PRO: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_PRO)), true));
            pLog->logMessage(
                ::std::format(" *           HT: {}", StringConverter::toString(hasCpuFeature(CPU_FEATURE_HTT)), true));
        }

        pLog->logMessage("-------------------------");

    }


}
