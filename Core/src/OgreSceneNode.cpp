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

#include <cassert>

module Ogre.Core;

import :AxisAlignedBox;
import :Camera;
import :Codec;
import :Common;
import :Exception;
import :Math;
import :Matrix3;
import :MovableObject;
import :Node;
import :Platform;
import :Prerequisites;
import :Quaternion;
import :RenderQueue;
import :ResourceGroupManager;
import :Root;
import :SceneManager;
import :SceneNode;
import :String;
import :Vector;

import <algorithm>;
import <memory>;
import <string>;
import <string_view>;
import <utility>;
import <vector>;

namespace Ogre {
    //-----------------------------------------------------------------------
    SceneNode::SceneNode(SceneManager* creator) : SceneNode(creator, BLANKSTRING)
    {
    }
    //-----------------------------------------------------------------------
    SceneNode::SceneNode(SceneManager* creator, std::string_view name)
        : Node(name)
        , mCreator(creator)
        , mAutoTrackTarget(nullptr)
        , mGlobalIndex(-1)
        , mYawFixed(false)
        , mIsInSceneGraph(false)
        , mShowBoundingBox(false)
    {
        needUpdate();
    }
    //-----------------------------------------------------------------------
    SceneNode::~SceneNode()
    {
        // Detach all objects, do this manually to avoid needUpdate() call 
        // which can fail because of deleted items
        for (auto & itr : mObjectsByName)
        {
            itr->_notifyAttached((SceneNode*)nullptr);
        }
        mObjectsByName.clear();
    }
    //-----------------------------------------------------------------------
    void SceneNode::_update(bool updateChildren, bool parentHasChanged)
    {
        Node::_update(updateChildren, parentHasChanged);
        _updateBounds();
    }
    //-----------------------------------------------------------------------
    void SceneNode::setParent(Node* parent)
    {
        Node::setParent(parent);

        if (parent)
        {
            auto* sceneParent = static_cast<SceneNode*>(parent);
            setInSceneGraph(sceneParent->isInSceneGraph());
        }
        else
        {
            setInSceneGraph(false);
        }
    }
    //-----------------------------------------------------------------------
    void SceneNode::setInSceneGraph(bool inGraph)
    {
        if (inGraph != mIsInSceneGraph)
        {
            mIsInSceneGraph = inGraph;
            // Tell children
            for (auto child : getChildren())
            {
                auto* sceneChild = static_cast<SceneNode*>(child);
                sceneChild->setInSceneGraph(inGraph);
            }
        }
    }
    //-----------------------------------------------------------------------
    struct MovableObjectNameExists {
        std::string_view name;
        auto operator()(const MovableObject* mo) -> bool {
            return mo->getName() == name;
        }
    };
    void SceneNode::attachObject(MovableObject* obj)
    {
        OgreAssert(!obj->isAttached(), "Object already attached to a SceneNode or a Bone");

        obj->_notifyAttached(this);

        // Also add to name index
        MovableObjectNameExists pred = {obj->getName()};
        auto it = std::ranges::find_if(mObjectsByName, pred);
        if (it != mObjectsByName.end())
            OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM,
                        ::std::format("An object named '{}' already attached to this SceneNode", obj->getName() ));
        mObjectsByName.push_back(obj);

        // Make sure bounds get updated (must go right to the top)
        needUpdate();
    }
    //-----------------------------------------------------------------------
    auto SceneNode::getAttachedObject(std::string_view name) const -> MovableObject*
    {
        // Look up 
        MovableObjectNameExists pred = {name};
        auto i = std::ranges::find_if(mObjectsByName, pred);

        if (i == mObjectsByName.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                ::std::format("Attached object {} not found.", "SceneNode::getAttachedObject",
                name));
        }

        return *i;
    }
    //-----------------------------------------------------------------------
    auto SceneNode::detachObject(unsigned short index) -> MovableObject*
    {
        OgreAssert(index < mObjectsByName.size(), "out of bounds");
        auto i = mObjectsByName.begin();
        i += index;

        MovableObject* ret = *i;
        std::swap(*i, mObjectsByName.back());
        mObjectsByName.pop_back();

        ret->_notifyAttached((SceneNode*)nullptr);

        // Make sure bounds get updated (must go right to the top)
        needUpdate();

        return ret;
    }
    //-----------------------------------------------------------------------
    auto SceneNode::detachObject(std::string_view name) -> MovableObject*
    {
        MovableObjectNameExists pred = {name};
        auto it = std::ranges::find_if(mObjectsByName, pred);

        if (it == mObjectsByName.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("Object {} is not attached "
                "to this node.", name), "SceneNode::detachObject");
        }

        MovableObject* ret = *it;
        std::swap(*it, mObjectsByName.back());
        mObjectsByName.pop_back();

        ret->_notifyAttached((SceneNode*)nullptr);
        // Make sure bounds get updated (must go right to the top)
        needUpdate();
        
        return ret;

    }
    //-----------------------------------------------------------------------
    void SceneNode::detachObject(MovableObject* obj)
    {
        for (auto i = mObjectsByName.begin(); i != mObjectsByName.end(); ++i)
        {
            if (*i == obj)
            {
                std::swap(*i, mObjectsByName.back());
                mObjectsByName.pop_back();
                break;
            }
        }
        obj->_notifyAttached((SceneNode*)nullptr);

        // Make sure bounds get updated (must go right to the top)
        needUpdate();

    }
    //-----------------------------------------------------------------------
    void SceneNode::detachAllObjects()
    {
        for (auto & itr : mObjectsByName)
        {
            itr->_notifyAttached((SceneNode*)nullptr);
        }
        mObjectsByName.clear();
        // Make sure bounds get updated (must go right to the top)
        needUpdate();
    }
    //-----------------------------------------------------------------------
    void SceneNode::_updateBounds()
    {
        // Reset bounds first
        mWorldAABB.setNull();

        // Update bounds from own attached objects
        for (auto & i : mObjectsByName)
        {
            // Merge world bounds of each object
            mWorldAABB.merge(i->getWorldBoundingBox(true));
        }

        // Merge with children
        for (auto child : getChildren())
        {
            auto* sceneChild = static_cast<SceneNode*>(child);
            mWorldAABB.merge(sceneChild->mWorldAABB);
        }

    }
    //-----------------------------------------------------------------------
    void SceneNode::_findVisibleObjects(Camera* cam, RenderQueue* queue, 
        VisibleObjectsBoundsInfo* visibleBounds, bool includeChildren, 
        bool displayNodes, bool onlyShadowCasters)
    {
        // Check self visible
        if (!cam->isVisible(mWorldAABB))
            return;

        // Add all entities
        for (auto mo : mObjectsByName)
        {
            queue->processVisibleObject(mo, cam, onlyShadowCasters, visibleBounds);
        }

        if (includeChildren)
        {
            for (auto child : getChildren())
            {
                auto* sceneChild = static_cast<SceneNode*>(child);
                sceneChild->_findVisibleObjects(cam, queue, visibleBounds, includeChildren, 
                    displayNodes, onlyShadowCasters);
            }
        }

        if (mCreator && mCreator->getDebugDrawer())
        {
            mCreator->getDebugDrawer()->drawSceneNode(this);
        }
    }

    //-----------------------------------------------------------------------
    void SceneNode::updateFromParentImpl() const
    {
        Node::updateFromParentImpl();

        // Notify objects that it has been moved
        for (auto o : mObjectsByName)
        {
            o->_notifyMoved();
        }
    }
    //-----------------------------------------------------------------------
    auto SceneNode::createChildImpl() -> Node*
    {
        assert(mCreator);
        return mCreator->createSceneNode();
    }
    //-----------------------------------------------------------------------
    auto SceneNode::createChildImpl(std::string_view name) -> Node*
    {
        assert(mCreator);
        return mCreator->createSceneNode(name);
    }
    //-----------------------------------------------------------------------
    void SceneNode::removeAndDestroyChild(std::string_view name)
    {
        auto* pChild = static_cast<SceneNode*>(getChild(name));
        pChild->removeAndDestroyAllChildren();

        removeChild(name);
        pChild->getCreator()->destroySceneNode(name);

    }
    //-----------------------------------------------------------------------
    void SceneNode::removeAndDestroyChild(unsigned short index)
    {
        auto* pChild = static_cast<SceneNode*>(getChildren()[index]);
        pChild->removeAndDestroyAllChildren();

        removeChild(index);
        pChild->getCreator()->destroySceneNode(pChild);
    }
    //-----------------------------------------------------------------------
    void SceneNode::removeAndDestroyChild(SceneNode* child)
    {
        removeAndDestroyChild(std::ranges::find(getChildren(), child) -
                              getChildren().begin());
    }
    //-----------------------------------------------------------------------
    void SceneNode::removeAndDestroyAllChildren()
    {
        // do not store iterators (invalidated by
        // SceneManager::destroySceneNode because it causes removal from parent)
        while(!getChildren().empty()) {
            auto* sn = static_cast<SceneNode*>(getChildren().front());
            sn->removeAndDestroyAllChildren();
            sn->getCreator()->destroySceneNode(sn);
        }

        mChildren.clear();
        needUpdate();
    }
    void SceneNode::loadChildren(std::string_view filename)
    {
        std::string_view baseName, strExt;
        StringUtil::splitBaseFilename(filename, baseName, strExt);
        auto codec = Codec::getCodec(strExt);
        if (!codec)
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, ::std::format("No codec found to load {}", filename));

        auto stream = Root::openFileStream(
            filename, ResourceGroupManager::getSingleton().getWorldResourceGroupName());
        codec->decode(stream, this);
    }
    void SceneNode::saveChildren(std::string_view filename)
    {
        std::string_view baseName, strExt;
        StringUtil::splitBaseFilename(filename, baseName, strExt);
        auto codec = Codec::getCodec(strExt);
        codec->encodeToFile(this, filename);
    }
    //-----------------------------------------------------------------------
    auto SceneNode::createChildSceneNode(const Vector3& inTranslate, 
        const Quaternion& inRotate) -> SceneNode*
    {
        return static_cast<SceneNode*>(this->createChild(inTranslate, inRotate));
    }
    //-----------------------------------------------------------------------
    auto SceneNode::createChildSceneNode(std::string_view name, const Vector3& inTranslate,
        const Quaternion& inRotate) -> SceneNode*
    {
        return static_cast<SceneNode*>(this->createChild(name, inTranslate, inRotate));
    }
    //-----------------------------------------------------------------------
    void SceneNode::findLights(LightList& destList, Real radius, QueryTypeMask lightMask) const
    {
        // No any optimisation here, hope inherits more smart for that.
        //
        // If a scene node is static and lights have moved, light list won't change
        // can't use a simple global boolean flag since this is only called for
        // visible nodes, so temporarily visible nodes will not be updated
        // Since this is only called for visible nodes, skip the check for now
        //
        if (mCreator)
        {
            // Use SceneManager to calculate
            mCreator->_populateLightList(this, radius, destList, lightMask);
        }
        else
        {
            destList.clear();
        }
    }
    //-----------------------------------------------------------------------
    void SceneNode::setAutoTracking(bool enabled, SceneNode* const target, 
        const Vector3& localDirectionVector,
        const Vector3& offset)
    {
        if (enabled)
        {
            mAutoTrackTarget = target;
            mAutoTrackOffset = offset;
            mAutoTrackLocalDirection = localDirectionVector;
        }
        else
        {
            mAutoTrackTarget = nullptr;
        }
        if (mCreator)
            mCreator->_notifyAutotrackingSceneNode(this, enabled);
    }
    //-----------------------------------------------------------------------
    void SceneNode::setFixedYawAxis(bool useFixed, const Vector3& fixedAxis)
    {
        mYawFixed = useFixed;
        mYawFixedAxis = fixedAxis;
    }

    //-----------------------------------------------------------------------
    void SceneNode::yaw(const Radian& angle, TransformSpace relativeTo)
    {
        if (mYawFixed)
        {
            rotate(mYawFixedAxis, angle, relativeTo);
        }
        else
        {
            rotate(Vector3::UNIT_Y, angle, relativeTo);
        }

    }
    //-----------------------------------------------------------------------
    void SceneNode::setDirection(Real x, Real y, Real z, TransformSpace relativeTo, 
        const Vector3& localDirectionVector)
    {
        setDirection(Vector3{x,y,z}, relativeTo, localDirectionVector);
    }

    //-----------------------------------------------------------------------
    void SceneNode::setDirection(const Vector3& vec, TransformSpace relativeTo, 
        const Vector3& localDirectionVector)
    {
        // Do nothing if given a zero vector
        if (vec == Vector3::ZERO) return;

        // The direction we want the local direction point to
        Vector3 targetDir = vec.normalisedCopy();

        // Transform target direction to world space
        using enum TransformSpace;
        switch (relativeTo)
        {
        case PARENT:
            if (getInheritOrientation())
            {
                if (getParent())
                {
                    targetDir = getParent()->_getDerivedOrientation() * targetDir;
                }
            }
            break;
        case LOCAL:
            targetDir = _getDerivedOrientation() * targetDir;
            break;
        case WORLD:
            // default orientation
            break;
        }

        // Calculate target orientation relative to world space
        Quaternion targetOrientation;
        if( mYawFixed )
        {
            // Calculate the quaternion for rotate local Z to target direction
            Vector3 yawAxis = mYawFixedAxis;

            if (getInheritOrientation() && getParent())
            {
                yawAxis = getParent()->_getDerivedOrientation() * yawAxis;
            }

            Quaternion unitZToTarget = Quaternion::FromMatrix3(Math::lookRotation(targetDir, yawAxis));

            if (localDirectionVector == Vector3::NEGATIVE_UNIT_Z)
            {
                // Specail case for avoid calculate 180 degree turn
                targetOrientation =
                    Quaternion{-unitZToTarget.y, -unitZToTarget.z, unitZToTarget.w, unitZToTarget.x};
            }
            else
            {
                // Calculate the quaternion for rotate local direction to target direction
                Quaternion localToUnitZ = localDirectionVector.getRotationTo(Vector3::UNIT_Z);
                targetOrientation = unitZToTarget * localToUnitZ;
            }
        }
        else
        {
            const Quaternion& currentOrient = _getDerivedOrientation();

            // Get current local direction relative to world space
            Vector3 currentDir = currentOrient * localDirectionVector;

            if ((currentDir+targetDir).squaredLength() < 0.00005f)
            {
                // Oops, a 180 degree turn (infinite possible rotation axes)
                // Default to yaw i.e. use current UP
                targetOrientation =
                    Quaternion{-currentOrient.y, -currentOrient.z, currentOrient.w, currentOrient.x};
            }
            else
            {
                // Derive shortest arc to new direction
                Quaternion rotQuat = currentDir.getRotationTo(targetDir);
                targetOrientation = rotQuat * currentOrient;
            }
        }

        // Set target orientation, transformed to parent space
        if (getParent() && getInheritOrientation())
            setOrientation(getParent()->_getDerivedOrientation().UnitInverse() * targetOrientation);
        else
            setOrientation(targetOrientation);
    }
    //-----------------------------------------------------------------------
    void SceneNode::lookAt( const Vector3& targetPoint, TransformSpace relativeTo, 
        const Vector3& localDirectionVector)
    {
        // Calculate ourself origin relative to the given transform space
        Vector3 origin;
        using enum TransformSpace;
        switch (relativeTo)
        {
        default:    // Just in case
        case WORLD:
            origin = _getDerivedPosition();
            break;
        case PARENT:
            origin = getPosition();
            break;
        case LOCAL:
            origin = Vector3::ZERO;
            break;
        }

        setDirection(targetPoint - origin, relativeTo, localDirectionVector);
    }
    //-----------------------------------------------------------------------
    void SceneNode::_autoTrack()
    {
        // NB assumes that all scene nodes have been updated
        if (mAutoTrackTarget)
        {
            lookAt(mAutoTrackTarget->_getDerivedPosition() + mAutoTrackOffset, 
                TransformSpace::WORLD, mAutoTrackLocalDirection);
            // update self & children
            _update(true, true);
        }
    }
    //-----------------------------------------------------------------------
    auto SceneNode::getParentSceneNode() const noexcept -> SceneNode*
    {
        return static_cast<SceneNode*>(getParent());
    }
    //-----------------------------------------------------------------------
    void SceneNode::setVisible(bool visible, bool cascade) const
    {
        for (auto o : mObjectsByName)
        {
            o->setVisible(visible);
        }

        if (cascade)
        {
            for (auto c : getChildren())
            {
                static_cast<SceneNode*>(c)->setVisible(visible, cascade);
            }
        }
    }
    //-----------------------------------------------------------------------
    void SceneNode::setDebugDisplayEnabled(bool enabled, bool cascade) const
    {
        for (auto o : mObjectsByName)
        {
            o->setDebugDisplayEnabled(enabled);
        }

        if (cascade)
        {
            for (auto c : getChildren())
            {
                static_cast<SceneNode*>(c)->setDebugDisplayEnabled(enabled, cascade);
            }
        }
    }
    //-----------------------------------------------------------------------
    void SceneNode::flipVisibility(bool cascade) const
    {
        for (auto o : mObjectsByName)
        {
            o->setVisible(!o->getVisible());
        }

        if (cascade)
        {
            for (auto c : getChildren())
            {
                static_cast<SceneNode*>(c)->flipVisibility(cascade);
            }
        }
    }



}
