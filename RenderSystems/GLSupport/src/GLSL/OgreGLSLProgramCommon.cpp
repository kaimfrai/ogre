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

//  a  builtin              custom attrib name
// ----------------------------------------------
//  0  gl_Vertex            vertex/ position
//  1  n/a                  blendWeights
//  2  gl_Normal            normal
//  3  gl_Color             colour
//  4  gl_SecondaryColor    secondary_colour
//  5  gl_FogCoord          n/a
//  6  n/a                  n/a
//  7  n/a                  blendIndices
//  8  gl_MultiTexCoord0    uv0
//  9  gl_MultiTexCoord1    uv1
//  10 gl_MultiTexCoord2    uv2
//  11 gl_MultiTexCoord3    uv3
//  12 gl_MultiTexCoord4    uv4
//  13 gl_MultiTexCoord5    uv5
//  14 gl_MultiTexCoord6    uv6, tangent
//  15 gl_MultiTexCoord7    uv7, binormal
GLSLProgramCommon::CustomAttribute GLSLProgramCommon::msCustomAttributes[17] = {
    {"vertex", getFixedAttributeIndex(VertexElementSemantic::POSITION, 0), VertexElementSemantic::POSITION},
    {"position", getFixedAttributeIndex(VertexElementSemantic::POSITION, 0), VertexElementSemantic::POSITION}, // allow alias for "vertex"
    {"blendWeights", getFixedAttributeIndex(VertexElementSemantic::BLEND_WEIGHTS, 0), VertexElementSemantic::BLEND_WEIGHTS},
    {"normal", getFixedAttributeIndex(VertexElementSemantic::NORMAL, 0), VertexElementSemantic::NORMAL},
    {"colour", getFixedAttributeIndex(VertexElementSemantic::DIFFUSE, 0), VertexElementSemantic::DIFFUSE},
    {"secondary_colour", getFixedAttributeIndex(VertexElementSemantic::SPECULAR, 0), VertexElementSemantic::SPECULAR},
    {"blendIndices", getFixedAttributeIndex(VertexElementSemantic::BLEND_INDICES, 0), VertexElementSemantic::BLEND_INDICES},
    {"uv0", getFixedAttributeIndex(VertexElementSemantic::TEXTURE_COORDINATES, 0), VertexElementSemantic::TEXTURE_COORDINATES},
    {"uv1", getFixedAttributeIndex(VertexElementSemantic::TEXTURE_COORDINATES, 1), VertexElementSemantic::TEXTURE_COORDINATES},
    {"uv2", getFixedAttributeIndex(VertexElementSemantic::TEXTURE_COORDINATES, 2), VertexElementSemantic::TEXTURE_COORDINATES},
    {"uv3", getFixedAttributeIndex(VertexElementSemantic::TEXTURE_COORDINATES, 3), VertexElementSemantic::TEXTURE_COORDINATES},
    {"uv4", getFixedAttributeIndex(VertexElementSemantic::TEXTURE_COORDINATES, 4), VertexElementSemantic::TEXTURE_COORDINATES},
    {"uv5", getFixedAttributeIndex(VertexElementSemantic::TEXTURE_COORDINATES, 5), VertexElementSemantic::TEXTURE_COORDINATES},
    {"uv6", getFixedAttributeIndex(VertexElementSemantic::TEXTURE_COORDINATES, 6), VertexElementSemantic::TEXTURE_COORDINATES},
    {"uv7", getFixedAttributeIndex(VertexElementSemantic::TEXTURE_COORDINATES, 7), VertexElementSemantic::TEXTURE_COORDINATES},
    {"tangent", getFixedAttributeIndex(VertexElementSemantic::TANGENT, 0), VertexElementSemantic::TANGENT},
    {"binormal", getFixedAttributeIndex(VertexElementSemantic::BINORMAL, 0), VertexElementSemantic::BINORMAL},
};

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
