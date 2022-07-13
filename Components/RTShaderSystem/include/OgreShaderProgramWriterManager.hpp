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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_PROGRAMWRITERMANAGER_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_PROGRAMWRITERMANAGER_H

#include <format>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "OgreException.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderProgramWriter.hpp"
#include "OgreSingleton.hpp"

namespace Ogre::RTShader {

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

class ProgramWriterManager 
    : public Singleton<ProgramWriterManager>, public RTShaderSystemAlloc
{
    std::map<std::string_view, ::std::unique_ptr<ProgramWriter>> mProgramWriters;

public:
    ProgramWriterManager();

    /// register and transfer ownership of writer
    void addProgramWriter(std::string_view lang, ::std::unique_ptr<ProgramWriter> writer);

    /** Returns whether a given high-level language is supported. */
    auto isLanguageSupported(std::string_view lang) -> bool;

    [[nodiscard]] auto getProgramWriter(std::string_view language) const -> ProgramWriter*
    {
        auto it = mProgramWriters.find(language);
        if (it != mProgramWriters.end())
            return it->second.get();
        OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("No program writer for language {}", language));
        return nullptr;
    }

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
    static auto getSingleton() noexcept -> ProgramWriterManager&;

    /// @copydoc Singleton::getSingleton()
    static auto getSingletonPtr() noexcept -> ProgramWriterManager*;
};
/** @} */
/** @} */
}


#endif
