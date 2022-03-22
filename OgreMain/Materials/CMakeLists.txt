file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Materials STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(Core.Materials PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.Materials PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Materials/>
)

target_link_libraries(Core.Materials PUBLIC Core.Config)
target_precompile_headers(Core.Materials PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.Materials TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Materials/
)
