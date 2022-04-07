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

#   include <dlfcn.h>
}

module Ogre.Core:DynLib.Obj;

import :DynLib;
import :Exception;
import :LogManager;
import :String;

import <cstddef>;
import <string>;

#define DYNLIB_LOAD( a ) dlopen( a, RTLD_LAZY | RTLD_GLOBAL)
#define DYNLIB_GETSYM( a, b ) dlsym( a, b )
#define DYNLIB_UNLOAD( a ) dlclose( a )
extern "C" {

namespace Ogre {

    //-----------------------------------------------------------------------
    DynLib::DynLib( const String& name )
    {
        mName = name;
        mInst = nullptr;
    }

    //-----------------------------------------------------------------------
    DynLib::~DynLib()
    = default;

    //-----------------------------------------------------------------------
    void DynLib::load()
    {
        String name = mName;

        // dlopen() does not add .so to the filename, like windows does for .dll
        if (name.find(".so") == String::npos)
        {
            name += StringUtil::format(".so.%d.%d", /*OGRE_VERSION_MAJOR*/13, /*OGRE_VERSION_MINOR*/3);
        }

        // Log library load
        LogManager::getSingleton().logMessage("Loading library " + name);

        mInst = (DYNLIB_HANDLE)DYNLIB_LOAD( name.c_str() );

        if( !mInst )
            OGRE_EXCEPT(
                Exception::ERR_INTERNAL_ERROR, 
                "Could not load dynamic library " + mName + 
                ".  System Error: " + dynlibError(),
                "DynLib::load" );
    }

    //-----------------------------------------------------------------------
    void DynLib::unload()
    {
        // Log library unload
        LogManager::getSingleton().logMessage("Unloading library " + mName);

        if( DYNLIB_UNLOAD( mInst ) )
        {
            OGRE_EXCEPT(
                Exception::ERR_INTERNAL_ERROR, 
                "Could not unload dynamic library " + mName +
                ".  System Error: " + dynlibError(),
                "DynLib::unload");
        }

    }

    //-----------------------------------------------------------------------
    auto DynLib::getSymbol( const String& strName ) const noexcept -> void*
    {
        return (void*)DYNLIB_GETSYM( mInst, strName.c_str() );
    }
    //-----------------------------------------------------------------------
    auto DynLib::dynlibError( ) -> String 
    {
        return {dlerror()};
    }

}
