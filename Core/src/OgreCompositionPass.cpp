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
#include <cassert>

#include "OgreCompositionPass.hpp"
#include "OgreMaterial.hpp"
#include "OgreMaterialManager.hpp"

namespace Ogre {
class CompositionTargetPass;

CompositionPass::CompositionPass(CompositionTargetPass *parent):
    mParent(parent),
    mType(PT_RENDERQUAD)
{
}
//-----------------------------------------------------------------------
CompositionPass::~CompositionPass()
{
}
//-----------------------------------------------------------------------
void CompositionPass::setType(CompositionPass::PassType type)
{
    mType = type;
}
//-----------------------------------------------------------------------
auto CompositionPass::getType() const -> CompositionPass::PassType
{
    return mType;
}
//-----------------------------------------------------------------------
void CompositionPass::setIdentifier(uint32 id)
{
    mMaterial.identifier = id;
}
//-----------------------------------------------------------------------
auto CompositionPass::getIdentifier() const -> uint32
{
    return mMaterial.identifier;
}
//-----------------------------------------------------------------------
void CompositionPass::setMaterial(const MaterialPtr& mat)
{
    mMaterial.material = mat;
}
//-----------------------------------------------------------------------
void CompositionPass::setMaterialName(const String &name)
{
    mMaterial.material = MaterialManager::getSingleton().getByName(name);
}
//-----------------------------------------------------------------------
auto CompositionPass::getMaterial() const -> const MaterialPtr&
{
    return mMaterial.material;
}
//-----------------------------------------------------------------------
void CompositionPass::setClearBuffers(uint32 val)
{
    mClear.buffers = val;
}
//-----------------------------------------------------------------------
auto CompositionPass::getClearBuffers() const -> uint32
{
    return mClear.buffers;
}
//-----------------------------------------------------------------------
void CompositionPass::setClearColour(const ColourValue &val)
{
    mClear.colour = val;
}
//-----------------------------------------------------------------------
auto CompositionPass::getClearColour() const -> const ColourValue &
{
    return mClear.colour;
}
//-----------------------------------------------------------------------
void CompositionPass::setAutomaticColour(bool val)
{
    mClear.automaticColour = val;
}
//-----------------------------------------------------------------------
auto CompositionPass::getAutomaticColour() const -> bool
{
    return mClear.automaticColour;
}
//-----------------------------------------------------------------------
void CompositionPass::setInput(size_t id, const String &input, size_t mrtIndex)
{
    assert(id<OGRE_MAX_TEXTURE_LAYERS);
    mMaterial.inputs[id] = InputTex(input, mrtIndex);
}
//-----------------------------------------------------------------------
auto CompositionPass::getInput(size_t id) const -> const CompositionPass::InputTex &
{
    assert(id<OGRE_MAX_TEXTURE_LAYERS);
    return mMaterial.inputs[id];
}
//-----------------------------------------------------------------------
auto CompositionPass::getNumInputs() const -> size_t
{
    size_t count = 0;
    for(size_t x=0; x<OGRE_MAX_TEXTURE_LAYERS; ++x)
    {
        if(!mMaterial.inputs[x].name.empty())
            count = x+1;
    }
    return count;
}
//-----------------------------------------------------------------------
void CompositionPass::clearAllInputs()
{
    for(size_t x=0; x<OGRE_MAX_TEXTURE_LAYERS; ++x)
    {
        mMaterial.inputs[x].name.clear();
    }
}
//-----------------------------------------------------------------------
auto CompositionPass::getParent() -> CompositionTargetPass *
{
    return mParent;
}
//-----------------------------------------------------------------------
void CompositionPass::setFirstRenderQueue(uint8 id)
{
    mRenderScene.firstRenderQueue = id;
}
//-----------------------------------------------------------------------
auto CompositionPass::getFirstRenderQueue() const -> uint8
{
    return mRenderScene.firstRenderQueue;
}
//-----------------------------------------------------------------------
void CompositionPass::setLastRenderQueue(uint8 id)
{
    mRenderScene.lastRenderQueue = id;
}
//-----------------------------------------------------------------------
auto CompositionPass::getLastRenderQueue() const -> uint8
{
    return mRenderScene.lastRenderQueue ;
}
//-----------------------------------------------------------------------
void CompositionPass::setMaterialScheme(const String& schemeName)
{
    mRenderScene.materialScheme = schemeName;
}
//-----------------------------------------------------------------------
auto CompositionPass::getMaterialScheme() const -> const String&
{
    return mRenderScene.materialScheme;
}
//-----------------------------------------------------------------------
void CompositionPass::setClearDepth(float depth)
{
    mClear.depth = depth;
}
auto CompositionPass::getClearDepth() const -> float
{
    return mClear.depth;
}
void CompositionPass::setClearStencil(uint16 value)
{
    mClear.stencil = value;
}
auto CompositionPass::getClearStencil() const -> uint16
{
    return mClear.stencil;
}

void CompositionPass::setStencilCheck(bool value)
{
    mStencilState.enabled = value;
}
auto CompositionPass::getStencilCheck() const -> bool
{
    return mStencilState.enabled;
}
void CompositionPass::setStencilFunc(CompareFunction value)
{
    mStencilState.compareOp = value;
}
auto CompositionPass::getStencilFunc() const -> CompareFunction
{
    return mStencilState.compareOp;
} 
void CompositionPass::setStencilRefValue(uint32 value)
{
    mStencilState.referenceValue = value;
}
auto CompositionPass::getStencilRefValue() const -> uint32
{
    return mStencilState.referenceValue;
}
void CompositionPass::setStencilMask(uint32 value)
{
    mStencilState.compareMask = value;
}
auto CompositionPass::getStencilMask() const -> uint32
{
    return mStencilState.compareMask ;
}
void CompositionPass::setStencilFailOp(StencilOperation value)
{
    mStencilState.stencilFailOp = value;
}
auto CompositionPass::getStencilFailOp() const -> StencilOperation
{
    return mStencilState.stencilFailOp;
}
void CompositionPass::setStencilDepthFailOp(StencilOperation value)
{
    mStencilState.depthFailOp = value;
}
auto CompositionPass::getStencilDepthFailOp() const -> StencilOperation
{
    return mStencilState.depthFailOp;
}
void CompositionPass::setStencilPassOp(StencilOperation value)
{
    mStencilState.depthStencilPassOp = value;
}
auto CompositionPass::getStencilPassOp() const -> StencilOperation
{
    return mStencilState.depthStencilPassOp;
}
void CompositionPass::setStencilTwoSidedOperation(bool value)
{
    mStencilState.twoSidedOperation = value;
}
auto CompositionPass::getStencilTwoSidedOperation() const -> bool
{
    return mStencilState.twoSidedOperation;
}
void CompositionPass::setQuadFarCorners(bool farCorners, bool farCornersViewSpace)
{
    mQuad.farCorners = farCorners;
    mQuad.farCornersViewSpace = farCornersViewSpace;
}
auto CompositionPass::getQuadFarCorners() const -> bool
{
    return mQuad.farCorners;
}
auto CompositionPass::getQuadFarCornersViewSpace() const -> bool
{
    return mQuad.farCornersViewSpace;
}
        
void CompositionPass::setCustomType(const String& customType)
{
    mCustomType = customType;
}

auto CompositionPass::getCustomType() const -> const String&
{
    return mCustomType;
}
//-----------------------------------------------------------------------
auto CompositionPass::_isSupported() -> bool
{
    // A pass is supported if material referenced have a supported technique

    if (mType == PT_RENDERQUAD)
    {
        if (!mMaterial.material)
        {
            return false;
        }

        mMaterial.material->load();
        if (mMaterial.material->getSupportedTechniques().empty())
        {
            return false;
        }
    }

    return true;
}

}
