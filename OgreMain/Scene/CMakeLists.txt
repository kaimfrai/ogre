file(GLOB HEADER_FILES "${CMAKE_SOURCE_DIR}/OgreMain/include/Scene/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Scene STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(Core.Scene PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.Scene PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/OgreMain/Scene>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Scene>
)

add_dependencies(Core.Scene Core.General)
target_link_libraries(Core.Scene PUBLIC Core.General)

target_precompile_headers(Core.Scene PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.Scene TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Scene
)
