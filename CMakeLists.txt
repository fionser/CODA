cmake_minimum_required(VERSION 3.2)
project(coda_clion)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

include(ExternalProject)
set(GMP_PATH ${PROJECT_BINARY_DIR}/tmp/gmp)
ExternalProject_Add(GMP
	TMP_DIR ${GMP_PATH}/tmp
	STAMP_DIR ${GMP_PATH}/stmp
	DOWNLOAD_DIR ${GMP_PATH}/download
	URL https://gmplib.org/download/gmp/gmp-6.0.0a.tar.bz2
	SOURCE_DIR ${GMP_PATH}/source
	CONFIGURE_COMMAND ${GMP_PATH}/source/configure --prefix=${PROJECT_BINARY_DIR}/tmp
	BINARY_DIR ${GMP_PATH}/build
	BUILD_COMMAND make
	INSTALL_COMMAND make install)

set(GMP_INCLUDE_DIR ${PROJECT_BINARY_DIR}/tmp/include)
set(GMP_LIBRARIES ${PROJECT_BINARY_DIR}/tmp/lib)

set(NTL_PATH ${PROJECT_BINARY_DIR}/tmp/ntl)
ExternalProject_Add(NTL
	DEPENDS GMP
    TMP_DIR ${NTL_PATH}/tmp
    STAMP_DIR ${NTL_PATH}/stmp
    DOWNLOAD_DIR ${NTL_PATH}/download
    URL http://www.shoup.net/ntl/ntl-9.6.0.tar.gz
    SOURCE_DIR ${NTL_PATH}/source
	CONFIGURE_COMMAND ${NTL_PATH}/source/src/configure GMP_PREFIX=${PROJECT_BINARY_DIR}/tmp NTL_GMP_LIP=on PREFIX=${PROJECT_BINARY_DIR}/tmp
    BINARY_DIR ${NTL_PATH}/source/src/
    BUILD_COMMAND make
    INSTALL_COMMAND make install)

add_subdirectory(core)
add_subdirectory(ui)
