file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(Core.General STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(Core.General PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.General PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/General/>
)

target_link_libraries(Core.General PUBLIC pthread Core.Config PRIVATE ${CMAKE_DL_LIBS})
target_precompile_headers(Core.General PUBLIC ${HEADER_FILES})

ogre_config_lib(Core.General TRUE)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Core/General/
)
