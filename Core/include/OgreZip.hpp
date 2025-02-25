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

#include <cstddef>

export module Ogre.Core:Zip;

export import :ArchiveFactory;
export import :Platform;
export import :Prerequisites;

export
namespace Ogre {
class Archive;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    /** Specialisation to allow reading of files from a zip
        format source archive.

        This archive format supports all archives compressed in the standard
        zip format, including iD pk3 files.
    */
    class ZipArchiveFactory : public ArchiveFactory
    {
    public:
        ~ZipArchiveFactory() override = default;
        /// @copydoc FactoryObj::getType
        [[nodiscard]] auto getType() const noexcept -> std::string_view override;

        using ArchiveFactory::createInstance;

        auto createInstance( std::string_view name, bool readOnly ) -> Archive * override;
    };

    /** Specialisation of ZipArchiveFactory for embedded Zip files. */
    class EmbeddedZipArchiveFactory : public ZipArchiveFactory
    {
    public:
        EmbeddedZipArchiveFactory();
        ~EmbeddedZipArchiveFactory() override;

        [[nodiscard]] auto getType() const noexcept -> std::string_view override;

        using ArchiveFactory::createInstance;

        auto createInstance( std::string_view name, bool readOnly ) -> Archive * override;
        void destroyInstance( Archive* ptr) override;
        
        /** a function type to decrypt embedded zip file
        @param pos pos in file
        @param buf current buffer to decrypt
        @param len - length of buffer
        @return success
        */  
        using DecryptEmbeddedZipFileFunc = bool (*)(size_t, void *, size_t);

        /// Add an embedded file to the embedded file list
        static void addEmbbeddedFile(std::string_view name, const uint8 * fileData, 
                        size_t fileSize, DecryptEmbeddedZipFileFunc decryptFunc);

        /// Remove an embedded file to the embedded file list
        static void removeEmbbeddedFile(std::string_view name);

    };

    /** @} */
    /** @} */

}
