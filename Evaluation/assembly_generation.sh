#!/bin/bash

if	[[ $# < 1 ]]
then
	echo "Path to cmake source directory required as first argument!"
	exit 1
elif [[ $# < 2 ]]
then
	echo "CMake build type required as second argument. Must be Debug or Release!"
	exit 1
elif [[ $# < 3 ]]
then
	echo "C++ standard version required as third argument."
	exit 1
elif [[ $# < 4 ]]
then
	echo "Clang path required as third argument."
	exit 1
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

CMAKE_CXX_STANDARD=$3
CMAKE_CXX_COMPILER=$4

#do not overwrite configurations of different branches
GIT_BRANCH_NAME=$(git -C $CMAKE_SOURCE_DIR branch --show-current)
CMAKE_BINARY_DIR=$CMAKE_SOURCE_DIR/build/Assembly-Build/$CMAKE_CXX_COMPILER-$CMAKE_BUILD_TYPE-$CMAKE_CXX_STANDARD-$GIT_BRANCH_NAME
ASSEMBLY_DIR=$CMAKE_SOURCE_DIR/build/Assembly/$CMAKE_CXX_COMPILER-$CMAKE_BUILD_TYPE-$CMAKE_CXX_STANDARD-$GIT_BRANCH_NAME
THIRD_PARTY_DIR=$(realpath $CMAKE_SOURCE_DIR/../ThirdParty/)

cmake\
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON\
	--install-prefix $THIRD_PARTY_DIR\
	-G Ninja\
	-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE\
	-DOGRE_DEPENDENCIES_DIR=$THIRD_PARTY_DIR\
	-S $CMAKE_SOURCE_DIR\
	-B $CMAKE_BINARY_DIR\
	-DCMAKE_CXX_COMPILER=$CMAKE_CXX_COMPILER\
	-DCMAKE_CXX_FLAGS="-g -stdlib=libc++"\
	-DCMAKE_CXX_STANDARD=$CMAKE_CXX_STANDARD

cmake\
	--build $CMAKE_BINARY_DIR\
	--target SampleBrowser\
	--config $CMAKE_BUILD_TYPE

#switch to cmake binary dir to omit prefixes
pushd $CMAKE_BINARY_DIR > /dev/null

OBJECT_FILE_STRING=$(find ./ -type f -name *.cpp.o -exec echo "{}" \;)
readarray -t OBJECT_FILE_LIST <<<"${OBJECT_FILE_STRING[0]}"

disassemble()
{
	local obj=$1
	echo -ne "\033[2K\rGenerating assembly for ${obj}"
	local assembly=$(realpath $obj)
	assembly=${assembly/$CMAKE_BINARY_DIR/$ASSEMBLY_DIR}

	#trim as much noise as possible
	assembly=${assembly/"/CMakeFiles/"/"/"}
	assembly=${assembly/".dir/"/"/"}
	assembly=${assembly/"/src/"/"/"}
	assembly=${assembly/.cpp.o/.s}

	mkdir -p $(dirname $assembly)

	llvm-objdump\
		--demangle\
		--disassemble\
		--no-leading-addr\
		--no-show-raw-insn\
		-M intel\
		$obj\
		> "${assembly}"
}

for	obj in "${OBJECT_FILE_LIST[@]}"
do
	disassemble $obj
done

echo -e "\033[2K\rGenerated assembly files can be found in $ASSEMBLY_DIR/."

#switch back from cmake binary dir
popd > /dev/null
