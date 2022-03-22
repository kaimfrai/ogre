file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Script.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.Script.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Script/>
)

target_precompile_headers(Core.Script.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Script.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Script/
)
