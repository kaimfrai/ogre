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

import :Billboard;

namespace Ogre {
    //-----------------------------------------------------------------------
    Billboard::Billboard():
        mOwnDimensions(false),
        mUseTexcoordRect(false),
        mTexcoordIndex(0),
        mPosition(Vector3::ZERO),
        mDirection(Vector3::ZERO),        
        mColour(0xFFFFFFFF),
        mRotation{0}
    {
    }
    //-----------------------------------------------------------------------
    Billboard::~Billboard()
    = default;
    //-----------------------------------------------------------------------
    Billboard::Billboard(const Vector3& position, BillboardSet* owner, const ColourValue& colour)
        : mOwnDimensions(false)
        , mUseTexcoordRect(false)
        , mTexcoordIndex(0)
        , mPosition(position)
        , mDirection(Vector3::ZERO)
        , mColour(colour.getAsBYTE())
        , mRotation{0}
    {
    }
    //-----------------------------------------------------------------------
    void Billboard::setDimensions(float width, float height)
    {
        mOwnDimensions = true;
        mWidth = width;
        mHeight = height;
    }
    //-----------------------------------------------------------------------
    void Billboard::setTexcoordIndex(uint16 texcoordIndex)
    {
        mTexcoordIndex = texcoordIndex;
        mUseTexcoordRect = false;
    }
    //-----------------------------------------------------------------------
    void Billboard::setTexcoordRect(const FloatRect& texcoordRect)
    {
        mTexcoordRect = texcoordRect;
        mUseTexcoordRect = true;
    }
}
