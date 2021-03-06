cmake_minimum_required(VERSION 3.2)
project("coda-core")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

include(ExternalProject)
if(${BUILD_gmp} OR ${BUILD_lib})
set(GMP_PATH ${PROJECT_BINARY_DIR}/tmp/gmp)
ExternalProject_Add(GMP
	TMP_DIR ${GMP_PATH}/tmp
	PREFIX ${GMP_PATH}
	STAMP_DIR ${GMP_PATH}/stmp
	DOWNLOAD_DIR ${GMP_PATH}/download
	URL ${PROJECT_SOURCE_DIR}/thirdparties/gmp-6.1.0.tar.bz2
	SOURCE_DIR ${GMP_PATH}/source
	CONFIGURE_COMMAND ${GMP_PATH}/source/configure --prefix=${PROJECT_BINARY_DIR}
	BINARY_DIR ${GMP_PATH}/build
	BUILD_COMMAND make
	INSTALL_COMMAND make install)
set(GMP_INCLUDE_DIR ${PROJECT_BINARY_DIR}/tmp/include)
set(GMP_LIBRARIES ${PROJECT_BINARY_DIR}/tmp/lib)
endif(${BUILD_gmp} OR ${BUILD_lib})


set(NTL_PATH ${PROJECT_BINARY_DIR}/tmp/ntl)
string(TOLOWER ${MULT_THREADS} NTL_THREADS)
if(${BUILD_lib})
ExternalProject_Add(NTL
    DEPENDS GMP
    PREFIX ${NTL_PATH}
    TMP_DIR ${NTL_PATH}/tmp
    STAMP_DIR ${NTL_PATH}/stmp
    DOWNLOAD_DIR ${NTL_PATH}/download
    URL ${PROJECT_SOURCE_DIR}/thirdparties/ntl-9.11.0.tar.gz
    SOURCE_DIR ${NTL_PATH}/source
    CONFIGURE_COMMAND ${NTL_PATH}/source/src/configure GMP_PREFIX=${PROJECT_BINARY_DIR} NTL_GMP_LIP=on NTL_THREADS=${NTL_THREADS} PREFIX=${PROJECT_BINARY_DIR} WIZARD=off
    BINARY_DIR ${NTL_PATH}/source/src/
    BUILD_COMMAND make
    INSTALL_COMMAND make install)
elseif(${BUILD_ntl})
ExternalProject_Add(NTL
    PREFIX ${NTL_PATH}
    TMP_DIR ${NTL_PATH}/tmp
    STAMP_DIR ${NTL_PATH}/stmp
    DOWNLOAD_DIR ${NTL_PATH}/download
    URL ${PROJECT_SOURCE_DIR}/thirdparties/ntl-9.11.0.tar.gz
    SOURCE_DIR ${NTL_PATH}/source
    CONFIGURE_COMMAND ${NTL_PATH}/source/src/configure GMP_PREFIX=${PROJECT_BINARY_DIR} NTL_GMP_LIP=on NTL_THREADS=${NTL_THREADS} PREFIX=${PROJECT_BINARY_DIR} WIZARD=off
    BINARY_DIR ${NTL_PATH}/source/src/
    BUILD_COMMAND make
    INSTALL_COMMAND make install)
endif(${BUILD_lib})

if(${BUILD_core})
SET(NTL_INCLUDE_PATH "${PROJECT_BINARY_DIR}/include" CACHE INTERNAL "")
include_directories(${NTL_INCLUDE_PATH})

if (${MULT_THREADS})
add_definitions(-DFHE_THREADS)
add_definitions(-DNR_THREADS=${NR_THREADS})
MESSAGE(STATUS "Using ${NR_THREADS} threads")
else(${MULT_THREADS})
add_definitions(-DNFHE_THREADS)
add_definitions(-DNR_THREADS=1)
endif(${MULT_THREADS})

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}/include   # add the binary tree to the search path so that we will find Version.h
    )

add_subdirectory(src)

add_executable(core core.cpp)
find_library(local-ntl
             NAMES libntl.a ntl
	     HINTS "${PROJECT_BINARY_DIR}/lib")
find_library(local-gmp
             NAMES libgmp.a gmp
	     HINTS "${PROJECT_BINARY_DIR}/lib")
     #MESSAGE(STATUS ${local-ntl})
     #MESSAGE(STATUS ${local-gmp})
target_link_libraries(core core-imp fhe ${local-ntl} ${local-gmp} pthread docopt)

if (${BUILD_test})
add_subdirectory(test test)
endif(${BUILD_test})


endif(${BUILD_core})
