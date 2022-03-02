set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})

find_program(CMAKE_C_COMPILER clang-13 REQUIRED)
find_program(CMAKE_CXX_COMPILER clang++-13 REQUIRED)

#do not use libc++ for C objects
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options(-fuse-ld=lld)

#from miniz.h:
# On platforms using glibc, Be sure to "#define _LARGEFILE64_SOURCE 1" before
# including miniz.c to ensure miniz uses the 64-bit variants
add_compile_options(-D_LARGEFILE64_SOURCE=1)

