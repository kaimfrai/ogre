add_subdirectory(include)

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.RenderSystem STATIC ${SOURCE_FILES})
set_target_properties(Core.RenderSystem PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(Core.RenderSystem PUBLIC Core.RenderSystem.include)

ogre_config_lib(Core.RenderSystem TRUE)
