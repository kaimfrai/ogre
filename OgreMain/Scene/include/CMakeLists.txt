file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Scene.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.Scene.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Scene/>
)

target_link_libraries(
  Core.Scene.include
INTERFACE
  Core.Animation.include
  Core.LOD.include
  Core.Materials.include
  Core.RenderSystem.include
  Core.Resources.include
)
target_precompile_headers(Core.Scene.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Scene.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Scene/
)
