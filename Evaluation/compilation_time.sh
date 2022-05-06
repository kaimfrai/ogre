#!/bin/bash

TARGETS=(\
# 	Ogre.Core\
# 	Ogre.PlugIns.STBICodec\
# 	Ogre.RenderSystems.GLSupport\
# 	Ogre.RenderSystems.GL\
# 	Ogre.Components.RTShaderSystem\
# 	Ogre.Components.Overlay\
# 	Ogre.Components.Bites\
	SampleBrowser
)

if	[[ $# < 1 ]]
then
	echo "Path to cmake source directory required as first argument!"
	exit 1
elif [[ $# < 2 ]]
then
	echo "CMake build type required as second argument. Must be Debug or Release!"
	exit 1
fi

if	[[ $# > 2 ]]
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

if	[[ $LOOP_ITERATIONS > 1 ]]
then
	initialize_sums ${TARGETS[@]}
fi

CMAKE_SOURCE_DIR=$(realpath $1)
if	[[ ! -d $CMAKE_SOURCE_DIR ]]
then
	echo "$CMAKE_SOURCE_DIR is a not a directory!"
	exit 1
fi

CMAKE_BUILD_TYPE=$2
if	[[	$CMAKE_BUILD_TYPE != "Debug"\
	&&	$CMAKE_BUILD_TYPE != "Release"\
	&&	$CMAKE_BUILD_TYPE != "RelWithDebInfo"\
	]]
then
	echo "CMake build type $CMAKE_BUILD_TYPE not supported!"
	exit 1
fi

#do not overwrite configurations of different branches
GIT_BRANCH_NAME=$(git -C $CMAKE_SOURCE_DIR branch --show-current)
CMAKE_BINARY_DIR=$CMAKE_SOURCE_DIR/build/Compilation/Clang-$CMAKE_BUILD_TYPE-$GIT_BRANCH_NAME
THIRD_PARTY_DIR=$(realpath $CMAKE_SOURCE_DIR/../ThirdParty/)
TRACE_DIR=$CMAKE_SOURCE_DIR/build/Compilation/$CMAKE_BUILD_TYPE-$GIT_BRANCH_NAME

#ensure the directory is empty
if	[[ -d $CMAKE_BINARY_DIR ]]
then
	if	[[ -d "$CMAKE_BINARY_DIR/CMakeFiles" ]]
	then
		rm -rf $CMAKE_BINARY_DIR
	else
		echo "$CMAKE_BINARY_DIR is a directory but not a CMake binary directory. Bailing out!"
		exit 1
	fi
fi

cmake\
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON\
	--install-prefix $THIRD_PARTY_DIR\
	-G Ninja\
	--toolchain $CMAKE_SOURCE_DIR/CMake/toolchain/linux.toolchain.clang.cmake\
	-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE\
	-DOGRE_DEPENDENCIES_DIR=$THIRD_PARTY_DIR\
	-S $CMAKE_SOURCE_DIR\
	-B $CMAKE_BINARY_DIR\
	1> /dev/null

if	[[ ! -d "$CMAKE_BINARY_DIR/CMakeFiles" ]]
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
	local time=$(\time -f "%e"\
		cmake\
			--build $CMAKE_BINARY_DIR\
			--target $1\
			--config $CMAKE_BUILD_TYPE\
			2>&1 1>/dev/null\
		)
	echo $time
	if	[[ $LOOP_ITERATIONS > 1 ]]
	then
		local -n sum_ref="$1_SUM"
		sum_ref=$(echo $sum_ref + $time | bc -l)
	fi
}

time_ninja_targets()
{
	for	target in $@
	do
		time_ninja_target $target
	done
}

if	[[ ! -d $TRACE_DIR ]]
then
	mkdir\
		$TRACE_DIR
fi

loop_iteration()
{
	ninja -C $CMAKE_BINARY_DIR clean > /dev/null
	rm $CMAKE_BINARY_DIR/.ninja_log

	local time_stamp=$(date +"%Y-%m-%d_%H-%M-%S")
	local trace_log="$TRACE_DIR/ninja_$time_stamp.log"

	echo "Compilation run #$1 timestamp $time_stamp"
	time_ninja_targets ${TARGETS[@]}

	cp $CMAKE_BINARY_DIR/.ninja_log $trace_log
	echo "Trace log created in $trace_log."
	echo ""
}

for	((i = 1; i <= $LOOP_ITERATIONS; ++i))
do
	loop_iteration $i
done

if	[[ $LOOP_ITERATIONS > 1 ]]
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

echo "Trace logs can be opened with https://ui.perfetto.dev."
echo "Data can be evaluated with SQL, e.g."
echo "SELECT name AS Name, (dur / 1000000000.0) AS Seconds FROM slice ORDER BY dur DESC"
