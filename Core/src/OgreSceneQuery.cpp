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
        : mParentSceneMgr(mgr) 
        
    {
        // default type mask to everything except lights & fx (previous behaviour)
        mQueryTypeMask = (QueryTypeMask{0xFFFFFFFF} & ~QueryTypeMask::FX)
            & ~QueryTypeMask::LIGHT;

    }
    //-----------------------------------------------------------------------
    SceneQuery::~SceneQuery()
    = default;
    //-----------------------------------------------------------------------
    void SceneQuery::setQueryMask(QueryTypeMask mask)
    {
        mQueryMask = mask;
    }
    //-----------------------------------------------------------------------
    auto SceneQuery::getQueryMask() const noexcept -> QueryTypeMask
    {
        return mQueryMask;
    }
    //-----------------------------------------------------------------------
    void SceneQuery::setQueryTypeMask(QueryTypeMask mask)
    {
        mQueryTypeMask = mask;
    }
    //-----------------------------------------------------------------------
    auto SceneQuery::getQueryTypeMask() const noexcept -> QueryTypeMask
    {
        return mQueryTypeMask;
    }
    //-----------------------------------------------------------------------
    void SceneQuery::setWorldFragmentType(enum SceneQuery::WorldFragmentType wft)
    {
        // Check supported
        if (mSupportedWorldFragments.find(wft) == mSupportedWorldFragments.end())
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "This world fragment type is not supported.",
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
        :SceneQuery(mgr) 
    {
    }
    //-----------------------------------------------------------------------
    RegionSceneQuery::~RegionSceneQuery()
    {
        clearResults();
    }
    //-----------------------------------------------------------------------
    auto RegionSceneQuery::getLastResults() const noexcept -> SceneQueryResult&
    {
        assert(mLastResult);
        return *mLastResult;
    }
    //-----------------------------------------------------------------------
    void RegionSceneQuery::clearResults()
    {
        delete mLastResult;
        mLastResult = nullptr;
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
    = default;
    //-----------------------------------------------------------------------
    void AxisAlignedBoxSceneQuery::setBox(const AxisAlignedBox& box)
    {
        mAABB = box;
    }
    //-----------------------------------------------------------------------
    auto AxisAlignedBoxSceneQuery::getBox() const noexcept -> const AxisAlignedBox&
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
    = default;
    //-----------------------------------------------------------------------
    void SphereSceneQuery::setSphere(const Sphere& sphere)
    {
        mSphere = sphere;
    }
    //-----------------------------------------------------------------------
    auto SphereSceneQuery::getSphere() const noexcept -> const Sphere&
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
    = default;
    //-----------------------------------------------------------------------
    void PlaneBoundedVolumeListSceneQuery::setVolumes(const PlaneBoundedVolumeList& volumes)
    {
        mVolumes = volumes;
    }
    //-----------------------------------------------------------------------
    auto PlaneBoundedVolumeListSceneQuery::getVolumes() const noexcept -> const PlaneBoundedVolumeList&
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
    = default;
    //-----------------------------------------------------------------------
    void RaySceneQuery::setRay(const Ray& ray)
    {
        mRay = ray;
    }
    //-----------------------------------------------------------------------
    auto RaySceneQuery::getRay() const noexcept -> const Ray&
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
    auto RaySceneQuery::getSortByDistance() const noexcept -> bool
    {
        return mSortByDistance;
    }
    //-----------------------------------------------------------------------
    auto RaySceneQuery::getMaxResults() const noexcept -> ushort
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
                std::ranges::sort(mResult);
            }
        }

        return mResult;
    }
    //-----------------------------------------------------------------------
    auto RaySceneQuery::getLastResults() noexcept -> RaySceneQueryResult&
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
        dets.worldFragment = nullptr;
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
        dets.movable = nullptr;
        dets.worldFragment = fragment;
        mResult.push_back(dets);
        // Continue
        return true;
    }
    //-----------------------------------------------------------------------
    IntersectionSceneQuery::IntersectionSceneQuery(SceneManager* mgr)
    : SceneQuery(mgr) 
    {
    }
    //-----------------------------------------------------------------------
    IntersectionSceneQuery::~IntersectionSceneQuery()
    {
        clearResults();
    }
    //-----------------------------------------------------------------------
    auto IntersectionSceneQuery::getLastResults() const noexcept -> IntersectionSceneQueryResult&
    {
        assert(mLastResult);
        return *mLastResult;
    }
    //-----------------------------------------------------------------------
    void IntersectionSceneQuery::clearResults()
    {
        delete mLastResult;
        mLastResult = nullptr;
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
    



