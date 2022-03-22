file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Image.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.Image.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Image/>
)

target_link_libraries(
  Core.Image.include
INTERFACE
  Core.Config.include
  Core.General.include
)
#target_precompile_headers(Core.Image.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Image.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Image/
)
