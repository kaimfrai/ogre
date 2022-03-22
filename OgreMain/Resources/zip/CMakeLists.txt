add_library(Core.Resources.zip OBJECT zip.c)
set_target_properties(Core.Resources.zip PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.Resources.zip PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Resources/zip>
)

ogre_config_lib(Core.Resources.zip TRUE)

install(FILES zip.h
  DESTINATION include/OGRE/Core/Resources/zip
)
