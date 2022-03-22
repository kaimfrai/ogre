add_subdirectory(include)

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.General STATIC ${SOURCE_FILES})
set_target_properties(Core.General PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(
	Core.General
PUBLIC
	Core.General.include
)

ogre_config_lib(Core.General TRUE)
