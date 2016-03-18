add_subdirectory(src/NTL lib/ntl)
add_subdirectory(src/HElib lib/fhe)
add_subdirectory(src/doctopt.cpp lib/doctopt)
add_subdirectory(src/core lib/core-imp)
include_directories(include/)
include(ExternalProject)
ExternalProject_Add(GMP
    PREFIX ${CMAKE_SOURCE_DIR}/core
    TMP_DIR ${CMAKE_SOURCE_DIR}/core/tmp
    STAMP_DIR ${CMAKE_SOURCE_DIR}/core/tmp/stmp
    DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/core/tmp/download
    URL https://gmplib.org/download/gmp/gmp-6.0.0a.tar.bz2
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/core/include/gmp"
    CONFIGURE_COMMAND "${CMAKE_SOURCE_DIR}/core/include/gmp/configure"
    BINARY_DIR ${CMAKE_SOURCE_DIR}/core/tmp/build
    BUILD_COMMAND make
    INSTALL_DIR ${CMAKE_BINARY_DIR}/lib
    INSTALL_COMMAND
    )
add_executable(core core.cpp)
target_link_libraries(core ntl fhe docopt core-imp)
