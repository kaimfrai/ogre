#ifndef OGRE_COMPONENTS_BITES_SGTECHNIQUERESOLVERLISTENER_H
#define OGRE_COMPONENTS_BITES_SGTECHNIQUERESOLVERLISTENER_H

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

#pragma once

#include "OgreMaterialManager.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreTechnique.hpp"

namespace Ogre {
    class Material;
    class Renderable;

    namespace RTShader {
        class ShaderGenerator;
    }  // namespace RTShader
}  // namespace Ogre

namespace OgreBites {
/** \addtogroup Optional
*  @{
*/
/** \addtogroup Bites
*  @{
*/
/** Default implementation of a Listener to use with the Ogre::RTShader system.
    When a target scheme callback is invoked with the shader generator scheme it tries to create an equivalent shader
    based technique based on the default technique of the given material.
*/
class SGTechniqueResolverListener : public Ogre::MaterialManager::Listener {
public:
    explicit SGTechniqueResolverListener(Ogre::RTShader::ShaderGenerator* pShaderGenerator);

    /** This is the hook point where shader based technique will be created.
        It will be called whenever the material manager won't find appropriate technique
        that satisfy the target scheme name. If the scheme name is out target RT Shader System
        scheme name we will try to create shader generated technique for it.
    */
    Ogre::Technique* handleSchemeNotFound(unsigned short schemeIndex,
                                          const Ogre::String& schemeName,
                                          Ogre::Material* originalMaterial, unsigned short lodIndex,
                                          const Ogre::Renderable* rend) override;

    bool afterIlluminationPassesCreated(Ogre::Technique* tech) noexcept override;

    bool beforeIlluminationPassesCleared(Ogre::Technique* tech) noexcept override;

protected:
    Ogre::RTShader::ShaderGenerator* mShaderGenerator; // The shader generator instance.
};
/** @} */
/** @} */
}

#endif
