file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.LOD.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.LOD.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/LOD/>
)

target_link_libraries(
  Core.LOD.include
INTERFACE
  Core.Config.include
  Core.Resources.include
)
target_precompile_headers(Core.LOD.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.LOD.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/LOD/
)
