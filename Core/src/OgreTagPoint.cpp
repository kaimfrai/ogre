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
module Ogre.Core;

import :Entity;
import :MovableObject;
import :Node;
import :Prerequisites;
import :Quaternion;
import :TagPoint;
import :Vector;

namespace Ogre {
    //-----------------------------------------------------------------------------
    TagPoint::TagPoint(unsigned short handle, Skeleton* creator)
        : Bone(handle, creator)
         
    {
    }
    //-----------------------------------------------------------------------------
    auto TagPoint::getParentEntity() const -> Entity *
    {
        return mParentEntity;
    }
    //-----------------------------------------------------------------------------
    auto TagPoint::getChildObject() const noexcept -> MovableObject*
    {
        return mChildObject;
    }
    //-----------------------------------------------------------------------------
    void TagPoint::setParentEntity(Entity *pEntity)
    {
        mParentEntity = pEntity;
    }
    //-----------------------------------------------------------------------------
    void TagPoint::setChildObject(MovableObject *pObject)
    {
        mChildObject = pObject;
    }
    //-----------------------------------------------------------------------------
    void TagPoint::setInheritParentEntityOrientation(bool inherit)
    {
        mInheritParentEntityOrientation = inherit;
        needUpdate();
    }
    //-----------------------------------------------------------------------------
    auto TagPoint::getInheritParentEntityOrientation() const noexcept -> bool
    {
        return mInheritParentEntityOrientation;
    }
    //-----------------------------------------------------------------------------
    void TagPoint::setInheritParentEntityScale(bool inherit)
    {
        mInheritParentEntityScale = inherit;
        needUpdate();
    }
    //-----------------------------------------------------------------------------
    auto TagPoint::getInheritParentEntityScale() const noexcept -> bool
    {
        return mInheritParentEntityScale;
    }
    //-----------------------------------------------------------------------------
    auto TagPoint::_getFullLocalTransform() const -> const Affine3&
    {
        return mFullLocalTransform;
    }
    //-----------------------------------------------------------------------------
    auto TagPoint::getParentEntityTransform() const noexcept -> const Affine3&
    {

        return mParentEntity->_getParentNodeFullTransform();
    }
    //-----------------------------------------------------------------------------
    void TagPoint::needUpdate(bool forceParentUpdate)
    {
        Bone::needUpdate(forceParentUpdate);

        // We need to tell parent entities node
        if (mParentEntity)
        {
            Node* n = mParentEntity->getParentNode();
            if (n)
            {
                n->needUpdate();
            }

        }

    }
    //-----------------------------------------------------------------------------
    void TagPoint::updateFromParentImpl() const
    {
        // Call superclass
        Bone::updateFromParentImpl();

        // Save transform for local skeleton
        mFullLocalTransform.makeTransform(
            mDerivedPosition,
            mDerivedScale,
            mDerivedOrientation);

        // Include Entity transform
        if (mParentEntity)
        {
            Node* entityParentNode = mParentEntity->getParentNode();
            if (entityParentNode)
            {
                // Note: orientation/scale inherits from parent node already take care with
                // Bone::_updateFromParent, don't do that with parent entity transform.

                // Combine orientation with that of parent entity
                const Quaternion& parentOrientation = entityParentNode->_getDerivedOrientation();
                if (mInheritParentEntityOrientation)
                {
                    mDerivedOrientation = parentOrientation * mDerivedOrientation;
                }

                // Incorporate parent entity scale
                const Vector3& parentScale = entityParentNode->_getDerivedScale();
                if (mInheritParentEntityScale)
                {
                    mDerivedScale *= parentScale;
                }

                // Change position vector based on parent entity's orientation & scale
                mDerivedPosition = parentOrientation * (parentScale * mDerivedPosition);

                // Add altered position vector to parent entity
                mDerivedPosition += entityParentNode->_getDerivedPosition();
            }
        }

        if (mChildObject)
        {
            mChildObject->_notifyMoved();
        }
    }

}
