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

#ifndef OGRE_CORE_SCRIPTLEXER_H
#define OGRE_CORE_SCRIPTLEXER_H

#include <algorithm>
#include <vector>

#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"

namespace Ogre {

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */
    /** These codes represent token IDs which are numerical translations of
        specific lexemes. Specific compilers using the lexer can register their
        own token IDs which are given precedence over these built-in ones.
    */
    enum{
        TID_LBRACKET = 0, // {
        TID_RBRACKET, // }
        TID_COLON, // :
        TID_VARIABLE, // $...
        TID_WORD, // *
        TID_QUOTE, // "*"
        TID_NEWLINE, // \n
        TID_UNKNOWN,
        TID_END
    };

    /** This struct represents a token, which is an ID'd lexeme from the
        parsing input stream.
    */
    struct ScriptToken
    {
        /// This is the lexeme for this token
        std::string lexeme;
        /// This is the id associated with the lexeme, which comes from a lexeme-token id mapping
        uint32 type;
        /// This holds the line number of the input stream where the token was found.
        uint32 line;
    };
    using ScriptTokenList = std::vector<ScriptToken>;

    class ScriptLexer : public ScriptCompilerAlloc
    {
    public:
        /** Tokenizes the given input and returns the list of tokens found */
        static auto tokenize(std::string_view str, std::string_view source) -> ScriptTokenList;
    private: // Private utility operations
        static auto _tokenize(std::string_view str, const char* source, String& error) -> ScriptTokenList;
        static void setToken(std::string_view lexeme, uint32 line, ScriptTokenList& tokens);
        static auto isWhitespace(Ogre::String::value_type c) -> bool;
        static auto isNewline(Ogre::String::value_type c) -> bool;
    };

    /** @} */
    /** @} */
}

#endif
