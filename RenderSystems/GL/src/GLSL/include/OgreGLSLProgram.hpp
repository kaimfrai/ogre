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
#ifndef OGRE_RENDERSYSTEMS_GL_GLSL_PROGRAM_H
#define OGRE_RENDERSYSTEMS_GL_GLSL_PROGRAM_H

#include "OgreException.hpp"
#include "OgreGLGpuProgram.hpp"
#include "OgreGLSLShaderCommon.hpp"
#include "OgreHardwareVertexBuffer.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderOperation.hpp"
#include "OgreResource.hpp"

namespace Ogre {
class ResourceManager;

    namespace GLSL {
    class GLSLProgram : public GLSLShaderCommon, public GLGpuProgramBase
    {
    public:
        GLSLProgram(ResourceManager* creator, 
            const String& name, ResourceHandle handle,
            const String& group, bool isManual, ManualResourceLoader* loader);
        ~GLSLProgram() override;

        void attachToProgramObject( const uint programObject ) override;
        void detachFromProgramObject( const uint programObject ) override;

        /// Overridden from GpuProgram
        const String& getLanguage() const override;

        bool getPassTransformStates() const override {
            return mPassFFPStates;
        }
        bool getPassSurfaceAndLightStates() const override {
            return mPassFFPStates;
        }
        bool getPassFogStates() const override {
            return mPassFFPStates;
        }

        /** Returns the operation type that this geometry program expects to
            receive as input
        */
        RenderOperation::OperationType getInputOperationType() const
        { return mInputOperationType; }
        /** Returns the operation type that this geometry program will emit
        */
        RenderOperation::OperationType getOutputOperationType() const
        { return mOutputOperationType; }
        /** Returns the maximum number of vertices that this geometry program can
            output in a single run
        */
        int getMaxOutputVertices() const { return mMaxOutputVertices; }

        /** Sets the operation type that this geometry program expects to receive
        */
        void setInputOperationType(RenderOperation::OperationType operationType)
        { mInputOperationType = operationType; }
        /** Set the operation type that this geometry program will emit
        */
        void setOutputOperationType(RenderOperation::OperationType operationType)
        {
            switch (operationType)
            {
            case RenderOperation::OT_POINT_LIST:
            case RenderOperation::OT_LINE_STRIP:
            case RenderOperation::OT_TRIANGLE_STRIP:
                break;
            default:
                OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
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
        void bindProgramParameters(GpuProgramParametersSharedPtr params, uint16 mask) override;
        bool isAttributeValid(VertexElementSemantic semantic, uint index) override;
    protected:
        void loadFromSource() override;
        /// Internal unload implementation, must be implemented by subclasses
        void unloadHighLevelImpl() override;

        /// Populate the passed parameters with name->index map, must be overridden
        void buildConstantDefinitions() override;

        // legacy GL_EXT_geometry_shader4 functionality
        RenderOperation::OperationType mInputOperationType;
        RenderOperation::OperationType mOutputOperationType;
        int mMaxOutputVertices;
        bool mPassFFPStates;
    };
    }
}

#endif // OGRE_RENDERSYSTEMS_GL_GLSL_PROGRAM_H
