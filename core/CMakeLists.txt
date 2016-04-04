set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/core/modules")

include(ExternalProject)
find_package(GMP 6.0 MODULE REQUIRED)
set(GMP_PATH ${CMAKE_SOURCE_DIR}/tmp/gmp)
if (GMP_NOT_FOUND)
ExternalProject_Add(GMP
    TMP_DIR ${GMP_PATH}/tmp
    STAMP_DIR ${GMP_PATH}/stmp
    DOWNLOAD_DIR ${GMP_PATH}/download
    URL https://gmplib.org/download/gmp/gmp-6.0.0a.tar.bz2
    SOURCE_DIR ${GMP_PATH}/source
    CONFIGURE_COMMAND ${GMP_PATH}/source/configure --prefix=${CMAKE_SOURCE_DIR}/tmp
    BINARY_DIR ${GMP_PATH}/build
    BUILD_COMMAND make
    INSTALL_COMMAND make install)
endif(GMP_NOT_FOUND)

set(GMP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/tmp/include)
set(GMP_LIBRARIES ${CMAKE_SOURCE_DIR}/tmp/lib)
find_package(NTL 9.6 MODULE REQUIRED)
if (NTL_NOT_FOUND)
set(NTL_PATH ${CMAKE_SOURCE_DIR}/tmp/ntl)
ExternalProject_Add(NTL
    DEPENDS GMP
    TMP_DIR ${NTL_PATH}/tmp
    STAMP_DIR ${NTL_PATH}/stmp
    DOWNLOAD_DIR ${NTL_PATH}/download
    URL http://www.shoup.net/ntl/ntl-9.6.0.tar.gz
    SOURCE_DIR ${NTL_PATH}/source
    CONFIGURE_COMMAND ${NTL_PATH}/source/src/configure GMP_PREFIX=${CMAKE_SOURCE_DIR}/tmp NTL_GMP_LIP=on PREFIX=${CMAKE_SOURCE_DIR}/tmp
    BINARY_DIR ${NTL_PATH}/source/src/
    BUILD_COMMAND make
    INSTALL_COMMAND make install)
endif(NTL_NOT_FOUND)
find_library(local-ntl ntl HINTS ${CMAKE_SOURCE_DIR}/tmp/lib)
find_library(local-gmp gmp HINTS ${CMAKE_SOURCE_DIR}/tmp/lib)
add_subdirectory(src/core core-imp)
add_subdirectory(src/doctopt.cpp doctopt)
add_subdirectory(src/HElib fhe)
include_directories(include/)

add_executable(core core.cpp)
target_link_libraries(core docopt core-imp fhe ${local-ntl} ${local-gmp})

