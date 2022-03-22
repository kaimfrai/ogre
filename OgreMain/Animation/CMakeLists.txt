file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Animation STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(Core.Animation PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.Animation PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Animation/>
)

target_link_libraries(Core.Animation PUBLIC Core.Config)
target_precompile_headers(Core.Animation PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.Animation TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Animation/
)

