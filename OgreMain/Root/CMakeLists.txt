add_subdirectory(include)
add_subdirectory(zip)

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Root STATIC ${SOURCE_FILES})
set_target_properties(Core.Root PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(
	Core.Root
PUBLIC
	Core.Root.include
)

ogre_config_lib(Core.Root TRUE)
