#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure OpenGL Support build

# Add platform specific settings

if(EGL_FOUND)
  cmake_dependent_option(OGRE_GLSUPPORT_USE_EGL "use EGL for GL Context Creation instead of GLX/ WGL" FALSE "NOT WIN32" FALSE)
endif()

file(GLOB PLATFORM_HEADERS "include/GLX/*.hpp")
file(GLOB PLATFORM_SOURCES "src/GLX/*.cpp")

set(NATIVE_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/GLX
    ${OPENGL_INCLUDE_DIR})

set(PLATFORM_LIBS ${X11_LIBRARIES} ${X11_Xrandr_LIB} ${OPENGL_glx_LIBRARY})
list(APPEND NATIVE_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/src/X11/")

file(GLOB GLSUPPORT_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB GLSUPPORT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB GLSL_HEADERS include/GLSL/*.hpp)
file(GLOB GLSL_SOURCES src/GLSL/*.cpp)

if(OGRE_CONFIG_ENABLE_GL_STATE_CACHE_SUPPORT)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/OgreGLUniformCache.cpp 
    PROPERTIES COMPILE_DEFINITIONS OGRE_ENABLE_STATE_CACHE)
endif()

# GLX contains defition for getGLSupport declared in OgreGLUtil.hpp
# GLX => X11 => GLSupport => GLX
add_module(
  ../GLSupport.hpp
PARTITION
  ${GLSUPPORT_HEADERS}
  ${PLATFORM_HEADERS}
  ${GLSL_HEADERS}
  src/X11/OgreX11.hpp
IMPLEMENTATION
  ${GLSUPPORT_SOURCES}
  ${PLATFORM_SOURCES}
  ${GLSL_SOURCES}
  src/X11/OgreX11.cpp
INCLUDES
  ${NATIVE_INCLUDES}
)

target_link_libraries(Ogre.RenderSystems.GLSupport PRIVATE ${PLATFORM_LIBS})

set_property(TARGET Ogre.RenderSystems.GLSupport PROPERTY POSITION_INDEPENDENT_CODE ON)

if(OGRE_STATIC)
  ogre_config_lib(Ogre.RenderSystems.GLSupport TRUE)
endif()
