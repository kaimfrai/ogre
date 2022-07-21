// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
module;

#include <cstddef>

module Ogre.RenderSystems.GLSupport;

import :GLSL.ProgramCommon;

import Ogre.Core;

import <sstream>;
import <string>;

namespace Ogre
{
GLSLProgramCommon::GLSLProgramCommon(const GLShaderList& shaders)
    : mShaders(shaders)
      
{
    // compute shader presence means no other shaders are allowed
    if(shaders[std::to_underlying(GpuProgramType::COMPUTE_PROGRAM)])
    {
        mShaders.fill(nullptr);
        mShaders[std::to_underlying(GpuProgramType::COMPUTE_PROGRAM)] = shaders[std::to_underlying(GpuProgramType::COMPUTE_PROGRAM)];
    }
}

// Switching attribute bindings requires re-creating VAOs. So avoid!
// Fixed builtins (from ARB_vertex_program Table X.2) are:

static int32 attributeIndex[std::to_underlying(VertexElementSemantic::COUNT) + 1] = {
        -1,// n/a
        0, // VertexElementSemantic::POSITION
        1, // VertexElementSemantic::BLEND_WEIGHTS
        7, // VertexElementSemantic::BLEND_INDICES
        2, // VertexElementSemantic::NORMAL
        3, // VertexElementSemantic::DIFFUSE
        4, // VertexElementSemantic::SPECULAR
        8, // VertexElementSemantic::TEXTURE_COORDINATES
        15,// VertexElementSemantic::BINORMAL
        14 // VertexElementSemantic::TANGENT
};

void GLSLProgramCommon::useTightAttributeLayout() {
    //  a  builtin              custom attrib name
    // ----------------------------------------------
    //  0  gl_Vertex            vertex/ position
    //  1  gl_Normal            normal
    //  2  gl_Color             colour
    //  3  gl_MultiTexCoord0    uv0
    //  4  gl_MultiTexCoord1    uv1, blendWeights
    //  5  gl_MultiTexCoord2    uv2, blendIndices
    //  6  gl_MultiTexCoord3    uv3, tangent
    //  7  gl_MultiTexCoord4    uv4, binormal

    size_t numAttribs = sizeof(msCustomAttributes)/sizeof(CustomAttribute);
    for (size_t i = 0; i < numAttribs; ++i)
    {
        CustomAttribute& a = msCustomAttributes[i];
        a.attrib -= attributeIndex[std::to_underlying(a.semantic)]; // only keep index (for uvXY)
    }

    attributeIndex[std::to_underlying(VertexElementSemantic::NORMAL)] = 1;
    attributeIndex[std::to_underlying(VertexElementSemantic::DIFFUSE)] = 2;
    attributeIndex[std::to_underlying(VertexElementSemantic::TEXTURE_COORDINATES)] = 3;
    attributeIndex[std::to_underlying(VertexElementSemantic::BLEND_WEIGHTS)] = 4;
    attributeIndex[std::to_underlying(VertexElementSemantic::BLEND_INDICES)] = 5;
    attributeIndex[std::to_underlying(VertexElementSemantic::TANGENT)] = 6;
    attributeIndex[std::to_underlying(VertexElementSemantic::BINORMAL)] = 7;

    for (size_t i = 0; i < numAttribs; ++i)
    {
        CustomAttribute& a = msCustomAttributes[i];
        a.attrib += attributeIndex[std::to_underlying(a.semantic)];
    }
}

auto GLSLProgramCommon::getFixedAttributeIndex(VertexElementSemantic semantic, uint index) -> int32
{
    OgreAssertDbg(semantic > VertexElementSemantic{} && semantic <= VertexElementSemantic::COUNT, "Missing attribute!");

    if(semantic == VertexElementSemantic::TEXTURE_COORDINATES)
        return attributeIndex[std::to_underlying(semantic)] + index;

    return attributeIndex[std::to_underlying(semantic)];
}

auto GLSLProgramCommon::getCombinedName() -> String
{
    StringStream ss;

    for(auto s : mShaders)
    {
        if (s)
        {
            ss << s->getName() << "\n";
        }
    }

    return ss.str();
}

auto GLSLProgramCommon::getCombinedHash() noexcept -> uint32
{
    uint32 hash = 0;

    for (auto p : mShaders)
    {
        if(!p) continue;
        hash = p->_getHash(hash);
    }
    return hash;
}

} /* namespace Ogre */
