set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})

#resolve real path for clang-tidy
find_program(CMAKE_C_COMPILER clang-15 REQUIRED)
file(REAL_PATH ${CMAKE_C_COMPILER} CMAKE_C_COMPILER EXPAND_TILDE)
find_program(CMAKE_CXX_COMPILER clang++-15 REQUIRED)
file(REAL_PATH ${CMAKE_CXX_COMPILER} CMAKE_CXX_COMPILER EXPAND_TILDE)

#do not use libc++ for C objects
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options($<$<COMPILE_LANGUAGE:CXX>:-lm>)
add_link_options($<$<COMPILE_LANGUAGE:CXX>:-lc++>)
add_link_options($<$<COMPILE_LANGUAGE:CXX>:-lc++abi>)
add_link_options(-fuse-ld=lld)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-O1)
endif ()

if	(BUILD_WITH_SANITIZER)
	add_compile_options(
		-fsanitize=address
		-fno-omit-frame-pointer
		-fno-optimize-sibling-calls
		-fsanitize-address-use-after-return=always
		-fsanitize-address-use-after-scope
	)
	add_link_options(
		-fsanitize=address
	)

endif()
