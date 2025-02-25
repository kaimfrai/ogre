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

#include <cassert>

module Ogre.Core;

import :Archive;
import :ArchiveFactory;
import :ArchiveManager;
import :Exception;
import :LogManager;
import :Prerequisites;
import :Singleton;

import <format>;
import <map>;
import <string>;
import <utility>;

namespace Ogre {
    using createFunc = void (*)(Archive **, std::string_view );

    //-----------------------------------------------------------------------
    template<> ArchiveManager* Singleton<ArchiveManager>::msSingleton = nullptr;
    auto ArchiveManager::getSingletonPtr() noexcept -> ArchiveManager*
    {
        return msSingleton;
    }
    auto ArchiveManager::getSingleton() noexcept -> ArchiveManager&
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    ArchiveManager::ArchiveManager()
    = default;
    //-----------------------------------------------------------------------
    auto ArchiveManager::load( std::string_view filename, std::string_view archiveType, bool readOnly) -> Archive*
    {
        auto i = mArchives.find(filename);
        Archive* pArch = nullptr;

        if (i == mArchives.end())
        {
            // Search factories
            auto it = mArchFactories.find(archiveType);
            if (it == mArchFactories.end())
            {
                // Factory not found
                OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                            ::std::format("Cannot find an ArchiveFactory for type '{}'", archiveType ));
            }

            pArch = it->second->createInstance(filename, readOnly);
            pArch->load();
            mArchives[std::string{filename}] = pArch;

        }
        else
        {
            pArch = i->second;
            OgreAssert(pArch->isReadOnly() == readOnly, "existing archive location has different readOnly status");
        }

        return pArch;
    }
    //-----------------------------------------------------------------------
    void ArchiveManager::unload(Archive* arch)
    {
        unload(arch->getName());
    }
    //-----------------------------------------------------------------------
    void ArchiveManager::unload(std::string_view filename)
    {
        auto i = mArchives.find(filename);

        if (i != mArchives.end())
        {
            i->second->unload();
            // Find factory to destroy. An archive factory created this file, it should still be there!
            auto fit = mArchFactories.find(i->second->getType());
            assert( fit != mArchFactories.end() && "Cannot find an ArchiveFactory "
                    "to deal with archive this type" );
            fit->second->destroyInstance(i->second);
            mArchives.erase(i);
        }
    }
    //-----------------------------------------------------------------------
    auto ArchiveManager::getArchiveIterator() -> ArchiveManager::ArchiveMapIterator
    {
        return {mArchives.begin(), mArchives.end()};
    }
    //-----------------------------------------------------------------------
    ArchiveManager::~ArchiveManager()
    {
        // Thanks to http://www.viva64.com/en/examples/V509/ for finding the error for us!
        // (originally, it detected we were throwing using OGRE_EXCEPT in the destructor)
        // We now raise an assert.

        // Unload & delete resources in turn
        for(auto const& [key, arch] : mArchives)
        {
            // Unload
            arch->unload();
            // Find factory to destroy. An archive factory created this file, it should still be there!
            auto fit = mArchFactories.find(arch->getType());
            assert( fit != mArchFactories.end() && "Cannot find an ArchiveFactory "
                    "to deal with archive this type" );
            fit->second->destroyInstance(arch);
            
        }
        // Empty the list
        mArchives.clear();
    }
    //-----------------------------------------------------------------------
    void ArchiveManager::addArchiveFactory(ArchiveFactory* factory)
    {
        mArchFactories.emplace(factory->getType(), factory);
        LogManager::getSingleton().logMessage(::std::format("ArchiveFactory for type '{}' registered", factory->getType() ));
    }

}
