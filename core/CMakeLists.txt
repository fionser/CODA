set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/core/modules")
include(ExternalProject)
set(GMP_PATH ${CMAKE_SOURCE_DIR}/tmp/gmp)
ExternalProject_Add(GMP
    PREFIX ${GMP_PATH}
    TMP_DIR ${GMP_PATH}/tmp
    STAMP_DIR ${GMP_PATH}/stmp
    DOWNLOAD_DIR ${GMP_PATH}/download
    URL https://gmplib.org/download/gmp/gmp-6.0.0a.tar.bz2
    SOURCE_DIR ${GMP_PATH}/source
    CONFIGURE_COMMAND ${GMP_PATH}/source/configure --prefix=${CMAKE_SOURCE_DIR}/tmp
    BINARY_DIR ${GMP_PATH}/build
    BUILD_COMMAND make
    INSTALL_COMMAND make install
    )

set(NTL_PATH ${CMAKE_SOURCE_DIR}/tmp/ntl)
ExternalProject_Add(NTL
    DEPENDS GMP
    PREFIX ${NTL_PATH}
    TMP_DIR ${NTL_PATH}/tmp
    STAMP_DIR ${NTL_PATH}/stmp
    DOWNLOAD_DIR ${NTL_PATH}/download
    URL http://www.shoup.net/ntl/ntl-9.6.0.tar.gz
    SOURCE_DIR ${NTL_PATH}/source
    CONFIGURE_COMMAND ${NTL_PATH}/source/src/configure GMP_PREFIX=${CMAKE_SOURCE_DIR}/tmp NTL_GMP_LIP=on PREFIX=${CMAKE_SOURCE_DIR}/tmp
    BINARY_DIR ${NTL_PATH}/source/src/
    BUILD_COMMAND make
    INSTALL_COMMAND make install
    )

add_subdirectory(src/core lib/core-imp)
add_subdirectory(src/doctopt.cpp lib/doctopt)
add_subdirectory(src/HElib lib/fhe)
include_directories(include/)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L ${CMAKE_SOURCE_DIR}/tmp/lib -lntl -lgmp")
add_executable(core core.cpp)
target_link_libraries(core docopt core-imp fhe)


