file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Effects STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(Core.Effects PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.Effects PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Effects/>
)

target_link_libraries(Core.Effects PUBLIC Core.Config)
target_precompile_headers(Core.Effects PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.Effects TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Effects/
)
