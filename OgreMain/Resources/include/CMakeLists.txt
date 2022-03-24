file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Resources.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.Resources.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Resources/>
)

target_link_libraries(
  Core.Resources.include
INTERFACE
  Core.Config.include
  #Core.Animation.include
  #Core.Script.include
  #Core.Math.include
  #Core.General.include
)
#target_precompile_headers(Core.Resources.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Resources.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Resources/
)
