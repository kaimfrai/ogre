#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

############################################################
# Overlay optional component
############################################################

# define header and source files for the library
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB PRIVATE_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if(OGRE_BUILD_COMPONENT_OVERLAY_IMGUI)
  set(IMGUI_DIR "${PROJECT_BINARY_DIR}/imgui-1.85" CACHE PATH "")
  if(NOT EXISTS ${IMGUI_DIR})
    message(STATUS "Downloading imgui")
    file(DOWNLOAD
        https://github.com/ocornut/imgui/archive/v1.85.tar.gz
        ${PROJECT_BINARY_DIR}/imgui.tar.gz)
    execute_process(COMMAND ${CMAKE_COMMAND}
        -E tar xf imgui.tar.gz WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  endif()
  list(APPEND SOURCE_FILES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp)
  if(FREETYPE_FOUND)
    set(IMGUI_ENABLE_FREETYPE TRUE)
    list(APPEND SOURCE_FILES ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp)
  endif()
else()
    list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/OgreImGuiOverlay.cpp")
endif()

add_module(
  OgreOverlay.hpp
PARTITION
  ${HEADER_FILES}
  ${PRIVATE_HEADER_FILES}
IMPLEMENTATION
  ${SOURCE_FILES}
INCLUDES
  ${FREETYPE_INCLUDE_DIRS}
)

# setup target
set_target_properties(Ogre.Components.Overlay+ PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(Ogre.Components.Overlay+ PUBLIC Ogre.Core)

if(FREETYPE_FOUND)
  target_compile_definitions(Ogre.Components.Overlay+ PRIVATE HAVE_FREETYPE)
  target_link_libraries(Ogre.Components.Overlay+ PRIVATE ${FREETYPE_LIBRARIES})
  if(ZLIB_FOUND)
    target_link_libraries(Ogre.Components.Overlay+ PRIVATE ZLIB::ZLIB)
  endif()
elseif(UNIX)
  set_source_files_properties(src/OgreFont.cpp PROPERTIES
    COMPILE_FLAGS "-Wno-cast-qual -Wno-unused-function")
endif()

if(OGRE_BUILD_COMPONENT_OVERLAY_IMGUI)
  target_include_directories(Ogre.Components.Overlay+ PUBLIC
    PUBLIC "$<BUILD_INTERFACE:${IMGUI_DIR}>"
    PRIVATE "$<BUILD_INTERFACE:${IMGUI_DIR}/misc/freetype>")

  if(UNIX)
    set_source_files_properties(${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp PROPERTIES
      COMPILE_FLAGS "-Wno-missing-declarations")

    set_source_files_properties(${IMGUI_DIR}/imgui_widgets.cpp PROPERTIES
      COMPILE_FLAGS "-Wno-cast-qual")
    set_source_files_properties(${IMGUI_DIR}/imgui_draw.cpp PROPERTIES
      COMPILE_FLAGS "-Wno-cast-qual")
    set_source_files_properties(${IMGUI_DIR}/imgui.cpp PROPERTIES
      COMPILE_FLAGS "-Wno-cast-qual")
  endif()

  if(MINGW)
    target_link_libraries(Ogre.Components.Overlay+ PRIVATE imm32)
  endif()

  configure_file(${OGRE_TEMPLATES_DIR}/imconfig.h ${IMGUI_DIR}/imconfig.h)

  list(APPEND HEADER_FILES
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imconfig.h)
endif()

# install 
ogre_config_framework(Ogre.Components.Overlay)
ogre_config_framework(Ogre.Components.Overlay+)
ogre_config_component(Ogre.Components.Overlay)
ogre_config_component(Ogre.Components.Overlay+)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Overlay
)
install(FILES "${PROJECT_SOURCE_DIR}/Media/packs/profiler.zip"
  DESTINATION "${OGRE_MEDIA_PATH}/packs/"
)
