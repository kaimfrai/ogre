#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Browser demo build
if (OGRE_TRACK_MEMORY)
  add_compile_definitions(OGRE_TRACK_MEMORY=1)
endif()

set (HEADER_FILES
	${PROJECT_SOURCE_DIR}/Samples/Browser/include/SampleBrowser.hpp
	${PROJECT_SOURCE_DIR}/Samples/Common/include/Sample.hpp
	${PROJECT_SOURCE_DIR}/Samples/Common/include/SampleContext.hpp
	${PROJECT_SOURCE_DIR}/Samples/Common/include/SamplePlugin.hpp
	${PROJECT_SOURCE_DIR}/Samples/Common/include/SdkSample.hpp
)

# Get the list of configured samples
get_property(OGRE_SAMPLES_LIST GLOBAL PROPERTY "OGRE_SAMPLES_LIST")


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#if (OGRE_BUILD_TESTS)
	#add_definitions(-DSAMPLES_INCLUDE_PLAYPEN)
	#include_directories(${PROJECT_SOURCE_DIR}/Tests/PlayPen/include
	#${PROJECT_SOURCE_DIR}/Tests/VisualTests/PlayPen/include)
#endif()

# Fix for static build with MinGW
if (OGRE_BUILD_RENDERSYSTEM_D3D9 AND OGRE_STATIC)
    link_directories(${DirectX9_LIBRARY_DIR})
endif()


set(SOURCE_FILES ${PROJECT_SOURCE_DIR}/Samples/Browser/src/main.cpp)
add_module_executable(SampleBrowser ${SOURCE_FILES})

# configure SampleBrowser build

ogre_config_common(SampleBrowser)

# append _d for debug builds
if (WIN32 AND NOT (WINDOWS_STORE OR WINDOWS_PHONE))
	set_property(TARGET SampleBrowser APPEND PROPERTY DEBUG_POSTFIX "_d")
endif ()

if (WINDOWS_STORE OR WINDOWS_PHONE)
	set_target_properties(SampleBrowser PROPERTIES VS_WINRT_COMPONENT "true")
endif()


if (OGRE_INSTALL_SAMPLES)
	ogre_install_target(SampleBrowser "" FALSE)
	if (OGRE_INSTALL_PDB)
		# install debug pdb files
		install(FILES $<TARGET_PDB_FILE:SampleBrowser>
			DESTINATION bin${OGRE_DEBUG_PATH} CONFIGURATIONS Debug
			)
		install(FILES $<TARGET_PDB_FILE:SampleBrowser>
			DESTINATION bin${OGRE_RELWDBG_PATH} CONFIGURATIONS RelWithDebInfo
			)
	endif ()
endif ()
