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
#include <algorithm>
#include <cassert>
#include <cstddef>

#include "OgreException.hpp"
#include "OgreSceneManager.hpp"
#include "OgreSceneQuery.hpp"

namespace Ogre {
class MovableObject;

    //-----------------------------------------------------------------------
    SceneQuery::SceneQuery(SceneManager* mgr)
        : mParentSceneMgr(mgr), mQueryMask(0xFFFFFFFF), 
        mWorldFragmentType(SceneQuery::WFT_NONE)
    {
        // default type mask to everything except lights & fx (previous behaviour)
        mQueryTypeMask = (0xFFFFFFFF & ~SceneManager::FX_TYPE_MASK) 
            & ~SceneManager::LIGHT_TYPE_MASK;

    }
    //-----------------------------------------------------------------------
    SceneQuery::~SceneQuery()
    {
    }
    //-----------------------------------------------------------------------
    void SceneQuery::setQueryMask(uint32 mask)
    {
        mQueryMask = mask;
    }
    //-----------------------------------------------------------------------
    auto SceneQuery::getQueryMask() const -> uint32
    {
        return mQueryMask;
    }
    //-----------------------------------------------------------------------
    void SceneQuery::setQueryTypeMask(uint32 mask)
    {
        mQueryTypeMask = mask;
    }
    //-----------------------------------------------------------------------
    auto SceneQuery::getQueryTypeMask() const -> uint32
    {
        return mQueryTypeMask;
    }
    //-----------------------------------------------------------------------
    void SceneQuery::setWorldFragmentType(enum SceneQuery::WorldFragmentType wft)
    {
        // Check supported
        if (mSupportedWorldFragments.find(wft) == mSupportedWorldFragments.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "This world fragment type is not supported.",
                "SceneQuery::setWorldFragmentType");
        }
        mWorldFragmentType = wft;
    }
    //-----------------------------------------------------------------------
    auto 
    SceneQuery::getWorldFragmentType() const -> SceneQuery::WorldFragmentType
    {
        return mWorldFragmentType;
    }
    //-----------------------------------------------------------------------
    RegionSceneQuery::RegionSceneQuery(SceneManager* mgr)
        :SceneQuery(mgr), mLastResult(NULL)
    {
    }
    //-----------------------------------------------------------------------
    RegionSceneQuery::~RegionSceneQuery()
    {
        clearResults();
    }
    //-----------------------------------------------------------------------
    auto RegionSceneQuery::getLastResults() const -> SceneQueryResult&
    {
        assert(mLastResult);
        return *mLastResult;
    }
    //-----------------------------------------------------------------------
    void RegionSceneQuery::clearResults()
    {
        delete mLastResult;
        mLastResult = NULL;
    }
    //---------------------------------------------------------------------
    auto
    RegionSceneQuery::execute() -> SceneQueryResult&
    {
        clearResults();
        mLastResult = new SceneQueryResult();
        // Call callback version with self as listener
        execute(this);
        return *mLastResult;
    }
    //---------------------------------------------------------------------
    auto RegionSceneQuery::
        queryResult(MovableObject* obj) -> bool
    {
        // Add to internal list
        mLastResult->movables.push_back(obj);
        // Continue
        return true;
    }
    //---------------------------------------------------------------------
    auto RegionSceneQuery::queryResult(SceneQuery::WorldFragment* fragment) -> bool
    {
        // Add to internal list
        mLastResult->worldFragments.push_back(fragment);
        // Continue
        return true;
    }
    //-----------------------------------------------------------------------
    AxisAlignedBoxSceneQuery::AxisAlignedBoxSceneQuery(SceneManager* mgr)
        : RegionSceneQuery(mgr)
    {
    }
    //-----------------------------------------------------------------------
    AxisAlignedBoxSceneQuery::~AxisAlignedBoxSceneQuery()
    {
    }
    //-----------------------------------------------------------------------
    void AxisAlignedBoxSceneQuery::setBox(const AxisAlignedBox& box)
    {
        mAABB = box;
    }
    //-----------------------------------------------------------------------
    auto AxisAlignedBoxSceneQuery::getBox() const -> const AxisAlignedBox&
    {
        return mAABB;
    }
    //-----------------------------------------------------------------------
    SphereSceneQuery::SphereSceneQuery(SceneManager* mgr)
        : RegionSceneQuery(mgr)
    {
    }
    //-----------------------------------------------------------------------
    SphereSceneQuery::~SphereSceneQuery()
    {
    }
    //-----------------------------------------------------------------------
    void SphereSceneQuery::setSphere(const Sphere& sphere)
    {
        mSphere = sphere;
    }
    //-----------------------------------------------------------------------
    auto SphereSceneQuery::getSphere() const -> const Sphere&
    {
        return mSphere;
    }

    //-----------------------------------------------------------------------
    PlaneBoundedVolumeListSceneQuery::PlaneBoundedVolumeListSceneQuery(SceneManager* mgr)
        : RegionSceneQuery(mgr)
    {
    }
    //-----------------------------------------------------------------------
    PlaneBoundedVolumeListSceneQuery::~PlaneBoundedVolumeListSceneQuery()
    {
    }
    //-----------------------------------------------------------------------
    void PlaneBoundedVolumeListSceneQuery::setVolumes(const PlaneBoundedVolumeList& volumes)
    {
        mVolumes = volumes;
    }
    //-----------------------------------------------------------------------
    auto PlaneBoundedVolumeListSceneQuery::getVolumes() const -> const PlaneBoundedVolumeList&
    {
        return mVolumes;
    }

    //-----------------------------------------------------------------------
    RaySceneQuery::RaySceneQuery(SceneManager* mgr) : SceneQuery(mgr)
    {
        mSortByDistance = false;
        mMaxResults = 0;
    }
    //-----------------------------------------------------------------------
    RaySceneQuery::~RaySceneQuery()
    {
    }
    //-----------------------------------------------------------------------
    void RaySceneQuery::setRay(const Ray& ray)
    {
        mRay = ray;
    }
    //-----------------------------------------------------------------------
    auto RaySceneQuery::getRay() const -> const Ray&
    {
        return mRay;
    }
    //-----------------------------------------------------------------------
    void RaySceneQuery::setSortByDistance(bool sort, ushort maxresults)
    {
        mSortByDistance = sort;
        mMaxResults = maxresults;
    }
    //-----------------------------------------------------------------------
    auto RaySceneQuery::getSortByDistance() const -> bool
    {
        return mSortByDistance;
    }
    //-----------------------------------------------------------------------
    auto RaySceneQuery::getMaxResults() const -> ushort
    {
        return mMaxResults;
    }
    //-----------------------------------------------------------------------
    auto RaySceneQuery::execute() -> RaySceneQueryResult&
    {
        // Clear without freeing the vector buffer
        mResult.clear();
        
        // Call callback version with self as listener
        this->execute(this);

        if (mSortByDistance)
        {
            if (mMaxResults != 0 && mMaxResults < mResult.size())
            {
                // Partially sort the N smallest elements, discard others
                std::partial_sort(mResult.begin(), mResult.begin()+mMaxResults, mResult.end());
                mResult.resize(mMaxResults);
            }
            else
            {
                // Sort entire result array
                std::sort(mResult.begin(), mResult.end());
            }
        }

        return mResult;
    }
    //-----------------------------------------------------------------------
    auto RaySceneQuery::getLastResults() -> RaySceneQueryResult&
    {
        return mResult;
    }
    //-----------------------------------------------------------------------
    void RaySceneQuery::clearResults()
    {
        // C++ idiom to free vector buffer: swap with empty vector
        RaySceneQueryResult().swap(mResult);
    }
    //-----------------------------------------------------------------------
    auto RaySceneQuery::queryResult(MovableObject* obj, Real distance) -> bool
    {
        // Add to internal list
        RaySceneQueryResultEntry dets;
        dets.distance = distance;
        dets.movable = obj;
        dets.worldFragment = NULL;
        mResult.push_back(dets);
        // Continue
        return true;
    }
    //-----------------------------------------------------------------------
    auto RaySceneQuery::queryResult(SceneQuery::WorldFragment* fragment, Real distance) -> bool
    {
        // Add to internal list
        RaySceneQueryResultEntry dets;
        dets.distance = distance;
        dets.movable = NULL;
        dets.worldFragment = fragment;
        mResult.push_back(dets);
        // Continue
        return true;
    }
    //-----------------------------------------------------------------------
    IntersectionSceneQuery::IntersectionSceneQuery(SceneManager* mgr)
    : SceneQuery(mgr), mLastResult(NULL)
    {
    }
    //-----------------------------------------------------------------------
    IntersectionSceneQuery::~IntersectionSceneQuery()
    {
        clearResults();
    }
    //-----------------------------------------------------------------------
    auto IntersectionSceneQuery::getLastResults() const -> IntersectionSceneQueryResult&
    {
        assert(mLastResult);
        return *mLastResult;
    }
    //-----------------------------------------------------------------------
    void IntersectionSceneQuery::clearResults()
    {
        delete mLastResult;
        mLastResult = NULL;
    }
    //---------------------------------------------------------------------
    auto
    IntersectionSceneQuery::execute() -> IntersectionSceneQueryResult&
    {
        clearResults();
        mLastResult = new IntersectionSceneQueryResult();
        // Call callback version with self as listener
        execute(this);
        return *mLastResult;
    }
    //---------------------------------------------------------------------
    auto IntersectionSceneQuery::
        queryResult(MovableObject* first, MovableObject* second) -> bool
    {
        // Add to internal list
        mLastResult->movables2movables.push_back(
            SceneQueryMovableObjectPair(first, second)
            );
        // Continue
        return true;
    }
    //---------------------------------------------------------------------
    auto IntersectionSceneQuery::
        queryResult(MovableObject* movable, SceneQuery::WorldFragment* fragment) -> bool
    {
        // Add to internal list
        mLastResult->movables2world.push_back(
            SceneQueryMovableObjectWorldFragmentPair(movable, fragment)
            );
        // Continue
        return true;
    }




}
    



