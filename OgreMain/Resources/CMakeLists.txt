add_subdirectory(zip)

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Resources STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(Core.Resources PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.Resources PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Resources/>
)

target_link_libraries(Core.Resources Core.Resources.Zip)
target_precompile_headers(Core.Resources PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.Resources TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Resources/
)
