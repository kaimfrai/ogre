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
add_link_options(-fuse-ld=lld)
