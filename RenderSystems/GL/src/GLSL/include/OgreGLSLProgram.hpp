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
module Ogre.RenderSystems.GL:GLSL.SLProgram;

import :GpuProgram;

import Ogre.Core;
import Ogre.RenderSystems.GLSupport;

namespace Ogre {
    namespace GLSL {
    class GLSLProgram : public GLSLShaderCommon, public GLGpuProgramBase
    {
    public:
        GLSLProgram(ResourceManager* creator, 
            std::string_view name, ResourceHandle handle,
            std::string_view group, bool isManual, ManualResourceLoader* loader);
        ~GLSLProgram() override;

        void attachToProgramObject( const uint programObject ) override;
        void detachFromProgramObject( const uint programObject ) override;

        /// Overridden from GpuProgram
        auto getLanguage() const noexcept -> std::string_view override;

        auto getPassTransformStates() const noexcept -> bool override {
            return mPassFFPStates;
        }
        auto getPassSurfaceAndLightStates() const noexcept -> bool override {
            return mPassFFPStates;
        }
        auto getPassFogStates() const noexcept -> bool override {
            return mPassFFPStates;
        }

        /** Returns the operation type that this geometry program expects to
            receive as input
        */
        auto getInputOperationType() const
        noexcept -> RenderOperation::OperationType { return mInputOperationType; }
        /** Returns the operation type that this geometry program will emit
        */
        auto getOutputOperationType() const
        noexcept -> RenderOperation::OperationType { return mOutputOperationType; }
        /** Returns the maximum number of vertices that this geometry program can
            output in a single run
        */
        auto getMaxOutputVertices() const noexcept -> int { return mMaxOutputVertices; }

        /** Sets the operation type that this geometry program expects to receive
        */
        void setInputOperationType(RenderOperation::OperationType operationType)
        { mInputOperationType = operationType; }
        /** Set the operation type that this geometry program will emit
        */
        void setOutputOperationType(RenderOperation::OperationType operationType)
        {
            using enum RenderOperation::OperationType;
            switch (operationType)
            {
            case POINT_LIST:
            case LINE_STRIP:
            case TRIANGLE_STRIP:
                break;
            default:
                OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR,
                            "Geometry shader output operation type can only be point list,"
                            "line strip or triangle strip");
            }

            mOutputOperationType = operationType;
        }
        /** Set the maximum number of vertices that a single run of this geometry program
            can emit.
        */
        void setMaxOutputVertices(int maxOutputVertices)
        { mMaxOutputVertices = maxOutputVertices; }

        void bindProgram() override;
        void unbindProgram() override;
        void bindProgramParameters(GpuProgramParametersSharedPtr params, GpuParamVariability mask) override;
        auto isAttributeValid(VertexElementSemantic semantic, uint index) -> bool override;
    protected:
        void loadFromSource() override;
        /// Internal unload implementation, must be implemented by subclasses
        void unloadHighLevelImpl() override;

        /// Populate the passed parameters with name->index map, must be overridden
        void buildConstantDefinitions() override;

        // legacy GL_EXT_geometry_shader4 functionality
        RenderOperation::OperationType mInputOperationType{RenderOperation::OperationType::TRIANGLE_LIST};
        RenderOperation::OperationType mOutputOperationType{RenderOperation::OperationType::TRIANGLE_LIST};
        int mMaxOutputVertices{3};
        bool mPassFFPStates;
    };
    }
}
