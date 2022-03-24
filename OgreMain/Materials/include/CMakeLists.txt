file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Materials.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.Materials.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Materials/>
)

target_link_libraries(
  Core.Materials.include
INTERFACE
  Core.Config.include
  #Core.Resources.include
  #Core.General.include
)
#target_precompile_headers(Core.Materials.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Materials.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Materials/
)
