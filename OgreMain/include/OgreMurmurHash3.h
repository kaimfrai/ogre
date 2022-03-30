//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef _MURMURHASH3_H_
#define _MURMURHASH3_H_

#include <cstddef>
#include <stdint.h>

//-----------------------------------------------------------------------------
// Platform-specific functions and macros
#include "OgrePlatform.h"

//-----------------------------------------------------------------------------

namespace Ogre
{
    void MurmurHash3_x86_32  ( const void * key, size_t len, uint32_t seed, void * out );

    void MurmurHash3_x86_128 ( const void * key, size_t len, uint32_t seed, void * out );

    void MurmurHash3_x64_128 ( const void * key, size_t len, uint32_t seed, void * out );

    inline void MurmurHash3_128( const void * key, size_t len, uint32_t seed, void * out ) {
        MurmurHash3_x64_128(key, len, seed, out);
    }
}

//-----------------------------------------------------------------------------

#endif // _MURMURHASH3_H_
