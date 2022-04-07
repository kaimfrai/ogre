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
module Ogre.Core:Node.Obj;

import :Camera;
import :Common;
import :Exception;
import :Math;
import :Matrix3;
import :Matrix4;
import :Node;
import :Prerequisites;
import :Quaternion;
import :Vector;

import <algorithm>;
import <cassert>;
import <cstddef>;
import <memory>;
import <set>;
import <string>;
import <utility>;
import <vector>;

namespace Ogre {

    Node::QueuedUpdates Node::msQueuedUpdates;
    //-----------------------------------------------------------------------
    Node::Node() : Node(BLANKSTRING) {}
    //-----------------------------------------------------------------------
    Node::Node(String  name)
        :mParent(nullptr),
        mName(std::move(name)),
        mNeedParentUpdate(false),
        mNeedChildUpdate(false),
        mParentNotified(false),
        mQueuedForUpdate(false),
        mInheritOrientation(true),
        mInheritScale(true),
        mCachedTransformOutOfDate(true),
        mOrientation(Quaternion::IDENTITY),
        mPosition(Vector3::ZERO),
        mScale(Vector3::UNIT_SCALE),
        mDerivedOrientation(Quaternion::IDENTITY),
        mDerivedPosition(Vector3::ZERO),
        mDerivedScale(Vector3::UNIT_SCALE),
        mInitialPosition(Vector3::ZERO),
        mInitialOrientation(Quaternion::IDENTITY),
        mInitialScale(Vector3::UNIT_SCALE),
        mListener(nullptr)
    {
        needUpdate();
    }

    //-----------------------------------------------------------------------
    Node::~Node()
    {
        // Call listener (note, only called if there's something to do)
        if (mListener)
        {
            mListener->nodeDestroyed(this);
        }

        removeAllChildren();
        if(mParent)
            mParent->removeChild(this);

        if (mQueuedForUpdate)
        {
            // Erase from queued updates
            auto it =
                std::find(msQueuedUpdates.begin(), msQueuedUpdates.end(), this);
            assert(it != msQueuedUpdates.end());
            if (it != msQueuedUpdates.end())
            {
                // Optimised algorithm to erase an element from unordered vector.
                *it = msQueuedUpdates.back();
                msQueuedUpdates.pop_back();
            }
        }

    }

    //-----------------------------------------------------------------------
    void Node::setParent(Node* parent)
    {
        bool different = (parent != mParent);

        mParent = parent;
        // Request update from parent
        mParentNotified = false ;
        needUpdate();

        // Call listener (note, only called if there's something to do)
        if (mListener && different)
        {
            if (mParent)
                mListener->nodeAttached(this);
            else
                mListener->nodeDetached(this);
        }

    }

    //-----------------------------------------------------------------------
    auto Node::_getFullTransform() const -> const Affine3&
    {
        if (mCachedTransformOutOfDate)
        {
            // Use derived values
            mCachedTransform.makeTransform(
                _getDerivedPosition(),
                _getDerivedScale(),
                _getDerivedOrientation());

            mCachedTransformOutOfDate = false;
        }
        return mCachedTransform;
    }
    //-----------------------------------------------------------------------
    void Node::_update(bool updateChildren, bool parentHasChanged)
    {
        // always clear information about parent notification
        mParentNotified = false;

        // See if we should process everyone
        if (mNeedParentUpdate || parentHasChanged)
        {
            // Update transforms from parent
            _updateFromParent();
        }

        if(updateChildren)
        {
            if (mNeedChildUpdate || parentHasChanged)
            {
                ChildNodeMap::iterator it, itend;
                itend = mChildren.end();
                for (it = mChildren.begin(); it != itend; ++it)
                {
                    Node* child = *it;
                    child->_update(true, true);
                }
            }
            else
            {
                // Just update selected children
                ChildUpdateSet::iterator it, itend;
                itend = mChildrenToUpdate.end();
                for(it = mChildrenToUpdate.begin(); it != itend; ++it)
                {
                    Node* child = *it;
                    child->_update(true, false);
                }

            }

            mChildrenToUpdate.clear();
            mNeedChildUpdate = false;
        }
    }
    //-----------------------------------------------------------------------
    void Node::_updateFromParent() const
    {
        updateFromParentImpl();

        // Call listener (note, this method only called if there's something to do)
        if (mListener)
        {
            mListener->nodeUpdated(this);
        }
    }
    //-----------------------------------------------------------------------
    void Node::updateFromParentImpl() const
    {
        mCachedTransformOutOfDate = true;

        if (mParent)
        {
            // Update orientation
            const Quaternion& parentOrientation = mParent->_getDerivedOrientation();
            if (mInheritOrientation)
            {
                // Combine orientation with that of parent
                mDerivedOrientation = parentOrientation * mOrientation;
            }
            else
            {
                // No inheritance
                mDerivedOrientation = mOrientation;
            }

            // Update scale
            const Vector3& parentScale = mParent->_getDerivedScale();
            if (mInheritScale)
            {
                // Scale own position by parent scale, NB just combine
                // as equivalent axes, no shearing
                mDerivedScale = parentScale * mScale;
            }
            else
            {
                // No inheritance
                mDerivedScale = mScale;
            }

            // Change position vector based on parent's orientation & scale
            mDerivedPosition = parentOrientation * (parentScale * mPosition);

            // Add altered position vector to parents
            mDerivedPosition += mParent->_getDerivedPosition();
        }
        else
        {
            // Root node, no parent
            mDerivedOrientation = mOrientation;
            mDerivedPosition = mPosition;
            mDerivedScale = mScale;
        }

        mNeedParentUpdate = false;

    }
    //-----------------------------------------------------------------------
    auto Node::createChild(const Vector3& inTranslate, const Quaternion& inRotate) -> Node*
    {
        Node* newNode = createChildImpl();
        newNode->setPosition(inTranslate);
        newNode->setOrientation(inRotate);
        this->addChild(newNode);

        return newNode;
    }
    //-----------------------------------------------------------------------
    auto Node::createChild(const String& name, const Vector3& inTranslate, const Quaternion& inRotate) -> Node*
    {
        OgreAssert(!name.empty(), "");
        Node* newNode = createChildImpl(name);
        newNode->setPosition(inTranslate);
        newNode->setOrientation(inRotate);
        this->addChild(newNode);

        return newNode;
    }
    //-----------------------------------------------------------------------
    void Node::addChild(Node* child)
    {
        if (child->mParent)
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                "Node '" + child->getName() + "' already was a child of '" +
                child->mParent->getName() + "'.",
                "Node::addChild");
        }

        mChildren.push_back(child);
        child->setParent(this);

    }
    //-----------------------------------------------------------------------
    auto Node::getChild(unsigned short index) const -> Node*
    {
        if( index < mChildren.size() )
        {
            return mChildren[index];
        }
        else
            return nullptr;
    }
    //-----------------------------------------------------------------------
    auto Node::removeChild(unsigned short index) -> Node*
    {
        OgreAssert(index < mChildren.size(), "");

        auto i = mChildren.begin();
        i += index;
        Node* ret = *i;

        // cancel any pending update
        cancelUpdate(ret);

        std::swap(*i, mChildren.back());
        mChildren.pop_back();
        ret->setParent(nullptr);
        return ret;
    }
    //-----------------------------------------------------------------------
    auto Node::removeChild(Node* child) -> Node*
    {
        if (child)
        {
            auto i = std::find(mChildren.begin(), mChildren.end(), child);
            if(i != mChildren.end() && *i == child)
            {
                // cancel any pending update
                cancelUpdate(child);

                std::swap(*i, mChildren.back());
                mChildren.pop_back();
                child->setParent(nullptr);
            }
        }
        return child;
    }

    //-----------------------------------------------------------------------
    void Node::setOrientation( const Quaternion & q )
    {
        OgreAssertDbg(!q.isNaN(), "Invalid orientation supplied as parameter");
        mOrientation = q;
        mOrientation.normalise();
        needUpdate();
    }
    //-----------------------------------------------------------------------
    void Node::setOrientation( Real w, Real x, Real y, Real z)
    {
        setOrientation(Quaternion(w, x, y, z));
    }
    //-----------------------------------------------------------------------
    void Node::resetOrientation()
    {
        mOrientation = Quaternion::IDENTITY;
        needUpdate();
    }

    //-----------------------------------------------------------------------
    void Node::setPosition(const Vector3& pos)
    {
        assert(!pos.isNaN() && "Invalid vector supplied as parameter");
        mPosition = pos;
        needUpdate();
    }

    //-----------------------------------------------------------------------
    auto Node::getLocalAxes() const -> Matrix3
    {
        Matrix3 ret;
        mOrientation.ToRotationMatrix(ret);
        return ret;
    }

    //-----------------------------------------------------------------------
    void Node::translate(const Vector3& d, TransformSpace relativeTo)
    {
        switch(relativeTo)
        {
        case TS_LOCAL:
            // position is relative to parent so transform downwards
            mPosition += mOrientation * d;
            break;
        case TS_WORLD:
            // position is relative to parent so transform upwards
            if (mParent)
            {
                mPosition += mParent->convertWorldToLocalDirection(d, true);
            }
            else
            {
                mPosition += d;
            }
            break;
        case TS_PARENT:
            mPosition += d;
            break;
        }
        needUpdate();

    }
    //-----------------------------------------------------------------------
    void Node::rotate(const Quaternion& q, TransformSpace relativeTo)
    {
        switch(relativeTo)
        {
        case TS_PARENT:
            // Rotations are normally relative to local axes, transform up
            mOrientation = q * mOrientation;
            break;
        case TS_WORLD:
            // Rotations are normally relative to local axes, transform up
            mOrientation = mOrientation * _getDerivedOrientation().Inverse()
                * q * _getDerivedOrientation();
            break;
        case TS_LOCAL:
            // Note the order of the mult, i.e. q comes after
            mOrientation = mOrientation * q;
            break;
        }

        // Normalise quaternion to avoid drift
        mOrientation.normalise();

        needUpdate();
    }

    
    //-----------------------------------------------------------------------
    void Node::_setDerivedPosition( const Vector3& pos )
    {
        //find where the node would end up in parent's local space
        if(mParent)
            setPosition( mParent->convertWorldToLocalPosition( pos ) );
    }
    //-----------------------------------------------------------------------
    void Node::_setDerivedOrientation( const Quaternion& q )
    {
        //find where the node would end up in parent's local space
        if(mParent)
            setOrientation( mParent->convertWorldToLocalOrientation( q ) );
    }

    //-----------------------------------------------------------------------
    auto Node::_getDerivedOrientation() const -> const Quaternion &
    {
        if (mNeedParentUpdate)
        {
            _updateFromParent();
        }
        return mDerivedOrientation;
    }
    //-----------------------------------------------------------------------
    auto Node::_getDerivedPosition() const -> const Vector3 &
    {
        if (mNeedParentUpdate)
        {
            _updateFromParent();
        }
        return mDerivedPosition;
    }
    //-----------------------------------------------------------------------
    auto Node::_getDerivedScale() const -> const Vector3 &
    {
        if (mNeedParentUpdate)
        {
            _updateFromParent();
        }
        return mDerivedScale;
    }
    //-----------------------------------------------------------------------
    auto Node::convertWorldToLocalPosition( const Vector3 &worldPos ) -> Vector3
    {
        if (mNeedParentUpdate)
        {
            _updateFromParent();
        }

        return mDerivedOrientation.Inverse() * (worldPos - mDerivedPosition) / mDerivedScale;
    }
    //-----------------------------------------------------------------------
    auto Node::convertLocalToWorldPosition( const Vector3 &localPos ) -> Vector3
    {
        if (mNeedParentUpdate)
        {
            _updateFromParent();
        }
        return _getFullTransform() * localPos;
    }
    //-----------------------------------------------------------------------
    auto Node::convertWorldToLocalDirection( const Vector3 &worldDir, bool useScale ) -> Vector3
    {
        if (mNeedParentUpdate)
        {
            _updateFromParent();
        }

        return useScale ? 
            mDerivedOrientation.Inverse() * worldDir / mDerivedScale :
            mDerivedOrientation.Inverse() * worldDir;
    }
    //-----------------------------------------------------------------------
    auto Node::convertLocalToWorldDirection( const Vector3 &localDir, bool useScale ) -> Vector3
    {
        if (mNeedParentUpdate)
        {
            _updateFromParent();
        }
        return useScale ? _getFullTransform().linear() * localDir : mDerivedOrientation * localDir;
    }
    //-----------------------------------------------------------------------
    auto Node::convertWorldToLocalOrientation( const Quaternion &worldOrientation ) -> Quaternion
    {
        if (mNeedParentUpdate)
        {
            _updateFromParent();
        }
        return mDerivedOrientation.Inverse() * worldOrientation;
    }
    //-----------------------------------------------------------------------
    auto Node::convertLocalToWorldOrientation( const Quaternion &localOrientation ) -> Quaternion
    {
        if (mNeedParentUpdate)
        {
            _updateFromParent();
        }
        return mDerivedOrientation * localOrientation;

    }
    //-----------------------------------------------------------------------
    void Node::removeAllChildren()
    {
        ChildNodeMap::iterator i, iend;
        iend = mChildren.end();
        for (i = mChildren.begin(); i != iend; ++i)
        {
            (*i)->setParent(nullptr);
        }
        mChildren.clear();
        mChildrenToUpdate.clear();
    }
    //-----------------------------------------------------------------------
    void Node::setScale(const Vector3& inScale)
    {
        assert(!inScale.isNaN() && "Invalid vector supplied as parameter");
        mScale = inScale;
        needUpdate();
    }
    //-----------------------------------------------------------------------
    void Node::setInheritOrientation(bool inherit)
    {
        mInheritOrientation = inherit;
        needUpdate();
    }
    //-----------------------------------------------------------------------
    void Node::setInheritScale(bool inherit)
    {
        mInheritScale = inherit;
        needUpdate();
    }
    //-----------------------------------------------------------------------
    void Node::scale(const Vector3& inScale)
    {
        mScale = mScale * inScale;
        needUpdate();

    }
    //-----------------------------------------------------------------------
    void Node::scale(Real x, Real y, Real z)
    {
        mScale.x *= x;
        mScale.y *= y;
        mScale.z *= z;
        needUpdate();

    }
    //-----------------------------------------------------------------------
    void Node::setInitialState()
    {
        mInitialPosition = mPosition;
        mInitialOrientation = mOrientation;
        mInitialScale = mScale;
    }
    //-----------------------------------------------------------------------
    void Node::resetToInitialState()
    {
        mPosition = mInitialPosition;
        mOrientation = mInitialOrientation;
        mScale = mInitialScale;

        needUpdate();
    }
    //-----------------------------------------------------------------------
    struct NodeNameExists {
        const String& name;
        auto operator()(const Node* mo) -> bool {
            return mo->getName() == name;
        }
    };
    auto Node::getChild(const String& name) const -> Node*
    {
        NodeNameExists pred = {name};
        auto i = std::find_if(mChildren.begin(), mChildren.end(), pred);

        if (i == mChildren.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Child node named " + name +
                " does not exist.", "Node::getChild");
        }

        return *i;
    }
    //-----------------------------------------------------------------------
    auto Node::removeChild(const String& name) -> Node*
    {
        OgreAssert(!name.empty(), "");
        NodeNameExists pred = {name};
        auto i = std::find_if(mChildren.begin(), mChildren.end(), pred);

        if (i == mChildren.end())
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Child node named " + name +
                " does not exist.", "Node::removeChild");
        }

        Node* ret = *i;

        // Cancel any pending update
        cancelUpdate(ret);
        std::swap(*i, mChildren.back());
        mChildren.pop_back();
        ret->setParent(nullptr);

        return ret;


    }

    //-----------------------------------------------------------------------
    auto Node::getSquaredViewDepth(const Camera* cam) const -> Real
    {
        Vector3 diff = _getDerivedPosition() - cam->getDerivedPosition();
        Vector3 zAxis = cam->getDerivedDirection();

        // NB use squared length to avoid square root
        return cam->getSortMode() == SM_DISTANCE ? diff.squaredLength() : Math::Sqr(zAxis.dotProduct(diff));
    }
    //-----------------------------------------------------------------------
    void Node::needUpdate(bool forceParentUpdate)
    {

        mNeedParentUpdate = true;
        mNeedChildUpdate = true;
        mCachedTransformOutOfDate = true;

        // Make sure we're not root and parent hasn't been notified before
        if (mParent && (!mParentNotified || forceParentUpdate))
        {
            mParent->requestUpdate(this, forceParentUpdate);
            mParentNotified = true ;
        }

        // all children will be updated
        mChildrenToUpdate.clear();
    }
    //-----------------------------------------------------------------------
    void Node::requestUpdate(Node* child, bool forceParentUpdate)
    {
        // If we're already going to update everything this doesn't matter
        if (mNeedChildUpdate)
        {
            return;
        }

        mChildrenToUpdate.insert(child);
        // Request selective update of me, if we didn't do it before
        if (mParent && (!mParentNotified || forceParentUpdate))
        {
            mParent->requestUpdate(this, forceParentUpdate);
            mParentNotified = true ;
        }

    }
    //-----------------------------------------------------------------------
    void Node::cancelUpdate(Node* child)
    {
        mChildrenToUpdate.erase(child);

        // Propagate this up if we're done
        if (mChildrenToUpdate.empty() && mParent && !mNeedChildUpdate)
        {
            mParent->cancelUpdate(this);
            mParentNotified = false ;
        }
    }
    //-----------------------------------------------------------------------
    void Node::queueNeedUpdate(Node* n)
    {
        // Don't queue the node more than once
        if (!n->mQueuedForUpdate)
        {
            n->mQueuedForUpdate = true;
            msQueuedUpdates.push_back(n);
        }
    }
    //-----------------------------------------------------------------------
    void Node::processQueuedUpdates()
    {
        for (auto n : msQueuedUpdates)
        {
            // Update, and force parent update since chances are we've ended
            // up with some mixed state in there due to re-entrancy
            n->mQueuedForUpdate = false;
            n->needUpdate(true);
        }
        msQueuedUpdates.clear();
    }
}
