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
export module Ogre.Core:ArchiveManager;

export import :IteratorWrapper;
export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :Singleton;

export import <map>;
export import <string>;

export
namespace Ogre {
class Archive;
class ArchiveFactory;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    /** This class manages the available ArchiveFactory plugins. 
    */
    class ArchiveManager : public Singleton<ArchiveManager>, public ArchiveAlloc
    {
    private:
        using ArchiveFactoryMap = std::map<String, ArchiveFactory *>;
        /// Factories available to create archives, indexed by archive type (String identifier e.g. 'Zip')
        ArchiveFactoryMap mArchFactories;
        /// Currently loaded archives
        using ArchiveMap = std::map<String, Archive *>;
        ArchiveMap mArchives;

    public:
        /** Default constructor - should never get called by a client app.
        */
        ArchiveManager();
        /** Default destructor.
        */
        virtual ~ArchiveManager();

        /** Opens an archive for file reading.
            @remarks
                The archives are created using class factories within
                extension libraries.
            @param filename
                The filename that will be opened
            @param archiveType
                The type of archive that this is. For example: "Zip".
            @param readOnly
                Whether the Archive is read only
            @return
                If the function succeeds, a valid pointer to an Archive
                object is returned.
            @par
                If the function fails, an exception is thrown.
        */
        auto load( const String& filename, const String& archiveType, bool readOnly) -> Archive*;

        /** Unloads an archive.
        @remarks
            You must ensure that this archive is not being used before removing it.
        */
        void unload(Archive* arch);
        /** Unloads an archive by name.
        @remarks
            You must ensure that this archive is not being used before removing it.
        */
        void unload(const String& filename);
        using ArchiveMapIterator = MapIterator<ArchiveMap>;
        /** Get an iterator over the Archives in this Manager. */
        auto getArchiveIterator() -> ArchiveMapIterator;

        /** Adds a new ArchiveFactory to the list of available factories.
            @remarks
                Plugin developers who add new archive codecs need to call
                this after defining their ArchiveFactory subclass and
                Archive subclasses for their archive type.
        */
        void addArchiveFactory(ArchiveFactory* factory);
        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() -> ArchiveManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() -> ArchiveManager*;
    };
    /** @} */
    /** @} */

}
