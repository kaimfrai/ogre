#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

set(HEADER_FILES
  include/PlayPenTestPlugin.hpp
  include/PlayPenTests.hpp)

set(SOURCE_FILES
  src/PlayPenTests.cpp
  src/PlayPenTestPlugin.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/Samples/Common/include)
include_directories(${PROJECT_SOURCE_DIR}/Samples/Browser/include)
include_directories(${PROJECT_SOURCE_DIR}/Tests/VisualTests/Common/include)

add_module(
  PlayPen.hpp
PARTITION
  include/PlayPenTestPlugin.hpp
  include/PlayPenTests.hpp
IMPLEMENTATION
  src/PlayPenTestPlugin.cpp
  src/PlayPenTests.cpp
)
target_link_libraries(Ogre.Tests.VisualTests.PlayPen PRIVATE ${OGRE_LIBRARIES} ${SDL2_LIBRARY})
#ogre_config_sample_lib(PlayPenTests)

if (APPLE AND NOT APPLE_IOS)
  # Set the INSTALL_PATH so that Samples can be installed in the application package
  set_target_properties(PlayPenTests
    PROPERTIES BUILD_WITH_INSTALL_RPATH 1
    INSTALL_NAME_DIR "@executable_path/../Plugins"
    )
endif()
