#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Ogre RTShader System build

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB PRIVATE_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_module(
  OgreRTShaderSystem.hpp
PARTITION
  ${HEADER_FILES}
  ${PRIVATE_HEADER_FILES}
IMPLEMENTATION
  ${SOURCE_FILES}
)

set_target_properties(Ogre.Components.RTShaderSystem+ PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(Ogre.Components.RTShaderSystem+ PUBLIC Ogre.Core)

include_directories("${PROJECT_SOURCE_DIR}/Core/include" "${PROJECT_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")

ogre_config_framework(Ogre.Components.RTShaderSystem)
ogre_config_framework(Ogre.Components.RTShaderSystem+)
ogre_config_component(Ogre.Components.RTShaderSystem)
ogre_config_component(Ogre.Components.RTShaderSystem+)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/RTShaderSystem
)
