/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rightsA
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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_EXALPHATEST_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_EXALPHATEST_H

#include "OgreCommon.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreShaderSubRenderState.hpp"

namespace Ogre {
    class AutoParamDataSource;
    class Pass;
    class Renderable;
namespace RTShader {
    class ProgramSet;
    class RenderState;
    }  // namespace RTShader
}  // namespace Ogre

namespace Ogre {
namespace RTShader {

/**
A factory that enables creation of LayeredBlending instances.
@remarks Sub class of SubRenderStateFactory
*/
	class FFPAlphaTest : public SubRenderState
	{

	private:
	UniformParameterPtr mPSAlphaRef;
	UniformParameterPtr mPSAlphaFunc;
	ParameterPtr mPSOutDiffuse;



	protected:

    /**
    @see SubRenderState::resolveParameters.
    */
    bool resolveParameters(ProgramSet* programSet) override;

    /**
    @see SubRenderState::resolveDependencies.
    */
    bool resolveDependencies(ProgramSet* programSet) override;

    /**
    @see SubRenderState::addFunctionInvocations.
    */
    bool addFunctionInvocations(ProgramSet* programSet) override;

public:

	/// The type.
	static String Type;
    /**
    @see SubRenderState::getType.
    */
    const String& getType() const noexcept override;

    /**
    @see SubRenderState::getExecutionOrder.
    */
    int getExecutionOrder() const noexcept override;

    /**
    @see SubRenderState::preAddToRenderState.
    */
    bool preAddToRenderState (const RenderState* renderState, Pass* srcPass, Pass* dstPass) noexcept override;

    /**
    @see SubRenderState::copyFrom.
    */
    void copyFrom(const SubRenderState& rhs) override;

    /**
    @see SubRenderState::updateGpuProgramsParams.
    */
    void updateGpuProgramsParams(Renderable* rend, const Pass* pass, const AutoParamDataSource* source, const LightList* pLightList) override;

	};

class FFPAlphaTestFactory : public SubRenderStateFactory
{

public:
static String Type;

	/**
	@see SubRenderStateFactory::getType.
	*/
	[[nodiscard]] const String& getType() const noexcept override;

protected:

	/**
	@see SubRenderStateFactory::createInstanceImpl.
	*/
	SubRenderState* createInstanceImpl() override;
};

} // namespace RTShader
} // namespace Ogre

#endif
