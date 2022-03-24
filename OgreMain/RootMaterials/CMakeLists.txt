add_subdirectory(include)

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.RootMaterials STATIC ${SOURCE_FILES})
set_target_properties(Core.RootMaterials PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(
	Core.RootMaterials
PUBLIC
	Core.RootMaterials.include
)

ogre_config_lib(Core.RootMaterials TRUE)
