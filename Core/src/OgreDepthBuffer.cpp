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

import :DepthBuffer;
import :RenderTarget;

namespace Ogre
{
    DepthBuffer::DepthBuffer( DepthBuffer::PoolId poolId, uint32 width, uint32 height,
                              uint32 fsaa, bool manual ) :
                mPoolId(poolId),
                mWidth(width),
                mHeight(height),
                mFsaa(fsaa),
                mManual(manual)
    {
    }

    DepthBuffer::~DepthBuffer()
    {
        detachFromAllRenderTargets();
    }

    void DepthBuffer::_setPoolId( DepthBuffer::PoolId poolId )
    {
        //Change the pool Id
        mPoolId = poolId;

        //Render Targets were attached to us, but they have a different pool Id,
        //so detach ourselves from them
        detachFromAllRenderTargets();
    }
    //-----------------------------------------------------------------------
    auto DepthBuffer::getPoolId() const noexcept -> DepthBuffer::PoolId
    {
        return mPoolId;
    }
    //-----------------------------------------------------------------------
    auto DepthBuffer::getWidth() const noexcept -> uint32
    {
        return mWidth;
    }
    //----------------------------------------------------------------------
    auto DepthBuffer::getHeight() const noexcept -> uint32
    {
        return mHeight;
    }
    //-----------------------------------------------------------------------
    auto DepthBuffer::isManual() const noexcept -> bool
    {
        return mManual;
    }
    //-----------------------------------------------------------------------
    auto DepthBuffer::isCompatible( RenderTarget *renderTarget ) const -> bool
    {
        if( this->getWidth() >= renderTarget->getWidth() &&
            this->getHeight() >= renderTarget->getHeight() &&
            this->getFSAA() == renderTarget->getFSAA() )
        {
            return true;
        }

        return false;
    }
    //-----------------------------------------------------------------------
    void DepthBuffer::_notifyRenderTargetAttached( RenderTarget *renderTarget )
    {
        assert( mAttachedRenderTargets.find( renderTarget ) == mAttachedRenderTargets.end() );

        mAttachedRenderTargets.insert( renderTarget );
    }
    //-----------------------------------------------------------------------
    void DepthBuffer::_notifyRenderTargetDetached( RenderTarget *renderTarget )
    {
        auto itor = mAttachedRenderTargets.find( renderTarget );
        assert( itor != mAttachedRenderTargets.end() );

        mAttachedRenderTargets.erase( itor );
    }
    //-----------------------------------------------------------------------
    void DepthBuffer::detachFromAllRenderTargets()
    {
        for(auto const& itor : mAttachedRenderTargets)
        {
            //If we call, detachDepthBuffer, we'll invalidate the iterators
            itor->_detachDepthBuffer();
        }

        mAttachedRenderTargets.clear();
    }
}
