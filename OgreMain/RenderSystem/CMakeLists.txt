file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.RenderSystem STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(Core.RenderSystem PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/RenderSystem/>
)

target_link_libraries(Core.RenderSystem Core.Config)
target_precompile_headers(Core.RenderSystem PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.RenderSystem TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/RenderSystem/
)
