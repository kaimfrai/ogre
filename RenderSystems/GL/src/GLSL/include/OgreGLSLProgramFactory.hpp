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
module Ogre.RenderSystems.GL:GLSL.SLProgramFactory;

import :GLSL.SLLinkProgramManager;

import Ogre.Core;

import <memory>;

namespace Ogre
{
    namespace GLSL {

    /** Factory class for GLSL programs. */
    class GLSLProgramFactory : public HighLevelGpuProgramFactory
    {
    protected:
        static std::string_view const sLanguageName;
    public:
        GLSLProgramFactory() = default;
        ~GLSLProgramFactory() override = default;
        /// Get the name of the language this factory creates programs for
        [[nodiscard]] auto getLanguage() const noexcept -> std::string_view override;
        /// Create an instance of GLSLProgram
        auto create(ResourceManager* creator,
            std::string_view name, ResourceHandle handle,
            std::string_view group, bool isManual, ManualResourceLoader* loader) -> GpuProgram* override;

    private:
        ::std::unique_ptr<GLSLLinkProgramManager> mLinkProgramManager = ::std::make_unique<GLSLLinkProgramManager>();

    };
    }
}
