# Try to find the GMP librairies
# GMP_FOUND - system has GMP lib
# GMP_INCLUDE_DIR - the GMP include directory
# GMP_LIBRARIES - Libraries needed to use GMP

find_path(GMP_INCLUDE_DIR
		  HINTS ${PROJECT_BINARY_DIR}/tmp
	      PATH_SUFFIXES include
          NAMES gmp.h)

find_library(GMP_LIBRARIES
	         HINTS ${PROJECT_BINARY_DIR}/tmp
	         PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
             NAMES gmp libgmp)
find_library(GMPXX_LIBRARIES NAMES gmpxx libgmpxx)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GMP DEFAULT_MSG GMP_INCLUDE_DIR GMP_LIBRARIES)
message(STATUS "GMP_INCLUDE_DIR " ${GMP_INCLUDE_DIR} " GMP_LIBRARIES " ${GMP_LIBRARIES})
mark_as_advanced(GMP_INCLUDE_DIR GMP_LIBRARIES)
