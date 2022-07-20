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
module Ogre.Core;

import :Common;
import :Prerequisites;
import :StringInterface;

import <map>;
import <string>;
import <type_traits>;
import <utility>;
import <vector>;

namespace Ogre {
    /// Dictionary of parameters
    static ParamDictionaryMap msDictionary;

    ParamDictionary::ParamDictionary() = default;
    ParamDictionary::~ParamDictionary() = default;

    auto ParamDictionary::getParamCommand(std::string_view name) -> ParamCommand*
    {
        auto i = mParamCommands.find(name);
        if (i != mParamCommands.end())
        {
            return i->second;
        }
        else
        {
            return nullptr;
        }
    }

    auto ParamDictionary::getParamCommand(std::string_view name) const -> const ParamCommand*
    {
        auto i = mParamCommands.find(name);
        if (i != mParamCommands.end())
        {
            return i->second;
        }
        else
        {
            return nullptr;
        }
    }

    void ParamDictionary::addParameter(std::string_view name, ParamCommand* paramCmd)
    {
        mParamDefs.emplace_back(name);
        mParamCommands[std::string{name}] = paramCmd;
    }

    auto StringInterface::createParamDictionary(std::string_view className) -> bool
    {
        auto it = msDictionary.find(className);

        if ( it == msDictionary.end() )
        {
            mParamDict = &msDictionary.insert( std::make_pair( className, ParamDictionary() ) ).first->second;
            mParamDictName = className;
            return true;
        }
        else
        {
            mParamDict = &it->second;
            mParamDictName = className;
            return false;
        }
    }

    auto StringInterface::getParameters() const noexcept -> const ParameterList&
    {
        static ParameterList emptyList;

        const ParamDictionary* dict = getParamDictionary();
        if (dict)
            return dict->getParameters();
        else
            return emptyList;

    }

    auto StringInterface::getParameter(std::string_view name) const -> String
    {
        // Get dictionary
        const ParamDictionary* dict = getParamDictionary();

        if (dict)
        {
            // Look up command object
            const ParamCommand* cmd = dict->getParamCommand(name);

            if (cmd)
            {
                return cmd->doGet(this);
            }
        }

        // Fallback
        return "";
    }

    auto StringInterface::setParameter(std::string_view name, std::string_view value) -> bool
    {
        // Get dictionary
        ParamDictionary* dict = getParamDictionary();

        if (dict)
        {
            // Look up command object
            ParamCommand* cmd = dict->getParamCommand(name);
            if (cmd)
            {
                cmd->doSet(this, value);
                return true;
            }
        }
        // Fallback
        return false;
    }
    //-----------------------------------------------------------------------
    void StringInterface::setParameterList(const NameValuePairList& paramList)
    {
        for (const auto & i : paramList)
        {
            setParameter(i.first, i.second);
        }
    }

    void StringInterface::copyParametersTo(StringInterface* dest) const
    {
        // Get dictionary
        if (const ParamDictionary* dict = getParamDictionary())
        {
            // Iterate through own parameters
            for (const auto& name : dict->mParamDefs)
            {
                dest->setParameter(name, getParameter(name));
            }
        }
    }

    //-----------------------------------------------------------------------
    void StringInterface::cleanupDictionary ()
    {
        msDictionary.clear();
    }
}
