find_library(local-ntl ntl HINTS ${PROJECT_BINARY_DIR}/tmp/lib)
find_library(local-gmp gmp HINTS ${PROJECT_BINARY_DIR}/tmp/lib)

add_subdirectory(src/core core-imp)
add_subdirectory(src/doctopt.cpp doctopt)
add_subdirectory(src/HElib fhe)

include_directories(include/)
add_executable(core core.cpp)
target_link_libraries(core core-imp fhe ${local-ntl} ${local-gmp} pthread docopt)
