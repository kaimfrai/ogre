file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.General.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.General.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/General/>
)

target_link_libraries(
  Core.General.include
INTERFACE
  Core.Config.include
  Core.Math.include
)
target_precompile_headers(Core.General.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.General.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/General/
)
