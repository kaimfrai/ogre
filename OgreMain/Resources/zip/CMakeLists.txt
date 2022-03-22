add_library(Core.Resources.Zip OBJECT zip.c)
set_target_properties(Core.Resources.Zip PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})

target_include_directories(Core.Resources.Zip PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  $<INSTALL_INTERFACE:include/OGRE/Core/Resources/zip>
)

ogre_config_lib(Core.Resources.Zip TRUE)

install(FILES zip.h
  DESTINATION include/OGRE/Core/Resources/zip
)
