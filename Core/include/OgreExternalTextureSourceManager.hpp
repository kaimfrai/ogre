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
export module Ogre.Core:ExternalTextureSourceManager;

/***************************************************************************
OgreExternalTextureSourceManager.h  -
    Handles the registering / unregistering of texture modifier plugins

-------------------
date                 : Jan 1 2004
email                : pjcast@yahoo.com
***************************************************************************/
export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :ResourceGroupManager;
export import :Singleton;

export import <map>;
export import <string>;

export
namespace Ogre
{
class ExternalTextureSource;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Materials
    *  @{
    */
    /** 
    Singleton Class which handles the registering and control of texture plugins. The plugins
    will be mostly controlled via a string interface. */
    class ExternalTextureSourceManager : public Singleton<ExternalTextureSourceManager>, public ResourceAlloc
    {
    public:
        /** Constructor */
        ExternalTextureSourceManager();
        /** Destructor */
        ~ExternalTextureSourceManager();

        /** Sets active plugin (ie. "video", "effect", "generic", etc..) */
        void setCurrentPlugIn( std::string_view sTexturePlugInType );

        /** Returns currently selected plugin, may be null if none selected */
        [[nodiscard]] auto getCurrentPlugIn( ) const noexcept -> ExternalTextureSource* { return mCurrExternalTextureSource; }
    
        /** Calls the destroy method of all registered plugins... 
        Only the owner plugin should perform the destroy action. */
        void destroyAdvancedTexture( std::string_view sTextureName,
            std::string_view groupName = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        /** Returns the plugin which registered itself with a specific name 
        (eg. "video"), or null if specified plugin not found */
        auto getExternalTextureSource( std::string_view sTexturePlugInType ) -> ExternalTextureSource*;

        /** Called from plugin to register itself */
        void setExternalTextureSource( std::string_view sTexturePlugInType, ExternalTextureSource* pTextureSystem );

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> ExternalTextureSourceManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> ExternalTextureSourceManager*;
    private:
        /// The current texture controller selected
        ExternalTextureSource* mCurrExternalTextureSource;
        
        // Collection of loaded texture System PlugIns, keyed by registered type
        using TextureSystemList = std::map<std::string_view, ExternalTextureSource *>;
        TextureSystemList mTextureSystems;
    };
    /** @} */
    /** @} */
} 
