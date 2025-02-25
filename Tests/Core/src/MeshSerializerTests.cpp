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

#include <cmath>
#include <cstdlib>
#include <cstring>

#include <gtest/gtest.h>

module Ogre.Tests;

import :Core.MeshSerializer;

import Ogre.Core;

import <algorithm>;
import <format>;
import <fstream>;
import <list>;
import <map>;
import <memory>;
import <string>;
import <utility>;
import <vector>;

// Register the test suite
//--------------------------------------------------------------------------
void MeshSerializerTests::SetUp()
{
    mErrorFactor = 0.05;

    mFSLayer = std::make_unique<Ogre::FileSystemLayer>(/*OGRE_VERSION_NAME*/"Tsathoggua");

    mResGroupMgr = std::make_unique<ResourceGroupManager>();
    mLodMgr = std::make_unique<LodStrategyManager>();
    mHardMgr = std::make_unique<DefaultHardwareBufferManager>();
    mMeshMgr = std::make_unique<MeshManager>();
    mSkelMgr = std::make_unique<SkeletonManager>();
    mArchiveFactory = std::make_unique<FileSystemArchiveFactory>();
    mArchiveMgr = std::make_unique<ArchiveManager>();
    mArchiveMgr->addArchiveFactory(mArchiveFactory.get());

    mMatMgr = std::make_unique<MaterialManager>();
    mMatMgr->initialise();

    // Load resource paths from config file
    ConfigFile cf;
    String resourcesPath = mFSLayer->getConfigFilePath("resources.cfg");

    // Go through all sections & settings in the file
    cf.load(resourcesPath);
    for (auto const& [secName, settings] : cf.getSettingsBySection()) {
        for (auto const& [typeName, archName] : settings) {
            if (typeName == "FileSystem") {
                ResourceGroupManager::getSingleton().addResourceLocation(
                    archName, typeName, secName);
            }
        }
    }

    mMesh = MeshManager::getSingleton().load("facial.mesh", "General");

    getResourceFullPath(mMesh, mMeshFullPath);
    if (!copyFile(::std::format("{}.bak", mMeshFullPath), mMeshFullPath)) {
        // If there is no backup, create one.
        copyFile(mMeshFullPath, ::std::format("{}.bak", mMeshFullPath));
    }
    mSkeletonFullPath = "";
    mSkeleton = static_pointer_cast<Skeleton>(SkeletonManager::getSingleton().load("jaiqua.skeleton", "General"));
    getResourceFullPath(mSkeleton, mSkeletonFullPath);
    if (!copyFile(::std::format("{}.bak", mSkeletonFullPath), mSkeletonFullPath)) {
        // If there is no backup, create one.
        copyFile(mSkeletonFullPath, ::std::format("{}.bak", mSkeletonFullPath));
    }

    mMesh->reload();

    mOrigMesh = mMesh->clone(::std::format("{}.orig.mesh", mMesh->getName()), mMesh->getGroup());
}

//--------------------------------------------------------------------------
void MeshSerializerTests::TearDown()
{
    // Copy back original file.
    if (!mMeshFullPath.empty()) {
        copyFile(::std::format("{}.bak", mMeshFullPath), mMeshFullPath);
    }
    if (!mSkeletonFullPath.empty()) {
        copyFile(::std::format("{}.bak", mSkeletonFullPath), mSkeletonFullPath);
    }
    if (mMesh) {
        mMesh->unload();
        mMesh.reset();
    }
    if (mOrigMesh) {
        mOrigMesh->unload();
        mOrigMesh.reset();
    }
    if (mSkeleton) {
        mSkeleton->unload();
        mSkeleton.reset();
    }    
    
    mMatMgr.reset();
    mArchiveMgr.reset();
    mArchiveFactory.reset();
    mSkelMgr.reset();
    mMeshMgr.reset();
    mHardMgr.reset();
    mLodMgr.reset();
    mResGroupMgr.reset();
    mFSLayer.reset();
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_clone)
{
    MeshPtr cloneMesh = mMesh->clone(::std::format("{}.clone.mesh", mMesh->getName()), mMesh->getGroup());
    assertMeshClone(mMesh.get(), cloneMesh.get());
}

//--------------------------------------------------------------------------
void MeshSerializerTests::testMesh(MeshVersion version)
{
    MeshSerializer serializer;
    serializer.exportMesh(mOrigMesh.get(), mMeshFullPath, version);
    mMesh->reload();
    assertMeshClone(mOrigMesh.get(), mMesh.get(), version);
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Skeleton_Version_1_8)
{
    if (mSkeleton) {
        SkeletonSerializer skeletonSerializer;
        skeletonSerializer.exportSkeleton(mSkeleton.get(), mSkeletonFullPath, SkeletonVersion::_1_8);
        mSkeleton->reload();
    }
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Skeleton_Version_1_0)
{
    if (mSkeleton) {
        SkeletonSerializer skeletonSerializer;
        skeletonSerializer.exportSkeleton(mSkeleton.get(), mSkeletonFullPath, SkeletonVersion::_1_0);
        mSkeleton->reload();
    }
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_Version_1_10)
{
    testMesh(MeshVersion::LATEST);
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_Version_1_8)
{
    testMesh(MeshVersion::_1_8);
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_Version_1_41)
{
    testMesh(MeshVersion::_1_7);
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_Version_1_4)
{
    testMesh(MeshVersion::_1_4);
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_Version_1_3)
{
    testMesh(MeshVersion::_1_0);
}

namespace Ogre
{

[[nodiscard]] static auto operator==(const VertexPoseKeyFrame::PoseRef& a, const VertexPoseKeyFrame::PoseRef& b) noexcept -> bool
{

    return a.poseIndex == b.poseIndex && a.influence == b.influence;
}
}

//--------------------------------------------------------------------------
void MeshSerializerTests::assertMeshClone(Mesh* a, Mesh* b, MeshVersion version /*= MeshVersion::LATEST*/)
{
    // TODO: Compare skeleton
    // TODO: Compare animations

    // EXPECT_TRUE(a->getGroup() == b->getGroup());
    // EXPECT_TRUE(a->getName() == b->getName());

    // XML serializer fails on these!
    EXPECT_TRUE(isEqual(a->getBoundingSphereRadius(), b->getBoundingSphereRadius()));
    EXPECT_TRUE(isEqual(a->getBounds().getMinimum(), b->getBounds().getMinimum()));
    EXPECT_TRUE(isEqual(a->getBounds().getMaximum(), b->getBounds().getMaximum()));

    // AutobuildEdgeLists is not saved to mesh file. You need to set it after loading a mesh!
    // EXPECT_TRUE(a->getAutoBuildEdgeLists() == b->getAutoBuildEdgeLists());
    EXPECT_TRUE(isHashMapClone(a->getSubMeshNameMap(), b->getSubMeshNameMap()));

    assertVertexDataClone(a->sharedVertexData, b->sharedVertexData);
    EXPECT_TRUE(a->getCreator() == b->getCreator());
    EXPECT_TRUE(a->getIndexBufferUsage() == b->getIndexBufferUsage());
    EXPECT_TRUE(a->getSharedVertexDataAnimationIncludesNormals() == b->getSharedVertexDataAnimationIncludesNormals());
    EXPECT_TRUE(a->getSharedVertexDataAnimationType() == b->getSharedVertexDataAnimationType());
    EXPECT_TRUE(a->getVertexBufferUsage() == b->getVertexBufferUsage());
    EXPECT_TRUE(a->hasVertexAnimation() == b->hasVertexAnimation());

    EXPECT_TRUE(a->isEdgeListBuilt() == b->isEdgeListBuilt()); // <== OgreXMLSerializer is doing post processing to generate edgelists!

    if ((a->getNumLodLevels() > 1 || b->getNumLodLevels() > 1) &&
        ((version < MeshVersion::_1_8 || (!isLodMixed(a) && !isLodMixed(b))) && // mixed lod only supported in v1.10+
         (version < MeshVersion::_1_4 || (a->getLodStrategy() == DistanceLodBoxStrategy::getSingletonPtr() &&
                                         b->getLodStrategy() == DistanceLodBoxStrategy::getSingletonPtr())))) { // Lod Strategy only supported in v1.41+
        EXPECT_TRUE(a->getNumLodLevels() == b->getNumLodLevels());
        EXPECT_TRUE(a->hasManualLodLevel() == b->hasManualLodLevel());
        EXPECT_TRUE(a->getLodStrategy() == b->getLodStrategy());

        int numLods = a->getNumLodLevels();
        for (int i = 0; i < numLods; i++) {
            if (version != MeshVersion::_1_0 && a->getAutoBuildEdgeLists() == b->getAutoBuildEdgeLists()) {
                assertEdgeDataClone(a->getEdgeList(i), b->getEdgeList(i));
            } else if (a->getLodLevel(i).edgeData != nullptr && b->getLodLevel(i).edgeData != nullptr) {
                assertEdgeDataClone(a->getLodLevel(i).edgeData, b->getLodLevel(i).edgeData);
            }
            assertLodUsageClone(a->getLodLevel(i), b->getLodLevel(i));
        }
    }

    EXPECT_TRUE(a->getNumSubMeshes() == b->getNumSubMeshes());
    int numLods = std::min(a->getNumLodLevels(), b->getNumLodLevels());
    size_t numSubmeshes = a->getNumSubMeshes();
    for (size_t i = 0; i < numSubmeshes; i++) {
        SubMesh* aSubmesh = a->getSubMesh(i);
        SubMesh* bSubmesh = b->getSubMesh(i);

        EXPECT_TRUE(aSubmesh->getMaterialName() == bSubmesh->getMaterialName());
        EXPECT_EQ(bool(aSubmesh->getMaterial()), bool(bSubmesh->getMaterial()));
        EXPECT_TRUE(aSubmesh->useSharedVertices == bSubmesh->useSharedVertices);
        EXPECT_TRUE(aSubmesh->getVertexAnimationIncludesNormals() == bSubmesh->getVertexAnimationIncludesNormals());
        EXPECT_TRUE(aSubmesh->getVertexAnimationType() == bSubmesh->getVertexAnimationType());
        EXPECT_TRUE(isContainerClone(aSubmesh->blendIndexToBoneIndexMap, bSubmesh->blendIndexToBoneIndexMap));
        // TODO: Compare getBoneAssignments and getTextureAliases
        for (int n = 0; n < numLods; n++) {
            if (a->_isManualLodLevel(n)) {
                continue;
            }
            RenderOperation aop, bop;
            aSubmesh->_getRenderOperation(aop, n);
            bSubmesh->_getRenderOperation(bop, n);
            assertIndexDataClone(aop.indexData, bop.indexData);
            assertVertexDataClone(aop.vertexData, bop.vertexData);
            EXPECT_TRUE(aop.operationType == bop.operationType);
            EXPECT_TRUE(aop.useIndexes == bop.useIndexes);
        }
    }

    // animations
    ASSERT_EQ(b->getNumAnimations(), a->getNumAnimations());

    // pose animations
    const PoseList& aPoseList = a->getPoseList();
    const PoseList& bPoseList = a->getPoseList();
    ASSERT_EQ(bPoseList.size(), aPoseList.size());
    for (size_t i = 0; i < aPoseList.size(); i++)
    {
        EXPECT_EQ(bPoseList[i]->getName(), aPoseList[i]->getName());
        EXPECT_EQ(bPoseList[i]->getTarget(), aPoseList[i]->getTarget());
        EXPECT_EQ(bPoseList[i]->getNormals(), aPoseList[i]->getNormals());
        EXPECT_EQ(bPoseList[i]->getVertexOffsets(), aPoseList[i]->getVertexOffsets());
    }

    for (int i = 0; i < a->getNumAnimations(); i++)
    {
        EXPECT_EQ(b->getAnimation(i)->getLength(), a->getAnimation(i)->getLength());
        ASSERT_EQ(b->getAnimation(i)->getNumVertexTracks(),
                  a->getAnimation(i)->getNumVertexTracks());

        const Animation::VertexTrackList& aList = a->getAnimation(i)->_getVertexTrackList();
        const Animation::VertexTrackList& bList = b->getAnimation(i)->_getVertexTrackList();

        for (auto const& [key, value] : aList)
        {
            EXPECT_EQ(bList.at(key)->getAnimationType(), value->getAnimationType());
            ASSERT_EQ(bList.at(key)->getNumKeyFrames(), value->getNumKeyFrames());

            for (size_t j = 0; j < value->getNumKeyFrames(); j++)
            {
                VertexPoseKeyFrame* aKeyFrame = value->getVertexPoseKeyFrame(j);
                VertexPoseKeyFrame* bKeyFrame = bList.at(key)->getVertexPoseKeyFrame(j);
                ASSERT_EQ(bKeyFrame->getPoseReferences(), aKeyFrame->getPoseReferences());
            }
        }
    }
}

//--------------------------------------------------------------------------
auto MeshSerializerTests::isLodMixed(const Mesh* pMesh) -> bool
{
    if (!pMesh->hasManualLodLevel()) {
        return false;
    }

    unsigned short numLods = pMesh->getNumLodLevels();
    for (unsigned short i = 1; i < numLods; ++i) {
        if (!pMesh->_isManualLodLevel(i)) {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------
void MeshSerializerTests::assertVertexDataClone(VertexData* a, VertexData* b, MeshVersion version /*= MeshVersion::LATEST*/)
{
    EXPECT_TRUE((a == nullptr) == (b == nullptr));
    if (a) {
        // compare bindings
        {
            const VertexBufferBinding::VertexBufferBindingMap& aBindings = a->vertexBufferBinding->getBindings();
            const VertexBufferBinding::VertexBufferBindingMap& bBindings = b->vertexBufferBinding->getBindings();
            EXPECT_TRUE(aBindings.size() == bBindings.size());

            for (auto bIt = bBindings.begin();
                auto const& aIt : aBindings) {
                EXPECT_TRUE(aIt.first == bIt->first);
                EXPECT_TRUE((aIt.second.get() == nullptr) == (bIt->second.get() == nullptr));
                if (a) {
                    EXPECT_TRUE(aIt.second->getManager() == bIt->second->getManager());
                    EXPECT_TRUE(aIt.second->getNumVertices() == bIt->second->getNumVertices());
                }
                bIt++;
            }
        }

        {
            const VertexDeclaration::VertexElementList& aElements = a->vertexDeclaration->getElements();
            const VertexDeclaration::VertexElementList& bElements = a->vertexDeclaration->getElements();
            EXPECT_TRUE(aElements.size() == bElements.size());

            for (auto const& aIt : aElements) {
                auto bIt = std::ranges::find(bElements, aIt);
                EXPECT_TRUE(bIt != bElements.end());

                const VertexElement& aElem = aIt;
                const VertexElement& bElem = *bIt;
                HardwareVertexBufferSharedPtr abuf = a->vertexBufferBinding->getBuffer(aElem.getSource());
                HardwareVertexBufferSharedPtr bbuf = b->vertexBufferBinding->getBuffer(bElem.getSource());
                auto* avertex = static_cast<unsigned char*>(abuf->lock(HardwareBuffer::LockOptions::READ_ONLY));
                auto* bvertex = static_cast<unsigned char*>(bbuf->lock(HardwareBuffer::LockOptions::READ_ONLY));
                size_t avSize = abuf->getVertexSize();
                size_t bvSize = bbuf->getVertexSize();
                size_t elemSize = VertexElement::getTypeSize(aElem.getType());
                unsigned char* avEnd = avertex + a->vertexCount * avSize;
                bool error = false;
                for (; avertex < avEnd; avertex += avSize, bvertex += bvSize) {
                    float* afloat, * bfloat;
                    aElem.baseVertexPointerToElement(avertex, &afloat);
                    bElem.baseVertexPointerToElement(bvertex, &bfloat);
                    error |= (memcmp(afloat, bfloat, elemSize) != 0);
                }
                abuf->unlock();
                bbuf->unlock();
                EXPECT_TRUE(!error && "Content of vertex buffer differs!");
            }
        }

        EXPECT_TRUE(a->vertexStart == b->vertexStart);
        EXPECT_TRUE(a->vertexCount == b->vertexCount);
        EXPECT_TRUE(a->hwAnimDataItemsUsed == b->hwAnimDataItemsUsed);

        // Compare hwAnimationData
        {
            const VertexData::HardwareAnimationDataList& aAnimData = a->hwAnimationDataList;
            const VertexData::HardwareAnimationDataList& bAnimData = b->hwAnimationDataList;
            EXPECT_TRUE(aAnimData.size() == bAnimData.size());
            for (auto bIt = bAnimData.begin();
                 auto const& aIt : aAnimData) {
                EXPECT_TRUE(aIt.parametric == bIt->parametric);
                EXPECT_TRUE(aIt.targetBufferIndex == bIt->targetBufferIndex);
                bIt++;
            }
        }
    }
}

//--------------------------------------------------------------------------
template<typename T>

auto MeshSerializerTests::isContainerClone(T& a, T& b) -> bool
{
    return a.size() == b.size() && std::ranges::equal(a, b);
}

//--------------------------------------------------------------------------
template<typename K, typename V, typename H, typename E>

auto MeshSerializerTests::isHashMapClone(const std::unordered_map<K, V, H, E>& a, const std::unordered_map<K, V, H, E>& b) -> bool
{
    // if you recreate a HashMap with same elements, then iteration order may differ!
    // So isContainerClone is not always working on HashMap.
    if (a.size() != b.size()) {
        return false;
    }
    for (auto const& [key, value] : a) {
        auto itFind = b.find(key);
        if (itFind == b.end() || itFind->second != value) {
            return false;
        }
    }
    return true;
}

//--------------------------------------------------------------------------
void MeshSerializerTests::assertIndexDataClone(IndexData* a, IndexData* b, MeshVersion version /*= MeshVersion::LATEST*/)
{
    EXPECT_TRUE((a == nullptr) == (b == nullptr));
    if (a) {
        EXPECT_TRUE(a->indexCount == b->indexCount);
        // EXPECT_TRUE(a->indexStart == b->indexStart);
        EXPECT_TRUE((a->indexBuffer.get() == nullptr) == (b->indexBuffer.get() == nullptr));
        if (a->indexBuffer) {
            EXPECT_TRUE(a->indexBuffer->getManager() == b->indexBuffer->getManager());
            // EXPECT_TRUE(a->indexBuffer->getNumIndexes() == b->indexBuffer->getNumIndexes());
            EXPECT_TRUE(a->indexBuffer->getIndexSize() == b->indexBuffer->getIndexSize());
            EXPECT_TRUE(a->indexBuffer->getType() == b->indexBuffer->getType());

            char* abuf = (char*) a->indexBuffer->lock(HardwareBuffer::LockOptions::READ_ONLY);
            char* bbuf = (char*) b->indexBuffer->lock(HardwareBuffer::LockOptions::READ_ONLY);
            size_t size = a->indexBuffer->getIndexSize();
            char* astart = abuf + a->indexStart * size;
            char* bstart = bbuf + b->indexStart * size;
            EXPECT_TRUE(memcmp(astart, bstart, a->indexCount * size) == 0);
            a->indexBuffer->unlock();
            b->indexBuffer->unlock();
        }
    }
}

//--------------------------------------------------------------------------
void MeshSerializerTests::assertEdgeDataClone(EdgeData* a, EdgeData* b, MeshVersion version /*= MeshVersion::LATEST*/)
{
    EXPECT_TRUE((a == nullptr) == (b == nullptr));
    if (a) {
        EXPECT_TRUE(a->isClosed == b->isClosed);
        EXPECT_TRUE(isContainerClone(a->triangleFaceNormals, b->triangleFaceNormals));
        EXPECT_TRUE(isContainerClone(a->triangleLightFacings, b->triangleLightFacings));
        // TODO: Compare triangles and edgeGroups in more detail.
        EXPECT_TRUE(a->triangles.size() == b->triangles.size());
        EXPECT_TRUE(a->edgeGroups.size() == b->edgeGroups.size());
    }
}

//--------------------------------------------------------------------------
void MeshSerializerTests::assertLodUsageClone(const MeshLodUsage& a, const MeshLodUsage& b, MeshVersion version /*= MeshVersion::LATEST*/)
{
    EXPECT_TRUE(a.manualName == b.manualName);
    EXPECT_TRUE(isEqual(a.userValue, b.userValue));
    EXPECT_TRUE(isEqual(a.value, b.value));
}

//--------------------------------------------------------------------------
void MeshSerializerTests::getResourceFullPath(const ResourcePtr& resource, String& outPath)
{
    ResourceGroupManager& resourceGroupMgr = ResourceGroupManager::getSingleton();
    auto const group = resource->getGroup();
    auto const name = resource->getName();
    FileInfo const* info = nullptr;
    FileInfoListPtr locPtr = resourceGroupMgr.listResourceFileInfo(group);
    for (auto const& it : *locPtr) {
        if (StringUtil::startsWith(name, it.filename)) {
            info = &it;
            break;
        }
    }
    if(!info) {
        outPath = name;
        return;
    }
    outPath = info->archive->getName();
    if (outPath[outPath .size()-1] != '/' && outPath[outPath .size()-1] != '\\') {
        outPath += '/';
    }
    outPath += info->path;
    if (outPath[outPath .size()-1] != '/' && outPath[outPath .size()-1] != '\\') {
        outPath += '/';
    }
    outPath += info->filename;

    OgreAssert(info->archive->getType() == "FileSystem", "");
}

//--------------------------------------------------------------------------
auto MeshSerializerTests::copyFile(std::string_view srcPath, std::string_view dstPath) -> bool
{
    std::ifstream src(std::filesystem::path{srcPath}, std::ios::binary);
    if (!src.is_open()) {
        return false;
    }
    std::ofstream dst(std::filesystem::path{dstPath}, std::ios::binary);
    if (!dst.is_open()) {
        return false;
    }

    dst << src.rdbuf();
    return true;
}

//--------------------------------------------------------------------------
auto MeshSerializerTests::isEqual(Real a, Real b) -> bool
{
    Real absoluteError = std::abs(a * mErrorFactor);
    return ((a - absoluteError) <= b) && ((a + absoluteError) >= b);
}

//--------------------------------------------------------------------------
auto MeshSerializerTests::isEqual(const Vector3& a, const Vector3& b) -> bool
{
    return isEqual(a.x, b.x) && isEqual(a.y, b.y) && isEqual(a.z, b.z);
}
//--------------------------------------------------------------------------
