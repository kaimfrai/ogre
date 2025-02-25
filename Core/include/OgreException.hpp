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
export module Ogre.Core:Exception;

// Precompiler options
export import :Platform;
export import :Prerequisites;

export import <exception>;
export import <source_location>;
export import <string_view>;

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
    /** Static definitions of error codes.
        @todo
            Add many more exception codes, since we want the user to be able
            to catch most of them.
    */
    enum class ExceptionCodes {
        CANNOT_WRITE_TO_FILE,
        INVALID_STATE,
        INVALIDPARAMS,
        RENDERINGAPI_ERROR,
        DUPLICATE_ITEM,
        ITEM_NOT_FOUND = DUPLICATE_ITEM,
        FILE_NOT_FOUND,
        INTERNAL_ERROR,
        RT_ASSERTION_FAILED,
        NOT_IMPLEMENTED,
        INVALID_CALL
    };
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

        /** Default constructor.
        */
        Exception( ExceptionCodes number, std::string_view description, std::string_view source );

        /** Advanced constructor.
        */
        Exception( ExceptionCodes number, std::string_view description, std::string_view source, const char* type, const char* file, long line );

        /** Copy constructor.
        */
        Exception(const Exception& rhs);

        /// Needed for compatibility with std::exception
        ~Exception() noexcept override = default;

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
        [[nodiscard]] auto getFullDescription() const noexcept -> std::string_view { return fullDesc; }

        /** Gets the source function.
        */
        [[nodiscard]] auto getSource() const noexcept -> std::string_view { return source; }

        /** Gets source file name.
        */
        [[nodiscard]] auto getFile() const noexcept -> const char* { return file; }

        /** Gets line number.
        */
        [[nodiscard]] auto getLine() const noexcept -> long { return line; }

        /** Returns a string with only the 'description' field of this exception. Use 
            getFullDescriptionto get a full description of the error including line number,
            error number and what function threw the exception.
        */
        [[nodiscard]] auto getDescription() const noexcept -> std::string_view { return description; }

        /// Override std::exception::what
        [[nodiscard]] auto what() const noexcept -> const char* override { return fullDesc.c_str(); }
        
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
        UnimplementedException(ExceptionCodes inNumber, std::string_view inDescription, std::string_view inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class FileNotFoundException : public Exception
    {
    public:
        FileNotFoundException(ExceptionCodes inNumber, std::string_view inDescription, std::string_view inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class IOException : public Exception
    {
    public:
        IOException(ExceptionCodes inNumber, std::string_view inDescription, std::string_view inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class InvalidStateException : public Exception
    {
    public:
        InvalidStateException(ExceptionCodes inNumber, std::string_view inDescription, std::string_view inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class InvalidParametersException : public Exception
    {
    public:
        InvalidParametersException(ExceptionCodes inNumber, std::string_view inDescription, std::string_view inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class ItemIdentityException : public Exception
    {
    public:
        ItemIdentityException(ExceptionCodes inNumber, std::string_view inDescription, std::string_view inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class InternalErrorException : public Exception
    {
    public:
        InternalErrorException(ExceptionCodes inNumber, std::string_view inDescription, std::string_view inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class RenderingAPIException : public Exception
    {
    public:
        RenderingAPIException(ExceptionCodes inNumber, std::string_view inDescription, std::string_view inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class RuntimeAssertionException : public Exception
    {
    public:
        RuntimeAssertionException(ExceptionCodes inNumber, std::string_view inDescription, std::string_view inSource, const char* inFile, long inLine)
            : Exception(inNumber, inDescription, inSource, __FUNCTION__, inFile, inLine) {}
    };
    class InvalidCallException : public Exception
    {
    public:
        InvalidCallException(ExceptionCodes inNumber, std::string_view inDescription, std::string_view inSource, const char* inFile, long inLine)
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
        ExceptionFactory() = default;
        [[noreturn]]
        static void _throwException(
            ExceptionCodes code, ExceptionCodes number,
            std::string_view desc, 
            std::string_view src, const char* file, long line)
        {
            using enum ExceptionCodes;
            switch (code)
            {
            case CANNOT_WRITE_TO_FILE:   throw IOException(number, desc, src, file, line);
            case INVALID_STATE:          throw InvalidStateException(number, desc, src, file, line);
            case INVALIDPARAMS:          throw InvalidParametersException(number, desc, src, file, line);
            case RENDERINGAPI_ERROR:     throw RenderingAPIException(number, desc, src, file, line);
            case DUPLICATE_ITEM:         throw ItemIdentityException(number, desc, src, file, line);
            case FILE_NOT_FOUND:         throw FileNotFoundException(number, desc, src, file, line);
            case INTERNAL_ERROR:         throw InternalErrorException(number, desc, src, file, line);
            case RT_ASSERTION_FAILED:    throw RuntimeAssertionException(number, desc, src, file, line);
            case NOT_IMPLEMENTED:        throw UnimplementedException(number, desc, src, file, line);
            case INVALID_CALL:           throw InvalidCallException(number, desc, src, file, line);
            default:                                    throw Exception(number, desc, src, "Exception", file, line);
            }
        }
    public:
        [[noreturn]]
        static void throwException(
            ExceptionCodes code,
            std::string_view desc,
            std::string_view src, const char* file, long line)
        {
            _throwException(code, code, desc, src, file, line);
        }
    };
    
    [[noreturn]]
    auto inline OGRE_EXCEPT(ExceptionCodes code,
                            ::std::string_view desc,
                            ::std::string_view src =
                                ::std::source_location::current().function_name(),
                            char const* file =
                                ::std::source_location::current().file_name(),
                            ::std::uint_least32_t line =
                               ::std::source_location::current().line()
                            )
    {
        Ogre::ExceptionFactory::throwException(code, desc, src, file, line);
    }

    // Check for OGRE assert mode
    auto inline OgreAssert(auto&& a, ::std::string_view b, std::source_location location = std::source_location::current() ) -> void
    {   if( !static_cast<decltype(a)>(a) )
            // TODO condition string is lost by conversion from macro to inline function
            OGRE_EXCEPT(Ogre::ExceptionCodes::RT_ASSERTION_FAILED, b, location.function_name(), location.file_name(), location.line());
    }

    /// replaced with OgreAssert(expr, mesg) in Debug configuration
    auto inline OgreAssertDbg(auto&& expr[[maybe_unused]], ::std::string_view mesg[[maybe_unused]]) -> void
    {

    }
    /** @} */
    /** @} */

} // Namespace Ogre
