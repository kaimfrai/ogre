file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Scene.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.Scene.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Scene/>
)

target_link_libraries(
  Core.Scene.include
INTERFACE
  Core.Config.include
  Core.Math.include
  Core.RenderSystem.include
  Core.General.include
  Core.Animation.include
  Core.Materials.include
)
#target_precompile_headers(Core.Scene.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Scene.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Scene/
)
