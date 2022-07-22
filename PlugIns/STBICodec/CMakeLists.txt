#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
set(SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/OgreSTBICodec.cpp")

if(UNIX)
  set_source_files_properties(src/OgreSTBICodec.cpp PROPERTIES
    COMPILE_FLAGS "-Wno-cast-qual -Wno-unused-function -Wno-missing-declarations -Wno-type-limits")
endif()

add_module(
  include/OgreSTBICodec.hpp
IMPLEMENTATION
  src/OgreSTBICodec.cpp
)

if(CMAKE_COMPILER_IS_GNUCXX)
  # workaround gcc5 bug on Ubuntu 16.04 regarding __builtin_cpu_supports
  target_link_libraries(Ogre.PlugIns.STBICodec PRIVATE gcc)
endif()

if(ZLIB_FOUND)
  target_compile_definitions(Ogre.PlugIns.STBICodec PRIVATE HAVE_ZLIB)
  target_link_libraries(Ogre.PlugIns.STBICodec PRIVATE ZLIB::ZLIB)
endif()

ogre_config_framework(Ogre.PlugIns.STBICodec)
ogre_config_plugin(Ogre.PlugIns.STBICodec)

install(FILES ${HEADER_FILES} DESTINATION include/OGRE/Plugins/STBICodec)
