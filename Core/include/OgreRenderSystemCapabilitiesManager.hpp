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
#ifndef OGRE_CORE_RENDERSYSTEMCAPABILITIESMANAGER_H
#define OGRE_CORE_RENDERSYSTEMCAPABILITIESMANAGER_H

#include <map>
#include <string>

#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreSingleton.hpp"

namespace Ogre {
    class RenderSystemCapabilities;
    class RenderSystemCapabilitiesSerializer;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */
    /** Class for managing RenderSystemCapabilities database for Ogre.
        @remarks This class behaves similarly to other ResourceManager, although .rendercaps are not resources.
                        It contains and abstract a .rendercaps Serializer
    */
    class RenderSystemCapabilitiesManager :  public Singleton<RenderSystemCapabilitiesManager>, public RenderSysAlloc
    {

    public:

        /** Default constructor.
        */
        RenderSystemCapabilitiesManager();

        /** Default destructor.
        */
        ~RenderSystemCapabilitiesManager();


        /** @see ScriptLoader::parseScript
        */
        void parseCapabilitiesFromArchive(const String& filename, const String& archiveType, bool recursive = true);
        
        /** Returns a capability loaded with RenderSystemCapabilitiesManager::parseCapabilitiesFromArchive method
        * @return NULL if the name is invalid, a parsed RenderSystemCapabilities otherwise.
        */
        auto loadParsedCapabilities(const String& name) -> RenderSystemCapabilities*;

        /** Access to the internal map of loaded capabilities */
        [[nodiscard]]
        auto getCapabilities() const -> const std::map<String, RenderSystemCapabilities*> &;

        /** Method used by RenderSystemCapabilitiesSerializer::parseScript */
        void _addRenderSystemCapabilities(const String& name, RenderSystemCapabilities* caps);

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() -> RenderSystemCapabilitiesManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() -> RenderSystemCapabilitiesManager*;

    private:

        RenderSystemCapabilitiesSerializer* mSerializer;

        typedef std::map<String, RenderSystemCapabilities*> CapabilitiesMap;
        CapabilitiesMap mCapabilitiesMap;

        const String mScriptPattern;

    };

    /** @} */
    /** @} */
}

#endif
