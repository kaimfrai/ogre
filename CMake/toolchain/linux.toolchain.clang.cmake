set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})

find_program(CMAKE_C_COMPILER clang-15 REQUIRED)
find_program(CMAKE_CXX_COMPILER clang++-15 REQUIRED)

#do not use libc++ for C objects
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options(-fuse-ld=lld)

#Keep assembly files for comparison
add_compile_options(-save-temps=obj)
