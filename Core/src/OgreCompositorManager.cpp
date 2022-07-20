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

import :CompositionPass;
import :CompositionTargetPass;
import :CompositionTechnique;
import :Compositor;
import :CompositorChain;
import :CompositorInstance;
import :CompositorManager;
import :Exception;
import :HardwareBuffer;
import :Rectangle2D;
import :RenderSystem;
import :RenderTarget;
import :Root;
import :TextureManager;
import :Viewport;

import <memory>;

namespace Ogre {
class CompositorLogic;
class CustomCompositionPass;
class Renderable;

auto CompositorManager::getSingletonPtr() noexcept -> CompositorManager*
{
    return msSingleton;
}
auto CompositorManager::getSingleton() noexcept -> CompositorManager&
{  
    assert( msSingleton );  return ( *msSingleton );  
}//-----------------------------------------------------------------------
CompositorManager::CompositorManager()
    
{
    initialise();

    // Loading order (just after materials)
    mLoadOrder = 110.0f;

    // Resource type
    mResourceType = "Compositor";

    // Register with resource group manager
    ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);

}
//-----------------------------------------------------------------------
CompositorManager::~CompositorManager()
{
    freeChains();
    freePooledTextures(false);

    // Resources cleared by superclass
    // Unregister with resource group manager
    ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    ResourceGroupManager::getSingleton()._unregisterScriptLoader(this);
}
//-----------------------------------------------------------------------
auto CompositorManager::createImpl(std::string_view name, ResourceHandle handle,
    std::string_view group, bool isManual, ManualResourceLoader* loader,
    const NameValuePairList* params) -> Resource*
{
    return new Compositor(this, name, handle, group, isManual, loader);
}
//-----------------------------------------------------------------------
auto CompositorManager::create (std::string_view name, std::string_view group,
                                bool isManual, ManualResourceLoader* loader,
                                const NameValuePairList* createParams) -> CompositorPtr
{
    return static_pointer_cast<Compositor>(createResource(name,group,isManual,loader,createParams));
}
//-----------------------------------------------------------------------
auto CompositorManager::getByName(std::string_view name, std::string_view groupName) const -> CompositorPtr
{
    return static_pointer_cast<Compositor>(getResourceByName(name, groupName));
}
//-----------------------------------------------------------------------
void CompositorManager::initialise()
{
}
//-----------------------------------------------------------------------
auto CompositorManager::getCompositorChain(Viewport *vp) -> CompositorChain *
{
    auto i=mChains.find(vp);
    if(i != mChains.end())
    {
        return i->second;
    }
    else
    {
        auto *chain = new CompositorChain(vp);
        mChains[vp] = chain;
        return chain;
    }
}
//-----------------------------------------------------------------------
auto CompositorManager::hasCompositorChain(const Viewport *vp) const -> bool
{
    return mChains.find(vp) != mChains.end();
}
//-----------------------------------------------------------------------
void CompositorManager::removeCompositorChain(const Viewport *vp)
{
    auto i = mChains.find(vp);
    if (i != mChains.end())
    {
        delete  i->second;
        mChains.erase(i);
    }
}
//-----------------------------------------------------------------------
void CompositorManager::removeAll()
{
    freeChains();
    ResourceManager::removeAll();
}
//-----------------------------------------------------------------------
void CompositorManager::freeChains()
{
    for (auto & mChain : mChains)
    {
        delete  mChain.second;
    }
    mChains.clear();
}
//-----------------------------------------------------------------------
auto CompositorManager::_getTexturedRectangle2D() -> Renderable *
{
    if(!mRectangle)
    {
        /// 2D rectangle, to use for render_quad passes
        mRectangle = ::std::make_unique<Rectangle2D>(true, HardwareBuffer::DYNAMIC_WRITE_ONLY_DISCARDABLE);
    }
    RenderSystem* rs = Root::getSingleton().getRenderSystem();
    Viewport* vp = rs->_getViewport();
    Real hOffset = rs->getHorizontalTexelOffset() / (0.5f * vp->getActualWidth());
    Real vOffset = rs->getVerticalTexelOffset() / (0.5f * vp->getActualHeight());
    mRectangle->setCorners(-1 + hOffset, 1 - vOffset, 1 + hOffset, -1 - vOffset);
    return mRectangle.get();
}
//-----------------------------------------------------------------------
auto CompositorManager::addCompositor(Viewport *vp, std::string_view compositor, int addPosition) -> CompositorInstance *
{
    CompositorPtr comp = getByName(compositor);
    if(!comp)
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, ::std::format("Compositor '{}' not found", compositor));
    CompositorChain *chain = getCompositorChain(vp);
    return chain->addCompositor(comp, addPosition==-1 ? CompositorChain::LAST : (size_t)addPosition);
}
//-----------------------------------------------------------------------
void CompositorManager::removeCompositor(Viewport *vp, std::string_view compositor)
{
    CompositorChain *chain = getCompositorChain(vp);
    size_t pos = chain->getCompositorPosition(compositor);

    if(pos == CompositorChain::NPOS)
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, ::std::format("Compositor '{}' not in chain", compositor));

    chain->removeCompositor(pos);
}
//-----------------------------------------------------------------------
void CompositorManager::setCompositorEnabled(Viewport *vp, std::string_view compositor, bool value)
{
    CompositorChain *chain = getCompositorChain(vp);
    size_t pos = chain->getCompositorPosition(compositor);

    if(pos == CompositorChain::NPOS)
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, ::std::format("Compositor '{}' not in chain", compositor));

    chain->setCompositorEnabled(pos, value);
}
//---------------------------------------------------------------------
void CompositorManager::_reconstructAllCompositorResources()
{
    // In order to deal with shared resources, we have to disable *all* compositors
    // first, that way shared resources will get freed
    using InstVec = std::vector<CompositorInstance *>;
    InstVec instancesToReenable;
    for (auto const& [key, chain] : mChains)
    {
        for (CompositorInstance* inst : chain->getCompositorInstances())
        {
            if (inst->getEnabled())
            {
                inst->setEnabled(false);
                instancesToReenable.push_back(inst);
            }
        }
    }

    //UVs are lost, and will never be reconstructed unless we do them again, now
    if( mRectangle )
        mRectangle->setDefaultUVs();

    for (auto inst : instancesToReenable)
    {
        inst->setEnabled(true);
    }
}
//---------------------------------------------------------------------
auto CompositorManager::getPooledTexture(std::string_view name,
    std::string_view localName,
    uint32 w, uint32 h, PixelFormat f, uint aa, std::string_view aaHint, bool srgb,
    CompositorManager::UniqueTextureSet& texturesAssigned, 
    CompositorInstance* inst, CompositionTechnique::TextureScope scope, TextureType type) -> TexturePtr
{
    OgreAssert(scope != CompositionTechnique::TextureScope::GLOBAL, "Global scope texture can not be pooled");

    TextureDef def{w, h, type, f, aa, aaHint, srgb};

    if (scope == CompositionTechnique::TextureScope::CHAIN)
    {
        StringPair pair = std::make_pair(std::string{inst->getCompositor()->getName()}, std::string{localName});
        TextureDefMap& defMap = mChainTexturesByDef[pair];
        auto it = defMap.find(def);
        if (it != defMap.end())
        {
            return it->second;
        }
        // ok, we need to create a new one
        TexturePtr newTex = TextureManager::getSingleton().createManual(
            name, 
            ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, TextureType::_2D, 
            (uint)w, (uint)h, TextureMipmap{}, f, TextureUsage::RENDERTARGET, nullptr,
            srgb, aa, aaHint);
        defMap.emplace(def, newTex);
        return newTex;
    }

    auto i = mTexturesByDef.emplace(def, TextureList()).first;

    CompositorInstance* previous = inst->getChain()->getPreviousInstance(inst);
    CompositorInstance* next = inst->getChain()->getNextInstance(inst);

    TexturePtr ret;
    TextureList& texList = i->second;
    // iterate over the existing textures and check if we can re-use
    for (auto & tex : texList)
    {
        // check not already used
        if (texturesAssigned.find(tex.get()) == texturesAssigned.end())
        {
            bool allowReuse = true;
            // ok, we didn't use this one already
            // however, there is an edge case where if we re-use a texture
            // which has an 'input previous' pass, and it is chained from another
            // compositor, we can end up trying to use the same texture for both
            // so, never allow a texture with an input previous pass to be 
            // shared with its immediate predecessor in the chain
            if (isInputPreviousTarget(inst, localName))
            {
                // Check whether this is also an input to the output target of previous
                // can't use CompositorInstance::mPreviousInstance, only set up
                // during compile
                if (previous && isInputToOutputTarget(previous, tex))
                    allowReuse = false;
            }
            // now check the other way around since we don't know what order they're bound in
            if (isInputToOutputTarget(inst, localName))
            {
                
                if (next && isInputPreviousTarget(next, tex))
                    allowReuse = false;
            }
            
            if (allowReuse)
            {
                ret = tex;
                break;
            }

        }
    }

    if (!ret)
    {
        // ok, we need to create a new one
        ret = TextureManager::getSingleton().createManual(
            name, 
            ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, TextureType::_2D, 
            w, h, TextureMipmap{}, f, TextureUsage::RENDERTARGET, nullptr,
            srgb, aa, aaHint); 

        texList.push_back(ret);

    }

    // record that we used this one in the requester's list
    texturesAssigned.insert(ret.get());


    return ret;
}
//---------------------------------------------------------------------
auto CompositorManager::isInputPreviousTarget(CompositorInstance* inst, std::string_view localName) -> bool
{
    const CompositionTechnique::TargetPasses& passes = inst->getTechnique()->getTargetPasses();
    for (auto tp : passes)
    {
        if (tp->getInputMode() == CompositionTargetPass::InputMode::PREVIOUS &&
            tp->getOutputName() == localName)
        {
            return true;
        }

    }

    return false;

}
//---------------------------------------------------------------------
auto CompositorManager::isInputPreviousTarget(CompositorInstance* inst, TexturePtr tex) -> bool
{
    const CompositionTechnique::TargetPasses& passes = inst->getTechnique()->getTargetPasses();
    for (auto tp : passes)
    {
        if (tp->getInputMode() == CompositionTargetPass::InputMode::PREVIOUS)
        {
            // Don't have to worry about an MRT, because no MRT can be input previous
            TexturePtr t = inst->getTextureInstance(tp->getOutputName(), 0);
            if (t && t.get() == tex.get())
                return true;
        }

    }

    return false;

}
//---------------------------------------------------------------------
auto CompositorManager::isInputToOutputTarget(CompositorInstance* inst, std::string_view localName) -> bool
{
    for (CompositionTargetPass* tp = inst->getTechnique()->getOutputTargetPass();
        CompositionPass* p : tp->getPasses())
    {
        for (size_t i = 0; i < p->getNumInputs(); ++i)
        {
            if (p->getInput(i).name == localName)
                return true;
        }
    }

    return false;

}
//---------------------------------------------------------------------()
auto CompositorManager::isInputToOutputTarget(CompositorInstance* inst, TexturePtr tex) -> bool
{
    for (CompositionTargetPass* tp = inst->getTechnique()->getOutputTargetPass();
            CompositionPass* p : tp->getPasses())
    {
        for (size_t i = 0; i < p->getNumInputs(); ++i)
        {
            TexturePtr t = inst->getTextureInstance(p->getInput(i).name, 0);
            if (t && t.get() == tex.get())
                return true;
        }
    }

    return false;

}
//---------------------------------------------------------------------
void CompositorManager::freePooledTextures(bool onlyIfUnreferenced)
{
    if (onlyIfUnreferenced)
    {
        for (auto& [key, texList] : mTexturesByDef)
        {
            for (auto j = texList.begin(); j != texList.end();)
            {
                // if the resource system, plus this class, are the only ones to have a reference..
                // NOTE: any material references will stop this texture getting freed (e.g. compositor demo)
                // until this routine is called again after the material no longer references the texture
                if (j->use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS + 1)
                {
                    TextureManager::getSingleton().remove((*j)->getHandle());
                    j = texList.erase(j);
                }
                else
                    ++j;
            }
        }
        for (auto& [key, texMap] : mChainTexturesByDef)
        {
            for (auto j = texMap.begin(); j != texMap.end();) 
            {
                const TexturePtr& tex = j->second;
                if (tex.use_count() == ResourceGroupManager::RESOURCE_SYSTEM_NUM_REFERENCE_COUNTS + 1)
                {
                    TextureManager::getSingleton().remove(tex->getHandle());
                    texMap.erase(j++);
                }
                else
                    ++j;
            }
        }
    }
    else
    {
        // destroy all
        mTexturesByDef.clear();
        mChainTexturesByDef.clear();
    }

}
//---------------------------------------------------------------------
void CompositorManager::registerCompositorLogic(std::string_view name, CompositorLogic* logic)
{   
    OgreAssert(!name.empty(), "Compositor logic name must not be empty");
    if (mCompositorLogics.find(name) != mCompositorLogics.end())
    {
        OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM,
            ::std::format("Compositor logic '{}' already exists.", name ),
            "CompositorManager::registerCompositorLogic");
    }
    mCompositorLogics[name] = logic;
}
//---------------------------------------------------------------------
void CompositorManager::unregisterCompositorLogic(std::string_view name)
{
    auto itor = mCompositorLogics.find(name);
    if( itor == mCompositorLogics.end() )
    {
        OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
            ::std::format("Compositor logic '{}' not registered.", name ),
            "CompositorManager::unregisterCompositorLogic");
    }

    mCompositorLogics.erase( itor );
}
//---------------------------------------------------------------------
auto CompositorManager::getCompositorLogic(std::string_view name) -> CompositorLogic*
{
    auto it = mCompositorLogics.find(name);
    if (it == mCompositorLogics.end())
    {
        OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
            ::std::format("Compositor logic '{}' not registered.", name ),
            "CompositorManager::getCompositorLogic");
    }
    return it->second;
}
//---------------------------------------------------------------------
auto CompositorManager::hasCompositorLogic(std::string_view name) -> bool
{
	return mCompositorLogics.find(name) != mCompositorLogics.end();
}
//---------------------------------------------------------------------
void CompositorManager::registerCustomCompositionPass(std::string_view name, CustomCompositionPass* logic)
{   
    OgreAssert(!name.empty(), "Compositor pass name must not be empty");
    if (mCustomCompositionPasses.find(name) != mCustomCompositionPasses.end())
    {
        OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM,
            ::std::format("Custom composition pass  '{}' already exists.", name ),
            "CompositorManager::registerCustomCompositionPass");
    }
    mCustomCompositionPasses[name] = logic;
}
//---------------------------------------------------------------------
void CompositorManager::unregisterCustomCompositionPass(std::string_view name)
{	
	auto itor = mCustomCompositionPasses.find(name);
	if( itor == mCustomCompositionPasses.end() )
	{
		OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
			::std::format("Custom composition pass '{}' not registered.", name ),
			"CompositorManager::unRegisterCustomCompositionPass");
	}
	mCustomCompositionPasses.erase( itor );
}
//---------------------------------------------------------------------
auto CompositorManager::hasCustomCompositionPass(std::string_view name) -> bool
{
	return mCustomCompositionPasses.find(name) != mCustomCompositionPasses.end();
}
//---------------------------------------------------------------------
auto CompositorManager::getCustomCompositionPass(std::string_view name) -> CustomCompositionPass*
{
    auto it = mCustomCompositionPasses.find(name);
    if (it == mCustomCompositionPasses.end())
    {
        OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
            ::std::format("Custom composition pass '{}' not registered.", name ),
            "CompositorManager::getCustomCompositionPass");
    }
    return it->second;
}
//-----------------------------------------------------------------------
void CompositorManager::_relocateChain( Viewport* sourceVP, Viewport* destVP )
{
    if (sourceVP != destVP)
    {
        CompositorChain *chain = getCompositorChain(sourceVP);
        Ogre::RenderTarget *srcTarget = sourceVP->getTarget();
        Ogre::RenderTarget *dstTarget = destVP->getTarget();
        if (srcTarget != dstTarget)
        {
            srcTarget->removeListener(chain);
            dstTarget->addListener(chain);
        }
        chain->_notifyViewport(destVP);
        mChains.erase(sourceVP);
        mChains[destVP] = chain;
    }
}
//-----------------------------------------------------------------------
}
