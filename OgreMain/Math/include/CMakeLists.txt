file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Math.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.Math.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Math/>
)

target_precompile_headers(Core.Math.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Math.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Math/
)
