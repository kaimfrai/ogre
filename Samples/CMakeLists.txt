# Configure Samples build

# Add android JNI binding example
if(OGRE_BUILD_ANDROID_JNI_SAMPLE)
  add_subdirectory(AndroidJNI)
endif()

if (OGRE_BUILD_SAMPLES)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Common/include)

  if (OGRE_BUILD_COMPONENT_RTSHADERSYSTEM)
    add_definitions(-DINCLUDE_RTSHADER_SYSTEM)
  	set(SAMPLE_DEPENDENCIES ${SAMPLE_DEPENDENCIES} Ogre.Components.RTShaderSystem)
  endif ()

  if (OGRE_BUILD_COMPONENT_OVERLAY)
    set(SAMPLE_DEPENDENCIES ${SAMPLE_DEPENDENCIES} Ogre.Components.Overlay)
  endif ()

  if (OGRE_BUILD_COMPONENT_TERRAIN)
  	set(SAMPLE_DEPENDENCIES ${SAMPLE_DEPENDENCIES} OgreTerrain)
  endif ()

  if (OGRE_BUILD_COMPONENT_VOLUME)
  	set(SAMPLE_DEPENDENCIES ${SAMPLE_DEPENDENCIES} OgreVolume)
  endif ()

  if (OGRE_BUILD_COMPONENT_MESHLODGENERATOR AND OGRE_CONFIG_ENABLE_MESHLOD)
  	set(SAMPLE_DEPENDENCIES ${SAMPLE_DEPENDENCIES} OgreMeshLodGenerator)
  endif ()

  if(EMSCRIPTEN)
    add_subdirectory(Emscripten)
    return()
  endif()

  ## Default Samples Plugin
  file (GLOB SAMPLES_HEADERS ${PROJECT_SOURCE_DIR}/Samples/*/include)
  include_directories(${SAMPLES_HEADERS})

  file(GLOB SAMPLES_SOURCES ${PROJECT_SOURCE_DIR}/Samples/*/src/*.cpp)

  set(BLACKLIST Browser TerrainTessellation)
  if(NOT OGRE_BUILD_PLUGIN_PCZ)
    list(APPEND BLACKLIST PCZTestApp)
  endif()
  if(NOT OGRE_BUILD_COMPONENT_TERRAIN)
    list(APPEND BLACKLIST EndlessWorld)
  endif()

  foreach(FILTER_SAMPLE ${BLACKLIST})
    file(GLOB BLACKLIST_CPP ${PROJECT_SOURCE_DIR}/Samples/${FILTER_SAMPLE}/src/*.cpp)
    list(REMOVE_ITEM SAMPLES_SOURCES ${BLACKLIST_CPP})
  endforeach()

  add_definitions(${OGRE_VISIBILITY_FLAGS})
  add_module(
    Browser/include/SampleBrowser.hpp
  PARTITION
    Common/include/DefaultSamplesPlugin.hpp
    Common/include/Sample.hpp
    Common/include/SampleContext.hpp
    Common/include/SamplePlugin.hpp
    Common/include/SdkSample.hpp
    Simple/include/NewInstancing.hpp
  IMPLEMENTATION
    Simple/src/NewInstancing.cpp
    Common/src/DefaultSamplesPlugin.cpp
  )

  ogre_config_sample_lib(Ogre.Samples)

  ## Add browser last
  add_subdirectory(Browser)
endif ()
