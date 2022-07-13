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

#ifndef OGRE_CORE_SCRIPTTRANSLATOR_H
#define OGRE_CORE_SCRIPTTRANSLATOR_H

#include <cstddef>
#include <vector>

#include "OgreBlendMode.hpp"
#include "OgreCommon.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderSystem.hpp"

namespace Ogre{
class ColourValue;
class Matrix4;
    class AbstractNode;
    class ObjectAbstractNode;
    using AbstractNodePtr = SharedPtr<AbstractNode>;
    using AbstractNodeList = std::list<AbstractNodePtr>;
    using AbstractNodeListPtr = SharedPtr<AbstractNodeList>;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Script
    *  @{
    */
    /** This class translates script AST (abstract syntax tree) into
     *  Ogre resources. It defines a common interface for subclasses
     *  which perform the actual translation.
     */

    class ScriptTranslator : public ScriptTranslatorAlloc
    {
    public:
        /**
         * This function translates the given node into Ogre resource(s).
         * @param compiler The compiler invoking this translator
         * @param node The current AST node to be translated
         */
        virtual void translate(ScriptCompiler *compiler, const AbstractNodePtr &node) = 0;
    protected:
        // needs virtual destructor
        virtual ~ScriptTranslator() = default;
        /// Retrieves a new translator from the factories and uses it to process the give node
        static void processNode(ScriptCompiler *compiler, const AbstractNodePtr &node);

        /// Retrieves the node iterator at the given index
        static auto getNodeAt(const AbstractNodeList &nodes, size_t index) -> AbstractNodeList::const_iterator;
        /// Converts the node to a boolean and returns true if successful
        static auto getBoolean(const AbstractNodePtr &node, bool *result) -> bool;
        /// Converts the node to a string and returns true if successful
        static auto getString(const AbstractNodePtr &node, String *result) -> bool;
        /// Converts the node to a Real and returns true if successful
        static auto getReal(const AbstractNodePtr& node, Real* result) -> bool
        {
            return getFloat(node, result);
        }
        /// Converts the node to a float and returns true if successful
        static auto getFloat(const AbstractNodePtr &node, float *result) -> bool;
        /// Converts the node to a float and returns true if successful
        static auto getDouble(const AbstractNodePtr &node, double *result) -> bool;
        /// Converts the node to an integer and returns true if successful
        static auto getInt(const AbstractNodePtr &node, int *result) -> bool; 
        /// Converts the node to an unsigned integer and returns true if successful
        static auto getUInt(const AbstractNodePtr &node, uint32 *result) -> bool; 
        /// Converts the range of nodes to a ColourValue and returns true if successful
        static auto getColour(AbstractNodeList::const_iterator i, AbstractNodeList::const_iterator end, ColourValue *result, int maxEntries = 4) -> bool;
        /// Converts the node to a SceneBlendFactor enum class and returns true if successful
        static auto getSceneBlendFactor(const AbstractNodePtr &node, SceneBlendFactor *sbf) -> bool;
        /// Converts the node to a CompareFunction enum class and returns true if successful
        static auto getCompareFunction(const AbstractNodePtr &node, CompareFunction *func) -> bool;
        /// Converts the range of nodes to a Matrix4 and returns true if successful
        static auto getMatrix4(AbstractNodeList::const_iterator i, AbstractNodeList::const_iterator end, Matrix4 *m) -> bool;

        /// read count values from the AbstractNodeList into vals. Fill with default value if AbstractNodeList is shorter then count
        static auto getVector(AbstractNodeList::const_iterator i, AbstractNodeList::const_iterator end, std::vector<int>& vals, size_t count) -> bool;
        /// @overload
        static auto getVector(AbstractNodeList::const_iterator i, AbstractNodeList::const_iterator end, std::vector<float>& vals, size_t count) -> bool;
        /// Converts the node to a StencilOperation enum class and returns true if successful
        static auto getStencilOp(const AbstractNodePtr &node, StencilOperation *op) -> bool; 
        /// Converts the node to a GpuConstantType enum class and returns true if successful
        static auto getConstantType(AbstractNodeList::const_iterator i, GpuConstantType *op) -> bool; 

        template<typename T>
        friend auto getValue(const AbstractNodePtr &node, T& result) -> bool;
    };

    /** The ScriptTranslatorManager manages the lifetime and access to
     *  script translators. You register these managers with the
     *  ScriptCompilerManager tied to specific object types.
     *  Each manager may manage multiple types.
     */
    class ScriptTranslatorManager : public ScriptTranslatorAlloc
    {
    public:
        // required - virtual destructor
        virtual ~ScriptTranslatorManager() = default;

        /// Returns a manager for the given object abstract node, or null if it is not supported
        virtual auto getTranslator(const AbstractNodePtr&) -> ScriptTranslator * = 0;
    };
    /** @} */
    /** @} */
}

#endif
