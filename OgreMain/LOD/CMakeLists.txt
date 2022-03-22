file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.LOD STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(Core.LOD PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.LOD PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/LOD/>
)

target_link_libraries(Core.LOD PUBLIC Core.Config)
target_precompile_headers(Core.LOD PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.LOD TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/LOD/
)
