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
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <format>
#include <string>
#include <strings.h>
#include <vector>

#include "OgreException.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreString.hpp"
#include "OgreStringVector.hpp"

// A quick define to overcome different names for the same function

#define strnicmp strncasecmp

namespace Ogre {

    //-----------------------------------------------------------------------
    void StringUtil::trim(String& str, bool left, bool right)
    {
        /*
        size_t lspaces, rspaces, len = length(), i;

        lspaces = rspaces = 0;

        if( left )
        {
            // Find spaces / tabs on the left
            for( i = 0;
                i < len && ( at(i) == ' ' || at(i) == '\t' || at(i) == '\r');
                ++lspaces, ++i );
        }
        
        if( right && lspaces < len )
        {
            // Find spaces / tabs on the right
            for( i = len - 1;
                i >= 0 && ( at(i) == ' ' || at(i) == '\t' || at(i) == '\r');
                rspaces++, i-- );
        }

        *this = substr(lspaces, len-lspaces-rspaces);
        */
        static const std::string_view delims = " \t\r\n";
        if(right)
            str.erase(str.find_last_not_of(delims)+1); // trim right
        if(left)
            str.erase(0, str.find_first_not_of(delims)); // trim left
    }

    //-----------------------------------------------------------------------
    auto StringUtil::split( std::string_view str, std::string_view delims, unsigned int maxSplits, bool preserveDelims) -> std::vector<std::string_view>
    {
        std::vector<std::string_view> ret;
        // Pre-allocate some space for performance
        ret.reserve(maxSplits ? maxSplits+1 : 10);    // 10 is guessed capacity for most case

        unsigned int numSplits = 0;

        // Use STL methods
        size_t start, pos;
        start = 0;
        do
        {
            pos = str.find_first_of(delims, start);
            if (pos == start)
            {
                // Do nothing
                start = pos + 1;
            }
            else if (pos == String::npos || (maxSplits && numSplits == maxSplits))
            {
                // Copy the rest of the string
                ret.push_back( str.substr(start) );
                break;
            }
            else
            {
                // Copy up to delimiter
                ret.push_back( str.substr(start, pos - start) );

                if(preserveDelims)
                {
                    // Sometimes there could be more than one delimiter in a row.
                    // Loop until we don't find any more delims
                    size_t delimStart = pos, delimPos;
                    delimPos = str.find_first_not_of(delims, delimStart);
                    if (delimPos == String::npos)
                    {
                        // Copy the rest of the string
                        ret.push_back( str.substr(delimStart) );
                    }
                    else
                    {
                        ret.push_back( str.substr(delimStart, delimPos - delimStart) );
                    }
                }

                start = pos + 1;
            }
            // parse up to next real data
            start = str.find_first_not_of(delims, start);
            ++numSplits;

        } while (pos != String::npos);



        return ret;
    }
    //-----------------------------------------------------------------------
    auto StringUtil::tokenise( std::string_view str, std::string_view singleDelims, std::string_view doubleDelims, unsigned int maxSplits) -> std::vector<std::string_view>
    {
        std::vector<std::string_view> ret;
        // Pre-allocate some space for performance
        ret.reserve(maxSplits ? maxSplits+1 : 10);    // 10 is guessed capacity for most case

        unsigned int numSplits = 0;
        auto const delims = std::format("{}{}", singleDelims, doubleDelims);

        // Use STL methods 
        size_t start, pos;
        char curDoubleDelim = 0;
        start = 0;
        do 
        {
            if (curDoubleDelim != 0)
            {
                pos = str.find(curDoubleDelim, start);
            }
            else
            {
                pos = str.find_first_of(delims, start);
            }

            if (pos == start)
            {
                char curDelim = str.at(pos);
                if (doubleDelims.find_first_of(curDelim) != String::npos)
                {
                    curDoubleDelim = curDelim;
                }
                // Do nothing
                start = pos + 1;
            }
            else if (pos == String::npos || (maxSplits && numSplits == maxSplits))
            {
                if (curDoubleDelim != 0)
                {
                    //Missing closer. Warn or throw exception?
                }
                // Copy the rest of the string
                ret.push_back( str.substr(start) );
                break;
            }
            else
            {
                if (curDoubleDelim != 0)
                {
                    curDoubleDelim = 0;
                }

                // Copy up to delimiter
                ret.push_back( str.substr(start, pos - start) );
                start = pos + 1;
            }
            if (curDoubleDelim == 0)
            {
                // parse up to next real data
                start = str.find_first_not_of(singleDelims, start);
            }
            
            ++numSplits;

        } while (start != String::npos);

        return ret;
    }
    //-----------------------------------------------------------------------
    void StringUtil::toLowerCase(String& str)
    {
        std::ranges::transform(
            str,
            str.begin(),
            tolower);
    }

    //-----------------------------------------------------------------------
    void StringUtil::toUpperCase(String& str) 
    {
        std::ranges::transform(
            str,
            str.begin(),
            toupper);
    }
    //-----------------------------------------------------------------------
    void StringUtil::toTitleCase(String& str) 
    {
        String::iterator it = str.begin();
        *it = toupper(*it);
        for (; it != str.end() - 1; it++)
        {
            if (*it == ' ') 
            {
                *(it + 1) = toupper(*(it + 1));
            }
        }
    }
    //-----------------------------------------------------------------------
    auto StringUtil::startsWith(std::string_view str, std::string_view pattern, bool lowerCase) -> bool
    {
        if (pattern.empty())
            return false;

        if (lowerCase)
        {
            auto const shortView = str.substr(0, pattern.size());
            return LowerView(shortView) == LowerView(pattern);
        }

        return str.starts_with(pattern);
    }
    //-----------------------------------------------------------------------
    auto StringUtil::endsWith(std::string_view str, std::string_view pattern, bool lowerCase) -> bool
    {
        if (pattern.empty())
            return false;

        if (lowerCase)
        {
            size_t const offset = str.size() - pattern.size();
            auto const shortView = str.substr(offset);
            return LowerView(shortView) == LowerView(pattern);
        }

        return str.ends_with(pattern);
    }
    //-----------------------------------------------------------------------
    auto StringUtil::standardisePath(std::string_view init) -> std::string
    {
        std::string path{init};

        std::ranges::replace(path, '\\', '/' );
        if( path[path.length() - 1] != '/' )
            path += '/';

        return path;
    }
    //-----------------------------------------------------------------------
    auto StringUtil::normalizeFilePath(std::string_view bufferSrc, bool makeLowerCase) -> std::string
    {
        int pathLen = (int)bufferSrc.size();
        int indexSrc = 0;
        int indexDst = 0;
        int metaPathArea = 0;

        char reservedBuf[1024];
        char* bufferDst = reservedBuf;
        bool isDestAllocated = false;
        if (pathLen > 1023)
        {
            //if source path is to long ensure we don't do a buffer overrun by allocating some
            //new memory
            isDestAllocated = true;
            bufferDst = new char[pathLen + 1];
        }

        //The outer loop loops over directories
        while (indexSrc < pathLen)
        {       
            if (indexSrc && ((bufferSrc[indexSrc] == '\\') || (bufferSrc[indexSrc] == '/')))
            {
                //check if we have a directory delimiter if so skip it (we should already
                //have written such a delimiter by this point
                ++indexSrc;
                continue;
            }
            else
            {
                //check if there is a directory to skip of type ".\"
                if ((bufferSrc[indexSrc] == '.') && 
                    ((bufferSrc[indexSrc + 1] == '\\') || (bufferSrc[indexSrc + 1] == '/')))
                {
                    indexSrc += 2;
                    continue;           
                }

                //check if there is a directory to skip of type "..\"
                else if ((bufferSrc[indexSrc] == '.') && (bufferSrc[indexSrc + 1] == '.') &&
                    ((bufferSrc[indexSrc + 2] == '\\') || (bufferSrc[indexSrc + 2] == '/')))
                {
                    if (indexDst > metaPathArea)
                    {
                        //skip a directory backward in the destination path
                        do {
                            --indexDst;
                        }
                        while ((indexDst > metaPathArea) && (bufferDst[indexDst - 1] != '/'));
                        indexSrc += 3;
                        continue;
                    }
                    else
                    {
                        //we are about to write "..\" to the destination buffer
                        //ensure we will not remove this in future "skip directories"
                        metaPathArea += 3;
                    }
                }
            }

            //transfer the current directory name from the source to the destination
            while (indexSrc < pathLen)
            {
                char curChar = bufferSrc[indexSrc];
                if (makeLowerCase) curChar = tolower(curChar);
                if ((curChar == '\\') || (curChar == '/')) curChar = '/';
                bufferDst[indexDst] = curChar;
                ++indexDst;
                ++indexSrc;
                if (curChar == '/') break;
            }
        }
        bufferDst[indexDst] = 0;

        String normalized(bufferDst); 
        if (isDestAllocated)
        {
            delete[] bufferDst;
        }

        return normalized;      
    }
    //-----------------------------------------------------------------------
    void StringUtil::splitFilename(std::string_view qualifiedName, 
        String& outBasename, String& outPath)
    {
        std::string path{ qualifiedName };
        // Replace \ with / first
        std::ranges::replace(path, '\\', '/' );
        // split based on final /
        size_t i = path.find_last_of('/');

        if (i == String::npos)
        {
            outPath.clear();
            outBasename = qualifiedName;
        }
        else
        {
            outBasename = path.substr(i+1, path.size() - i - 1);
            outPath = path.substr(0, i+1);
        }

    }
    //-----------------------------------------------------------------------
    void StringUtil::splitBaseFilename(std::string_view fullName, 
        Ogre::String& outBasename, Ogre::String& outExtention)
    {
        size_t i = fullName.find_last_of('.');
        if (i == Ogre::String::npos)
        {
            outExtention.clear();
            outBasename = fullName;
        }
        else
        {
            outExtention = fullName.substr(i+1);
            outBasename = fullName.substr(0, i);
        }
    }
    // ----------------------------------------------------------------------------------------------------------------------------------------------
    void StringUtil::splitFullFilename( std::string_view qualifiedName, 
        Ogre::String& outBasename, Ogre::String& outExtention, Ogre::String& outPath )
    {
        Ogre::String fullName;
        splitFilename( qualifiedName, fullName, outPath );
        splitBaseFilename( fullName, outBasename, outExtention );
    }
    //-----------------------------------------------------------------------
    auto StringUtil::match(std::string_view str, std::string_view pattern, bool caseSensitive) -> bool
    {
        using Iterator = std::string_view::iterator;

        auto const
            deref
        =   caseSensitive
        ?   +[] (Iterator it) -> char { return *it; }
        :   +[] (Iterator it) -> char { return static_cast<char>(std::tolower(*it)); }
        ;

        auto strIt = str.begin();
        auto patIt = pattern.begin();
        auto lastWildCardIt = pattern.end();
        while (strIt != str.end() && patIt != pattern.end())
        {
            if (deref(patIt) == '*')
            {
                lastWildCardIt = patIt;
                // Skip over looking for next character
                ++patIt;
                if (patIt == pattern.end())
                {
                    // Skip right to the end since * matches the entire rest of the string
                    strIt = str.end();
                }
                else
                {
                    // scan until we find next pattern character
                    while(strIt != str.end() && deref(strIt) != deref(patIt))
                        ++strIt;
                }
            }
            else
            {
                if (deref(patIt) != deref(strIt))
                {
                    if (lastWildCardIt != pattern.end())
                    {
                        // The last wildcard can match this incorrect sequence
                        // rewind pattern to wildcard and keep searching
                        patIt = lastWildCardIt;
                        lastWildCardIt = pattern.end();
                    }
                    else
                    {
                        // no wildwards left
                        return false;
                    }
                }
                else
                {
                    ++patIt;
                    ++strIt;
                }
            }

        }
        // If we reached the end of both the pattern and the string, we succeeded
        if ((patIt == pattern.end() || (deref(patIt) == '*' && patIt + 1 == pattern.end())) && strIt == str.end())
        {
            return true;
        }
        else
        {
            return false;
        }

    }
    //-----------------------------------------------------------------------
    auto StringUtil::replaceAll(std::string_view source, std::string_view replaceWhat, std::string_view replaceWithWhat) -> std::string
    {
        std::string result{source};
        String::size_type pos = 0;
        for(;;)
        {
            pos = result.find(replaceWhat,pos);
            if (pos == String::npos) break;
            result.replace(pos,replaceWhat.size(),replaceWithWhat);
            pos += replaceWithWhat.size();
        }
        return result;
    }
}
