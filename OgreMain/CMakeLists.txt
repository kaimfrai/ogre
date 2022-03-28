#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

############################################################
# OgreMain core library
############################################################

# define header and source files for the library
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(miniz INTERFACE src/zip/miniz.h)
target_include_directories(miniz INTERFACE src/zip)
#from miniz.h:
# On platforms using glibc, Be sure to "#define _LARGEFILE64_SOURCE 1" before
# including miniz.c to ensure miniz uses the 64-bit variants
target_compile_options(miniz INTERFACE -D_LARGEFILE64_SOURCE=1)

add_library(zip OBJECT src/zip/zip.c)
target_include_directories(zip PUBLIC src/zip)
target_link_libraries(zip PRIVATE miniz)

# setup OgreMain target

add_library(OgreMain ${OGRE_LIB_TYPE} ${SOURCE_FILES} $<TARGET_OBJECTS:zip>)

target_include_directories(
  OgreMain
PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" 
  "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
  $<INSTALL_INTERFACE:include/OGRE>
PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/zip>"
)

set_target_properties(OgreMain PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_link_libraries(OgreMain PUBLIC pthread PRIVATE ${CMAKE_DL_LIBS})

# specify a precompiled header to use
add_native_precompiled_header(OgreMain "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/OgreStableHeaders.h>")

# install OgreMain
ogre_config_lib(OgreMain TRUE)
install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE
)
