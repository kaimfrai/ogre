add_subdirectory(include)

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Config STATIC ${SOURCE_FILES})
set_target_properties(Core.Config PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(Core.Config PUBLIC Core.Config.include)

ogre_config_lib(Core.Config TRUE)
