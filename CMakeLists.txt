cmake_minimum_required(VERSION 3.2)
ENABLE_TESTING()
project(coda_clion)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

if (NOT DEFINED BUILD_CORE)
set(BUILD_CORE off)
endif (NOT DEFINED BUILD_CORE)

if (NOT DEFINED BUILD_LIB)
set(BUILD_LIB off)
endif (NOT DEFINED BUILD_LIB)

if (NOT DEFINED BUILD_UI)
set(BUILD_UI off)
endif (NOT DEFINED BUILD_UI)

add_subdirectory(core)
add_subdirectory(ui)
