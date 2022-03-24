add_subdirectory(include)

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Resources STATIC ${SOURCE_FILES})
set_target_properties(Core.Resources PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(
  Core.Resources
PUBLIC
  Core.Resources.include
)

ogre_config_lib(Core.Resources TRUE)
