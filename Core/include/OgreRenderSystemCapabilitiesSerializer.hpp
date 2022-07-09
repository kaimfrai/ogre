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
#ifndef OGRE_CORE_RENDERSYSTEMCAPABILITIESSERIALIZER_H
#define OGRE_CORE_RENDERSYSTEMCAPABILITIESSERIALIZER_H

#include <format>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderSystemCapabilities.hpp"
#include "OgreSharedPtr.hpp"


namespace Ogre {


    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */
    /** Class for serializing RenderSystemCapabilities to / from a .rendercaps script.*/
    class RenderSystemCapabilitiesSerializer : public RenderSysAlloc
    {

    public:
        /** default constructor*/
        RenderSystemCapabilitiesSerializer();

        /** Writes a RenderSystemCapabilities object to a data stream */
        void writeScript(const RenderSystemCapabilities* caps, std::string_view name, String filename);
        
        /** Writes a RenderSystemCapabilities object to a string */
        auto writeString(const RenderSystemCapabilities* caps, std::string_view name) -> String;

        /** Parses a RenderSystemCapabilities script file passed as a stream.
            Adds it to RenderSystemCapabilitiesManager::_addRenderSystemCapabilities
        */
        void parseScript(DataStreamPtr& stream);

    private:
        void write(const RenderSystemCapabilities* caps, std::string_view name, std::ostream &file);

        enum CapabilityKeywordType {UNDEFINED_CAPABILITY_TYPE = 0, SET_STRING_METHOD, SET_INT_METHOD, SET_BOOL_METHOD, SET_REAL_METHOD,
                                SET_CAPABILITY_ENUM_BOOL, ADD_SHADER_PROFILE_STRING};
        // determines what keyword is what type of capability. For example:
        // "automipmap" and "pbuffer" are both activated with setCapability (passing RSC_AUTOMIPMAP and RSC_PBUFFER respectivelly)
        // while "max_num_multi_render_targets" is an integer and has it's own method: setMaxMultiNumRenderTargets
        // we need to know these types to automatically parse each capability
        using KeywordTypeMap = std::map<std::string_view, CapabilityKeywordType>;
        KeywordTypeMap mKeywordTypeMap;

        using SetStringMethod = void (RenderSystemCapabilities::*)(std::string_view );
        // maps capability keywords to setCapability(String& cap) style methods
        using SetStringMethodDispatchTable = std::map<std::string_view, SetStringMethod>;
        SetStringMethodDispatchTable mSetStringMethodDispatchTable;

        // SET_INT_METHOD parsing tables
        using SetIntMethod = void (RenderSystemCapabilities::*)(ushort);
        using SetIntMethodDispatchTable = std::map<std::string_view, SetIntMethod>;
        SetIntMethodDispatchTable mSetIntMethodDispatchTable;

        // SET_BOOL_METHOD parsing tables
        using SetBoolMethod = void (RenderSystemCapabilities::*)(bool);
        using SetBoolMethodDispatchTable = std::map<std::string_view, SetBoolMethod>;
        SetBoolMethodDispatchTable mSetBoolMethodDispatchTable;

        // SET_REAL_METHOD parsing tables
        using SetRealMethod = void (RenderSystemCapabilities::*)(Real);
        using SetRealMethodDispatchTable = std::map<std::string_view, SetRealMethod>;
        SetRealMethodDispatchTable mSetRealMethodDispatchTable;

        using CapabilitiesMap = std::map<std::string_view, Capabilities>;
        CapabilitiesMap mCapabilitiesMap;

        inline void addCapabilitiesMapping(std::string_view name, Capabilities cap)
        {
            mCapabilitiesMap.emplace(name, cap);
        }


        // capabilities lines for parsing are collected along with their line numbers for debugging
        using CapabilitiesLinesList = std::vector<std::pair<String, int>>;
        // the set of states that the parser can be in
        enum ParseAction {PARSE_HEADER, FIND_OPEN_BRACE, COLLECT_LINES};

        int mCurrentLineNumber{0};
        String* mCurrentLine{nullptr};
        DataStreamPtr mCurrentStream;

        RenderSystemCapabilities* mCurrentCapabilities{nullptr};

        inline void addKeywordType(std::string_view keyword, CapabilityKeywordType type)
        {
            mKeywordTypeMap.emplace(keyword, type);
        }

        [[nodiscard]] auto getKeywordType(std::string_view keyword) const -> CapabilityKeywordType
        {
            auto it = mKeywordTypeMap.find(keyword);
            if (it != mKeywordTypeMap.end())
                return (*it).second;

            // default
            return SET_CAPABILITY_ENUM_BOOL;
        }

        inline void addSetStringMethod(std::string_view keyword, SetStringMethod method)
        {
            mSetStringMethodDispatchTable.emplace(keyword, method);
        }

        inline void callSetStringMethod(std::string_view keyword, String& val)
        {
            auto methodIter = mSetStringMethodDispatchTable.find(keyword);
            if (methodIter != mSetStringMethodDispatchTable.end())
            {
                            SetStringMethod m = (*methodIter).second;
                (mCurrentCapabilities->*m)(val);
            }
            else
            {
                logParseError(::std::format("undefined keyword: {}", keyword));
            }
        }


        inline void addSetIntMethod(std::string_view keyword, SetIntMethod method)
        {
            mSetIntMethodDispatchTable.emplace(keyword, method);
        }

        inline void callSetIntMethod(std::string_view keyword, ushort val)
        {
            auto methodIter = mSetIntMethodDispatchTable.find(keyword);
            if (methodIter != mSetIntMethodDispatchTable.end())
            {
                            SetIntMethod m = (*methodIter).second;
                (mCurrentCapabilities->*m)(val);
            }
            else
            {
                logParseError(::std::format("undefined keyword: {}", keyword));
            }  
        }


        inline void addSetBoolMethod(std::string_view keyword, SetBoolMethod method)
        {
            mSetBoolMethodDispatchTable.emplace(keyword, method);
        }

        inline void callSetBoolMethod(std::string_view keyword, bool val)
        {
            auto methodIter = mSetBoolMethodDispatchTable.find(keyword);
            if (methodIter != mSetBoolMethodDispatchTable.end())
            {
                            SetBoolMethod m = (*methodIter).second;
                (mCurrentCapabilities->*m)(val);
            }
            else
            {
                logParseError(::std::format("undefined keyword: {}", keyword));
                        }
        }


        inline void addSetRealMethod(std::string_view keyword, SetRealMethod method)
        {
            mSetRealMethodDispatchTable.emplace(keyword, method);
        }

        inline void callSetRealMethod(std::string_view keyword, Real val)
        {
            auto methodIter = mSetRealMethodDispatchTable.find(keyword);
            if (methodIter != mSetRealMethodDispatchTable.end())
            {
                            SetRealMethod m = (*methodIter).second;
                (mCurrentCapabilities->*m)(val);
            }
            else
            {
                logParseError(::std::format("undefined keyword: {}", keyword));
                        }
        }

        inline void addShaderProfile(std::string_view val)
        {
            mCurrentCapabilities->addShaderProfile(val);
        }

        inline void setCapabilityEnumBool(std::string_view name, bool val)
        {
            // check for errors
            if(mCapabilitiesMap.find(name) == mCapabilitiesMap.end())
            {
                logParseError(::std::format("Undefined capability: {}", name));
                return;
            }
            // only set true capabilities, we can't unset false
            if(val)
            {
                Capabilities cap = mCapabilitiesMap[name];
                mCurrentCapabilities->setCapability(cap);
            }
        }

        void initialiaseDispatchTables();

        void parseCapabilitiesLines(CapabilitiesLinesList& linesList);

        void logParseError(std::string_view error) const;

    };
    /** @} */
    /** @} */

}

#endif
