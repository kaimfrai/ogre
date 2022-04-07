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
module Ogre.Tests.Core:MeshSerializerTests.Obj;

import :MeshSerializerTests;

import Ogre.Core;

import <algorithm>;
import <cmath>;
import <cstdlib>;
import <cstring>;
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

    mFSLayer = new Ogre::FileSystemLayer(/*OGRE_VERSION_NAME*/"Tsathoggua");

    new ResourceGroupManager();
    new LodStrategyManager();
    new DefaultHardwareBufferManager();
    new MeshManager();
    new SkeletonManager();
    auto* archiveMgr = new ArchiveManager();
    archiveMgr->addArchiveFactory(new FileSystemArchiveFactory());

    auto* matMgr = new MaterialManager();
    matMgr->initialise();

    // Load resource paths from config file
    ConfigFile cf;
    String resourcesPath = mFSLayer->getConfigFilePath("resources.cfg");

    // Go through all sections & settings in the file
    cf.load(resourcesPath);
    String secName, typeName, archName;

    ConfigFile::SettingsBySection_::const_iterator seci;
    for(seci = cf.getSettingsBySection().begin(); seci != cf.getSettingsBySection().end(); ++seci) {
        secName = seci->first;
        const ConfigFile::SettingsMultiMap& settings = seci->second;
        ConfigFile::SettingsMultiMap::const_iterator i;
        for (i = settings.begin(); i != settings.end(); ++i) {
            typeName = i->first;
            archName = i->second;
            if (typeName == "FileSystem") {
                ResourceGroupManager::getSingleton().addResourceLocation(
                    archName, typeName, secName);
            }
        }
    }

    mMesh = MeshManager::getSingleton().load("facial.mesh", "General");

    getResourceFullPath(mMesh, mMeshFullPath);
    if (!copyFile(mMeshFullPath + ".bak", mMeshFullPath)) {
        // If there is no backup, create one.
        copyFile(mMeshFullPath, mMeshFullPath + ".bak");
    }
    mSkeletonFullPath = "";
    mSkeleton = static_pointer_cast<Skeleton>(SkeletonManager::getSingleton().load("jaiqua.skeleton", "General"));
    getResourceFullPath(mSkeleton, mSkeletonFullPath);
    if (!copyFile(mSkeletonFullPath + ".bak", mSkeletonFullPath)) {
        // If there is no backup, create one.
        copyFile(mSkeletonFullPath, mSkeletonFullPath + ".bak");
    }

    mMesh->reload();

    mOrigMesh = mMesh->clone(mMesh->getName() + ".orig.mesh", mMesh->getGroup());
}

//--------------------------------------------------------------------------
void MeshSerializerTests::TearDown()
{
    // Copy back original file.
    if (!mMeshFullPath.empty()) {
        copyFile(mMeshFullPath + ".bak", mMeshFullPath);
    }
    if (!mSkeletonFullPath.empty()) {
        copyFile(mSkeletonFullPath + ".bak", mSkeletonFullPath);
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
    
    delete MeshManager::getSingletonPtr();
    delete SkeletonManager::getSingletonPtr();
    delete DefaultHardwareBufferManager::getSingletonPtr();
    delete ArchiveManager::getSingletonPtr();    
    delete MaterialManager::getSingletonPtr();
    delete LodStrategyManager::getSingletonPtr();
    delete ResourceGroupManager::getSingletonPtr();
    delete mFSLayer;
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_clone)
{
    MeshPtr cloneMesh = mMesh->clone(mMesh->getName() + ".clone.mesh", mMesh->getGroup());
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
        skeletonSerializer.exportSkeleton(mSkeleton.get(), mSkeletonFullPath, SKELETON_VERSION_1_8);
        mSkeleton->reload();
    }
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Skeleton_Version_1_0)
{
    if (mSkeleton) {
        SkeletonSerializer skeletonSerializer;
        skeletonSerializer.exportSkeleton(mSkeleton.get(), mSkeletonFullPath, SKELETON_VERSION_1_0);
        mSkeleton->reload();
    }
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_Version_1_10)
{
    testMesh(MESH_VERSION_LATEST);
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_Version_1_8)
{
    testMesh(MESH_VERSION_1_8);
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_Version_1_41)
{
    testMesh(MESH_VERSION_1_7);
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_Version_1_4)
{
    testMesh(MESH_VERSION_1_4);
}
//--------------------------------------------------------------------------
TEST_F(MeshSerializerTests,Mesh_Version_1_3)
{
    testMesh(MESH_VERSION_1_0);
}

namespace Ogre
{
static auto operator==(const VertexPoseKeyFrame::PoseRef& a, const VertexPoseKeyFrame::PoseRef& b) -> bool
{

    return a.poseIndex == b.poseIndex && a.influence == b.influence;
}
}

//--------------------------------------------------------------------------
void MeshSerializerTests::assertMeshClone(Mesh* a, Mesh* b, MeshVersion version /*= MESH_VERSION_LATEST*/)
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
        ((version < MESH_VERSION_1_8 || (!isLodMixed(a) && !isLodMixed(b))) && // mixed lod only supported in v1.10+
         (version < MESH_VERSION_1_4 || (a->getLodStrategy() == DistanceLodBoxStrategy::getSingletonPtr() &&
                                         b->getLodStrategy() == DistanceLodBoxStrategy::getSingletonPtr())))) { // Lod Strategy only supported in v1.41+
        EXPECT_TRUE(a->getNumLodLevels() == b->getNumLodLevels());
        EXPECT_TRUE(a->hasManualLodLevel() == b->hasManualLodLevel());
        EXPECT_TRUE(a->getLodStrategy() == b->getLodStrategy());

        int numLods = a->getNumLodLevels();
        for (int i = 0; i < numLods; i++) {
            if (version != MESH_VERSION_1_0 && a->getAutoBuildEdgeLists() == b->getAutoBuildEdgeLists()) {
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

        auto it = aList.begin();
        for (; it != aList.end(); ++it)
        {
            EXPECT_EQ(bList.at(it->first)->getAnimationType(), it->second->getAnimationType());
            ASSERT_EQ(bList.at(it->first)->getNumKeyFrames(), it->second->getNumKeyFrames());

            for (size_t j = 0; j < it->second->getNumKeyFrames(); j++)
            {
                VertexPoseKeyFrame* aKeyFrame = it->second->getVertexPoseKeyFrame(j);
                VertexPoseKeyFrame* bKeyFrame = bList.at(it->first)->getVertexPoseKeyFrame(j);
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
void MeshSerializerTests::assertVertexDataClone(VertexData* a, VertexData* b, MeshVersion version /*= MESH_VERSION_LATEST*/)
{
    EXPECT_TRUE((a == nullptr) == (b == nullptr));
    if (a) {
        // compare bindings
        {
            const VertexBufferBinding::VertexBufferBindingMap& aBindings = a->vertexBufferBinding->getBindings();
            const VertexBufferBinding::VertexBufferBindingMap& bBindings = b->vertexBufferBinding->getBindings();
            EXPECT_TRUE(aBindings.size() == bBindings.size());
            auto aIt = aBindings.begin();
            auto aEndIt = aBindings.end();
            auto bIt = bBindings.begin();
            for (; aIt != aEndIt; aIt++, bIt++) {
                EXPECT_TRUE(aIt->first == bIt->first);
                EXPECT_TRUE((aIt->second.get() == nullptr) == (bIt->second.get() == nullptr));
                if (a) {
                    EXPECT_TRUE(aIt->second->getManager() == bIt->second->getManager());
                    EXPECT_TRUE(aIt->second->getNumVertices() == bIt->second->getNumVertices());
                }
            }
        }

        {
            const VertexDeclaration::VertexElementList& aElements = a->vertexDeclaration->getElements();
            const VertexDeclaration::VertexElementList& bElements = a->vertexDeclaration->getElements();
            EXPECT_TRUE(aElements.size() == bElements.size());
            using bindingIterator = VertexDeclaration::VertexElementList::const_iterator;
            auto aIt = aElements.begin();
            auto aEndIt = aElements.end();
            bindingIterator bIt;
            for (; aIt != aEndIt; aIt++) {
                bIt = std::find(bElements.begin(), bElements.end(), *aIt);
                EXPECT_TRUE(bIt != bElements.end());

                const VertexElement& aElem = *aIt;
                const VertexElement& bElem = *bIt;
                HardwareVertexBufferSharedPtr abuf = a->vertexBufferBinding->getBuffer(aElem.getSource());
                HardwareVertexBufferSharedPtr bbuf = b->vertexBufferBinding->getBuffer(bElem.getSource());
                auto* avertex = static_cast<unsigned char*>(abuf->lock(HardwareBuffer::HBL_READ_ONLY));
                auto* bvertex = static_cast<unsigned char*>(bbuf->lock(HardwareBuffer::HBL_READ_ONLY));
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
            auto aIt = aAnimData.begin();
            auto aEndIt = aAnimData.end();
            auto bIt = bAnimData.begin();
            for (; aIt != aEndIt; aIt++, bIt++) {
                EXPECT_TRUE(aIt->parametric == bIt->parametric);
                EXPECT_TRUE(aIt->targetBufferIndex == bIt->targetBufferIndex);
            }
        }
    }
}

//--------------------------------------------------------------------------
template<typename T>

auto MeshSerializerTests::isContainerClone(T& a, T& b) -> bool
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

//--------------------------------------------------------------------------
template<typename K, typename V>

auto MeshSerializerTests::isHashMapClone(const std::unordered_map<K, V>& a, const std::unordered_map<K, V>& b) -> bool
{
    // if you recreate a HashMap with same elements, then iteration order may differ!
    // So isContainerClone is not always working on HashMap.
    if (a.size() != b.size()) {
        return false;
    }
    typename std::unordered_map<K, V>::const_iterator it, itFind, itEnd;
    it = a.begin();
    itEnd = a.end();
    for (; it != itEnd; it++) {
        itFind = b.find(it->first);
        if (itFind == b.end() || itFind->second != it->second) {
            return false;
        }
    }
    return true;
}

//--------------------------------------------------------------------------
void MeshSerializerTests::assertIndexDataClone(IndexData* a, IndexData* b, MeshVersion version /*= MESH_VERSION_LATEST*/)
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

            char* abuf = (char*) a->indexBuffer->lock(HardwareBuffer::HBL_READ_ONLY);
            char* bbuf = (char*) b->indexBuffer->lock(HardwareBuffer::HBL_READ_ONLY);
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
void MeshSerializerTests::assertEdgeDataClone(EdgeData* a, EdgeData* b, MeshVersion version /*= MESH_VERSION_LATEST*/)
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
void MeshSerializerTests::assertLodUsageClone(const MeshLodUsage& a, const MeshLodUsage& b, MeshVersion version /*= MESH_VERSION_LATEST*/)
{
    EXPECT_TRUE(a.manualName == b.manualName);
    EXPECT_TRUE(isEqual(a.userValue, b.userValue));
    EXPECT_TRUE(isEqual(a.value, b.value));
}

//--------------------------------------------------------------------------
void MeshSerializerTests::getResourceFullPath(const ResourcePtr& resource, String& outPath)
{
    ResourceGroupManager& resourceGroupMgr = ResourceGroupManager::getSingleton();
    String group = resource->getGroup();
    String name = resource->getName();
    FileInfo* info = nullptr;
    FileInfoListPtr locPtr = resourceGroupMgr.listResourceFileInfo(group);
    FileInfoList::iterator it, itEnd;
    it = locPtr->begin();
    itEnd = locPtr->end();
    for (; it != itEnd; it++) {
        if (StringUtil::startsWith(name, it->filename)) {
            info = &*it;
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
auto MeshSerializerTests::copyFile(const String& srcPath, const String& dstPath) -> bool
{
    std::ifstream src(srcPath.c_str(), std::ios::binary);
    if (!src.is_open()) {
        return false;
    }
    std::ofstream dst(dstPath.c_str(), std::ios::binary);
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
