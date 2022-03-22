file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Config INTERFACE ${HEADER_FILES})

target_include_directories(Core.Config INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Config/>
)

target_precompile_headers(Core.Config INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Config TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Config/
)


