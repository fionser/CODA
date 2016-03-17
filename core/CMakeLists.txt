add_subdirectory(src/NTL lib/ntl)
add_subdirectory(src/HElib lib/fhe)
add_subdirectory(src/doctopt.cpp lib/doctopt)
add_subdirectory(src/core lib/core-imp)
include_directories(include/)

add_executable(core core.cpp)
target_link_libraries(core ntl fhe docopt core-imp)