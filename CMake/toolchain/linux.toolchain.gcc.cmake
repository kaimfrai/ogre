set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})

find_program(CMAKE_C_COMPILER gcc-11 REQUIRED)
find_program(CMAKE_CXX_COMPILER g++-11 REQUIRED)
