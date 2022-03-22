file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.Math STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(Core.Math PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Math/>
)

target_link_libraries(Core.Math Core.Config)
target_precompile_headers(Core.Math PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.Math TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/Math/
)
