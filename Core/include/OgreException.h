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

#include <exception>
#include <string>
#include <string_view>
// TODO use <source_location> once it is available in libc++
// #include <source_location>
// copied form https://github.com/llvm/llvm-project/commit/d61487490022aaacc34249475fac3e208c7d767e
namespace std {
    struct source_location {
        struct __impl {
            long _M_line;
            const char *_M_file_name;
            long _M_column;
            const char *_M_function_name;
        };
    };
}

export module Ogre.Core:Exception;

export import :Platform;
export import :Prerequisites;

// Check for OGRE assert mode
#define OgreAssert( a, b ) if( !(a) ) OGRE_EXCEPT_2( Ogre::Exception::ERR_RT_ASSERTION_FAILED, (#a " failed. " b) )
/// replaced with OgreAssert(expr, mesg) in Debug configuration
#define OgreAssertDbg( expr, mesg )
export
namespace Ogre {

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */
    /** When thrown, provides information about an error that has occurred inside the engine.
        @remarks
            OGRE never uses return values to indicate errors. Instead, if an
            error occurs, an exception is thrown, and this is the object that
            encapsulates the detail of the problem. The application using
            OGRE should always ensure that the exceptions are caught, so all
            OGRE engine functions should occur within a
            try{} catch(Ogre::Exception& e) {} block.
        @par
            The user application should never create any instances of this
            object unless it wishes to unify its error handling using the
            same object.
    */
    class Exception : public std::exception
    {
    protected:
        long line;
        const char* typeName;
        String description;
        String source;
        const char* file;
        String fullDesc; // storage for char* returned by what()
    public:
        /** Static definitions of error codes.
            @todo
                Add many more exception codes, since we want the user to be able
                to catch most of them.
        */
        enum ExceptionCodes {
            ERR_CANNOT_WRITE_TO_FILE,
            ERR_INVALID_STATE,
            ERR_INVALIDPARAMS,
            ERR_RENDERINGAPI_ERROR,
            ERR_DUPLICATE_ITEM,
            ERR_ITEM_NOT_FOUND = ERR_DUPLICATE_ITEM,
            ERR_FILE_NOT_FOUND,
            ERR_INTERNAL_ERROR,
            ERR_RT_ASSERTION_FAILED,
            ERR_NOT_IMPLEMENTED,
            ERR_INVALID_CALL
        };

        /** Default constructor.
        */
        Exception( int number, ::std::string_view description, ::std::string_view  source );

        /** Advanced constructor.
        */
        Exception( int number, ::std::string_view  description, ::std::string_view  source, const char* type, const char* file, long line );

        /** Copy constructor.
        */
        Exception(const Exception& rhs);

        /// Needed for compatibility with std::exception
        ~Exception() throw() {}

        /** Returns a string with the full description of this error.
            @remarks
                The description contains the error number, the description
                supplied by the thrower, what routine threw the exception,
                and will also supply extra platform-specific information
                where applicable. For example - in the case of a rendering
                library error, the description of the error will include both
                the place in which OGRE found the problem, and a text
                description from the 3D rendering library, if available.
        */
        ::std::string_view  getFullDescription(void) const { return fullDesc; }

        /** Gets the source function.
        */
        ::std::string_view getSource() const { return source; }

        /** Gets source file name.
        */
        const char* getFile() const { return file; }

        /** Gets line number.
        */
        long getLine() const { return line; }

        /** Returns a string with only the 'description' field of this exception. Use 
            getFullDescriptionto get a full description of the error including line number,
            error number and what function threw the exception.
        */
        ::std::string_view getDescription(void) const { return description; }

        /// Override std::exception::what
        const char* what() const throw() { return fullDesc.c_str(); }
        
    };


    /** Template struct which creates a distinct type for each exception code.
    @note
    This is useful because it allows us to create an overloaded method
    for returning different exception types by value without ambiguity. 
    From 'Modern C++ Design' (Alexandrescu 2001).
    */
    class UnimplementedException : public Exception 
    {
    public:
        UnimplementedException(int inNumber, ::std::string_view  inDescription, ::std::string_view  inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class FileNotFoundException : public Exception
    {
    public:
        FileNotFoundException(int inNumber, ::std::string_view  inDescription, ::std::string_view  inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class IOException : public Exception
    {
    public:
        IOException(int inNumber, ::std::string_view  inDescription, ::std::string_view  inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class InvalidStateException : public Exception
    {
    public:
        InvalidStateException(int inNumber, ::std::string_view  inDescription, ::std::string_view  inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class InvalidParametersException : public Exception
    {
    public:
        InvalidParametersException(int inNumber, ::std::string_view  inDescription, ::std::string_view  inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class ItemIdentityException : public Exception
    {
    public:
        ItemIdentityException(int inNumber, ::std::string_view  inDescription, ::std::string_view  inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class InternalErrorException : public Exception
    {
    public:
        InternalErrorException(int inNumber, ::std::string_view  inDescription, ::std::string_view  inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class RenderingAPIException : public Exception
    {
    public:
        RenderingAPIException(int inNumber, ::std::string_view  inDescription, ::std::string_view  inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class RuntimeAssertionException : public Exception
    {
    public:
        RuntimeAssertionException(int inNumber, ::std::string_view & inDescription, ::std::string_view  inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class InvalidCallException : public Exception
    {
    public:
        InvalidCallException(int inNumber, ::std::string_view  inDescription, ::std::string_view  inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };

    /** Class implementing dispatch methods in order to construct by-value
        exceptions of a derived type based just on an exception code.
    @remarks
        This nicely handles construction of derived Exceptions by value (needed
        for throwing) without suffering from ambiguity - each code is turned into
        a distinct type so that methods can be overloaded. This allows OGRE_EXCEPT
        to stay small in implementation (desirable since it is embedded) whilst
        still performing rich code-to-type mapping. 
    */
    class ExceptionFactory
    {
    private:
        /// Private constructor, no construction
        ExceptionFactory() {}
        [[noreturn]] static void _throwException(
            Exception::ExceptionCodes code, int number,
            ::std::string_view desc,
            ::std::string_view src, const char* file, long line)
        {
            switch (code)
            {
            case Exception::ERR_CANNOT_WRITE_TO_FILE:   throw IOException(number, desc, src, file, line);
            case Exception::ERR_INVALID_STATE:          throw InvalidStateException(number, desc, src, file, line);
            case Exception::ERR_INVALIDPARAMS:          throw InvalidParametersException(number, desc, src, file, line);
            case Exception::ERR_RENDERINGAPI_ERROR:     throw RenderingAPIException(number, desc, src, file, line);
            case Exception::ERR_DUPLICATE_ITEM:         throw ItemIdentityException(number, desc, src, file, line);
            case Exception::ERR_FILE_NOT_FOUND:         throw FileNotFoundException(number, desc, src, file, line);
            case Exception::ERR_INTERNAL_ERROR:         throw InternalErrorException(number, desc, src, file, line);
            case Exception::ERR_RT_ASSERTION_FAILED:    throw RuntimeAssertionException(number, desc, src, file, line);
            case Exception::ERR_NOT_IMPLEMENTED:        throw UnimplementedException(number, desc, src, file, line);
            case Exception::ERR_INVALID_CALL:           throw InvalidCallException(number, desc, src, file, line);
            default:                                    throw Exception(number, desc, src, "Exception", file, line);
            }
        }
    public:
        [[noreturn]] static void throwException(
            Exception::ExceptionCodes code,
            ::std::string_view desc,
            ::std::string_view src, const char* file, long line)
        {
            _throwException(code, code, desc, src, file, line);
        }
    };
    
    //  TODO use ::std::source_location once it is available in libc++

    [[noreturn]]
    auto inline OGRE_EXCEPT(Exception::ExceptionCodes code, ::std::string_view desc,
                            ::std::string_view src = __builtin_source_location()->_M_function_name,
                            const char* file = __builtin_source_location()->_M_file_name,
                            long line = __builtin_source_location()->_M_line
                            )
    {
        Ogre::ExceptionFactory::throwException(code, desc, src, file, line);
    }
    /** @} */
    /** @} */

} // Namespace Ogre
