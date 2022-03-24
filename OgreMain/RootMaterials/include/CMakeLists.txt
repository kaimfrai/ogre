file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core.RootMaterials.include INTERFACE ${HEADER_FILES})

target_include_directories(Core.RootMaterials.include INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/RootMaterials/>
)

target_link_libraries(
  Core.RootMaterials.include
INTERFACE
  Core.Root.include
)
#target_precompile_headers(Core.RootMaterials.include INTERFACE ${HEADER_FILES})

ogre_config_lib(Core.RootMaterials.include TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/RootMaterials/
)
