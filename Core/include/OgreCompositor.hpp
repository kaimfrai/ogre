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
#ifndef OGRE_CORE_COMPOSITOR_H
#define OGRE_CORE_COMPOSITOR_H

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "OgreCommon.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreResource.hpp"
#include "OgreSharedPtr.hpp"

namespace Ogre {
    template <typename T> class VectorIterator;
class CompositionTechnique;
class MultiRenderTarget;
class RenderTarget;
class ResourceManager;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */
    /** Class representing a Compositor object. Compositors provide the means 
        to flexibly "composite" the final rendering result from multiple scene renders
        and intermediate operations like rendering fullscreen quads. This makes 
        it possible to apply postfilter effects, HDRI postprocessing, and shadow 
        effects to a Viewport.
     */
    class Compositor: public Resource
    {
    public:
        Compositor(ResourceManager* creator, const String& name, ResourceHandle handle,
            const String& group, bool isManual = false, ManualResourceLoader* loader = nullptr);
        ~Compositor() override;
        
        /// Data types for internal lists
        using Techniques = std::vector<CompositionTechnique *>;
        using TechniqueIterator = VectorIterator<Techniques>;
        
        /** Create a new technique, and return a pointer to it.
        */
        auto createTechnique() -> CompositionTechnique *;
        
        /** Remove a technique. It will also be destroyed.
        */
        void removeTechnique(size_t idx);
        
        /** Get a technique.
        */
        auto getTechnique(size_t idx) const -> CompositionTechnique * { return mTechniques.at(idx); }
        
        /** Get the number of techniques.
        */
        auto getNumTechniques() const -> size_t { return mTechniques.size(); }
        
        /** Remove all techniques
        */
        void removeAllTechniques();
        
        /** Get an iterator over the Techniques in this compositor. */
        auto getTechniqueIterator() -> TechniqueIterator;
        
        /** Get a supported technique.
        @remarks
            The supported technique list is only available after this compositor has been compiled,
            which typically happens on loading it. Therefore, if this method returns
            an empty list, try calling Compositor::load.
        */
        auto getSupportedTechnique(size_t idx) const -> CompositionTechnique * { return mSupportedTechniques.at(idx); }
        
        /** Get the number of supported techniques.
        @remarks
            The supported technique list is only available after this compositor has been compiled,
            which typically happens on loading it. Therefore, if this method returns
            an empty list, try calling Compositor::load.
        */
        auto getNumSupportedTechniques() const -> size_t { return mSupportedTechniques.size(); }
        
        /** Gets an iterator over all the Techniques which are supported by the current card. 
        @remarks
            The supported technique list is only available after this compositor has been compiled,
            which typically happens on loading it. Therefore, if this method returns
            an empty list, try calling Compositor::load.
        */
        auto getSupportedTechniqueIterator() -> TechniqueIterator;

        /** Get a pointer to a supported technique for a given scheme. 
        @remarks
            If there is no specific supported technique with this scheme name, 
            then the first supported technique with no specific scheme will be returned.
        @param schemeName The scheme name you are looking for. Blank means to 
            look for techniques with no scheme associated
        */
        auto getSupportedTechnique(const String& schemeName = BLANKSTRING) -> CompositionTechnique *;

        /** Get the instance name for a global texture.
        @param name The name of the texture in the original compositor definition
        @param mrtIndex If name identifies a MRT, which texture attachment to retrieve
        @return The instance name for the texture, corresponds to a real texture
        */
        auto getTextureInstanceName(const String& name, size_t mrtIndex) -> const String&;

        /** Get the instance of a global texture.
        @param name The name of the texture in the original compositor definition
        @param mrtIndex If name identifies a MRT, which texture attachment to retrieve
        @return The texture pointer, corresponds to a real texture
        */
        auto getTextureInstance(const String& name, size_t mrtIndex) -> const TexturePtr&;

        /** Get the render target for a given render texture name. 
        @remarks
            You can use this to add listeners etc, but do not use it to update the
            targets manually or any other modifications, the compositor instance 
            is in charge of this.
        */
        auto getRenderTarget(const String& name, int slice = 0) -> RenderTarget*;

    protected:
        /// @copydoc Resource::loadImpl
        void loadImpl() override;

        /// @copydoc Resource::unloadImpl
        void unloadImpl() override;
        /// @copydoc Resource::calculateSize
        auto calculateSize() const -> size_t override;
        
        /** Check supportedness of techniques.
         */
        void compile();
    private:
        Techniques mTechniques;
        Techniques mSupportedTechniques;
        
        /// Compilation required
        /// This is set if the techniques change and the supportedness of techniques has to be
        /// re-evaluated.
        bool mCompilationRequired{true};

        /** Create global rendertextures.
        */
        void createGlobalTextures();
        
        /** Destroy global rendertextures.
        */
        void freeGlobalTextures();

        //TODO GSOC : These typedefs are duplicated from CompositorInstance. Solve?
        /// Map from name->local texture
        using GlobalTextureMap = std::map<String, TexturePtr>;
        GlobalTextureMap mGlobalTextures;
        /// Store a list of MRTs we've created
        using GlobalMRTMap = std::map<String, MultiRenderTarget *>;
        GlobalMRTMap mGlobalMRTs;
    };
    /** @} */
    /** @} */
}

#endif
