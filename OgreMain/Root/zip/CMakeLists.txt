add_library(Core.Root.zip OBJECT zip.c)
set_target_properties(Core.Root.zip PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.Root.zip PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Root/zip>
)

ogre_config_lib(Core.Root.zip TRUE)

install(FILES zip.h
  DESTINATION include/OGRE/Core/Root/zip
)
