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
    mParent(parent)
    
{
}
//-----------------------------------------------------------------------
CompositionPass::~CompositionPass()
= default;
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
auto CompositionPass::getIdentifier() const noexcept -> uint32
{
    return mMaterial.identifier;
}
//-----------------------------------------------------------------------
void CompositionPass::setMaterial(const MaterialPtr& mat)
{
    mMaterial.material = mat;
}
//-----------------------------------------------------------------------
void CompositionPass::setMaterialName(std::string_view name)
{
    mMaterial.material = MaterialManager::getSingleton().getByName(name);
}
//-----------------------------------------------------------------------
auto CompositionPass::getMaterial() const noexcept -> const MaterialPtr&
{
    return mMaterial.material;
}
//-----------------------------------------------------------------------
void CompositionPass::setClearBuffers(uint32 val)
{
    mClear.buffers = val;
}
//-----------------------------------------------------------------------
auto CompositionPass::getClearBuffers() const noexcept -> uint32
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
auto CompositionPass::getAutomaticColour() const noexcept -> bool
{
    return mClear.automaticColour;
}
//-----------------------------------------------------------------------
void CompositionPass::setInput(size_t id, std::string_view input, size_t mrtIndex)
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
    for(auto & input : mMaterial.inputs)
    {
        input.name.clear();
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
auto CompositionPass::getFirstRenderQueue() const noexcept -> uint8
{
    return mRenderScene.firstRenderQueue;
}
//-----------------------------------------------------------------------
void CompositionPass::setLastRenderQueue(uint8 id)
{
    mRenderScene.lastRenderQueue = id;
}
//-----------------------------------------------------------------------
auto CompositionPass::getLastRenderQueue() const noexcept -> uint8
{
    return mRenderScene.lastRenderQueue ;
}
//-----------------------------------------------------------------------
void CompositionPass::setMaterialScheme(std::string_view schemeName)
{
    mRenderScene.materialScheme = schemeName;
}
//-----------------------------------------------------------------------
auto CompositionPass::getMaterialScheme() const noexcept -> std::string_view 
{
    return mRenderScene.materialScheme;
}
//-----------------------------------------------------------------------
void CompositionPass::setClearDepth(float depth)
{
    mClear.depth = depth;
}
auto CompositionPass::getClearDepth() const noexcept -> float
{
    return mClear.depth;
}
void CompositionPass::setClearStencil(uint16 value)
{
    mClear.stencil = value;
}
auto CompositionPass::getClearStencil() const noexcept -> uint16
{
    return mClear.stencil;
}

void CompositionPass::setStencilCheck(bool value)
{
    mStencilState.enabled = value;
}
auto CompositionPass::getStencilCheck() const noexcept -> bool
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
auto CompositionPass::getStencilRefValue() const noexcept -> uint32
{
    return mStencilState.referenceValue;
}
void CompositionPass::setStencilMask(uint32 value)
{
    mStencilState.compareMask = value;
}
auto CompositionPass::getStencilMask() const noexcept -> uint32
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
auto CompositionPass::getStencilTwoSidedOperation() const noexcept -> bool
{
    return mStencilState.twoSidedOperation;
}
void CompositionPass::setQuadFarCorners(bool farCorners, bool farCornersViewSpace)
{
    mQuad.farCorners = farCorners;
    mQuad.farCornersViewSpace = farCornersViewSpace;
}
auto CompositionPass::getQuadFarCorners() const noexcept -> bool
{
    return mQuad.farCorners;
}
auto CompositionPass::getQuadFarCornersViewSpace() const noexcept -> bool
{
    return mQuad.farCornersViewSpace;
}
        
void CompositionPass::setCustomType(std::string_view customType)
{
    mCustomType = customType;
}

auto CompositionPass::getCustomType() const noexcept -> std::string_view 
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
