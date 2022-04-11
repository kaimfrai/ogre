#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure OpenGL RenderSystem build

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB NVPARSE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/nvparse/*.cpp")
file(GLOB NVPARSE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/nvparse/*.hpp")

if(UNIX)
  # easier than porting to new flex  
  foreach(NVSRC ${NVPARSE_SOURCE})
    set_source_files_properties(${NVSRC} 
      PROPERTIES COMPILE_FLAGS "-Wno-unused-function -Wno-shadow -Wno-cast-qual -Wno-missing-declarations -Wno-undef"
      COMPILE_DEFINITIONS "register="
    )
  endforeach()
  set_source_files_properties(src/atifs/src/Compiler2Pass.cpp
      PROPERTIES COMPILE_FLAGS -Wno-implicit-fallthrough)
endif()

# Remove test app file
list(REMOVE_ITEM NVPARSE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/nvparse/ps1.0__test_main.cpp")

file(GLOB ATIFS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/atifs/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/atifs/src/*.cpp")
file(GLOB ATIFS_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/atifs/include/*.hpp")

file(GLOB GLSL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/GLSL/include/*.hpp")
file(GLOB GLSL_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/GLSL/src/*.cpp")

source_group(NVparse FILES ${NVPARSE_SOURCE} ${NVPARSE_HEADERS})
source_group(ATIFS FILES ${ATIFS_SOURCE} ${ATIFS_HEADERS})
source_group(GLSL FILES ${GLSL_SOURCE} ${GLSL_HEADERS})

if(OGRE_CONFIG_ENABLE_GL_STATE_CACHE_SUPPORT)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/OgreGLStateCacheManager.cpp 
    PROPERTIES COMPILE_DEFINITIONS OGRE_ENABLE_STATE_CACHE)
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/GLSL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src/atifs/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src/nvparse
)

add_definitions(-DYY_NEVER_INTERACTIVE)
add_definitions(-DOGRE_GLPLUGIN_EXPORTS ${OGRE_VISIBILITY_FLAGS})
#Note that in the next row SOURCE_FILES are added last. This is to prevent compilation problems of unity build found on Windows Visual Studio. 
#In this situation any file added after the "glew.cpp" file, which belongs to the SOURCE_FILES package, does not compile

add_library(
  Ogre.RenderSystems.GL.glad
OBJECT
  "${CMAKE_CURRENT_SOURCE_DIR}/src/glad/glad.c"
)

target_include_directories(
  Ogre.RenderSystems.GL.glad
PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

#cyclic dependencies => 1 module
add_module(
  ../GL.hpp
PARTITION
  ${HEADER_FILES}
  ${GLSL_HEADERS}
  ${ATIFS_HEADERS}
IMPLEMENTATION
  ${SOURCE_FILES}
  ${GLSL_SOURCE}
  ${ATIFS_SOURCE}
INCLUDES
  ${OPENGL_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(
  Ogre.RenderSystems.GL
PRIVATE
  $<TARGET_OBJECTS:Ogre.RenderSystems.GL.glad>
)

target_link_libraries(Ogre.RenderSystems.GL PRIVATE ${CMAKE_DL_LIBS})

ogre_config_framework(Ogre.RenderSystems.GL)

ogre_config_plugin(Ogre.RenderSystems.GL)

install(FILES ${HEADER_FILES} ${GLSUPPORT_HEADERS} DESTINATION include/OGRE/RenderSystems/GL)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/glad DESTINATION include/OGRE/RenderSystems/glad)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/KHR DESTINATION include/OGRE/RenderSystems/KHR)
