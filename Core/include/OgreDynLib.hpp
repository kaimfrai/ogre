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
export module Ogre.Core:DynLib;

export import :MemoryAllocatorConfig;
export import :Prerequisites;

export
namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */

    using DYNLIB_HANDLE = void *;

    /** Resource holding data about a dynamic library.
        @remarks
            This class holds the data required to get symbols from
            libraries loaded at run-time (i.e. from DLL's for so's)
        @author
            Adrian Cearn„u (cearny@cearny.ro)
        @since
            27 January 2002
    */
    class DynLib : public DynLibAlloc
    {
    private:
        String mName;
        /// Gets the last loading error
        auto dynlibError() -> String;
    public:
        /** Default constructor - used by DynLibManager.
            @warning
                Do not call directly
        */
        DynLib( std::string_view name );

        /** Default destructor.
        */
        ~DynLib();

        /** Load the library
        */
        void load();
        /** Unload the library
        */
        void unload();
        /// Get the name of the library
        [[nodiscard]] auto getName() const noexcept -> std::string_view { return mName; }

        /**
            Returns the address of the given symbol from the loaded library.
            @param
                strName The name of the symbol to search for
            @return
                If the function succeeds, the returned value is a handle to
                the symbol.
            @par
                If the function fails, the returned value is <b>NULL</b>.

        */
        [[nodiscard]] auto getSymbol( std::string_view strName ) const noexcept -> void*;

    private:

        /// Handle to the loaded library.
        DYNLIB_HANDLE mInst;
    };
    /** @} */
    /** @} */

}
