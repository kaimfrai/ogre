add_subdirectory(include)

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Script STATIC ${SOURCE_FILES})
set_target_properties(Core.Script PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(Core.Script PUBLIC Core.Script.include)

ogre_config_lib(Core.Script TRUE)
