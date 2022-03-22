file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Image STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(Core.Image PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.Image PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Image/>
)

target_link_libraries(Core.Image PUBLIC Core.Config)
target_precompile_headers(Core.Image PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.Image TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Image/
)
