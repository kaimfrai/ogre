set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})

find_program(CMAKE_C_COMPILER gcc-11 REQUIRED)
find_program(CMAKE_CXX_COMPILER g++-11 REQUIRED)

#from miniz.h:
# On platforms using glibc, Be sure to "#define _LARGEFILE64_SOURCE 1" before
# including miniz.c to ensure miniz uses the 64-bit variants
add_compile_options(-D_LARGEFILE64_SOURCE=1)
