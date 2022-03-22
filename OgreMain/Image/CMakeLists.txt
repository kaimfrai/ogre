add_subdirectory(include)

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Image STATIC ${SOURCE_FILES})
set_target_properties(Core.Image PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(Core.Image PUBLIC Core.Image.include)

ogre_config_lib(Core.Image TRUE)
