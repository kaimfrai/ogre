/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2019 Torus Knot Software Ltd

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
export module Ogre.Tests:Core.ResourceLocationPriority;

export import Ogre.Core;

export import <ctime>;
export import <memory>;
export import <string>;
export import <vector>;

// Barebones archive containing a single 1-byte file "dummyArchiveTest" whose
// contents are an unsigned char that increments on each construction of the
// archive.
export
class DummyArchive : public Ogre::Archive
{
public:
    DummyArchive(const Ogre::String& name, const Ogre::String& archType)
        : Ogre::Archive(name, archType), mContents(DummyArchive::makeContents()) {}

    ~DummyArchive() override = default;

    [[nodiscard]]
    auto exists(const Ogre::String& name) const -> bool override { return name == "dummyArchiveTest"; }

    [[nodiscard]]
    auto find(const Ogre::String& pattern, bool recursive = true, bool dirs = false) const -> Ogre::StringVectorPtr override
    {
        Ogre::StringVectorPtr results = std::make_shared<Ogre::StringVector>();
        if (dirs) return results;
        if (Ogre::StringUtil::match("dummyArchiveTest", pattern))
        {
            results->push_back("dummyArchiveTest");
        }
        return results;
    }

    [[nodiscard]]
    auto findFileInfo(const Ogre::String& pattern, bool recursive = true,
                                               bool dirs = false) const -> Ogre::FileInfoListPtr override
    {
        Ogre::FileInfoListPtr results = std::make_shared<Ogre::FileInfoList>();
        if (dirs) return results;
        if (Ogre::StringUtil::match("dummyArchiveTest", pattern))
        {
            results->push_back(Ogre::FileInfo{this, "dummyArchiveTest", "/", "dummyArchiveTest", 0, 1});
        }
        return results;
    }

    [[nodiscard]]
    auto getModifiedTime(const Ogre::String& filename) const -> time_t override { return 0; }

    [[nodiscard]]
    auto isCaseSensitive() const -> bool override { return true; }

    [[nodiscard]]
    auto list(bool recursive = true, bool dirs = false) const -> Ogre::StringVectorPtr override
    {
        Ogre::StringVectorPtr results = std::make_shared<Ogre::StringVector>();
        if (dirs) return results;
        results->push_back("dummyArchiveTest");
        return results;
    }

    [[nodiscard]]
    auto listFileInfo(bool recursive = true, bool dirs = false) const -> Ogre::FileInfoListPtr override
    {
        Ogre::FileInfoListPtr results = std::make_shared<Ogre::FileInfoList>();
        if (dirs) return results;
        results->push_back(Ogre::FileInfo{this, "dummyArchiveTest", "/", "dummyArchiveTest", 0, 1});
        return results;
    }

    void load() override {}

    void unload() override {}

    [[nodiscard]]
    auto open(const Ogre::String& filename, bool readOnly = true) const -> Ogre::DataStreamPtr override
    {
        if (filename == "dummyArchiveTest")
        {
            auto* ptr = new unsigned char[1];
            *ptr = mContents;
            return std::make_shared<Ogre::MemoryDataStream>(ptr, 1, true, true);
        }
        return Ogre::MemoryDataStreamPtr();
    }

private:
    static auto makeContents() -> unsigned char
    {
        // Don't start at zero so it's obvious if things aren't initialized.
        static unsigned char counter = 1;
        return counter++;
    }

    unsigned char mContents;
};

export
class DummyArchiveFactory : public Ogre::ArchiveFactory
{
public:
    ~DummyArchiveFactory() override = default;

    auto createInstance(const Ogre::String& name, bool) -> Ogre::Archive* override
    {
        return new DummyArchive(name, "DummyArchive");
    }

    void destroyInstance(Ogre::Archive* ptr) override { delete ptr; }

    [[nodiscard]]
    auto getType() const -> const Ogre::String& override
    {
        static Ogre::String type = "DummyArchive";
        return type;
    }
};
