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
#include <set>
#include <utility>

#include "OgreCompositionTechnique.hpp"
#include "OgreCompositor.hpp"
#include "OgreCompositorInstance.hpp"
#include "OgreException.hpp"
#include "OgreHardwarePixelBuffer.hpp"
#include "OgreIteratorWrapper.hpp"
#include "OgreLogManager.hpp"
#include "OgrePixelFormat.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderTarget.hpp"
#include "OgreRenderTexture.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreRoot.hpp"
#include "OgreStringConverter.hpp"
#include "OgreTexture.hpp"
#include "OgreTextureManager.hpp"

namespace Ogre {
class ResourceManager;

//-----------------------------------------------------------------------
Compositor::Compositor(ResourceManager* creator, const String& name, ResourceHandle handle,
            const String& group, bool isManual, ManualResourceLoader* loader):
    Resource(creator, name, handle, group, isManual, loader)
    
{
}
//-----------------------------------------------------------------------

Compositor::~Compositor()
{
    removeAllTechniques();
    // have to call this here reather than in Resource destructor
    // since calling virtual methods in base destructors causes crash
    unload(); 
}
//-----------------------------------------------------------------------
CompositionTechnique *Compositor::createTechnique()
{
    auto *t = new CompositionTechnique(this);
    mTechniques.push_back(t);
    mCompilationRequired = true;
    return t;
}
//-----------------------------------------------------------------------

void Compositor::removeTechnique(size_t index)
{
    assert (index < mTechniques.size() && "Index out of bounds.");
    auto i = mTechniques.begin() + index;
    delete (*i);
    mTechniques.erase(i);
    mSupportedTechniques.clear();
    mCompilationRequired = true;
}
//-----------------------------------------------------------------------
void Compositor::removeAllTechniques()
{
    for (auto & mTechnique : mTechniques)
    {
        delete mTechnique;
    }
    mTechniques.clear();
    mSupportedTechniques.clear();
    mCompilationRequired = true;
}
//-----------------------------------------------------------------------
Compositor::TechniqueIterator Compositor::getTechniqueIterator()
{
    return {mTechniques.begin(), mTechniques.end()};
}
//-----------------------------------------------------------------------

Compositor::TechniqueIterator Compositor::getSupportedTechniqueIterator()
{
    return {mSupportedTechniques.begin(), mSupportedTechniques.end()};
}
//-----------------------------------------------------------------------
void Compositor::loadImpl()
{
    // compile if required
    if (mCompilationRequired)
        compile();

    createGlobalTextures();
}
//-----------------------------------------------------------------------
void Compositor::unloadImpl()
{
    freeGlobalTextures();
}
//-----------------------------------------------------------------------
size_t Compositor::calculateSize() const
{
    return 0;
}

//-----------------------------------------------------------------------
void Compositor::compile()
{
    /// Sift out supported techniques
    mSupportedTechniques.clear();

    for (auto t : mTechniques)
    {
        // Allow texture support with degraded pixel format
        if (t->isSupported(true))
        {
            mSupportedTechniques.push_back(t);
        }
    }

    if (mSupportedTechniques.empty())
        LogManager::getSingleton().logError(::std::format("Compositor '{}' has no supported techniques", getName() ));

    mCompilationRequired = false;
}
//---------------------------------------------------------------------
CompositionTechnique* Compositor::getSupportedTechnique(const String& schemeName)
{
    for(auto & mSupportedTechnique : mSupportedTechniques)
    {
        if (mSupportedTechnique->getSchemeName() == schemeName)
        {
            return mSupportedTechnique;
        }
    }

    // didn't find a matching one
    for(auto & mSupportedTechnique : mSupportedTechniques)
    {
        if (mSupportedTechnique->getSchemeName().empty())
        {
            return mSupportedTechnique;
        }
    }

    return nullptr;

}
//-----------------------------------------------------------------------
void Compositor::createGlobalTextures()
{
    static size_t dummyCounter = 0;
    if (mSupportedTechniques.empty())
        return;

    //To make sure that we are consistent, it is demanded that all composition
    //techniques define the same set of global textures.

    using StringSet = std::set<String>;
    StringSet globalTextureNames;

    //Initialize global textures from first supported technique
    CompositionTechnique* firstTechnique = mSupportedTechniques[0];

    for (const CompositionTechnique::TextureDefinitions& tdefs = firstTechnique->getTextureDefinitions();
            CompositionTechnique::TextureDefinition* def : tdefs)
    {
        if (def->scope == CompositionTechnique::TS_GLOBAL) 
        {
            //Check that this is a legit global texture
            OgreAssert(def->refCompName.empty(), "Global compositor texture definition can not be a reference");
            OgreAssert(def->width && def->height, "Global compositor texture definition must have absolute size");
            if (def->pooled) 
            {
                LogManager::getSingleton().logWarning("Pooling global compositor textures has no effect");
            }
            globalTextureNames.insert(def->name);

            //TODO GSOC : Heavy copy-pasting from CompositorInstance. How to we solve it?

            /// Make the tetxure
            RenderTarget* rendTarget;
            if (def->formatList.size() > 1)
            {
                String MRTbaseName = ::std::format("mrt/c{}/{}/{}",
                    StringConverter::toString(dummyCounter++),
                    mName,
                    def->name);
                MultiRenderTarget* mrt = 
                    Root::getSingleton().getRenderSystem()->createMultiRenderTarget(MRTbaseName);
                mGlobalMRTs[def->name] = mrt;

                // create and bind individual surfaces
                size_t atch = 0;
                for (auto p = def->formatList.begin(); 
                    p != def->formatList.end(); ++p, ++atch)
                {

                    String texname = ::std::format("{}/{}", MRTbaseName, atch);
                    TexturePtr tex;
                    
                    tex = TextureManager::getSingleton().createManual(
                            texname, 
                            ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 
                            (uint)def->width, (uint)def->height, 0, *p, TU_RENDERTARGET, nullptr, 
                            def->hwGammaWrite && !PixelUtil::isFloatingPoint(*p), def->fsaa); 
                    
                    RenderTexture* rt = tex->getBuffer()->getRenderTarget();
                    rt->setAutoUpdated(false);
                    mrt->bindSurface(atch, rt);

                    // Also add to local textures so we can look up
                    String mrtLocalName = CompositorInstance::getMRTTexLocalName(def->name, atch);
                    mGlobalTextures[mrtLocalName] = tex;
                    
                }

                rendTarget = mrt;
            }
            else
            {
                String texName =  ::std::format("c{}/{}/{}",
                    StringConverter::toString(dummyCounter++),
                    mName,  def->name);
                
                // space in the name mixup the cegui in the compositor demo
                // this is an auto generated name - so no spaces can't hart us.
                std::ranges::replace(texName, ' ', '_' ); 

                TexturePtr tex;
                tex = TextureManager::getSingleton().createManual(
                    texName, 
                    ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 
                    (uint)def->width, (uint)def->height, 0, def->formatList[0], TU_RENDERTARGET, nullptr,
                    def->hwGammaWrite && !PixelUtil::isFloatingPoint(def->formatList[0]), def->fsaa); 
                

                rendTarget = tex->getBuffer()->getRenderTarget();
                mGlobalTextures[def->name] = tex;
            }

            //Set DepthBuffer pool for sharing
            rendTarget->setDepthBufferPool( def->depthBufferId );
        }
    }

    //Validate that all other supported techniques expose the same set of global textures.
    for (size_t i=1; i<mSupportedTechniques.size(); i++)
    {
        CompositionTechnique* technique = mSupportedTechniques[i];
        bool isConsistent = true;
        size_t numGlobals = 0;
        for (const CompositionTechnique::TextureDefinitions& tdefs2 = technique->getTextureDefinitions();
                CompositionTechnique::TextureDefinition* texDef : tdefs2)
        {
            if (texDef->scope == CompositionTechnique::TS_GLOBAL) 
            {
                if (globalTextureNames.find(texDef->name) == globalTextureNames.end()) 
                {
                    isConsistent = false;
                    break;
                }
                numGlobals++;
            }
        }
        if (numGlobals != globalTextureNames.size())
            isConsistent = false;

        OgreAssert(isConsistent, "Different composition techniques define different global textures");
    }
    
}
//-----------------------------------------------------------------------
void Compositor::freeGlobalTextures()
{
    for (auto const& i : mGlobalTextures)
    {
        TextureManager::getSingleton().remove(i.second);
    }
    mGlobalTextures.clear();

    for (auto const& mrti : mGlobalMRTs)
    {
        // remove MRT
        Root::getSingleton().getRenderSystem()->destroyRenderTarget(mrti.second->getName());
    }
    mGlobalMRTs.clear();

}
//-----------------------------------------------------------------------
const String& Compositor::getTextureInstanceName(const String& name, size_t mrtIndex)
{
    return getTextureInstance(name, mrtIndex)->getName();
}
//-----------------------------------------------------------------------       
const TexturePtr& Compositor::getTextureInstance(const String& name, size_t mrtIndex)
{
    //Try simple texture
    auto i = mGlobalTextures.find(name);
    if(i != mGlobalTextures.end())
    {
        return i->second;
    }
    //Try MRT
    String mrtName = CompositorInstance::getMRTTexLocalName(name, mrtIndex);
    i = mGlobalTextures.find(mrtName);
    if(i != mGlobalTextures.end())
    {
        return i->second;
    }

    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Non-existent global texture name", 
        "Compositor::getTextureInstance");
        
}
//---------------------------------------------------------------------
RenderTarget* Compositor::getRenderTarget(const String& name, int slice)
{
    // try simple texture
    auto i = mGlobalTextures.find(name);
    if(i != mGlobalTextures.end())
        return i->second->getBuffer(slice)->getRenderTarget();

    // try MRTs
    auto mi = mGlobalMRTs.find(name);
    if (mi != mGlobalMRTs.end())
        return mi->second;
    else
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Non-existent global texture name", 
            "Compositor::getRenderTarget");
}
//---------------------------------------------------------------------
}
