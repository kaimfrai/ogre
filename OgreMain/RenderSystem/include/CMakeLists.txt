file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.RenderSystem.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.RenderSystem.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/RenderSystem/>
)

target_precompile_headers(Core.RenderSystem.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.RenderSystem.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/RenderSystem/
)
