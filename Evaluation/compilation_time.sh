if [[ $# < 1 ]]
then
	echo "Path to cmake binary directory required as first argument!"
	exit 1
fi

CMAKE_BINARY_DIR=$1
if [[ ! -d $CMAKE_BINARY_DIR ]]
then
    echo "$CMAKE_BINARY_DIR is a not a directory!"
	exit 1
elif [[ ! -d "$CMAKE_BINARY_DIR/CMakeFiles" ]]
then
    echo "$CMAKE_BINARY_DIR does not appear to be a valid CMake binary directory!"
	exit 1
elif [[ ! -f "$CMAKE_BINARY_DIR/build.ninja" ]]
then
    echo "$CMAKE_BINARY_DIR does not appear to be configured with Ninja!"
    exit 1
fi

ninja -C $CMAKE_BINARY_DIR clean > /dev/null

echo -n "Time OgreMain: "
ogre_main_time=$(\time -f "%e" ninja -C $CMAKE_BINARY_DIR OgreMain 2>&1 1>/dev/null)
echo $(echo $ogre_main_time | bc)

echo -n "Time Codec_STBI: "
codec_stbi_time=$(\time -f "%e" ninja -C $CMAKE_BINARY_DIR Codec_STBI 2>&1 1>/dev/null)
echo $(echo $codec_stbi_time | bc)

echo -n "Time OgreGLSupport: "
ogre_glsupport_time=$(\time -f "%e" ninja -C $CMAKE_BINARY_DIR OgreGLSupport 2>&1 1>/dev/null)
echo $(echo $ogre_glsupport_time | bc)

echo -n "Time RenderSystem_GL: "
rendersystem_gl_time=$(\time -f "%e" ninja -C $CMAKE_BINARY_DIR RenderSystem_GL 2>&1 1>/dev/null)
echo $(echo $rendersystem_gl_time | bc)

echo -n "Time OgreRTShaderSystem: "
ogre_rtshadersystem_time=$(\time -f "%e" ninja -C $CMAKE_BINARY_DIR OgreRTShaderSystem 2>&1 1>/dev/null)
echo $(echo $ogre_rtshadersystem_time | bc)

echo -n "Time OgreOverlay: "
ogre_overlay_time=$(\time -f "%e" ninja -C $CMAKE_BINARY_DIR OgreOverlay 2>&1 1>/dev/null)
echo $(echo $ogre_overlay_time | bc)

echo -n "Time OgreBites: "
ogre_bites_time=$(\time -f "%e" ninja -C $CMAKE_BINARY_DIR OgreBites 2>&1 1>/dev/null)
echo $(echo $ogre_bites_time | bc)

echo -n "Time DefaultSamples: "
default_samples_time=$(\time -f "%e" ninja -C $CMAKE_BINARY_DIR DefaultSamples 2>&1 1>/dev/null)
echo $(echo $default_samples_time | bc)

echo -n "Time SampleBrowser: "
sample_browser_time=$(\time -f "%e" ninja -C $CMAKE_BINARY_DIR SampleBrowser 2>&1 1>/dev/null)
echo $(echo $sample_browser_time | bc)
