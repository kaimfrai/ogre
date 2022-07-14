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
        static const std::string_view constexpr delims = " \t\r\n";
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
                        ret.push_back(str.substr(delimStart));
                    }
                    else
                    {
                        ret.push_back(str.substr(delimStart, delimPos - delimStart));
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
        String delims = std::format("{}{}", singleDelims, doubleDelims);

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
                ret.push_back(str.substr(start));
                break;
            }
            else
            {
                if (curDoubleDelim != 0)
                {
                    curDoubleDelim = 0;
                }

                // Copy up to delimiter
                ret.push_back(str.substr(start, pos - start));
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
            return strnicmp(str.data(), pattern.data(), pattern.size()) == 0;
        }

        return strncmp(str.data(), pattern.data(), pattern.size()) == 0;
    }
    //-----------------------------------------------------------------------
    auto StringUtil::endsWith(std::string_view str, std::string_view pattern, bool lowerCase) -> bool
    {
        if (pattern.empty())
            return false;

        size_t offset = str.size() - pattern.size();

        if (lowerCase)
        {
            return strnicmp(str.data() + offset, pattern.data(), pattern.size()) == 0;
        }

        return strncmp(str.data() + offset, pattern.data(), pattern.size()) == 0;
    }
    //-----------------------------------------------------------------------
    auto StringUtil::standardisePath(std::string_view init) -> String
    {
        String path{ init };

        std::ranges::replace(path, '\\', '/' );
        if( path[path.length() - 1] != '/' )
            path += '/';

        return path;
    }
    //-----------------------------------------------------------------------
    auto StringUtil::normalizeFilePath(std::string_view init, bool makeLowerCase) -> String
    {
        const char* bufferSrc = init.data();
        int pathLen = (int)init.size();
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
        std::string_view& outBasename, std::string_view& outPath)
    {
        String path{ qualifiedName };
        // Replace \ with / first
        std::ranges::replace(path, '\\', '/' );
        // split based on final /
        size_t i = path.find_last_of('/');

        if (i == String::npos)
        {
            outPath = "";
            outBasename = qualifiedName;
        }
        else
        {
            outBasename = qualifiedName.substr(i+1, qualifiedName.size() - i - 1);
            outPath = qualifiedName.substr(0, i+1);
        }

    }
    //-----------------------------------------------------------------------
    void StringUtil::splitBaseFilename(std::string_view fullName,
        std::string_view& outBasename, std::string_view& outExtention)
    {
        size_t i = fullName.find_last_of('.');
        if (i == Ogre::String::npos)
        {
            outExtention = "";
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
        std::string_view& outBasename, std::string_view& outExtention, std::string_view& outPath )
    {
        std::string_view fullName;
        splitFilename( qualifiedName, fullName, outPath );
        splitBaseFilename( fullName, outBasename, outExtention );
    }
    //-----------------------------------------------------------------------
    auto StringUtil::match(std::string_view str, std::string_view pattern, bool caseSensitive) -> bool
    {
        String tmpStr{ str };
        String tmpPattern{ pattern };
        if (!caseSensitive)
        {
            StringUtil::toLowerCase(tmpStr);
            StringUtil::toLowerCase(tmpPattern);
        }

        String::const_iterator strIt = tmpStr.begin();
        String::const_iterator patIt = tmpPattern.begin();
        String::const_iterator lastWildCardIt = tmpPattern.end();
        while (strIt != tmpStr.end() && patIt != tmpPattern.end())
        {
            if (*patIt == '*')
            {
                lastWildCardIt = patIt;
                // Skip over looking for next character
                ++patIt;
                if (patIt == tmpPattern.end())
                {
                    // Skip right to the end since * matches the entire rest of the string
                    strIt = tmpStr.end();
                }
                else
                {
                    // scan until we find next pattern character
                    while(strIt != tmpStr.end() && *strIt != *patIt)
                        ++strIt;
                }
            }
            else
            {
                if (*patIt != *strIt)
                {
                    if (lastWildCardIt != tmpPattern.end())
                    {
                        // The last wildcard can match this incorrect sequence
                        // rewind pattern to wildcard and keep searching
                        patIt = lastWildCardIt;
                        lastWildCardIt = tmpPattern.end();
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
        if ((patIt == tmpPattern.end() || (*patIt == '*' && patIt + 1 == tmpPattern.end())) && strIt == tmpStr.end())
        {
            return true;
        }
        else
        {
            return false;
        }

    }
    //-----------------------------------------------------------------------
    auto StringUtil::replaceAll(std::string_view source, std::string_view replaceWhat, std::string_view replaceWithWhat) -> const String
    {
        String result{ source };
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
