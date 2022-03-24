file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Effects.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.Effects.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Effects/>
)

target_link_libraries(
  Core.Effects.include
INTERFACE
  Core.Config.include
)
#target_precompile_headers(Core.Effects.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Effects.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Effects/
)
