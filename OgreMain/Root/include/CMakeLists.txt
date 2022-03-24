file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Root.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.Root.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Root/>
)

target_link_libraries(
  Core.Root.include
INTERFACE
  Core.Animation.include
  Core.Config.include
  Core.Effects.include
  Core.Image.include
  Core.Math.include
  Core.Materials.include
  Core.RenderSystem.include
  Core.Resources.include
  Core.Root.zip
  Core.Scene.include
  Core.Script.include
  #Core.General.include
  #Core.Effects.include
  #Core.Image.include
  #Core.Materials.include
)
#target_precompile_headers(Core.Root.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Root.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Root/
)
