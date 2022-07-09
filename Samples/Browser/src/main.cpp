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
#if defined(OGRE_TRACK_MEMORY) && OGRE_TRACK_MEMORY != 0
namespace
{
    unsigned long NewByteCount = 0;
    unsigned long NewCallCount = 0;
    unsigned long DelCallCount = 0;
    struct
        TrackMemory
    {
        TrackMemory()
        {
            NewCallCount = 0;
            NewByteCount = 0;
            DelCallCount = 0;
        }

        ~TrackMemory() noexcept;

    } TrackMemoryGlobal;
}
#endif

#include <iostream>
#include <string>

#include "OgreException.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreStringConverter.hpp"
#include "SampleBrowser.hpp"

#if defined(OGRE_TRACK_MEMORY) && OGRE_TRACK_MEMORY != 0
void* operator new  ( std::size_t count )
{
    ++NewCallCount;
    NewByteCount += count;
    return ::std::malloc(count);
}
void* operator new[]( std::size_t count )
{
    ++NewCallCount;
    NewByteCount += count;
    return ::std::malloc(count);
}
void operator delete  ( void* ptr ) noexcept
{
    ++DelCallCount;
    return ::std::free(ptr);
}
void operator delete[]( void* ptr ) noexcept
{
    ++DelCallCount;
    ::std::free(ptr);
}

TrackMemory::~TrackMemory() noexcept
{
    printf("\n\nNewCallCount: %lu\nDelCallCount: %lu\nNewByteCount: %lu\n", NewCallCount, DelCallCount, NewByteCount);
}
#endif

auto main(int argc, char *argv[]) -> int {

    try
    {
        ulong frameCount = 666;
        if (argc >= 2)
        {
            Ogre::StringConverter::parse(Ogre::String(argv[1]), frameCount);
        }
        // always no grab
        OgreBites::SampleBrowser brows (true, 0);
        brows.go(nullptr, frameCount);
    }
    catch (Ogre::Exception& e)
    {
        std::cerr << "An exception has occurred: " << e.getFullDescription().c_str() << std::endl;
    }
    return 0;
}
