file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Scripts STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(Core.Scripts PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.Scripts PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Scripts/>
)

target_link_libraries(Core.Scripts PUBLIC Core.Config)
target_precompile_headers(Core.Scripts PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.Scripts TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Scripts/
)
