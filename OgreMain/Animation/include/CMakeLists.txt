file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.Animation.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.Animation.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Animation/>
)

target_link_libraries(
  Core.Animation.include
INTERFACE
  Core.Config.include
  Core.General.include
  Core.Scene.include
  Core.Math.include
  Core.Resource.include
)
#target_precompile_headers(Core.Animation.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.Animation.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Animation/
)
