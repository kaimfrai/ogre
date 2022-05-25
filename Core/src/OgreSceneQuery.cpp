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
        mQueryTypeMask = (0xFFFFFFFF & ~SceneManager::FX_TYPE_MASK) 
            & ~SceneManager::LIGHT_TYPE_MASK;

    }
    //-----------------------------------------------------------------------
    SceneQuery::~SceneQuery()
    = default;
    //-----------------------------------------------------------------------
    void SceneQuery::setQueryMask(uint32 mask)
    {
        mQueryMask = mask;
    }
    //-----------------------------------------------------------------------
    uint32 SceneQuery::getQueryMask() const noexcept
    {
        return mQueryMask;
    }
    //-----------------------------------------------------------------------
    void SceneQuery::setQueryTypeMask(uint32 mask)
    {
        mQueryTypeMask = mask;
    }
    //-----------------------------------------------------------------------
    uint32 SceneQuery::getQueryTypeMask() const noexcept
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
    SceneQuery::WorldFragmentType 
    SceneQuery::getWorldFragmentType() const
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
    SceneQueryResult& RegionSceneQuery::getLastResults() const noexcept
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
    SceneQueryResult&
    RegionSceneQuery::execute()
    {
        clearResults();
        mLastResult = new SceneQueryResult();
        // Call callback version with self as listener
        execute(this);
        return *mLastResult;
    }
    //---------------------------------------------------------------------
    bool RegionSceneQuery::
        queryResult(MovableObject* obj)
    {
        // Add to internal list
        mLastResult->movables.push_back(obj);
        // Continue
        return true;
    }
    //---------------------------------------------------------------------
    bool RegionSceneQuery::queryResult(SceneQuery::WorldFragment* fragment)
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
    const AxisAlignedBox& AxisAlignedBoxSceneQuery::getBox() const noexcept
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
    const Sphere& SphereSceneQuery::getSphere() const noexcept
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
    const PlaneBoundedVolumeList& PlaneBoundedVolumeListSceneQuery::getVolumes() const noexcept
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
    const Ray& RaySceneQuery::getRay() const noexcept
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
    bool RaySceneQuery::getSortByDistance() const noexcept
    {
        return mSortByDistance;
    }
    //-----------------------------------------------------------------------
    ushort RaySceneQuery::getMaxResults() const noexcept
    {
        return mMaxResults;
    }
    //-----------------------------------------------------------------------
    RaySceneQueryResult& RaySceneQuery::execute()
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
    RaySceneQueryResult& RaySceneQuery::getLastResults() noexcept
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
    bool RaySceneQuery::queryResult(MovableObject* obj, Real distance)
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
    bool RaySceneQuery::queryResult(SceneQuery::WorldFragment* fragment, Real distance)
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
    IntersectionSceneQueryResult& IntersectionSceneQuery::getLastResults() const noexcept
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
    IntersectionSceneQueryResult&
    IntersectionSceneQuery::execute()
    {
        clearResults();
        mLastResult = new IntersectionSceneQueryResult();
        // Call callback version with self as listener
        execute(this);
        return *mLastResult;
    }
    //---------------------------------------------------------------------
    bool IntersectionSceneQuery::
        queryResult(MovableObject* first, MovableObject* second)
    {
        // Add to internal list
        mLastResult->movables2movables.push_back(
            SceneQueryMovableObjectPair(first, second)
            );
        // Continue
        return true;
    }
    //---------------------------------------------------------------------
    bool IntersectionSceneQuery::
        queryResult(MovableObject* movable, SceneQuery::WorldFragment* fragment)
    {
        // Add to internal list
        mLastResult->movables2world.push_back(
            SceneQueryMovableObjectWorldFragmentPair(movable, fragment)
            );
        // Continue
        return true;
    }




}
    



