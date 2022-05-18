/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_SCRIPTTRANSLATOR_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_SCRIPTTRANSLATOR_H

#include "OgrePrerequisites.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreScriptTranslator.hpp"

namespace Ogre {
namespace RTShader {
class RenderState;
class SubRenderState;

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** This class responsible for translating core features of the RT Shader System for
Ogre material scripts.
*/
class SGScriptTranslator : public ScriptTranslator
{
public:
    
    SGScriptTranslator();

    /**
    *@see ScriptTranslator::translate.
    */
    void translate(ScriptCompiler *compiler, const AbstractNodePtr &node) override;

    using ScriptTranslator::getBoolean;
    using ScriptTranslator::getString;
    using ScriptTranslator::getReal;
    using ScriptTranslator::getFloat;
    using ScriptTranslator::getInt;
    using ScriptTranslator::getUInt;
    using ScriptTranslator::getColour;

    /**
    * Returns a sub render state of a given name which has already 
    * been created for material currently being generated by the 
    * script translator
    * @note
    * This function is meant to be used from within the createInstance 
    * function of sub-render state factories to avoid creating multiple
    * sub-render state to the same material.
    *
    * @param typeName The type of the sub-render state to find.
    */
    virtual SubRenderState* getGeneratedSubRenderState(const String& typeName);


protected:
    /**
    * Translates RT Shader System section within a pass context.
    * @param compiler The compiler invoking this translator
    * @param node The current AST node to be translated
    */
    void translatePass(ScriptCompiler *compiler, const AbstractNodePtr &node);

    /**
    * Translates RT Shader System section within a texture_unit context.
    * @param compiler The compiler invoking this translator
    * @param node The current AST node to be translated
    */
    void translateTextureUnit(ScriptCompiler *compiler, const AbstractNodePtr &node);

    /**
    * Adds a newly created subrender state to the material being translated
    * @param newSubRenderState The sub-render state to add
    * @param dstTechniqueSchemeName The technique which the sub render state is associated with
    * @param materialName The material name which the sub render state is associated with
    * @param groupName The material group name which the sub render state is associated with
    * @param passIndex The index of the pass which the sub render state is associated with
    */
    void addSubRenderState(SubRenderState* newSubRenderState, 
        const String& dstTechniqueSchemeName, const String& materialName, 
        const String& groupName, unsigned short passIndex);

            
private:

    //Render state created as a result of the current node being parsed
    RenderState* mGeneratedRenderState;

};

}
}

#endif
