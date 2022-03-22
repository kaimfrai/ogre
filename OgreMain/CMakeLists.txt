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

add_subdirectory(Config)
add_subdirectory(RenderSystem)
add_subdirectory(Math)
add_subdirectory(General)
add_subdirectory(Scene)

# setup OgreMain target
add_library(OgreMain INTERFACE include/Ogre.h)
target_include_directories(OgreMain INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  $<INSTALL_INTERFACE:include/OGRE>
)

set_target_properties(OgreMain PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_link_libraries(OgreMain INTERFACE Core.Config)

# install OgreMain
ogre_config_lib(OgreMain TRUE)

install(FILES include/Ogre.h
  DESTINATION include/OGRE/
)
