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
module Ogre.Components.RTShaderSystem;

import :ShaderFFPAlphaTest;
import :ShaderFFPRenderState;
import :ShaderFunction;
import :ShaderFunctionAtom;
import :ShaderParameter;
import :ShaderPrecompiledHeaders;
import :ShaderPrerequisites;
import :ShaderProgram;
import :ShaderProgramSet;

import Ogre.Core;

import <memory>;

namespace Ogre::RTShader {

		std::string_view const constinit FFPAlphaTest::Type = "FFP_Alpha_Test";


		//-----------------------------------------------------------------------
		auto FFPAlphaTest::getType() const noexcept -> std::string_view
		{
			return Type;
		}


		//-----------------------------------------------------------------------
		auto FFPAlphaTest::resolveParameters(ProgramSet* programSet) -> bool
		{
			Program* psProgram  = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
			Function* psMain = psProgram->getEntryPointFunction();

			mPSAlphaRef = psProgram->resolveParameter(GpuProgramParameters::AutoConstantType::SURFACE_ALPHA_REJECTION_VALUE);
			mPSAlphaFunc = psProgram->resolveParameter(GpuConstantType::FLOAT1, "gAlphaFunc");

			mPSOutDiffuse = psMain->resolveOutputParameter(Parameter::Content::COLOR_DIFFUSE);

			return true;
		}



		//-----------------------------------------------------------------------
		auto FFPAlphaTest::resolveDependencies(ProgramSet* programSet) -> bool
		{
			Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
			psProgram->addDependency(FFP_LIB_ALPHA_TEST);
			return true;
		}

		//-----------------------------------------------------------------------

		void FFPAlphaTest::copyFrom( const SubRenderState& rhs )
		{

		}

		auto FFPAlphaTest::addFunctionInvocations( ProgramSet* programSet ) -> bool
		{
			Program* psProgram = programSet->getCpuProgram(GpuProgramType::FRAGMENT_PROGRAM);
			Function* psMain = psProgram->getEntryPointFunction();

            psMain->getStage(std::to_underlying(FFPFragmentShaderStage::ALPHA_TEST))
                .callFunction(FFP_FUNC_ALPHA_TEST, {In(mPSAlphaFunc), In(mPSAlphaRef), In(mPSOutDiffuse)});

            return true;
		}

		auto FFPAlphaTest::getExecutionOrder() const noexcept -> FFPShaderStage
		{
			return FFPShaderStage::ALPHA_TEST;
		}

		auto FFPAlphaTest::preAddToRenderState( const RenderState* renderState, Pass* srcPass, Pass* dstPass ) noexcept -> bool
		{
			return srcPass->getAlphaRejectFunction() != CompareFunction::ALWAYS_PASS;
		}

		void FFPAlphaTest::updateGpuProgramsParams( Renderable* rend, const Pass* pass, const AutoParamDataSource* source, const LightList* pLightList )
		{
			mPSAlphaFunc->setGpuParameter((float)pass->getAlphaRejectFunction());
		}

		//----------------------Factory Implementation---------------------------
		//-----------------------------------------------------------------------
		auto FFPAlphaTestFactory ::getType() const noexcept -> std::string_view
		{
			return FFPAlphaTest::Type;
		}

		//-----------------------------------------------------------------------
		auto	FFPAlphaTestFactory::createInstanceImpl() -> SubRenderState*
		{
			return new FFPAlphaTest;
		}
	}
