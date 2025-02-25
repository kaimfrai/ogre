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
export module Ogre.Core:FileSystem;

export import :ArchiveFactory;
export import :Prerequisites;

export import <iosfwd>;

export
struct AAssetManager;

export
namespace Ogre {
class Archive;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    /// internal method to open a FileStreamDataStream
    auto _openFileStream(std::string_view path, std::ios::openmode mode, std::string_view name = "") -> DataStreamPtr;

    /** Specialisation of the ArchiveFactory to allow reading of files from
        filesystem folders / directories.
    */
    class FileSystemArchiveFactory : public ArchiveFactory
    {
    public:
        /// @copydoc FactoryObj::getType
        [[nodiscard]] auto getType() const noexcept -> std::string_view override;

        using ArchiveFactory::createInstance;

        auto createInstance( std::string_view name, bool readOnly ) -> Archive * override;

        /// Set whether filesystem enumeration will include hidden files or not.
        /// This should be called prior to declaring and/or initializing filesystem
        /// resource locations. The default is true (ignore hidden files).
        static void setIgnoreHidden(bool ignore);

        /// Get whether hidden files are ignored during filesystem enumeration.
        static auto getIgnoreHidden() noexcept -> bool;
    };

    class APKFileSystemArchiveFactory : public ArchiveFactory
    {
    public:
        APKFileSystemArchiveFactory(AAssetManager* assetMgr) : mAssetMgr(assetMgr) {}
        ~APKFileSystemArchiveFactory() override = default;
        /// @copydoc FactoryObj::getType
        [[nodiscard]] auto getType() const noexcept -> std::string_view override;
        /// @copydoc ArchiveFactory::createInstance
        auto createInstance( std::string_view name, bool readOnly ) -> Archive * override;
    private:
        AAssetManager* mAssetMgr;
    };

    /** @} */
    /** @} */

} // namespace Ogre
