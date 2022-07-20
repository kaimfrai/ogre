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

import :CompositionTargetPass;
import :CompositionTechnique;
import :Exception;
import :RenderSystem;
import :RenderSystemCapabilities;
import :Root;
import :TextureManager;

import <string>;

namespace Ogre {
class Compositor;

CompositionTechnique::CompositionTechnique(Compositor *parent):
    mParent(parent)
{
    mOutputTarget = ::std::make_unique<CompositionTargetPass>(this);
}
//-----------------------------------------------------------------------
CompositionTechnique::~CompositionTechnique()
{
    removeAllTextureDefinitions();
    removeAllTargetPasses();
}
//-----------------------------------------------------------------------
auto CompositionTechnique::createTextureDefinition(std::string_view name) -> CompositionTechnique::TextureDefinition *
{
    if(getTextureDefinition(name))
        OGRE_EXCEPT(ExceptionCodes::DUPLICATE_ITEM, ::std::format("Texture '{}' already exists", name));

    auto *t = new TextureDefinition();
    t->name = name;
    mTextureDefinitions.push_back(t);
    return t;
}
//-----------------------------------------------------------------------

void CompositionTechnique::removeTextureDefinition(size_t index)
{
    assert (index < mTextureDefinitions.size() && "Index out of bounds.");
    auto i = mTextureDefinitions.begin() + index;
    delete (*i);
    mTextureDefinitions.erase(i);
}
//---------------------------------------------------------------------
auto CompositionTechnique::getTextureDefinition(std::string_view name) const -> CompositionTechnique::TextureDefinition *
{
    for (auto const& i : mTextureDefinitions)
    {
        if (i->name == name)
            return i;
    }

    return nullptr;

}
//-----------------------------------------------------------------------
void CompositionTechnique::removeAllTextureDefinitions()
{
    for (auto & mTextureDefinition : mTextureDefinitions)
    {
        delete mTextureDefinition;
    }
    mTextureDefinitions.clear();
}

//-----------------------------------------------------------------------
auto CompositionTechnique::createTargetPass() -> CompositionTargetPass *
{
    auto *t = new CompositionTargetPass(this);
    mTargetPasses.push_back(t);
    return t;
}
//-----------------------------------------------------------------------

void CompositionTechnique::removeTargetPass(size_t index)
{
    assert (index < mTargetPasses.size() && "Index out of bounds.");
    auto i = mTargetPasses.begin() + index;
    delete (*i);
    mTargetPasses.erase(i);
}
//-----------------------------------------------------------------------
void CompositionTechnique::removeAllTargetPasses()
{
    for (auto & mTargetPasse : mTargetPasses)
    {
        delete mTargetPasse;
    }
    mTargetPasses.clear();
}

//-----------------------------------------------------------------------
auto CompositionTechnique::isSupported(bool acceptTextureDegradation) -> bool
{
    // A technique is supported if all materials referenced have a supported
    // technique, and the intermediate texture formats requested are supported
    // Material support is a cast-iron requirement, but if no texture formats 
    // are directly supported we can let the rendersystem create the closest 
    // match for the least demanding technique
    

    // Check output target pass is supported
    if (!mOutputTarget->_isSupported())
    {
        return false;
    }

    // Check all target passes is supported
    for (auto targetPass : mTargetPasses)
    {
        if (!targetPass->_isSupported())
        {
            return false;
        }
    }

    TextureManager& texMgr = TextureManager::getSingleton();
    for (auto td : mTextureDefinitions)
    {
        // Firstly check MRTs
        if (td->formatList.size() > 
            Root::getSingleton().getRenderSystem()->getCapabilities()->getNumMultiRenderTargets())
        {
            return false;
        }


        for (auto const& pfi : td->formatList)
        {
            // Check whether equivalent supported
            // Need a format which is the same number of bits to pass
            bool accepted = texMgr.isEquivalentFormatSupported(td->type, pfi, TextureUsage::RENDERTARGET);
            if(!accepted && acceptTextureDegradation)
            {
                // Don't care about exact format so long as something is supported
                accepted = texMgr.getNativeFormat(td->type, pfi, TextureUsage::RENDERTARGET) != PixelFormat::UNKNOWN;
            }

            if(!accepted)
                return false;
        }

        //Check all render targets have same number of bits
        if( !Root::getSingleton().getRenderSystem()->getCapabilities()->
            hasCapability( Capabilities::MRT_DIFFERENT_BIT_DEPTHS ) && !td->formatList.empty() )
        {
            PixelFormat nativeFormat = texMgr.getNativeFormat( td->type, td->formatList.front(),
                                                                TextureUsage::RENDERTARGET );
            size_t nativeBits = PixelUtil::getNumElemBits( nativeFormat );
            for( auto pfi = td->formatList.begin()+1;
                    pfi != td->formatList.end(); ++pfi )
            {
                PixelFormat nativeTmp = texMgr.getNativeFormat( td->type, *pfi, TextureUsage::RENDERTARGET );
                if( PixelUtil::getNumElemBits( nativeTmp ) != nativeBits )
                {
                    return false;
                }
            }
        }
    }
    
    // Must be ok
    return true;
}
//-----------------------------------------------------------------------
auto CompositionTechnique::getParent() -> Compositor *
{
    return mParent;
}
//---------------------------------------------------------------------
void CompositionTechnique::setSchemeName(std::string_view schemeName)
{
    mSchemeName = schemeName;
}

}
