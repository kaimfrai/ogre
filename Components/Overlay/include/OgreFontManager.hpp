/*-------------------------------------------------------------------------
This source file is a part of OGRE
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
THE SOFTWARE
-------------------------------------------------------------------------*/

#ifndef OGRE_COMPONENTS_OVERLAY_FONTMANAGER_H
#define OGRE_COMPONENTS_OVERLAY_FONTMANAGER_H

#include "OgreCommon.hpp"
#include "OgreFont.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResource.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreResourceManager.hpp"
#include "OgreSingleton.hpp"

namespace Ogre
{
    /** \addtogroup Optional
    *  @{
    */
    /** \addtogroup Overlays
    *  @{
    */
    /** Manages Font resources, parsing .fontdef files and generally organising them.*/
    class FontManager : public ResourceManager, public Singleton< FontManager >
    {
    public:

        FontManager();
        ~FontManager() override;

        /// Create a new font
        /// @see ResourceManager::createResource
        auto create (std::string_view name, std::string_view group,
                            bool isManual = false, ManualResourceLoader* loader = nullptr,
                            const NameValuePairList* createParams = nullptr) -> FontPtr;

        /// Get a resource by name
        /// @see ResourceManager::getResourceByName
        auto getByName(std::string_view name, std::string_view groupName = RGN_DEFAULT) const -> FontPtr;

        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static auto getSingleton() noexcept -> FontManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> FontManager*;

    private:

        /// Internal methods
        auto createImpl(std::string_view name, ResourceHandle handle, 
            std::string_view group, bool isManual, ManualResourceLoader* loader, 
            const NameValuePairList* params) -> Resource* override;
        void parseAttribute(std::string_view line, FontPtr& pFont);

        void logBadAttrib(std::string_view line, FontPtr& pFont);


    };
    /** @} */
    /** @} */
}

#endif
