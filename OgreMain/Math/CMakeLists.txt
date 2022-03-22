add_subdirectory(include)

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Math STATIC ${SOURCE_FILES})
set_target_properties(Core.Math PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(Core.Math PUBLIC Core.Math.include)

ogre_config_lib(Core.Math TRUE)
