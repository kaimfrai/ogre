#!/bin/bash

TARGETS=(\
	OgreMain\
	Codec_STBI\
	OgreGLSupport\
	RenderSystem_GL\
	OgreRTShaderSystem\
	OgreOverlay\
	OgreBites\
	DefaultSamples\
	SampleBrowser
)

if [[ $# < 1 ]]
then
	echo "Path to cmake source directory required as first argument!"
	exit 1
elif [[ $# < 2 ]]
then
	echo "CMake build type required as second argument. Must be Debug or Release!"
	exit 1
fi

if [[ $# > 2 ]]
then
	LOOP_ITERATIONS=$3
else
	LOOP_ITERATIONS=1
fi

initialize_sum()
{
	local -n sum_ref="$1_SUM"
	sum_ref=0
}

initialize_sums()
{
	for target in $@
	do
		initialize_sum $target
	done
}

if [[ $LOOP_ITERATIONS > 1 ]]
then
	initialize_sums ${TARGETS[@]}
fi

CMAKE_SOURCE_DIR=$1
if [[ ! -d $CMAKE_SOURCE_DIR ]]
then
    echo "$CMAKE_SOURCE_DIR is a not a directory!"
	exit 1
fi

CMAKE_BUILD_TYPE=$2
if [[ $CMAKE_BUILD_TYPE != "Debug" && $CMAKE_BUILD_TYPE != "Release" ]]
then
	echo "CMake build type $CMAKE_BUILD_TYPE not supported!"
	exit 1
fi

CMAKE_BINARY_DIR=$CMAKE_SOURCE_DIR/build/Clang-$CMAKE_BUILD_TYPE

#ensure the directory is empty
if [[ -d $CMAKE_BINARY_DIR ]]
then
	if	[[ -d "$CMAKE_BINARY_DIR/CMakeFiles" ]]
	then
		rm -rf $CMAKE_BINARY_DIR
	else
		echo "$CMAKE_BINARY_DIR is a directory but not a CMake binary directory. Bailing out!"
		exit 1
	fi
fi

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INSTALL_PREFIX=$CMAKE_SOURCE_DIR/../ThirdParty/ -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -G Ninja --toolchain $CMAKE_SOURCE_DIR/CMake/toolchain/linux.toolchain.clang.cmake -DOGRE_DEPENDENCIES_DIR=$CMAKE_SOURCE_DIR/../ThirdParty/ -S $CMAKE_SOURCE_DIR -B $CMAKE_BINARY_DIR 1> /dev/null

if [[ ! -d "$CMAKE_BINARY_DIR/CMakeFiles" ]]
then
    echo "$CMAKE_BINARY_DIR does not appear to be a valid CMake binary directory!"
	exit 1
elif [[ ! -f "$CMAKE_BINARY_DIR/build.ninja" ]]
then
    echo "$CMAKE_BINARY_DIR does not appear to be configured with Ninja!"
    exit 1
fi

time_ninja_target()
{
	echo -n "Time $1: "
	local time=$(\time -f "%e" ninja -C $CMAKE_BINARY_DIR $1 2>&1 1>/dev/null)
	echo $time
	local -n sum_ref="$1_SUM"
	sum_ref=$(echo $sum_ref + $time | bc -l)
}

time_ninja_targets()
{
	for target in $@
	do
		time_ninja_target $target
	done
}

for ((i = 1; i <= $LOOP_ITERATIONS; ++i))
do
	echo "Compilation run #$i"
	time_ninja_targets ${TARGETS[@]}
	echo ""
done

if [[ $LOOP_ITERATIONS > 1 ]]
then
	print_average()
	{
		local -n sum_ref="$1_SUM"
		local average=$(echo "scale=2; $sum_ref / $LOOP_ITERATIONS" | bc -l)
		echo "Avg. Time $1: $average"
	}

	print_averages()
	{
		for target in $@
		do
			print_average $target
		done
	}

	print_averages ${TARGETS[@]}
fi
