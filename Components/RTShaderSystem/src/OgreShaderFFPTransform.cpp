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
#include <cstddef>
#include <list>
#include <string>

#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramManager.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgreMaterialSerializer.hpp"
#include "OgrePass.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreScriptCompiler.hpp"
#include "OgreShaderFFPRenderState.hpp"
#include "OgreShaderFFPTransform.hpp"
#include "OgreShaderFunction.hpp"
#include "OgreShaderFunctionAtom.hpp"
#include "OgreShaderGenerator.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrecompiledHeaders.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderProgram.hpp"
#include "OgreShaderProgramSet.hpp"
#include "OgreShaderScriptTranslator.hpp"

namespace Ogre::RTShader {
        class RenderState;
    }  // namespace Ogre

namespace Ogre::RTShader {

/************************************************************************/
/*                                                                      */
/************************************************************************/
std::string_view const constinit FFPTransform::Type = "FFP_Transform";

//-----------------------------------------------------------------------
auto FFPTransform::getType() const noexcept -> std::string_view
{
    return Type;
}


//-----------------------------------------------------------------------
auto FFPTransform::getExecutionOrder() const noexcept -> FFPShaderStage
{
    return FFPShaderStage::TRANSFORM;
}

auto FFPTransform::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept -> bool
{
    mSetPointSize = srcPass->getPointSize() != 1.0f || srcPass->isPointAttenuationEnabled();
    mDoLightCalculations = srcPass->getLightingEnabled();
    return true;
}

//-----------------------------------------------------------------------
auto FFPTransform::createCpuSubPrograms(ProgramSet* programSet) -> bool
{
    //! [param_resolve]
    Program* vsProgram = programSet->getCpuProgram(GpuProgramType::VERTEX_PROGRAM);
    Function* vsEntry = vsProgram->getEntryPointFunction();
    
    // Resolve World View Projection Matrix.
    UniformParameterPtr wvpMatrix = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::WORLDVIEWPROJ_MATRIX);

    // Resolve input position parameter.
    ParameterPtr positionIn = vsEntry->resolveInputParameter(Parameter::Content::POSITION_OBJECT_SPACE);
    
    // Resolve output position parameter.
    ParameterPtr positionOut = vsEntry->resolveOutputParameter(Parameter::Content::POSITION_PROJECTIVE_SPACE);
    //! [param_resolve]

    // Add dependency.
    vsProgram->addDependency(FFP_LIB_TRANSFORM);

    bool isHLSL = ShaderGenerator::getSingleton().getTargetLanguage() == "hlsl";

    // This requires GLES3.0
    if (ShaderGenerator::getSingleton().getTargetLanguage() == "glsles" &&
        !GpuProgramManager::getSingleton().isSyntaxSupported("glsl300es"))
        mInstanced = false;

    auto stage = vsEntry->getStage(std::to_underlying(FFPVertexShaderStage::TRANSFORM));
    if(mInstanced)
    {
        if (isHLSL)
        {
            // set hlsl shader to use row-major matrices instead of column-major.
            // it enables the use of auto-bound 3x4 matrices in generated hlsl shader.
            vsProgram->setUseColumnMajorMatrices(false);
        }

        auto wMatrix = vsEntry->resolveInputParameter(mTexCoordIndex, GpuConstantType::MATRIX_3X4);
        stage.callFunction(FFP_FUNC_TRANSFORM, wMatrix, positionIn, Out(positionIn).xyz());

        if(mDoLightCalculations)
        {
            auto vsInNormal = vsEntry->resolveInputParameter(Parameter::Content::NORMAL_OBJECT_SPACE);
            stage.callFunction(FFP_FUNC_TRANSFORM, wMatrix, vsInNormal, vsInNormal);
        }
        // we can end here because the world matrix will be identity with instanced rendering
        // so the code below will work as indended
    }
    stage.callFunction(FFP_FUNC_TRANSFORM, wvpMatrix, positionIn, positionOut);

    if(!mSetPointSize || isHLSL) // not supported with DX11
        return true;

    UniformParameterPtr pointParams = vsProgram->resolveParameter(GpuProgramParameters::AutoConstantType::POINT_PARAMS);
    ParameterPtr pointSize = vsEntry->resolveOutputParameter(Parameter::Content::POINTSPRITE_SIZE);

    // using eye space depth only instead of the eye real distance
    // its faster to obtain, so lets call it close enough..
    stage.callFunction("FFP_DerivePointSize", pointParams, In(positionOut).w(), pointSize);

    return true;
}


//-----------------------------------------------------------------------
void FFPTransform::copyFrom(const SubRenderState& rhs)
{
    const auto& rhsTransform = static_cast<const FFPTransform&>(rhs);
    mSetPointSize = rhsTransform.mSetPointSize;
    mInstanced = rhsTransform.mInstanced;
    mTexCoordIndex = rhsTransform.mTexCoordIndex;
}

//-----------------------------------------------------------------------
auto FFPTransformFactory::getType() const noexcept -> std::string_view
{
    return FFPTransform::Type;
}

//-----------------------------------------------------------------------
auto FFPTransformFactory::createInstance(ScriptCompiler* compiler, 
                                                   PropertyAbstractNode* prop, Pass* pass, SGScriptTranslator* translator) noexcept -> SubRenderState*
{
    if (prop->name == "transform_stage")
    {
        if(prop->values.size() > 0)
        {
            bool hasError = false;
            String modelType;
            int texCoordSlot = 1;

            auto it = prop->values.begin();

            if(!SGScriptTranslator::getString(*it, &modelType))
            {
                hasError = true;
            }

            if(++it != prop->values.end() && !SGScriptTranslator::getInt(*++it, &texCoordSlot))
                hasError = true;

            if(hasError)
            {
                compiler->addError(ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line);
                return nullptr;
            }

            auto ret = static_cast<FFPTransform*>(createOrRetrieveInstance(translator));
            ret->setInstancingParams(modelType == "instanced", texCoordSlot);

            return ret;
        }
    }

    return nullptr;
}

//-----------------------------------------------------------------------
void FFPTransformFactory::writeInstance(MaterialSerializer* ser, SubRenderState* subRenderState, 
                                       Pass* srcPass, Pass* dstPass)
{
    ser->writeAttribute(4, "transform_stage");
    ser->writeValue("ffp");
}

//-----------------------------------------------------------------------
auto FFPTransformFactory::createInstanceImpl() -> SubRenderState*
{
    return new FFPTransform;
}


}
