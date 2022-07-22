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
module;

#include <cassert>

module Ogre.Core;

import :AutoParamDataSource;
import :ColourValue;
import :DataStream;
import :DualQuaternion;
import :Exception;
import :FileSystem;
import :GpuProgramManager;
import :GpuProgramParams;
import :HardwareBuffer;
import :LogManager;
import :Math;
import :Matrix3;
import :Matrix4;
import :RenderSystem;
import :RenderTarget;
import :Renderable;
import :Root;
import :String;
import :StringConverter;
import :Vector;

import <ios>;
import <iterator>;
import <memory>;
import <utility>;

namespace Ogre
{
    //---------------------------------------------------------------------
    GpuProgramParameters::AutoConstantDefinition GpuProgramParameters::AutoConstantDictionary[] = {
        AutoConstantDefinition{AutoConstantType::WORLD_MATRIX, "world_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_WORLD_MATRIX, "inverse_world_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::TRANSPOSE_WORLD_MATRIX, "transpose_world_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_WORLD_MATRIX, "inverse_transpose_world_matrix", 16, ElementType::REAL, ACDataType::NONE},

        AutoConstantDefinition{AutoConstantType::WORLD_MATRIX_ARRAY_3x4, "world_matrix_array_3x4", 12, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::WORLD_MATRIX_ARRAY, "world_matrix_array", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::WORLD_DUALQUATERNION_ARRAY_2x4, "world_dualquaternion_array_2x4", 8, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::WORLD_SCALE_SHEAR_MATRIX_ARRAY_3x4, "world_scale_shear_matrix_array_3x4", 12, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::VIEW_MATRIX, "view_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_VIEW_MATRIX, "inverse_view_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::TRANSPOSE_VIEW_MATRIX, "transpose_view_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_VIEW_MATRIX, "inverse_transpose_view_matrix", 16, ElementType::REAL, ACDataType::NONE},

        AutoConstantDefinition{AutoConstantType::PROJECTION_MATRIX, "projection_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_PROJECTION_MATRIX, "inverse_projection_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::TRANSPOSE_PROJECTION_MATRIX, "transpose_projection_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_PROJECTION_MATRIX, "inverse_transpose_projection_matrix", 16, ElementType::REAL, ACDataType::NONE},

        AutoConstantDefinition{AutoConstantType::VIEWPROJ_MATRIX, "viewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_VIEWPROJ_MATRIX, "inverse_viewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::TRANSPOSE_VIEWPROJ_MATRIX, "transpose_viewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_VIEWPROJ_MATRIX, "inverse_transpose_viewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},

        AutoConstantDefinition{AutoConstantType::WORLDVIEW_MATRIX, "worldview_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_WORLDVIEW_MATRIX, "inverse_worldview_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::TRANSPOSE_WORLDVIEW_MATRIX, "transpose_worldview_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_WORLDVIEW_MATRIX, "inverse_transpose_worldview_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::NORMAL_MATRIX, "normal_matrix", 9, ElementType::REAL, ACDataType::NONE},

        AutoConstantDefinition{AutoConstantType::WORLDVIEWPROJ_MATRIX, "worldviewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_WORLDVIEWPROJ_MATRIX, "inverse_worldviewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::TRANSPOSE_WORLDVIEWPROJ_MATRIX, "transpose_worldviewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_WORLDVIEWPROJ_MATRIX, "inverse_transpose_worldviewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},

        AutoConstantDefinition{AutoConstantType::RENDER_TARGET_FLIPPING, "render_target_flipping", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::VERTEX_WINDING, "vertex_winding", 1, ElementType::REAL, ACDataType::NONE},

        AutoConstantDefinition{AutoConstantType::FOG_COLOUR, "fog_colour", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::FOG_PARAMS, "fog_params", 4, ElementType::REAL, ACDataType::NONE},

        AutoConstantDefinition{AutoConstantType::SURFACE_AMBIENT_COLOUR, "surface_ambient_colour", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::SURFACE_DIFFUSE_COLOUR, "surface_diffuse_colour", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::SURFACE_SPECULAR_COLOUR, "surface_specular_colour", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::SURFACE_EMISSIVE_COLOUR, "surface_emissive_colour", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::SURFACE_SHININESS, "surface_shininess", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::SURFACE_ALPHA_REJECTION_VALUE, "surface_alpha_rejection_value", 1, ElementType::REAL, ACDataType::NONE},

        AutoConstantDefinition{AutoConstantType::LIGHT_COUNT, "light_count", 1, ElementType::REAL, ACDataType::NONE},

        AutoConstantDefinition{AutoConstantType::AMBIENT_LIGHT_COLOUR, "ambient_light_colour", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::LIGHT_DIFFUSE_COLOUR, "light_diffuse_colour", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_SPECULAR_COLOUR, "light_specular_colour", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_ATTENUATION, "light_attenuation", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::SPOTLIGHT_PARAMS, "spotlight_params", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_POSITION, "light_position", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_POSITION_OBJECT_SPACE, "light_position_object_space", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_POSITION_VIEW_SPACE, "light_position_view_space", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION, "light_direction", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION_OBJECT_SPACE, "light_direction_object_space", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION_VIEW_SPACE, "light_direction_view_space", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DISTANCE_OBJECT_SPACE, "light_distance_object_space", 1, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_POWER_SCALE, "light_power", 1, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DIFFUSE_COLOUR_POWER_SCALED, "light_diffuse_colour_power_scaled", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_SPECULAR_COLOUR_POWER_SCALED, "light_specular_colour_power_scaled", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DIFFUSE_COLOUR_ARRAY, "light_diffuse_colour_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_SPECULAR_COLOUR_ARRAY, "light_specular_colour_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DIFFUSE_COLOUR_POWER_SCALED_ARRAY, "light_diffuse_colour_power_scaled_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_SPECULAR_COLOUR_POWER_SCALED_ARRAY, "light_specular_colour_power_scaled_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_ATTENUATION_ARRAY, "light_attenuation_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_POSITION_ARRAY, "light_position_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_POSITION_OBJECT_SPACE_ARRAY, "light_position_object_space_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_POSITION_VIEW_SPACE_ARRAY, "light_position_view_space_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION_ARRAY, "light_direction_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION_OBJECT_SPACE_ARRAY, "light_direction_object_space_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION_VIEW_SPACE_ARRAY, "light_direction_view_space_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_DISTANCE_OBJECT_SPACE_ARRAY, "light_distance_object_space_array", 1, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_POWER_SCALE_ARRAY, "light_power_array", 1, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::SPOTLIGHT_PARAMS_ARRAY, "spotlight_params_array", 4, ElementType::REAL, ACDataType::INT},

        AutoConstantDefinition{AutoConstantType::DERIVED_AMBIENT_LIGHT_COLOUR, "derived_ambient_light_colour", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::DERIVED_SCENE_COLOUR, "derived_scene_colour", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::DERIVED_LIGHT_DIFFUSE_COLOUR, "derived_light_diffuse_colour", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::DERIVED_LIGHT_SPECULAR_COLOUR, "derived_light_specular_colour", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::DERIVED_LIGHT_DIFFUSE_COLOUR_ARRAY, "derived_light_diffuse_colour_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::DERIVED_LIGHT_SPECULAR_COLOUR_ARRAY, "derived_light_specular_colour_array", 4, ElementType::REAL, ACDataType::INT},

        AutoConstantDefinition{AutoConstantType::LIGHT_NUMBER, "light_number", 1, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_CASTS_SHADOWS, "light_casts_shadows", 1, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LIGHT_CASTS_SHADOWS_ARRAY, "light_casts_shadows_array", 1, ElementType::REAL, ACDataType::INT},

        AutoConstantDefinition{AutoConstantType::SHADOW_EXTRUSION_DISTANCE, "shadow_extrusion_distance", 1, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::CAMERA_POSITION, "camera_position", 3, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::CAMERA_POSITION_OBJECT_SPACE, "camera_position_object_space", 3, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::CAMERA_RELATIVE_POSITION, "camera_relative_position", 3, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::TEXTURE_VIEWPROJ_MATRIX, "texture_viewproj_matrix", 16, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::TEXTURE_VIEWPROJ_MATRIX_ARRAY, "texture_viewproj_matrix_array", 16, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::TEXTURE_WORLDVIEWPROJ_MATRIX, "texture_worldviewproj_matrix",16, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::TEXTURE_WORLDVIEWPROJ_MATRIX_ARRAY, "texture_worldviewproj_matrix_array",16, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::SPOTLIGHT_VIEWPROJ_MATRIX, "spotlight_viewproj_matrix", 16, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::SPOTLIGHT_VIEWPROJ_MATRIX_ARRAY, "spotlight_viewproj_matrix_array", 16, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::SPOTLIGHT_WORLDVIEWPROJ_MATRIX, "spotlight_worldviewproj_matrix",16, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::SPOTLIGHT_WORLDVIEWPROJ_MATRIX_ARRAY, "spotlight_worldviewproj_matrix_array",16, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::CUSTOM, "custom", 4, ElementType::REAL, ACDataType::INT}, // *** needs to be tested
        AutoConstantDefinition{AutoConstantType::TIME, "time", 1, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::TIME_0_X, "time_0_x", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::COSTIME_0_X, "costime_0_x", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::SINTIME_0_X, "sintime_0_x", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::TANTIME_0_X, "tantime_0_x", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::TIME_0_X_PACKED, "time_0_x_packed", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::TIME_0_1, "time_0_1", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::COSTIME_0_1, "costime_0_1", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::SINTIME_0_1, "sintime_0_1", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::TANTIME_0_1, "tantime_0_1", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::TIME_0_1_PACKED, "time_0_1_packed", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::TIME_0_2PI, "time_0_2pi", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::COSTIME_0_2PI, "costime_0_2pi", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::SINTIME_0_2PI, "sintime_0_2pi", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::TANTIME_0_2PI, "tantime_0_2pi", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::TIME_0_2PI_PACKED, "time_0_2pi_packed", 4, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::FRAME_TIME, "frame_time", 1, ElementType::REAL, ACDataType::REAL},
        AutoConstantDefinition{AutoConstantType::FPS, "fps", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::VIEWPORT_WIDTH, "viewport_width", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::VIEWPORT_HEIGHT, "viewport_height", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_VIEWPORT_WIDTH, "inverse_viewport_width", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::INVERSE_VIEWPORT_HEIGHT, "inverse_viewport_height", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::VIEWPORT_SIZE, "viewport_size", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::VIEW_DIRECTION, "view_direction", 3, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::VIEW_SIDE_VECTOR, "view_side_vector", 3, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::VIEW_UP_VECTOR, "view_up_vector", 3, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::FOV, "fov", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::NEAR_CLIP_DISTANCE, "near_clip_distance", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::FAR_CLIP_DISTANCE, "far_clip_distance", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::PASS_NUMBER, "pass_number", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::PASS_ITERATION_NUMBER, "pass_iteration_number", 1, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::ANIMATION_PARAMETRIC, "animation_parametric", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::TEXEL_OFFSETS, "texel_offsets", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::SCENE_DEPTH_RANGE, "scene_depth_range", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::SHADOW_SCENE_DEPTH_RANGE, "shadow_scene_depth_range", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::SHADOW_SCENE_DEPTH_RANGE_ARRAY, "shadow_scene_depth_range_array", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::SHADOW_COLOUR, "shadow_colour", 4, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::TEXTURE_SIZE, "texture_size", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::INVERSE_TEXTURE_SIZE, "inverse_texture_size", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::PACKED_TEXTURE_SIZE, "packed_texture_size", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::TEXTURE_MATRIX, "texture_matrix", 16, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::LOD_CAMERA_POSITION, "lod_camera_position", 3, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::LOD_CAMERA_POSITION_OBJECT_SPACE, "lod_camera_position_object_space", 3, ElementType::REAL, ACDataType::NONE},
        AutoConstantDefinition{AutoConstantType::LIGHT_CUSTOM, "light_custom", 4, ElementType::REAL, ACDataType::INT},
        AutoConstantDefinition{AutoConstantType::POINT_PARAMS, "point_params", 4, ElementType::REAL, ACDataType::NONE},
    };

    //---------------------------------------------------------------------
    //  GpuNamedConstants methods
    //---------------------------------------------------------------------
    void GpuNamedConstants::save(std::string_view filename) const
    {
        GpuNamedConstantsSerializer ser;
        ser.exportNamedConstants(this, filename);
    }
    //---------------------------------------------------------------------
    void GpuNamedConstants::load(DataStreamPtr& stream)
    {
        GpuNamedConstantsSerializer ser;
        ser.importNamedConstants(stream, this);
    }
    //-----------------------------------------------------------------------------
    auto GpuNamedConstants::calculateSize() const -> size_t
    {
        size_t memSize = sizeof(*this);
        // Tally up constant defs
        memSize += sizeof(GpuConstantDefinition) * map.size();

        return memSize;
    }
    //---------------------------------------------------------------------
    //  GpuNamedConstantsSerializer methods
    //---------------------------------------------------------------------
    GpuNamedConstantsSerializer::GpuNamedConstantsSerializer()
    {
        mVersion = "[v1.0]";
    }
    //---------------------------------------------------------------------
    GpuNamedConstantsSerializer::~GpuNamedConstantsSerializer()
    = default;
    //---------------------------------------------------------------------
    void GpuNamedConstantsSerializer::exportNamedConstants(
        const GpuNamedConstants* pConsts, std::string_view filename, std::endian endianMode)
    {
        DataStreamPtr stream = _openFileStream(filename, std::ios::binary | std::ios::out);
        exportNamedConstants(pConsts, stream, endianMode);

        stream->close();
    }
    //---------------------------------------------------------------------
    void GpuNamedConstantsSerializer::exportNamedConstants(
        const GpuNamedConstants* pConsts, DataStreamPtr stream, std::endian endianMode)
    {
        // Decide on endian mode
        determineEndianness(endianMode);

        mStream =stream;
        if (!stream->isWriteable())
        {
            OGRE_EXCEPT(ExceptionCodes::CANNOT_WRITE_TO_FILE,
                        ::std::format("Unable to write to stream {}", stream->getName()),
                        "GpuNamedConstantsSerializer::exportNamedConstants");
        }

        writeFileHeader();

        writeInts(((const uint32*)&pConsts->bufferSize), 1);
        writeInts(((const uint32*)&pConsts->bufferSize), 1);
        writeInts(((const uint32*)&pConsts->bufferSize), 1);

        // simple export of all the named constants, no chunks
        // name, physical index
        for (auto const& [name, def] : pConsts->map)
        {
            writeString(name);
            writeInts(((const uint32*)&def.physicalIndex), 1);
            writeInts(((const uint32*)&def.logicalIndex), 1);
            auto constType = static_cast<uint32>(def.constType);
            writeInts(&constType, 1);
            writeInts(((const uint32*)&def.elementSize), 1);
            writeInts(((const uint32*)&def.arraySize), 1);
        }

    }
    //---------------------------------------------------------------------
    void GpuNamedConstantsSerializer::importNamedConstants(
        DataStreamPtr& stream, GpuNamedConstants* pDest)
    {
        // Determine endianness (must be the first thing we do!)
        determineEndianness(stream);

        // Check header
        readFileHeader(stream);

        // simple file structure, no chunks
        pDest->map.clear();

        readInts(stream, ((uint32*)&pDest->bufferSize), 1);
        readInts(stream, ((uint32*)&pDest->bufferSize), 1);
        readInts(stream, ((uint32*)&pDest->bufferSize), 1);

        while (!stream->eof())
        {
            GpuConstantDefinition def;
            String name = readString(stream);
            // Hmm, deal with trailing information
            if (name.empty())
                continue;
            readInts(stream, ((uint32*)&def.physicalIndex), 1);
            readInts(stream, ((uint32*)&def.logicalIndex), 1);
            uint constType;
            readInts(stream, &constType, 1);
            def.constType = static_cast<GpuConstantType>(constType);
            readInts(stream, ((uint32*)&def.elementSize), 1);
            readInts(stream, ((uint32*)&def.arraySize), 1);

            pDest->map[name] = def;

        }



    }

    //-----------------------------------------------------------------------------
    //      GpuSharedParameters Methods
    //-----------------------------------------------------------------------------
    GpuSharedParameters::GpuSharedParameters(std::string_view name)
        :mName(name)
         
    {

    }
    //-----------------------------------------------------------------------------
    auto GpuSharedParameters::calculateSize() const -> size_t
    {
        size_t memSize = sizeof(*this);

        memSize += mConstants.size();
        memSize += mName.size() * sizeof(char);

        return memSize;
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::addConstantDefinition(std::string_view name, GpuConstantType constType, uint32 arraySize)
    {
        if (mNamedConstants.map.find(name) != mNamedConstants.map.end())
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        ::std::format("Constant entry with name '{}' already exists. ", name ),
                        "GpuSharedParameters::addConstantDefinition");
        }
        GpuConstantDefinition def;
        def.arraySize = arraySize;
        def.constType = constType;

		// here, we do not consider padding, but rather alignment
        def.elementSize = GpuConstantDefinition::getElementSize(constType, false);

		// we try to adhere to GLSL std140 packing rules
		// handle alignment requirements
        size_t align_size = std::min<size_t>(def.elementSize == 3 ? 4 : def.elementSize, 4); // vec3 is 16 byte aligned, which is max
        align_size *= 4; // bytes

        size_t nextAlignedOffset = ((mOffset + align_size - 1) / align_size) * align_size;

        // abuse logical index to store offset
        if (mOffset + align_size > nextAlignedOffset)
        {
            def.logicalIndex = nextAlignedOffset;
        }
        else
        {
            def.logicalIndex = mOffset;
        }

        mOffset = def.logicalIndex + (def.constType == GpuConstantType::MATRIX_4X3 ? 16 : def.elementSize) * 4; // mat4x3 have a size of 64 bytes

        def.variability = GpuParamVariability::GLOBAL;

        if (def.isFloat())
        {
            def.physicalIndex = mConstants.size();
            mConstants.resize(mConstants.size() + def.arraySize * def.elementSize*4);
        }
        else if (def.isDouble())
        {
            def.physicalIndex = mConstants.size();
            mConstants.resize(mConstants.size() + def.arraySize * def.elementSize*8);
        }
        else if (def.isInt() || def.isUnsignedInt() || def.isBool())
        {
            def.physicalIndex = mConstants.size();
            mConstants.resize(mConstants.size() + def.arraySize * def.elementSize*4);
        }
        else 
        {
            //FIXME Is this the right exception type?
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        ::std::format("Constant entry with name '{}' is not a known type.", name ),
                        "GpuSharedParameters::addConstantDefinition");
        }

        mNamedConstants.map[std::string{ name }] = def;

        ++mVersion;
    }


    void GpuSharedParameters::_upload() const
    {
        OgreAssert(mHardwareBuffer, "not backed by a HardwareBuffer");

        if (!mDirty)
            return;

        mHardwareBuffer->writeData(0, mConstants.size(), mConstants.data());
    }
    void GpuSharedParameters::download()
    {
        OgreAssert(mHardwareBuffer, "not backed by a HardwareBuffer");
        mHardwareBuffer->readData(0, mConstants.size(), mConstants.data());
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::removeAllConstantDefinitions()
    {
		mOffset = 0;
        mNamedConstants.map.clear();
        mNamedConstants.bufferSize = 0;
        mConstants.clear();
    }

    //---------------------------------------------------------------------
    auto GpuSharedParameters::getConstantDefinition(std::string_view name) const -> const GpuConstantDefinition&
    {
        auto i = mNamedConstants.map.find(name);
        if (i == mNamedConstants.map.end())
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        ::std::format("Constant entry with name '{}' does not exist. ", name ),
                        "GpuSharedParameters::getConstantDefinition");
        }
        return i->second;
    }
    //---------------------------------------------------------------------
    auto GpuSharedParameters::getConstantDefinitions() const noexcept -> const GpuNamedConstants&
    {
        return mNamedConstants;
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::setNamedConstant(std::string_view name, const Matrix4& m)
    {
        setNamedConstant(name, m[0], 16);
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::setNamedConstant(std::string_view name, const Matrix4* m, uint32 numEntries)
    {
        setNamedConstant(name, m[0][0], 16 * numEntries);
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::setNamedConstant(std::string_view name, const ColourValue& colour)
    {
        setNamedConstant(name, colour.ptr(), 4);
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::setNamedConstant(std::string_view name, const float *val, uint32 count)
    {
        auto i = mNamedConstants.map.find(name);
        if (i != mNamedConstants.map.end())
        {
            const GpuConstantDefinition& def = i->second;
            memcpy(&mConstants[def.physicalIndex], val,
                   sizeof(float) * std::min(count, def.elementSize * def.arraySize));
        }

        _markDirty();
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::setNamedConstant(std::string_view name, const double *val, uint32 count)
    {
        auto i = mNamedConstants.map.find(name);
        if (i != mNamedConstants.map.end())
        {
            const GpuConstantDefinition& def = i->second;

            count = std::min(count, def.elementSize * def.arraySize);
            const double* src = val;
            auto* dst = (double*)&mConstants[def.physicalIndex];
            for (size_t v = 0; v < count; ++v)
            {
                *dst++ = static_cast<double>(*src++);
            }
        }

        _markDirty();
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::setNamedConstant(std::string_view name, const int *val, uint32 count)
    {
        auto i = mNamedConstants.map.find(name);
        if (i != mNamedConstants.map.end())
        {
            const GpuConstantDefinition& def = i->second;
            memcpy(&mConstants[def.physicalIndex], val,
                   sizeof(int) * std::min(count, def.elementSize * def.arraySize));
        }

        _markDirty();
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::setNamedConstant(std::string_view name, const uint *val, uint32 count)
    {
        auto i = mNamedConstants.map.find(name);
        if (i != mNamedConstants.map.end())
        {
            const GpuConstantDefinition& def = i->second;
            memcpy(&mConstants[def.physicalIndex], val,
                   sizeof(uint) * std::min(count, def.elementSize * def.arraySize));
        }

        _markDirty();
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::_markClean()
    {
        mDirty = false;
    }
    //---------------------------------------------------------------------
    void GpuSharedParameters::_markDirty()
    {
        mDirty = true;
    }
    

    //-----------------------------------------------------------------------------
    //      GpuSharedParametersUsage Methods
    //-----------------------------------------------------------------------------
    GpuSharedParametersUsage::GpuSharedParametersUsage(GpuSharedParametersPtr sharedParams,
                                                       GpuProgramParameters* params)
        : mSharedParams(sharedParams)
        , mParams(params)
    {
        initCopyData();
    }
    //---------------------------------------------------------------------
    void GpuSharedParametersUsage::initCopyData()
    {

        mCopyDataList.clear();

        const GpuConstantDefinitionMap& sharedmap = mSharedParams->getConstantDefinitions().map;
        for (auto const& [pName, shareddef] : sharedmap)
        {
            const GpuConstantDefinition* instdef = mParams->_findNamedConstantDefinition(pName, false);
            if (instdef)
            {
                // Check that the definitions are the same
                if (instdef->constType == shareddef.constType &&
                    instdef->arraySize <= shareddef.arraySize)
                {
                    CopyDataEntry e;
                    e.srcDefinition = &shareddef;
                    e.dstDefinition = instdef;
                    mCopyDataList.push_back(e);
                }
                else
                    LogManager::getSingleton().logWarning(
                        ::std::format("cannot copy shared parameter '{}' - type or variability mismatch", pName));
            }
        }

        mCopyDataVersion = mSharedParams->getVersion();
    }
    //---------------------------------------------------------------------
    void GpuSharedParametersUsage::_copySharedParamsToTargetParams() const
    {
        // check copy data version
        if (mCopyDataVersion != mSharedParams->getVersion())
            const_cast<GpuSharedParametersUsage*>(this)->initCopyData();

        // force const call to get*Pointer
        const GpuSharedParameters* sharedParams = mSharedParams.get();

        for (const CopyDataEntry& e : mCopyDataList)
        {
            if (e.dstDefinition->isFloat())
            {
                const float* pSrc = sharedParams->getFloatPointer(e.srcDefinition->physicalIndex);
                float* pDst = mParams->getFloatPointer(e.dstDefinition->physicalIndex);

                // Deal with matrix transposition here!!!
                // transposition is specific to the dest param set, shared params don't do it
                if (mParams->getTransposeMatrices() && (e.dstDefinition->constType == GpuConstantType::MATRIX_4X4))
                {
                    // for each matrix that needs to be transposed and copied,
                    for (size_t iMat = 0; iMat < e.dstDefinition->arraySize; ++iMat)
                    {
                        for (int row = 0; row < 4; ++row)
                            for (int col = 0; col < 4; ++col)
                                pDst[row * 4 + col] = pSrc[col * 4 + row];
                        pSrc += 16;
                        pDst += 16;
                    }
                }
                else
                {
                    if (e.dstDefinition->elementSize == e.srcDefinition->elementSize)
                    {
                        // simple copy
                        memcpy(pDst, pSrc, sizeof(float) * e.dstDefinition->elementSize * e.dstDefinition->arraySize);
                    }
                    else
                    {
                        // target params may be padded to 4 elements, shared params are packed
                        assert(e.dstDefinition->elementSize % 4 == 0);
                        size_t iterations = e.dstDefinition->elementSize / 4
                            * e.dstDefinition->arraySize;
                        assert(iterations > 0);
                        size_t valsPerIteration = e.srcDefinition->elementSize;
                        for (size_t l = 0; l < iterations; ++l)
                        {
                            memcpy(pDst, pSrc, sizeof(float) * valsPerIteration);
                            pSrc += valsPerIteration;
                            pDst += 4;
                        }
                    }
                }
            }
            else if (e.dstDefinition->isDouble())
            {
                const double* pSrc = sharedParams->getDoublePointer(e.srcDefinition->physicalIndex);
                double* pDst = mParams->getDoublePointer(e.dstDefinition->physicalIndex);

                // Deal with matrix transposition here!!!
                // transposition is specific to the dest param set, shared params don't do it
                if (mParams->getTransposeMatrices() && (e.dstDefinition->constType == GpuConstantType::MATRIX_DOUBLE_4X4))
                {
                    // for each matrix that needs to be transposed and copied,
                    for (size_t iMat = 0; iMat < e.dstDefinition->arraySize; ++iMat)
                    {
                        for (int row = 0; row < 4; ++row)
                            for (int col = 0; col < 4; ++col)
                                pDst[row * 4 + col] = pSrc[col * 4 + row];
                        pSrc += 16;
                        pDst += 16;
                    }
                }
                else
                {
                    if (e.dstDefinition->elementSize == e.srcDefinition->elementSize)
                    {
                        // simple copy
                        memcpy(pDst, pSrc, sizeof(double) * e.dstDefinition->elementSize * e.dstDefinition->arraySize);
                    }
                    else
                    {
                        // target params may be padded to 4 elements, shared params are packed
                        assert(e.dstDefinition->elementSize % 4 == 0);
                        size_t iterations = e.dstDefinition->elementSize / 4
                            * e.dstDefinition->arraySize;
                        assert(iterations > 0);
                        size_t valsPerIteration = e.srcDefinition->elementSize;
                        for (size_t l = 0; l < iterations; ++l)
                        {
                            memcpy(pDst, pSrc, sizeof(double) * valsPerIteration);
                            pSrc += valsPerIteration;
                            pDst += 4;
                        }
                    }
                }
            }
            else if (e.dstDefinition->isInt())
            {
                const int* pSrc = sharedParams->getIntPointer(e.srcDefinition->physicalIndex);
                int* pDst = mParams->getIntPointer(e.dstDefinition->physicalIndex);

                if (e.dstDefinition->elementSize == e.srcDefinition->elementSize)
                {
                    // simple copy
                    memcpy(pDst, pSrc, sizeof(int) * e.dstDefinition->elementSize * e.dstDefinition->arraySize);
                }
                else
                {
                    // target params may be padded to 4 elements, shared params are packed
                    assert(e.dstDefinition->elementSize % 4 == 0);
                    size_t iterations = (e.dstDefinition->elementSize / 4)
                        * e.dstDefinition->arraySize;
                    assert(iterations > 0);
                    size_t valsPerIteration = e.srcDefinition->elementSize;
                    for (size_t l = 0; l < iterations; ++l)
                    {
                        memcpy(pDst, pSrc, sizeof(int) * valsPerIteration);
                        pSrc += valsPerIteration;
                        pDst += 4;
                    }
                }
            }
            else if (e.dstDefinition->isUnsignedInt() || e.dstDefinition->isBool()) 
            {
                const uint* pSrc = sharedParams->getUnsignedIntPointer(e.srcDefinition->physicalIndex);
                uint* pDst = mParams->getUnsignedIntPointer(e.dstDefinition->physicalIndex);

                if (e.dstDefinition->elementSize == e.srcDefinition->elementSize)
                {
                    // simple copy
                    memcpy(pDst, pSrc, sizeof(uint) * e.dstDefinition->elementSize * e.dstDefinition->arraySize);
                }
                else
                {
                    // target params may be padded to 4 elements, shared params are packed
                    assert(e.dstDefinition->elementSize % 4 == 0);
                    size_t iterations = (e.dstDefinition->elementSize / 4)
                        * e.dstDefinition->arraySize;
                    assert(iterations > 0);
                    size_t valsPerIteration = e.srcDefinition->elementSize;
                    for (size_t l = 0; l < iterations; ++l)
                    {
                        memcpy(pDst, pSrc, sizeof(uint) * valsPerIteration);
                        pSrc += valsPerIteration;
                        pDst += 4;
                    }
                }
            }
            else {
                //TODO add error
            }
        }
    }



    //-----------------------------------------------------------------------------
    //      GpuProgramParameters Methods
    //-----------------------------------------------------------------------------
    GpuProgramParameters::GpuProgramParameters() :
        
         mActivePassIterationIndex(std::numeric_limits<size_t>::max())
    {
    }
    GpuProgramParameters::~GpuProgramParameters() = default;

    //-----------------------------------------------------------------------------

    GpuProgramParameters::GpuProgramParameters(const GpuProgramParameters& oth)
    {
        *this = oth;
    }

    //-----------------------------------------------------------------------------
    auto GpuProgramParameters::operator=(const GpuProgramParameters& oth) -> GpuProgramParameters&
    {
        // let compiler perform shallow copies of structures
        // AutoConstantEntry, RealConstantEntry, IntConstantEntry
        mConstants = oth.mConstants;
        mRegisters = oth.mRegisters;
        mAutoConstants = oth.mAutoConstants;
        mLogicalToPhysical = oth.mLogicalToPhysical;
        mNamedConstants = oth.mNamedConstants;
        copySharedParamSetUsage(oth.mSharedParamSets);

        mCombinedVariability = oth.mCombinedVariability;
        mTransposeMatrices = oth.mTransposeMatrices;
        mIgnoreMissingParams  = oth.mIgnoreMissingParams;
        mActivePassIterationIndex = oth.mActivePassIterationIndex;

        return *this;
    }
    //---------------------------------------------------------------------
    void GpuProgramParameters::copySharedParamSetUsage(const GpuSharedParamUsageList& srcList)
    {
        mSharedParamSets.clear();
        for (const auto & i : srcList)
        {
            mSharedParamSets.push_back(GpuSharedParametersUsage(i.getSharedParams(), this));
        }

    }
    //-----------------------------------------------------------------------------
    auto GpuProgramParameters::calculateSize() const -> size_t
    {
        size_t memSize = sizeof(*this);

        memSize += mConstants.size();
        memSize += mRegisters.size()*4;

        for (const auto & mAutoConstant : mAutoConstants)
        {
            memSize += sizeof(mAutoConstant);
        }

        return memSize;
    }
    //---------------------------------------------------------------------
    void GpuProgramParameters::_setNamedConstants(
        const GpuNamedConstantsPtr& namedConstants)
    {
        mNamedConstants = namedConstants;

        // Determine any extension to local buffers

        // Size and reset buffer (fill with zero to make comparison later ok)
        if (namedConstants->bufferSize*4 > mConstants.size())
        {
            mConstants.insert(mConstants.end(), namedConstants->bufferSize * 4 - mConstants.size(), 0);
        }

        if(namedConstants->registerCount > mRegisters.size())
            mRegisters.insert(mRegisters.end(), namedConstants->registerCount - mRegisters.size(), 0);
    }
    //---------------------------------------------------------------------
    void GpuProgramParameters::_setLogicalIndexes(const GpuLogicalBufferStructPtr& indexMap)
    {
        mLogicalToPhysical = indexMap;

        // resize the internal buffers
        // Note that these will only contain something after the first parameter
        // set has set some parameters

        // Size and reset buffer (fill with zero to make comparison later ok)
        if (indexMap && indexMap->bufferSize*4 > mConstants.size())
        {
            mConstants.insert(mConstants.end(), indexMap->bufferSize * 4 - mConstants.size(), 0);
        }
    }
    //---------------------------------------------------------------------()
    void GpuProgramParameters::setConstant(size_t index, const Vector4& vec)
    {
        setConstant(index, vec.ptr(), 1);
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setConstant(size_t index, Real val)
    {
        setConstant(index, Vector4{val, 0.0f, 0.0f, 0.0f});
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setConstant(size_t index, const Vector3& vec)
    {
        setConstant(index, Vector4{vec.x, vec.y, vec.z, 1.0f});
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setConstant(size_t index, const Vector2& vec)
    {
        setConstant(index, Vector4{vec.x, vec.y, 1.0f, 1.0f});
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setConstant(size_t index, const Matrix4& m)
    {
        // set as 4x 4-element floats
        if (mTransposeMatrices)
        {
            Matrix4 t = m.transpose();
            GpuProgramParameters::setConstant(index, t[0], 4);
        }
        else
        {
            GpuProgramParameters::setConstant(index, m[0], 4);
        }

    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setConstant(size_t index, const Matrix4* pMatrix,
                                           size_t numEntries)
    {
        if (mTransposeMatrices)
        {
            for (size_t i = 0; i < numEntries; ++i)
            {
                Matrix4 t = pMatrix[i].transpose();
                GpuProgramParameters::setConstant(index, t[0], 4);
                index += 4;
            }
        }
        else
        {
            GpuProgramParameters::setConstant(index, pMatrix[0][0], 4 * numEntries);
        }

    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setConstant(size_t index, const ColourValue& colour)
    {
        setConstant(index, colour.ptr(), 1);
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setConstant(size_t index, const float *val, size_t count)
    {
        // Raw buffer size is 4x count
        size_t rawCount = count * 4;
        // get physical index
        assert(mLogicalToPhysical && "GpuProgram hasn't set up the logical -> physical map!");

        size_t physicalIndex = _getConstantPhysicalIndex(index, rawCount, GpuParamVariability::GLOBAL, BaseConstantType::FLOAT);

        // Copy
        _writeRawConstants(physicalIndex, val, rawCount);

    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setConstant(size_t index, const double *val, size_t count)
    {
        // Raw buffer size is 4x count
        size_t rawCount = count * 4;
        // get physical index
        assert(mLogicalToPhysical && "GpuProgram hasn't set up the logical -> physical map!");

        size_t physicalIndex = _getConstantPhysicalIndex(index, rawCount, GpuParamVariability::GLOBAL, BaseConstantType::DOUBLE);
        // Copy
        _writeRawConstants(physicalIndex, val, rawCount);
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setConstant(size_t index, const int *val, size_t count)
    {
        // Raw buffer size is 4x count
        size_t rawCount = count * 4;
        // get physical index
        assert(mLogicalToPhysical && "GpuProgram hasn't set up the logical -> physical map!");

        size_t physicalIndex = _getConstantPhysicalIndex(index, rawCount, GpuParamVariability::GLOBAL, BaseConstantType::INT);
        // Copy
        _writeRawConstants(physicalIndex, val, rawCount);
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setConstant(size_t index, const uint *val, size_t count)
    {
        // Raw buffer size is 4x count
        size_t rawCount = count * 4;
        // get physical index
        assert(mLogicalToPhysical && "GpuProgram hasn't set up the logical -> physical map!");

        size_t physicalIndex = _getConstantPhysicalIndex(index, rawCount, GpuParamVariability::GLOBAL, BaseConstantType::INT);
        // Copy
        _writeRawConstants(physicalIndex, val, rawCount);
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::_writeRawConstant(size_t physicalIndex, const Matrix4& m, size_t elementCount)
    {

        // remember, raw content access uses raw float count rather than float4
        if (mTransposeMatrices)
        {
            Matrix4 t = m.transpose();
            _writeRawConstants(physicalIndex, t[0], elementCount > 16 ? 16 : elementCount);
        }
        else
        {
            _writeRawConstants(physicalIndex, m[0], elementCount > 16 ? 16 : elementCount);
        }

    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::_writeRawConstant(size_t physicalIndex, const Matrix3& m, size_t elementCount)
    {

        // remember, raw content access uses raw float count rather than float4
        if (mTransposeMatrices)
        {
            Matrix3 t = m.transpose();
            _writeRawConstants(physicalIndex, t[0], elementCount > 9 ? 9 : elementCount);
        }
        else
        {
            _writeRawConstants(physicalIndex, m[0], elementCount > 9 ? 9 : elementCount);
        }

    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::_writeRawConstant(size_t physicalIndex, const TransformBaseReal* pMatrix, size_t numEntries)
    {
        // remember, raw content access uses raw float count rather than float4
        if (mTransposeMatrices)
        {
            for (size_t i = 0; i < numEntries; ++i)
            {
                Matrix4 t = pMatrix[i].transpose();
                _writeRawConstants(physicalIndex, t[0], 16);
                physicalIndex += 16*sizeof(Real);
            }
        }
        else
        {
            _writeRawConstants(physicalIndex, pMatrix[0][0], 16 * numEntries);
        }


    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::_writeRawConstant(size_t physicalIndex,
                                                 const ColourValue& colour, size_t count)
    {
        // write either the number requested (for packed types) or up to 4
        _writeRawConstants(physicalIndex, colour.ptr(), std::min(count, (size_t)4));
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::_writeRawConstants(size_t physicalIndex, const double* val, size_t count)
    {
        assert(physicalIndex + sizeof(float) * count <= mConstants.size());
        for (size_t i = 0; i < count; ++i)
        {
            float tmp = val[i];
            memcpy(&mConstants[physicalIndex + i * sizeof(float)], &tmp, sizeof(float));
        }
    }
    void GpuProgramParameters::_writeRegisters(size_t index, const int* val, size_t count)
    {
        assert(index + count <= mRegisters.size());
        memcpy(&mRegisters[index], val, sizeof(int) * count);
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::_readRawConstants(size_t physicalIndex, size_t count, float* dest)
    {
        assert(physicalIndex + count <= mConstants.size());
        memcpy(dest, &mConstants[physicalIndex], sizeof(float) * count);
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::_readRawConstants(size_t physicalIndex, size_t count, int* dest)
    {
        assert(physicalIndex + count <= mConstants.size());
        memcpy(dest, &mConstants[physicalIndex], sizeof(int) * count);
    }
    //---------------------------------------------------------------------
    auto GpuProgramParameters::deriveVariability(GpuProgramParameters::AutoConstantType act) -> GpuParamVariability
    {
        using enum AutoConstantType;
        switch(act)
        {
        case VIEW_MATRIX:
        case INVERSE_VIEW_MATRIX:
        case TRANSPOSE_VIEW_MATRIX:
        case INVERSE_TRANSPOSE_VIEW_MATRIX:
        case PROJECTION_MATRIX:
        case INVERSE_PROJECTION_MATRIX:
        case TRANSPOSE_PROJECTION_MATRIX:
        case INVERSE_TRANSPOSE_PROJECTION_MATRIX:
        case VIEWPROJ_MATRIX:
        case INVERSE_VIEWPROJ_MATRIX:
        case TRANSPOSE_VIEWPROJ_MATRIX:
        case INVERSE_TRANSPOSE_VIEWPROJ_MATRIX:
        case RENDER_TARGET_FLIPPING:
        case VERTEX_WINDING:
        case AMBIENT_LIGHT_COLOUR:
        case DERIVED_AMBIENT_LIGHT_COLOUR:
        case DERIVED_SCENE_COLOUR:
        case FOG_COLOUR:
        case FOG_PARAMS:
        case SURFACE_AMBIENT_COLOUR:
        case SURFACE_DIFFUSE_COLOUR:
        case SURFACE_SPECULAR_COLOUR:
        case SURFACE_EMISSIVE_COLOUR:
        case SURFACE_SHININESS:
        case SURFACE_ALPHA_REJECTION_VALUE:
        case CAMERA_POSITION:
        case CAMERA_RELATIVE_POSITION:
        case TIME:
        case TIME_0_X:
        case COSTIME_0_X:
        case SINTIME_0_X:
        case TANTIME_0_X:
        case TIME_0_X_PACKED:
        case TIME_0_1:
        case COSTIME_0_1:
        case SINTIME_0_1:
        case TANTIME_0_1:
        case TIME_0_1_PACKED:
        case TIME_0_2PI:
        case COSTIME_0_2PI:
        case SINTIME_0_2PI:
        case TANTIME_0_2PI:
        case TIME_0_2PI_PACKED:
        case FRAME_TIME:
        case FPS:
        case VIEWPORT_WIDTH:
        case VIEWPORT_HEIGHT:
        case INVERSE_VIEWPORT_WIDTH:
        case INVERSE_VIEWPORT_HEIGHT:
        case VIEWPORT_SIZE:
        case TEXEL_OFFSETS:
        case TEXTURE_SIZE:
        case INVERSE_TEXTURE_SIZE:
        case PACKED_TEXTURE_SIZE:
        case SCENE_DEPTH_RANGE:
        case VIEW_DIRECTION:
        case VIEW_SIDE_VECTOR:
        case VIEW_UP_VECTOR:
        case FOV:
        case NEAR_CLIP_DISTANCE:
        case FAR_CLIP_DISTANCE:
        case PASS_NUMBER:
        case TEXTURE_MATRIX:
        case LOD_CAMERA_POSITION:

            return GpuParamVariability::GLOBAL;

        case WORLD_MATRIX:
        case INVERSE_WORLD_MATRIX:
        case TRANSPOSE_WORLD_MATRIX:
        case INVERSE_TRANSPOSE_WORLD_MATRIX:
        case WORLD_MATRIX_ARRAY_3x4:
        case WORLD_MATRIX_ARRAY:
        case WORLD_DUALQUATERNION_ARRAY_2x4:
        case WORLD_SCALE_SHEAR_MATRIX_ARRAY_3x4:
        case WORLDVIEW_MATRIX:
        case INVERSE_WORLDVIEW_MATRIX:
        case TRANSPOSE_WORLDVIEW_MATRIX:
        case INVERSE_TRANSPOSE_WORLDVIEW_MATRIX:
        case NORMAL_MATRIX:
        case WORLDVIEWPROJ_MATRIX:
        case INVERSE_WORLDVIEWPROJ_MATRIX:
        case TRANSPOSE_WORLDVIEWPROJ_MATRIX:
        case INVERSE_TRANSPOSE_WORLDVIEWPROJ_MATRIX:
        case CAMERA_POSITION_OBJECT_SPACE:
        case LOD_CAMERA_POSITION_OBJECT_SPACE:
        case CUSTOM:
        case ANIMATION_PARAMETRIC:

            return GpuParamVariability::PER_OBJECT;

        case LIGHT_POSITION_OBJECT_SPACE:
        case LIGHT_DIRECTION_OBJECT_SPACE:
        case LIGHT_DISTANCE_OBJECT_SPACE:
        case LIGHT_POSITION_OBJECT_SPACE_ARRAY:
        case LIGHT_DIRECTION_OBJECT_SPACE_ARRAY:
        case LIGHT_DISTANCE_OBJECT_SPACE_ARRAY:
        case TEXTURE_WORLDVIEWPROJ_MATRIX:
        case TEXTURE_WORLDVIEWPROJ_MATRIX_ARRAY:
        case SPOTLIGHT_WORLDVIEWPROJ_MATRIX:
        case SPOTLIGHT_WORLDVIEWPROJ_MATRIX_ARRAY:
        case SHADOW_EXTRUSION_DISTANCE:

            // These depend on BOTH lights and objects
            return GpuParamVariability::PER_OBJECT | GpuParamVariability::LIGHTS;

        case LIGHT_COUNT:
        case LIGHT_DIFFUSE_COLOUR:
        case LIGHT_SPECULAR_COLOUR:
        case LIGHT_POSITION:
        case LIGHT_DIRECTION:
        case LIGHT_POSITION_VIEW_SPACE:
        case LIGHT_DIRECTION_VIEW_SPACE:
        case SHADOW_SCENE_DEPTH_RANGE:
        case SHADOW_SCENE_DEPTH_RANGE_ARRAY:
        case SHADOW_COLOUR:
        case LIGHT_POWER_SCALE:
        case LIGHT_DIFFUSE_COLOUR_POWER_SCALED:
        case LIGHT_SPECULAR_COLOUR_POWER_SCALED:
        case LIGHT_NUMBER:
        case LIGHT_CASTS_SHADOWS:
        case LIGHT_CASTS_SHADOWS_ARRAY:
        case LIGHT_ATTENUATION:
        case SPOTLIGHT_PARAMS:
        case LIGHT_DIFFUSE_COLOUR_ARRAY:
        case LIGHT_SPECULAR_COLOUR_ARRAY:
        case LIGHT_DIFFUSE_COLOUR_POWER_SCALED_ARRAY:
        case LIGHT_SPECULAR_COLOUR_POWER_SCALED_ARRAY:
        case LIGHT_POSITION_ARRAY:
        case LIGHT_DIRECTION_ARRAY:
        case LIGHT_POSITION_VIEW_SPACE_ARRAY:
        case LIGHT_DIRECTION_VIEW_SPACE_ARRAY:
        case LIGHT_POWER_SCALE_ARRAY:
        case LIGHT_ATTENUATION_ARRAY:
        case SPOTLIGHT_PARAMS_ARRAY:
        case TEXTURE_VIEWPROJ_MATRIX:
        case TEXTURE_VIEWPROJ_MATRIX_ARRAY:
        case SPOTLIGHT_VIEWPROJ_MATRIX:
        case SPOTLIGHT_VIEWPROJ_MATRIX_ARRAY:
        case LIGHT_CUSTOM:

            return GpuParamVariability::LIGHTS;

        case DERIVED_LIGHT_DIFFUSE_COLOUR:
        case DERIVED_LIGHT_SPECULAR_COLOUR:
        case DERIVED_LIGHT_DIFFUSE_COLOUR_ARRAY:
        case DERIVED_LIGHT_SPECULAR_COLOUR_ARRAY:

            return GpuParamVariability::GLOBAL | GpuParamVariability::LIGHTS;

        case PASS_ITERATION_NUMBER:

            return GpuParamVariability::PASS_ITERATION_NUMBER;

        default:
            return GpuParamVariability::GLOBAL;
        };

    }
    //---------------------------------------------------------------------
    auto GpuProgramParameters::getConstantLogicalIndexUse(
        size_t logicalIndex, size_t requestedSize, Ogre::GpuParamVariability variability, BaseConstantType type) -> GpuLogicalIndexUse*
    {
        OgreAssert(type != BaseConstantType::SAMPLER, "");
        if (!mLogicalToPhysical)
            return nullptr;

        GpuLogicalIndexUse* indexUse = nullptr;

        auto logi = mLogicalToPhysical->map.find(logicalIndex);
        if (logi == mLogicalToPhysical->map.end())
        {
            if (requestedSize)
            {
                size_t physicalIndex = mConstants.size();

                // Expand at buffer end
                mConstants.insert(mConstants.end(), requestedSize*4, 0);

                // Record extended size for future GPU params re-using this information
                mLogicalToPhysical->bufferSize = mConstants.size()/4;

                // low-level programs will not know about mapping ahead of time, so
                // populate it. Other params objects will be able to just use this
                // accepted mapping since the constant structure will be the same

                // Set up a mapping for all items in the count
                size_t currPhys = physicalIndex;
                size_t count = requestedSize / 4;
                GpuLogicalIndexUseMap::iterator insertedIterator;

                for (size_t logicalNum = 0; logicalNum < count; ++logicalNum)
                {
                    auto it =
                            mLogicalToPhysical->map.emplace(
                                logicalIndex + logicalNum,
                                GpuLogicalIndexUse{currPhys, requestedSize, variability, type}).first;
                    currPhys += 4;

                    if (logicalNum == 0)
                        insertedIterator = it;
                }

                indexUse = &(insertedIterator->second);
            }
            else
            {
                // no match & ignore
                return nullptr;
            }

        }
        else
        {
            size_t physicalIndex = logi->second.physicalIndex;
            indexUse = &(logi->second);
            // check size
            if (logi->second.currentSize < requestedSize)
            {
                // init buffer entry wasn't big enough; could be a mistake on the part
                // of the original use, or perhaps a variable length we can't predict
                // until first actual runtime use e.g. world matrix array
                size_t insertCount = requestedSize - logi->second.currentSize;
                auto insertPos = mConstants.begin();
                std::advance(insertPos, physicalIndex);
                mConstants.insert(insertPos, insertCount*4, 0);

                // shift all physical positions after this one
                for (auto& p : mLogicalToPhysical->map)
                {
                    if (p.second.physicalIndex > physicalIndex)
                        p.second.physicalIndex += insertCount*4;
                }
                mLogicalToPhysical->bufferSize += insertCount;
                for (auto& ac : mAutoConstants)
                {
                    auto def = getAutoConstantDefinition(ac.paramType);
                    if (ac.physicalIndex > physicalIndex && def)
                    {
                        ac.physicalIndex += insertCount*4;
                    }
                }
                if (mNamedConstants)
                {
                    for (auto& p : mNamedConstants->map)
                    {
                        if (p.second.physicalIndex > physicalIndex)
                            p.second.physicalIndex += insertCount * 4;
                    }
                    mNamedConstants->bufferSize += insertCount;
                }

                logi->second.currentSize += insertCount;
            }
        }

        if (indexUse && requestedSize)
            indexUse->variability = variability;

        return indexUse;
    }
    //-----------------------------------------------------------------------------
    auto GpuProgramParameters::_getConstantPhysicalIndex(
        size_t logicalIndex, size_t requestedSize, Ogre::GpuParamVariability variability, BaseConstantType type) -> size_t
    {
        GpuLogicalIndexUse* indexUse = getConstantLogicalIndexUse(logicalIndex, requestedSize, variability, type);
        return indexUse ? indexUse->physicalIndex : 0;
    }
    //-----------------------------------------------------------------------------
    auto GpuProgramParameters::getLogicalIndexForPhysicalIndex(size_t physicalIndex) -> size_t
    {
        // perhaps build a reverse map of this sometime (shared in GpuProgram)
        for (const auto& p : mLogicalToPhysical->map)
        {
            if (p.second.physicalIndex == physicalIndex)
                return p.first;
        }
        return std::numeric_limits<size_t>::max();
    }

    //-----------------------------------------------------------------------------
    auto GpuProgramParameters::getConstantDefinitions() const noexcept -> const GpuNamedConstants&
    {
        if (!mNamedConstants)
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "Named constants have not been initialised, perhaps a compile error");

        return *mNamedConstants;
    }
    //-----------------------------------------------------------------------------
    auto GpuProgramParameters::getConstantDefinition(std::string_view name) const -> const GpuConstantDefinition&
    {
        if (!mNamedConstants)
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "Named constants have not been initialised, perhaps a compile error",
                        "GpuProgramParameters::getConstantDefinitionIterator");


        // locate, and throw exception if not found
        const GpuConstantDefinition* def = _findNamedConstantDefinition(name, true);

        return *def;

    }
    //-----------------------------------------------------------------------------
    auto
    GpuProgramParameters::_findNamedConstantDefinition(std::string_view name,
                                                       bool throwExceptionIfNotFound) const -> const GpuConstantDefinition*
    {
        if (!mNamedConstants)
        {
            if (throwExceptionIfNotFound)
                OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                            "Named constants have not been initialised, perhaps a compile error");
            return nullptr;
        }

        // strip array extension
        size_t arrStart = name.back() == ']' ? name.find('[') : String::npos;
        auto i = mNamedConstants->map.find(arrStart == String::npos ? name : name.substr(0, arrStart));
        if (i == mNamedConstants->map.end() || (i->second.arraySize == 1 && arrStart != String::npos))
        {
            if (throwExceptionIfNotFound)
			{
                OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
				::std::format("Parameter called {} does not exist. ", name),
                            "GpuProgramParameters::_findNamedConstantDefinition");
			}
            return nullptr;
        }
        else
        {
            return &(i->second);
        }
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setAutoConstant(size_t index, AutoConstantType acType, uint32 extraInfo)
    {
        // Get auto constant definition for sizing
        const AutoConstantDefinition* autoDef = getAutoConstantDefinition(acType);

        if(!autoDef)
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("No constant definition found for type {}",
                        acType),
                        "GpuProgramParameters::setAutoConstant");

        // round up to nearest multiple of 4
        size_t sz = autoDef->elementCount;
        if (sz % 4 > 0)
        {
            sz += 4 - (sz % 4);
        }

        GpuLogicalIndexUse* indexUse = getConstantLogicalIndexUse(index, sz, deriveVariability(acType), BaseConstantType::FLOAT);

        if(indexUse)
            _setRawAutoConstant(indexUse->physicalIndex, acType, extraInfo, indexUse->variability, sz);
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::_setRawAutoConstant(size_t physicalIndex,
                                                   AutoConstantType acType, uint32 extraInfo, GpuParamVariability variability, uint8 elementSize)
    {
        // update existing index if it exists
        bool found = false;
        for (auto & mAutoConstant : mAutoConstants)
        {
            if (mAutoConstant.physicalIndex == physicalIndex)
            {
                mAutoConstant.paramType = acType;
                mAutoConstant.data = extraInfo;
                mAutoConstant.elementCount = elementSize;
                mAutoConstant.variability = variability;
                found = true;
                break;
            }
        }
        if (!found)
            mAutoConstants.push_back(AutoConstantEntry(acType, physicalIndex, extraInfo, variability, elementSize));

        mCombinedVariability |= variability;


    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::_setRawAutoConstantReal(size_t physicalIndex,
                                                       AutoConstantType acType, float rData, GpuParamVariability variability, uint8 elementSize)
    {
        // update existing index if it exists
        bool found = false;
        for (auto & mAutoConstant : mAutoConstants)
        {
            if (mAutoConstant.physicalIndex == physicalIndex)
            {
                mAutoConstant.paramType = acType;
                mAutoConstant.fData = rData;
                mAutoConstant.elementCount = elementSize;
                mAutoConstant.variability = variability;
                found = true;
                break;
            }
        }
        if (!found)
            mAutoConstants.push_back(AutoConstantEntry(acType, physicalIndex, rData, variability, elementSize));

        mCombinedVariability |= variability;
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::clearAutoConstant(size_t index)
    {
        GpuLogicalIndexUse* indexUse = getConstantLogicalIndexUse(index, 0, GpuParamVariability::GLOBAL, BaseConstantType::FLOAT);

        if (indexUse)
        {
            indexUse->variability = GpuParamVariability::GLOBAL;
            size_t physicalIndex = indexUse->physicalIndex;
            // update existing index if it exists
            for (auto i = mAutoConstants.begin();
                 i != mAutoConstants.end(); ++i)
            {
                if (i->physicalIndex == physicalIndex)
                {
                    mAutoConstants.erase(i);
                    break;
                }
            }
        }
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::clearNamedAutoConstant(std::string_view name)
    {
        const GpuConstantDefinition* def = _findNamedConstantDefinition(name);
        if (def)
        {
            def->variability = GpuParamVariability::GLOBAL;

            // Autos are always floating point
            if (def->isFloat()) {
                for (auto i = mAutoConstants.begin();
                     i != mAutoConstants.end(); ++i)
                {
                    if (i->physicalIndex == def->physicalIndex)
                    {
                        mAutoConstants.erase(i);
                        break;
                    }
                }
            }
        }
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::clearAutoConstants()
    {
        mAutoConstants.clear();
        mCombinedVariability = GpuParamVariability::GLOBAL;
    }
    //-----------------------------------------------------------------------------
    void GpuProgramParameters::setAutoConstantReal(size_t index, AutoConstantType acType, float rData)
    {
        // Get auto constant definition for sizing
        const AutoConstantDefinition* autoDef = getAutoConstantDefinition(acType);

        if(!autoDef)
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, ::std::format("No constant definition found for type {}",
                        acType),
                        "GpuProgramParameters::setAutoConstantReal");

        // round up to nearest multiple of 4
        size_t sz = autoDef->elementCount;
        if (sz % 4 > 0)
        {
            sz += 4 - (sz % 4);
        }

        GpuLogicalIndexUse* indexUse = getConstantLogicalIndexUse(index, sz, deriveVariability(acType), BaseConstantType::FLOAT);

        _setRawAutoConstantReal(indexUse->physicalIndex, acType, rData, indexUse->variability, sz);
    }
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    void GpuProgramParameters::_updateAutoParams(const AutoParamDataSource* source, GpuParamVariability mask)
    {
        // abort early if no autos
        if (!hasAutoConstants()) return;
        // abort early if variability doesn't match any param
        if (!(mask & mCombinedVariability))
            return;

        size_t index;
        size_t numMatrices;
        const Affine3* pMatrix;
        size_t m;
        Vector3 vec3;
        Matrix3 m3;
        Matrix4 scaleM;
        DualQuaternion dQuat;

        mActivePassIterationIndex = std::numeric_limits<size_t>::max();

        // Autoconstant index is not a physical index
        for (auto & mAutoConstant : mAutoConstants)
        {
            // Only update needed slots
            if (!!(mAutoConstant.variability & mask))
            {

                using enum AutoConstantType;
                switch(mAutoConstant.paramType)
                {
                case VIEW_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getViewMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_VIEW_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseViewMatrix(),mAutoConstant.elementCount);
                    break;
                case TRANSPOSE_VIEW_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTransposeViewMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_TRANSPOSE_VIEW_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseTransposeViewMatrix(),mAutoConstant.elementCount);
                    break;

                case PROJECTION_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getProjectionMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_PROJECTION_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseProjectionMatrix(),mAutoConstant.elementCount);
                    break;
                case TRANSPOSE_PROJECTION_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTransposeProjectionMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_TRANSPOSE_PROJECTION_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseTransposeProjectionMatrix(),mAutoConstant.elementCount);
                    break;

                case VIEWPROJ_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getViewProjectionMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_VIEWPROJ_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseViewProjMatrix(),mAutoConstant.elementCount);
                    break;
                case TRANSPOSE_VIEWPROJ_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTransposeViewProjMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_TRANSPOSE_VIEWPROJ_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseTransposeViewProjMatrix(),mAutoConstant.elementCount);
                    break;
                case RENDER_TARGET_FLIPPING:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getCurrentRenderTarget()->requiresTextureFlipping() ? -1.f : +1.f);
                    break;
                case VERTEX_WINDING:
                    {
                        RenderSystem* rsys = Root::getSingleton().getRenderSystem();
                        _writeRawConstant(mAutoConstant.physicalIndex, rsys->getInvertVertexWinding() ? -1.f : +1.f);
                    }
                    break;

                    // NB ambient light still here because it's not related to a specific light
                case AMBIENT_LIGHT_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getAmbientLightColour(),
                                      mAutoConstant.elementCount);
                    break;
                case DERIVED_AMBIENT_LIGHT_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getDerivedAmbientLightColour(),
                                      mAutoConstant.elementCount);
                    break;
                case DERIVED_SCENE_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getDerivedSceneColour(),
                                      mAutoConstant.elementCount);
                    break;

                case FOG_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getFogColour(), mAutoConstant.elementCount);
                    break;
                case FOG_PARAMS:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getFogParams(), mAutoConstant.elementCount);
                    break;
                case POINT_PARAMS:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getPointParams(), mAutoConstant.elementCount);
                    break;
                case SURFACE_AMBIENT_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSurfaceAmbientColour(),
                                      mAutoConstant.elementCount);
                    break;
                case SURFACE_DIFFUSE_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSurfaceDiffuseColour(),
                                      mAutoConstant.elementCount);
                    break;
                case SURFACE_SPECULAR_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSurfaceSpecularColour(),
                                      mAutoConstant.elementCount);
                    break;
                case SURFACE_EMISSIVE_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSurfaceEmissiveColour(),
                                      mAutoConstant.elementCount);
                    break;
                case SURFACE_SHININESS:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSurfaceShininess());
                    break;
                case SURFACE_ALPHA_REJECTION_VALUE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSurfaceAlphaRejectionValue());
                    break;

                case CAMERA_POSITION:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getCameraPosition(), mAutoConstant.elementCount);
                    break;
                case CAMERA_RELATIVE_POSITION:
                    _writeRawConstant (mAutoConstant.physicalIndex, source->getCameraRelativePosition(), mAutoConstant.elementCount);
                    break;
                case TIME:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTime() * mAutoConstant.fData);
                    break;
                case TIME_0_X:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTime_0_X(mAutoConstant.fData));
                    break;
                case COSTIME_0_X:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getCosTime_0_X(mAutoConstant.fData));
                    break;
                case SINTIME_0_X:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSinTime_0_X(mAutoConstant.fData));
                    break;
                case TANTIME_0_X:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTanTime_0_X(mAutoConstant.fData));
                    break;
                case TIME_0_X_PACKED:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTime_0_X_packed(mAutoConstant.fData), mAutoConstant.elementCount);
                    break;
                case TIME_0_1:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTime_0_1(mAutoConstant.fData));
                    break;
                case COSTIME_0_1:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getCosTime_0_1(mAutoConstant.fData));
                    break;
                case SINTIME_0_1:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSinTime_0_1(mAutoConstant.fData));
                    break;
                case TANTIME_0_1:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTanTime_0_1(mAutoConstant.fData));
                    break;
                case TIME_0_1_PACKED:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTime_0_1_packed(mAutoConstant.fData), mAutoConstant.elementCount);
                    break;
                case TIME_0_2PI:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTime_0_2Pi(mAutoConstant.fData));
                    break;
                case COSTIME_0_2PI:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getCosTime_0_2Pi(mAutoConstant.fData));
                    break;
                case SINTIME_0_2PI:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSinTime_0_2Pi(mAutoConstant.fData));
                    break;
                case TANTIME_0_2PI:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTanTime_0_2Pi(mAutoConstant.fData));
                    break;
                case TIME_0_2PI_PACKED:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTime_0_2Pi_packed(mAutoConstant.fData), mAutoConstant.elementCount);
                    break;
                case FRAME_TIME:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getFrameTime() * mAutoConstant.fData);
                    break;
                case FPS:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getFPS());
                    break;
                case VIEWPORT_WIDTH:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getViewportWidth());
                    break;
                case VIEWPORT_HEIGHT:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getViewportHeight());
                    break;
                case INVERSE_VIEWPORT_WIDTH:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseViewportWidth());
                    break;
                case INVERSE_VIEWPORT_HEIGHT:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseViewportHeight());
                    break;
                case VIEWPORT_SIZE:
                    _writeRawConstant(mAutoConstant.physicalIndex, Vector4f{
                        source->getViewportWidth(),
                        source->getViewportHeight(),
                        source->getInverseViewportWidth(),
                        source->getInverseViewportHeight()}, mAutoConstant.elementCount);
                    break;
                case TEXEL_OFFSETS:
                    {
                        RenderSystem* rsys = Root::getSingleton().getRenderSystem();
                        _writeRawConstant(mAutoConstant.physicalIndex, Vector4f{
                            rsys->getHorizontalTexelOffset(),
                            rsys->getVerticalTexelOffset(),
                            rsys->getHorizontalTexelOffset() * source->getInverseViewportWidth(),
                            rsys->getVerticalTexelOffset() * source->getInverseViewportHeight()},
                                          mAutoConstant.elementCount);
                    }
                    break;
                case TEXTURE_SIZE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTextureSize(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case INVERSE_TEXTURE_SIZE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseTextureSize(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case PACKED_TEXTURE_SIZE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getPackedTextureSize(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case SCENE_DEPTH_RANGE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSceneDepthRange(), mAutoConstant.elementCount);
                    break;
                case VIEW_DIRECTION:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getViewDirection());
                    break;
                case VIEW_SIDE_VECTOR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getViewSideVector());
                    break;
                case VIEW_UP_VECTOR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getViewUpVector());
                    break;
                case FOV:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getFOV());
                    break;
                case NEAR_CLIP_DISTANCE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getNearClipDistance());
                    break;
                case FAR_CLIP_DISTANCE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getFarClipDistance());
                    break;
                case PASS_NUMBER:
                    _writeRawConstant(mAutoConstant.physicalIndex, (float)source->getPassNumber());
                    break;
                case PASS_ITERATION_NUMBER:
                    // this is actually just an initial set-up, it's bound separately, so still global
                    _writeRawConstant(mAutoConstant.physicalIndex, 0.0f);
                    mActivePassIterationIndex = mAutoConstant.physicalIndex;
                    break;
                case TEXTURE_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTextureTransformMatrix(mAutoConstant.data),mAutoConstant.elementCount);
                    break;
                case LOD_CAMERA_POSITION:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLodCameraPosition(), mAutoConstant.elementCount);
                    break;

                case TEXTURE_WORLDVIEWPROJ_MATRIX:
                    // can also be updated in lights
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTextureWorldViewProjMatrix(mAutoConstant.data),mAutoConstant.elementCount);
                    break;
                case TEXTURE_WORLDVIEWPROJ_MATRIX_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                    {
                        // can also be updated in lights
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Matrix4),
                                          source->getTextureWorldViewProjMatrix(l),mAutoConstant.elementCount);
                    }
                    break;
                case SPOTLIGHT_WORLDVIEWPROJ_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSpotlightWorldViewProjMatrix(mAutoConstant.data),mAutoConstant.elementCount);
                    break;
                case SPOTLIGHT_WORLDVIEWPROJ_MATRIX_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Matrix4), source->getSpotlightWorldViewProjMatrix(l), mAutoConstant.elementCount);
                    break;
                case LIGHT_POSITION_OBJECT_SPACE:
                    _writeRawConstant(mAutoConstant.physicalIndex,
                                      source->getInverseWorldMatrix() *
                                          source->getLightAs4DVector(mAutoConstant.data),
                                      mAutoConstant.elementCount);
                    break;
                case LIGHT_DIRECTION_OBJECT_SPACE:
                    // We need the inverse of the inverse transpose
                    m3 = source->getTransposeWorldMatrix().linear();
                    vec3 = m3 * source->getLightDirection(mAutoConstant.data);
                    vec3.normalise();
                    // Set as 4D vector for compatibility
                    _writeRawConstant(mAutoConstant.physicalIndex, Vector4f{vec3.x, vec3.y, vec3.z, 0.0f}, mAutoConstant.elementCount);
                    break;
                case LIGHT_DISTANCE_OBJECT_SPACE:
                    vec3 = source->getInverseWorldMatrix() * source->getLightPosition(mAutoConstant.data);
                    _writeRawConstant(mAutoConstant.physicalIndex, vec3.length());
                    break;
                case LIGHT_POSITION_OBJECT_SPACE_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Vector4),
                                          source->getInverseWorldMatrix() *
                                              source->getLightAs4DVector(l),
                                          mAutoConstant.elementCount);
                    break;

                case LIGHT_DIRECTION_OBJECT_SPACE_ARRAY:
                    // We need the inverse of the inverse transpose
                    m3 = source->getTransposeWorldMatrix().linear();
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                    {
                        vec3 = m3 * source->getLightDirection(l);
                        vec3.normalise();
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Vector4f),
                                          Vector4f{vec3.x, vec3.y, vec3.z, 0.0f}, mAutoConstant.elementCount);
                    }
                    break;

                case LIGHT_DISTANCE_OBJECT_SPACE_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                    {
                        vec3 = source->getInverseWorldMatrix() * source->getLightPosition(l);
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Real), vec3.length());
                    }
                    break;

                case WORLD_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getWorldMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_WORLD_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseWorldMatrix(),mAutoConstant.elementCount);
                    break;
                case TRANSPOSE_WORLD_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTransposeWorldMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_TRANSPOSE_WORLD_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseTransposeWorldMatrix(),mAutoConstant.elementCount);
                    break;

                case WORLD_MATRIX_ARRAY_3x4:
                    // Loop over matrices
                    pMatrix = source->getWorldMatrixArray();
                    numMatrices = source->getWorldMatrixCount();
                    index = mAutoConstant.physicalIndex;
                    for (m = 0; m < numMatrices; ++m)
                    {
                        _writeRawConstants(index, (*pMatrix)[0], 12);
                        index += 12*sizeof(Real);
                        ++pMatrix;
                    }
                    break;
                case WORLD_MATRIX_ARRAY:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getWorldMatrixArray(),
                                      source->getWorldMatrixCount());
                    break;
                case WORLD_DUALQUATERNION_ARRAY_2x4:
                    // Loop over matrices
                    pMatrix = source->getWorldMatrixArray();
                    numMatrices = source->getWorldMatrixCount();
                    index = mAutoConstant.physicalIndex;
                    for (m = 0; m < numMatrices; ++m)
                    {
                        dQuat.fromTransformationMatrix(*pMatrix);
                        _writeRawConstants(index, dQuat.ptr(), 8);
                        index += sizeof(DualQuaternion);
                        ++pMatrix;
                    }
                    break;
                case WORLD_SCALE_SHEAR_MATRIX_ARRAY_3x4:
                    // Loop over matrices
                    pMatrix = source->getWorldMatrixArray();
                    numMatrices = source->getWorldMatrixCount();
                    index = mAutoConstant.physicalIndex;

                    scaleM = Matrix4::IDENTITY;

                    for (m = 0; m < numMatrices; ++m)
                    {
                        //Based on Matrix4::decompostion, but we don't need the rotation or position components
                        //but do need the scaling and shearing. Shearing isn't available from Matrix4::decomposition
                        m3 = pMatrix->linear();

                        Matrix3 matQ;
                        Vector3 scale;

                        //vecU is the scaling component with vecU[0] = u01, vecU[1] = u02, vecU[2] = u12
                        //vecU[0] is shearing (x,y), vecU[1] is shearing (x,z), and vecU[2] is shearing (y,z)
                        //The first component represents the coordinate that is being sheared,
                        //while the second component represents the coordinate which performs the shearing.
                        Vector3 vecU;
                        m3.QDUDecomposition( matQ, scale, vecU );

                        scaleM[0][0] = scale.x;
                        scaleM[1][1] = scale.y;
                        scaleM[2][2] = scale.z;

                        scaleM[0][1] = vecU[0];
                        scaleM[0][2] = vecU[1];
                        scaleM[1][2] = vecU[2];

                        _writeRawConstants(index, scaleM[0], 12);
                        index += 12*sizeof(Real);
                        ++pMatrix;
                    }
                    break;
                case WORLDVIEW_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getWorldViewMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_WORLDVIEW_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseWorldViewMatrix(),mAutoConstant.elementCount);
                    break;
                case TRANSPOSE_WORLDVIEW_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTransposeWorldViewMatrix(),mAutoConstant.elementCount);
                    break;
                case NORMAL_MATRIX:
                    if(mAutoConstant.elementCount == 9) // check if shader supports packed data
                    {
                        _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseTransposeWorldViewMatrix().linear(),mAutoConstant.elementCount);
                        break;
                    }
                    [[fallthrough]]; // fallthrough to padded 4x4 matrix
                case INVERSE_TRANSPOSE_WORLDVIEW_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseTransposeWorldViewMatrix(),mAutoConstant.elementCount);
                    break;

                case WORLDVIEWPROJ_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getWorldViewProjMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_WORLDVIEWPROJ_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseWorldViewProjMatrix(),mAutoConstant.elementCount);
                    break;
                case TRANSPOSE_WORLDVIEWPROJ_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTransposeWorldViewProjMatrix(),mAutoConstant.elementCount);
                    break;
                case INVERSE_TRANSPOSE_WORLDVIEWPROJ_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getInverseTransposeWorldViewProjMatrix(),mAutoConstant.elementCount);
                    break;
                case CAMERA_POSITION_OBJECT_SPACE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getCameraPositionObjectSpace(), mAutoConstant.elementCount);
                    break;
                case LOD_CAMERA_POSITION_OBJECT_SPACE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLodCameraPositionObjectSpace(), mAutoConstant.elementCount);
                    break;

                case CUSTOM:
                case ANIMATION_PARAMETRIC:
                    source->getCurrentRenderable()->_updateCustomGpuParameter(mAutoConstant, this);
                    break;
                case LIGHT_CUSTOM:
                    source->updateLightCustomGpuParameter(mAutoConstant, this);
                    break;
                case LIGHT_COUNT:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLightCount());
                    break;
                case LIGHT_DIFFUSE_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLightDiffuseColour(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case LIGHT_SPECULAR_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLightSpecularColour(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case LIGHT_POSITION:
                    // Get as 4D vector, works for directional lights too
                    // Use element count in case uniform slot is smaller
                    _writeRawConstant(mAutoConstant.physicalIndex,
                                      source->getLightAs4DVector(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case LIGHT_DIRECTION:
                    vec3 = source->getLightDirection(mAutoConstant.data);
                    // Set as 4D vector for compatibility
                    // Use element count in case uniform slot is smaller
                    _writeRawConstant(mAutoConstant.physicalIndex, Vector4f{vec3.x, vec3.y, vec3.z, 1.0f}, mAutoConstant.elementCount);
                    break;
                case LIGHT_POSITION_VIEW_SPACE:
                    _writeRawConstant(mAutoConstant.physicalIndex,
                                      source->getViewMatrix() * source->getLightAs4DVector(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case LIGHT_DIRECTION_VIEW_SPACE:
                    m3 = source->getInverseTransposeViewMatrix().linear();
                    // inverse transpose in case of scaling
                    vec3 = m3 * source->getLightDirection(mAutoConstant.data);
                    vec3.normalise();
                    // Set as 4D vector for compatibility
                    _writeRawConstant(mAutoConstant.physicalIndex, Vector4f{vec3.x, vec3.y, vec3.z, 0.0f},mAutoConstant.elementCount);
                    break;
                case SHADOW_EXTRUSION_DISTANCE:
                    // extrusion is in object-space, so we have to rescale by the inverse
                    // of the world scaling to deal with scaled objects
                    m3 = source->getWorldMatrix().linear();
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getShadowExtrusionDistance() /
                                      Math::Sqrt(std::max(std::max(m3.GetColumn(0).squaredLength(), m3.GetColumn(1).squaredLength()), m3.GetColumn(2).squaredLength())));
                    break;
                case SHADOW_SCENE_DEPTH_RANGE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getShadowSceneDepthRange(mAutoConstant.data));
                    break;
                case SHADOW_SCENE_DEPTH_RANGE_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*mAutoConstant.elementCount, source->getShadowSceneDepthRange(l), mAutoConstant.elementCount);
                    break;
                case SHADOW_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getShadowColour(), mAutoConstant.elementCount);
                    break;
                case LIGHT_POWER_SCALE:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLightPowerScale(mAutoConstant.data));
                    break;
                case LIGHT_DIFFUSE_COLOUR_POWER_SCALED:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLightDiffuseColourWithPower(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case LIGHT_SPECULAR_COLOUR_POWER_SCALED:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLightSpecularColourWithPower(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case LIGHT_NUMBER:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLightNumber(mAutoConstant.data));
                    break;
                case LIGHT_CASTS_SHADOWS:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLightCastsShadows(mAutoConstant.data));
                    break;
                case LIGHT_CASTS_SHADOWS_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(float), source->getLightCastsShadows(l));
                    break;
                case LIGHT_ATTENUATION:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getLightAttenuation(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case SPOTLIGHT_PARAMS:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSpotlightParams(mAutoConstant.data), mAutoConstant.elementCount);
                    break;
                case LIGHT_DIFFUSE_COLOUR_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(ColourValue),
                                          source->getLightDiffuseColour(l), mAutoConstant.elementCount);
                    break;

                case LIGHT_SPECULAR_COLOUR_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(ColourValue),
                                          source->getLightSpecularColour(l), mAutoConstant.elementCount);
                    break;
                case LIGHT_DIFFUSE_COLOUR_POWER_SCALED_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(ColourValue),
                                          source->getLightDiffuseColourWithPower(l), mAutoConstant.elementCount);
                    break;

                case LIGHT_SPECULAR_COLOUR_POWER_SCALED_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(ColourValue),
                                          source->getLightSpecularColourWithPower(l), mAutoConstant.elementCount);
                    break;

                case LIGHT_POSITION_ARRAY:
                    // Get as 4D vector, works for directional lights too
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Vector4),
                                          source->getLightAs4DVector(l), mAutoConstant.elementCount);
                    break;

                case LIGHT_DIRECTION_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                    {
                        vec3 = source->getLightDirection(l);
                        // Set as 4D vector for compatibility
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Vector4f),
                                          Vector4f{vec3.x, vec3.y, vec3.z, 0.0f}, mAutoConstant.elementCount);
                    }
                    break;

                case LIGHT_POSITION_VIEW_SPACE_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Vector4),
                                          source->getViewMatrix() *
                                              source->getLightAs4DVector(l),
                                          mAutoConstant.elementCount);
                    break;

                case LIGHT_DIRECTION_VIEW_SPACE_ARRAY:
                    m3 = source->getInverseTransposeViewMatrix().linear();
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                    {
                        vec3 = m3 * source->getLightDirection(l);
                        vec3.normalise();
                        // Set as 4D vector for compatibility
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Vector4f),
                                          Vector4f{vec3.x, vec3.y, vec3.z, 0.0f}, mAutoConstant.elementCount);
                    }
                    break;

                case LIGHT_POWER_SCALE_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Real),
                                          source->getLightPowerScale(l));
                    break;

                case LIGHT_ATTENUATION_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                    {
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Vector4f),
                                          source->getLightAttenuation(l), mAutoConstant.elementCount);
                    }
                    break;
                case SPOTLIGHT_PARAMS_ARRAY:
                    for (size_t l = 0 ; l < mAutoConstant.data; ++l)
                    {
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Vector4f), source->getSpotlightParams(l),
                                          mAutoConstant.elementCount);
                    }
                    break;
                case DERIVED_LIGHT_DIFFUSE_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex,
                                      source->getLightDiffuseColourWithPower(mAutoConstant.data) * source->getSurfaceDiffuseColour(),
                                      mAutoConstant.elementCount);
                    break;
                case DERIVED_LIGHT_SPECULAR_COLOUR:
                    _writeRawConstant(mAutoConstant.physicalIndex,
                                      source->getLightSpecularColourWithPower(mAutoConstant.data) * source->getSurfaceSpecularColour(),
                                      mAutoConstant.elementCount);
                    break;
                case DERIVED_LIGHT_DIFFUSE_COLOUR_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                    {
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(ColourValue),
                                          source->getLightDiffuseColourWithPower(l) * source->getSurfaceDiffuseColour(),
                                          mAutoConstant.elementCount);
                    }
                    break;
                case DERIVED_LIGHT_SPECULAR_COLOUR_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                    {
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(ColourValue),
                                          source->getLightSpecularColourWithPower(l) * source->getSurfaceSpecularColour(),
                                          mAutoConstant.elementCount);
                    }
                    break;
                case TEXTURE_VIEWPROJ_MATRIX:
                    // can also be updated in lights
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getTextureViewProjMatrix(mAutoConstant.data),mAutoConstant.elementCount);
                    break;
                case TEXTURE_VIEWPROJ_MATRIX_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                    {
                        // can also be updated in lights
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Matrix4),
                                          source->getTextureViewProjMatrix(l),mAutoConstant.elementCount);
                    }
                    break;
                case SPOTLIGHT_VIEWPROJ_MATRIX:
                    _writeRawConstant(mAutoConstant.physicalIndex, source->getSpotlightViewProjMatrix(mAutoConstant.data),mAutoConstant.elementCount);
                    break;
                case SPOTLIGHT_VIEWPROJ_MATRIX_ARRAY:
                    for (size_t l = 0; l < mAutoConstant.data; ++l)
                    {
                        // can also be updated in lights
                        _writeRawConstant(mAutoConstant.physicalIndex + l*sizeof(Matrix4),
                                          source->getSpotlightViewProjMatrix(l),mAutoConstant.elementCount);
                    }
                    break;

                default:
                    break;
                };
            }
        }

    }
    //---------------------------------------------------------------------------
    static auto withArrayOffset(const GpuConstantDefinition* def, std::string_view name) -> size_t
    {
        uint32 offset = 0;
        if(name.back() == ']')
        {
            size_t start = name.find('[');
            offset = StringConverter::parseInt(name.substr(start + 1, name.size() - start - 2));
            offset = std::min(offset, def->arraySize - 1);
            size_t type_sz = def->isDouble() ? 8 : ( def->isSampler() ? 1 : 4);
            offset *= type_sz;
        }

        return def->physicalIndex + offset * def->elementSize;
    }

    void GpuProgramParameters::setNamedConstant(std::string_view name, Real val)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstant(withArrayOffset(def, name), val);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name, int val)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def = _findNamedConstantDefinition(name, !mIgnoreMissingParams);

        if(!def)
            return;

        if(def->isSampler())
            _writeRegisters(withArrayOffset(def, name), &val, 1);
        else
            _writeRawConstant(withArrayOffset(def, name), val);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name, uint val)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstant(withArrayOffset(def, name), val);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name, const Vector4& vec)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstant(withArrayOffset(def, name), vec, def->elementSize);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name, const Vector3& vec)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstant(withArrayOffset(def, name), vec);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name, const Vector2& vec)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstant(withArrayOffset(def, name), vec);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name, const Matrix4& m)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstant(withArrayOffset(def, name), m, def->elementSize);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name, const Matrix4* m,
                                                size_t numEntries)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstant(withArrayOffset(def, name), m, numEntries);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name, const ColourValue& colour)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstant(withArrayOffset(def, name), colour, def->elementSize);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name,
                                                const float *val, size_t count, size_t multiple)
    {
        size_t rawCount = count * multiple;
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstants(withArrayOffset(def, name), val, rawCount);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name,
                                                const double *val, size_t count, size_t multiple)
    {
        size_t rawCount = count * multiple;
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstants(withArrayOffset(def, name), val, rawCount);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name,
                                                const int *val, size_t count, size_t multiple)
    {
        size_t rawCount = count * multiple;
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def = _findNamedConstantDefinition(name, !mIgnoreMissingParams);

        if(!def)
            return;

        if (def->isSampler())
            _writeRegisters(withArrayOffset(def, name), val, rawCount);
        else
            _writeRawConstants(withArrayOffset(def, name), val, rawCount);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedConstant(std::string_view name,
                                                const uint *val, size_t count, size_t multiple)
    {
        size_t rawCount = count * multiple;
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
            _writeRawConstants(withArrayOffset(def, name), val, rawCount);
    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedAutoConstant(std::string_view name,
                                                    AutoConstantType acType, uint32 extraInfo)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
        {
            def->variability = deriveVariability(acType);
            // make sure we also set variability on the logical index map
            getConstantLogicalIndexUse(def->logicalIndex, def->elementSize * def->arraySize, def->variability, BaseConstantType::FLOAT);
            _setRawAutoConstant(withArrayOffset(def, name), acType, extraInfo, def->variability, def->elementSize);
        }

    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::setNamedAutoConstantReal(std::string_view name,
                                                        AutoConstantType acType, Real rData)
    {
        // look up, and throw an exception if we're not ignoring missing
        const GpuConstantDefinition* def =
            _findNamedConstantDefinition(name, !mIgnoreMissingParams);
        if (def)
        {
            def->variability = deriveVariability(acType);
            // make sure we also set variability on the logical index map
            getConstantLogicalIndexUse(def->logicalIndex, def->elementSize * def->arraySize, def->variability, BaseConstantType::FLOAT);
            _setRawAutoConstantReal(withArrayOffset(def, name), acType, rData, def->variability, def->elementSize);
        }
    }
    //---------------------------------------------------------------------------
    auto GpuProgramParameters::getAutoConstantEntry(const size_t index) -> GpuProgramParameters::AutoConstantEntry*
    {
        if (index < mAutoConstants.size())
        {
            return &(mAutoConstants[index]);
        }
        else
        {
            return nullptr;
        }
    }
    //---------------------------------------------------------------------------
    auto
    GpuProgramParameters::findFloatAutoConstantEntry(size_t logicalIndex) -> const GpuProgramParameters::AutoConstantEntry*
    {
        if (!mLogicalToPhysical)
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "This is not a low-level parameter parameter object",
                        "GpuProgramParameters::findFloatAutoConstantEntry");

        return _findRawAutoConstantEntryFloat(
            _getConstantPhysicalIndex(logicalIndex, 0, GpuParamVariability::GLOBAL, BaseConstantType::FLOAT));

    }
    //---------------------------------------------------------------------------
    auto
    GpuProgramParameters::findAutoConstantEntry(std::string_view paramName) const -> const GpuProgramParameters::AutoConstantEntry*
    {
        if (!mNamedConstants)
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "This params object is not based on a program with named parameters.",
                        "GpuProgramParameters::findAutoConstantEntry");

        const GpuConstantDefinition& def = getConstantDefinition(paramName);
        if(def.isSampler())
            return nullptr;
        return _findRawAutoConstantEntryFloat(def.physicalIndex);
    }
    //---------------------------------------------------------------------------
    auto
    GpuProgramParameters::_findRawAutoConstantEntryFloat(size_t physicalIndex) const -> const GpuProgramParameters::AutoConstantEntry*
    {
        for(const auto & ac : mAutoConstants)
        {
            // should check that auto is float and not int so that physicalIndex
            // doesn't have any ambiguity
            // However, all autos are float I think so no need
            if (ac.physicalIndex == physicalIndex)
                return &ac;
        }

        return nullptr;

    }
    //---------------------------------------------------------------------------
    void GpuProgramParameters::copyConstantsFrom(const GpuProgramParameters& source)
    {
        // Pull buffers & auto constant list over directly
        mConstants = source.getConstantList();
        mRegisters = source.mRegisters;
        mAutoConstants = source.getAutoConstantList();
        mCombinedVariability = source.mCombinedVariability;
        copySharedParamSetUsage(source.mSharedParamSets);
    }
    //---------------------------------------------------------------------
    void GpuProgramParameters::copyMatchingNamedConstantsFrom(const GpuProgramParameters& source)
    {
        if (mNamedConstants && source.mNamedConstants)
        {
            std::map<size_t, String> srcToDestNamedMap;
            for (auto i = source.mNamedConstants->map.begin();
                 i != source.mNamedConstants->map.end(); ++i)
            {
                std::string_view paramName = i->first;
                const GpuConstantDefinition& olddef = i->second;
                const GpuConstantDefinition* newdef = _findNamedConstantDefinition(paramName, false);
                if (newdef)
                {
                    // Copy data across, based on smallest common definition size
                    size_t srcsz = olddef.elementSize * olddef.arraySize;
                    size_t destsz = newdef->elementSize * newdef->arraySize;
                    size_t sz = std::min(srcsz, destsz);
                    if (newdef->isFloat() || newdef->isInt() || newdef->isUnsignedInt() || newdef->isBool())
                    {
                        OgreAssertDbg(mConstants.size() >= (newdef->physicalIndex + sz * 4), "Invalid physical index");
                        memcpy(getFloatPointer(newdef->physicalIndex),
                               source.getFloatPointer(olddef.physicalIndex),
                               sz * 4);
                    }
                    else if (newdef->isDouble())
                    {

                        memcpy(getDoublePointer(newdef->physicalIndex),
                               source.getDoublePointer(olddef.physicalIndex),
                               sz * sizeof(double));
                    }
                    else if (newdef->isSampler())
                    {
                        *getRegPointer(newdef->physicalIndex) = *source.getRegPointer(olddef.physicalIndex);
                    }
                    else
                    {
                        //TODO exception handling
                    }
                    // we'll use this map to resolve autos later
                    // ignore the [0] aliases
                    if (!StringUtil::endsWith(paramName, "[0]") && source.findAutoConstantEntry(paramName))
                        srcToDestNamedMap[olddef.physicalIndex] = paramName;
                }
            }

            for (const auto & autoEntry : source.mAutoConstants)
            {
                // find dest physical index
                auto mi = srcToDestNamedMap.find(autoEntry.physicalIndex);
                if (mi != srcToDestNamedMap.end())
                {
                    if (getAutoConstantDefinition(autoEntry.paramType)->dataType == ACDataType::REAL)
                    {
                        setNamedAutoConstantReal(mi->second, autoEntry.paramType, autoEntry.fData);
                    }
                    else
                    {
                        setNamedAutoConstant(mi->second, autoEntry.paramType, autoEntry.data);
                    }
                }

            }

            // Copy shared param sets
            for (const auto & usage : source.mSharedParamSets)
            {
                if (!isUsingSharedParameters(usage.getName()))
                {
                    addSharedParameters(usage.getSharedParams());
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    auto
    GpuProgramParameters::getAutoConstantDefinition(std::string_view name) -> const GpuProgramParameters::AutoConstantDefinition*
    {
        // find a constant definition that matches name by iterating through the
        // constant definition array
        bool nameFound = false;
        size_t i = 0;
        const size_t numDefs = getNumAutoConstantDefinitions();
        while (!nameFound && (i < numDefs))
        {
            if (name == AutoConstantDictionary[i].name)
                nameFound = true;
            else
                ++i;
        }

        if (nameFound)
            return &AutoConstantDictionary[i];
        else
            return nullptr;
    }

    //-----------------------------------------------------------------------
    auto
    GpuProgramParameters::getAutoConstantDefinition(GpuProgramParameters::AutoConstantType idx) -> const GpuProgramParameters::AutoConstantDefinition*
    {

        if (std::to_underlying(idx) < getNumAutoConstantDefinitions())
        {
            // verify index is equal to acType
            // if they are not equal then the dictionary was not setup properly
            assert(idx == AutoConstantDictionary[std::to_underlying(idx)].acType);
            return &AutoConstantDictionary[std::to_underlying(idx)];
        }
        else
            return nullptr;
    }
    //-----------------------------------------------------------------------
    auto GpuProgramParameters::getNumAutoConstantDefinitions() -> size_t
    {
        return sizeof(AutoConstantDictionary)/sizeof(AutoConstantDefinition);
    }

    //-----------------------------------------------------------------------
    void GpuProgramParameters::incPassIterationNumber()
    {
        if (mActivePassIterationIndex != std::numeric_limits<size_t>::max())
        {
            // This is a physical index
            *getFloatPointer(mActivePassIterationIndex) += 1;
        }
    }
    //---------------------------------------------------------------------
    void GpuProgramParameters::addSharedParameters(GpuSharedParametersPtr sharedParams)
    {
        if (!isUsingSharedParameters(sharedParams->getName()))
        {
            mSharedParamSets.push_back(GpuSharedParametersUsage(sharedParams, this));
        }
    }
    //---------------------------------------------------------------------
    void GpuProgramParameters::addSharedParameters(std::string_view sharedParamsName)
    {
        addSharedParameters(GpuProgramManager::getSingleton().getSharedParameters(sharedParamsName));
    }
    //---------------------------------------------------------------------
    auto GpuProgramParameters::isUsingSharedParameters(std::string_view sharedParamsName) const -> bool
    {
        for (const auto & mSharedParamSet : mSharedParamSets)
        {
            if (mSharedParamSet.getName() == sharedParamsName)
                return true;
        }
        return false;
    }
    //---------------------------------------------------------------------
    void GpuProgramParameters::removeSharedParameters(std::string_view sharedParamsName)
    {
        for (auto i = mSharedParamSets.begin();
             i != mSharedParamSets.end(); ++i)
        {
            if (i->getName() == sharedParamsName)
            {
                mSharedParamSets.erase(i);
                break;
            }
        }
    }
    //---------------------------------------------------------------------
    void GpuProgramParameters::removeAllSharedParameters()
    {
        mSharedParamSets.clear();
    }
    //---------------------------------------------------------------------
    auto
    GpuProgramParameters::getSharedParameters() const noexcept -> const GpuProgramParameters::GpuSharedParamUsageList&
    {
        return mSharedParamSets;
    }
    //---------------------------------------------------------------------
    void GpuProgramParameters::_copySharedParams()
    {
        for (auto& usage : mSharedParamSets)
        {
            usage._copySharedParamsToTargetParams();
        }
    }

    void GpuProgramParameters::_updateSharedParams()
    {
        for (auto& usage : mSharedParamSets)
        {
            const GpuSharedParametersPtr& sharedParams = usage.getSharedParams();
            if(sharedParams->_getHardwareBuffer())
            {
                sharedParams->_upload();
                sharedParams->_markClean();
                continue;
            }

            usage._copySharedParamsToTargetParams();
        }
    }
}
