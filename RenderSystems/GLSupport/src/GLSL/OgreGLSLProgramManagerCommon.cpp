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

#include "OgreGLSLProgramManagerCommon.hpp"

#include <cstddef>
#include <format>
#include <utility>
#include <vector>

#include "OgreGLSLProgramCommon.hpp"
#include "OgreLog.hpp"
#include "OgreLogManager.hpp"
#include "OgreString.hpp"
#include "OgreStringConverter.hpp"
#include "OgreStringVector.hpp"

namespace Ogre {

    GLSLProgramManagerCommon::GLSLProgramManagerCommon()
    {
        mActiveShader.fill(nullptr);

        // Fill in the relationship between type names and enums
        mTypeEnumMap.emplace("float", GpuConstantType::FLOAT1);
        mTypeEnumMap.emplace("vec2", GpuConstantType::FLOAT2);
        mTypeEnumMap.emplace("vec3", GpuConstantType::FLOAT3);
        mTypeEnumMap.emplace("vec4", GpuConstantType::FLOAT4);
        mTypeEnumMap.emplace("sampler1D", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("sampler2D", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("sampler3D", GpuConstantType::SAMPLER3D);
        mTypeEnumMap.emplace("samplerCube", GpuConstantType::SAMPLERCUBE);
        mTypeEnumMap.emplace("sampler1DShadow", GpuConstantType::SAMPLER1DSHADOW);
        mTypeEnumMap.emplace("sampler2DShadow", GpuConstantType::SAMPLER2DSHADOW);
        mTypeEnumMap.emplace("int", GpuConstantType::INT1);
        mTypeEnumMap.emplace("ivec2", GpuConstantType::INT2);
        mTypeEnumMap.emplace("ivec3", GpuConstantType::INT3);
        mTypeEnumMap.emplace("ivec4", GpuConstantType::INT4);
        mTypeEnumMap.emplace("bool", GpuConstantType::BOOL1);
        mTypeEnumMap.emplace("bvec2", GpuConstantType::BOOL2);
        mTypeEnumMap.emplace("bvec3", GpuConstantType::BOOL3);
        mTypeEnumMap.emplace("bvec4", GpuConstantType::BOOL4);
        mTypeEnumMap.emplace("mat2", GpuConstantType::MATRIX_2X2);
        mTypeEnumMap.emplace("mat3", GpuConstantType::MATRIX_3X3);
        mTypeEnumMap.emplace("mat4", GpuConstantType::MATRIX_4X4);

        // GLES2 ext
        mTypeEnumMap.emplace("samplerExternalOES", GpuConstantType::SAMPLER_EXTERNAL_OES);

        // GL 2.1
        mTypeEnumMap.emplace("mat2x2", GpuConstantType::MATRIX_2X2);
        mTypeEnumMap.emplace("mat3x3", GpuConstantType::MATRIX_3X3);
        mTypeEnumMap.emplace("mat4x4", GpuConstantType::MATRIX_4X4);
        mTypeEnumMap.emplace("mat2x3", GpuConstantType::MATRIX_2X3);
        mTypeEnumMap.emplace("mat3x2", GpuConstantType::MATRIX_3X2);
        mTypeEnumMap.emplace("mat3x4", GpuConstantType::MATRIX_3X4);
        mTypeEnumMap.emplace("mat4x3", GpuConstantType::MATRIX_4X3);
        mTypeEnumMap.emplace("mat2x4", GpuConstantType::MATRIX_2X4);
        mTypeEnumMap.emplace("mat4x2", GpuConstantType::MATRIX_4X2);

        // GL 3.0
        mTypeEnumMap.emplace("uint", GpuConstantType::UINT1);
        mTypeEnumMap.emplace("uvec2", GpuConstantType::UINT2);
        mTypeEnumMap.emplace("uvec3", GpuConstantType::UINT3);
        mTypeEnumMap.emplace("uvec4", GpuConstantType::UINT4);
        mTypeEnumMap.emplace("samplerCubeShadow", GpuConstantType::SAMPLERCUBE);
        mTypeEnumMap.emplace("sampler1DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("sampler2DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("sampler1DArrayShadow", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("sampler2DArrayShadow", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("isampler1D", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("isampler2D", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("isampler3D", GpuConstantType::SAMPLER3D);
        mTypeEnumMap.emplace("isamplerCube", GpuConstantType::SAMPLERCUBE);
        mTypeEnumMap.emplace("isampler1DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("isampler2DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("usampler1D", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("usampler2D", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("usampler3D", GpuConstantType::SAMPLER3D);
        mTypeEnumMap.emplace("usamplerCube", GpuConstantType::SAMPLERCUBE);
        mTypeEnumMap.emplace("usampler1DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("usampler2DArray", GpuConstantType::SAMPLER2DARRAY);

        // GL 3.1
        mTypeEnumMap.emplace("sampler2DRect", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("sampler2DRectShadow", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("isampler2DRect", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("usampler2DRect", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("samplerBuffer", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("isamplerBuffer", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("usamplerBuffer", GpuConstantType::SAMPLER1D);

        // GL 3.2
        mTypeEnumMap.emplace("sampler2DMS", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("isampler2DMS", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("usampler2DMS", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("sampler2DMSArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("isampler2DMSArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("usampler2DMSArray", GpuConstantType::SAMPLER2DARRAY);

        // GL 4.0
        mTypeEnumMap.emplace("double", GpuConstantType::DOUBLE1);
        mTypeEnumMap.emplace("dmat2", GpuConstantType::MATRIX_DOUBLE_2X2);
        mTypeEnumMap.emplace("dmat3", GpuConstantType::MATRIX_DOUBLE_3X3);
        mTypeEnumMap.emplace("dmat4", GpuConstantType::MATRIX_DOUBLE_4X4);
        mTypeEnumMap.emplace("dmat2x2", GpuConstantType::MATRIX_DOUBLE_2X2);
        mTypeEnumMap.emplace("dmat3x3", GpuConstantType::MATRIX_DOUBLE_3X3);
        mTypeEnumMap.emplace("dmat4x4", GpuConstantType::MATRIX_DOUBLE_4X4);
        mTypeEnumMap.emplace("dmat2x3", GpuConstantType::MATRIX_DOUBLE_2X3);
        mTypeEnumMap.emplace("dmat3x2", GpuConstantType::MATRIX_DOUBLE_3X2);
        mTypeEnumMap.emplace("dmat3x4", GpuConstantType::MATRIX_DOUBLE_3X4);
        mTypeEnumMap.emplace("dmat4x3", GpuConstantType::MATRIX_DOUBLE_4X3);
        mTypeEnumMap.emplace("dmat2x4", GpuConstantType::MATRIX_DOUBLE_2X4);
        mTypeEnumMap.emplace("dmat4x2", GpuConstantType::MATRIX_DOUBLE_4X2);
        mTypeEnumMap.emplace("dvec2", GpuConstantType::DOUBLE2);
        mTypeEnumMap.emplace("dvec3", GpuConstantType::DOUBLE3);
        mTypeEnumMap.emplace("dvec4", GpuConstantType::DOUBLE4);
        mTypeEnumMap.emplace("samplerCubeArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("samplerCubeArrayShadow", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("isamplerCubeArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("usamplerCubeArray", GpuConstantType::SAMPLER2DARRAY);

        //TODO should image be its own type?
        mTypeEnumMap.emplace("image1D", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("iimage1D", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("uimage1D", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("image2D", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("iimage2D", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("uimage2D", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("image3D", GpuConstantType::SAMPLER3D);
        mTypeEnumMap.emplace("iimage3D", GpuConstantType::SAMPLER3D);
        mTypeEnumMap.emplace("uimage3D", GpuConstantType::SAMPLER3D);
        mTypeEnumMap.emplace("image2DRect", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("iimage2DRect", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("uimage2DRect", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("imageCube", GpuConstantType::SAMPLERCUBE);
        mTypeEnumMap.emplace("iimageCube", GpuConstantType::SAMPLERCUBE);
        mTypeEnumMap.emplace("uimageCube", GpuConstantType::SAMPLERCUBE);
        mTypeEnumMap.emplace("imageBuffer", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("iimageBuffer", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("uimageBuffer", GpuConstantType::SAMPLER1D);
        mTypeEnumMap.emplace("image1DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("iimage1DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("uimage1DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("image2DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("iimage2DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("uimage2DArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("imageCubeArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("iimageCubeArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("uimageCubeArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("image2DMS", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("iimage2DMS", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("uimage2DMS", GpuConstantType::SAMPLER2D);
        mTypeEnumMap.emplace("image2DMSArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("iimage2DMSArray", GpuConstantType::SAMPLER2DARRAY);
        mTypeEnumMap.emplace("uimage2DMSArray", GpuConstantType::SAMPLER2DARRAY);

        // GL 4.2
        mTypeEnumMap.emplace("atomic_uint", GpuConstantType::UINT1); //TODO should be its own type?
    }

    void GLSLProgramManagerCommon::destroyAllByShader(GLSLShaderCommon* shader)
    {
        std::vector<uint32> keysToErase;
        for (auto& [key, prgm] : mPrograms)
        {
            if(prgm->isUsingShader(shader))
            {
                prgm.reset();
                keysToErase.push_back(key);
            }
        }

        for(unsigned int & i : keysToErase)
        {
            mPrograms.erase(mPrograms.find(i));
        }
    }

    void GLSLProgramManagerCommon::parseGLSLUniform(std::string_view line, GpuNamedConstants& defs,
                                                    std::string_view filename)
    {
        GpuConstantDefinition def;

        // Remove spaces before opening square braces, otherwise
        // the following split() can split the line at inappropriate
        // places (e.g. "vec3 something [3]" won't work).
        //FIXME What are valid ways of including spaces in GLSL
        // variable declarations?  May need regex.

        // lifetime needs to be outside
        std::string linecopy;
        if (auto sqp = line.find(" ["); sqp != String::npos)
        {
            // work on copy
            linecopy = line;
            do
            {   linecopy.erase (sqp, 1);
                sqp = linecopy.find (" [");
            } while(sqp != String::npos);
            // reassign to modified copy
            line = linecopy;
        }
        // Split into tokens
        auto const parts = StringUtil::split(line, ", \t\r\n");

        for (auto const& part : parts)
        {
            // Is this a type?
            auto typei = mTypeEnumMap.find(part);
            if (typei != mTypeEnumMap.end())
            {
                def.constType = typei->second;
                // GL doesn't pad
                def.elementSize = GpuConstantDefinition::getElementSize(def.constType, false);
            }
            else
            {
                // if this is not a type, and not empty, it should be a name
                std::string trimpart{part};
                StringUtil::trim(trimpart);
                if (trimpart.empty()) continue;

                // Skip over precision keywords
                if(StringUtil::match(trimpart, "lowp") ||
                   StringUtil::match(trimpart, "mediump") ||
                   StringUtil::match(trimpart, "highp"))
                    continue;

                String paramName = "";
                String::size_type arrayStart = trimpart.find("[", 0);
                if (arrayStart != String::npos)
                {
                    // potential name (if butted up to array)
                    auto name = trimpart.substr(0, arrayStart);
                    StringUtil::trim(name);
                    if (!name.empty())
                        paramName = name;

                    def.arraySize = 1;

                    // N-dimensional arrays
                    while (arrayStart != String::npos) {
                        String::size_type arrayEnd = trimpart.find("]", arrayStart);
                        auto arrayDimTerm = trimpart.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                        StringUtil::trim(arrayDimTerm);
                        //TODO
                        // the array term might be a simple number or it might be
                        // an expression (e.g. 24*3) or refer to a constant expression
                        // we'd have to evaluate the expression which could get nasty
                        def.arraySize *= StringConverter::parseInt(arrayDimTerm);
                        arrayStart = trimpart.find("[", arrayEnd);
                    }
                }
                else
                {
                    paramName = trimpart;
                    def.arraySize = 1;
                }

                // Name should be after the type, so complete def and add
                // We do this now so that comma-separated params will do
                // this part once for each name mentioned
                if (def.constType == GpuConstantType::UNKNOWN)
                {
                    LogManager::getSingleton().logMessage(
                        ::std::format("Problem parsing the following GLSL Uniform: '"
                                                          "{}' in file {}", line , filename), LogMessageLevel::Critical);
                    // next uniform
                    break;
                }

                // Complete def and add
                // increment physical buffer location
                def.logicalIndex = 0; // not valid in GLSL
                if (def.isFloat() || def.isDouble() || def.isInt() || def.isUnsignedInt() || def.isBool())
                {
                    def.physicalIndex = defs.bufferSize * 4;
                    defs.bufferSize += def.arraySize * def.elementSize;
                }
                else if(def.isSampler())
                {
                    def.physicalIndex = defs.registerCount;
                    defs.registerCount += def.arraySize * def.elementSize;
                }
                else
                {
                    LogManager::getSingleton().logMessage(
                        ::std::format("Could not parse type of GLSL Uniform: '"
                                                          "{}' in file {}", line , filename));
                }
                defs.map.emplace(paramName, def);

                // warn if there is a default value, that we would overwrite
                if (line.find('=') != String::npos)
                {
                    LogManager::getSingleton().logWarning(
                        ::std::format("Default value of uniform '{}' is ignored in {}",
                                      paramName, filename));
                    break;
                }
            }
        }
    }

    void GLSLProgramManagerCommon::extractUniformsFromGLSL(std::string_view src,
        GpuNamedConstants& defs, std::string_view filename)
    {
        // Parse the output string and collect all uniforms
        // NOTE this relies on the source already having been preprocessed
        // which is done in GLSLESProgram::loadFromSource
        String line;
        String::size_type currPos = src.find("uniform");
        while (currPos != String::npos)
        {
            // Now check for using the word 'uniform' in a larger string & ignore
            bool inLargerString = false;
            if (currPos != 0)
            {
                char prev = src.at(currPos - 1);
                if (prev != ' ' && prev != '\t' && prev != '\r' && prev != '\n'
                    && prev != ';')
                    inLargerString = true;
            }
            if (!inLargerString && currPos + 7 < src.size())
            {
                char next = src.at(currPos + 7);
                if (next != ' ' && next != '\t' && next != '\r' && next != '\n')
                    inLargerString = true;
            }

            // skip 'uniform'
            currPos += 7;

            if (!inLargerString)
            {
                String::size_type endPos;
                String typeString;

                // Check for a type. If there is one, then the semicolon is missing
                // otherwise treat as if it is a uniform block
                String::size_type lineEndPos = src.find_first_of("\n\r", currPos);
                line = src.substr(currPos, lineEndPos - currPos);
                auto const parts = StringUtil::split(line, " \t");

                // Skip over precision keywords
                if(StringUtil::match((parts.front()), "lowp") ||
                   StringUtil::match((parts.front()), "mediump") ||
                   StringUtil::match((parts.front()), "highp"))
                    typeString = parts[1];
                else
                    typeString = parts[0];

                auto typei = mTypeEnumMap.find(typeString);
                if (typei == mTypeEnumMap.end())
                {
                    // Now there should be an opening brace
                    String::size_type openBracePos = src.find('{', currPos);
                    if (openBracePos != String::npos)
                    {
                        currPos = openBracePos + 1;
                    }
                    else
                    {
                        LogManager::getSingleton().logMessage(::std::format("Missing opening brace in GLSL Uniform Block in file {}", filename));
                        break;
                    }

                    // First we need to find the internal name for the uniform block
                    String::size_type endBracePos = src.find('}', currPos);

                    // Find terminating semicolon
                    currPos = endBracePos + 1;
                    endPos = src.find(';', currPos);
                    if (endPos == String::npos)
                    {
                        // problem, missing semicolon, abort
                        break;
                    }
                }
                else
                {
                    // find terminating semicolon
                    endPos = src.find(';', currPos);
                    if (endPos == String::npos)
                    {
                        // problem, missing semicolon, abort
                        break;
                    }

                    parseGLSLUniform(src.substr(currPos, endPos - currPos), defs, filename);
                }
                line = src.substr(currPos, endPos - currPos);
            } // not commented or a larger symbol

            // Find next one
            currPos = src.find("uniform", currPos);
        }
    }
}
