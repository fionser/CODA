# Locate NTL
# This module defines
# NTL_LIBRARY
# NTL_FOUND, if false, do not try to link to OpenAL
# NTL_INCLUDE_DIR, where to find the headers
#
# Created by Tai Chi Minh Ralph Eastwood <tcmreastwood@gmail.com>

FIND_PATH(NTL_INCLUDE_DIR RR.h
	HINTS ${PROJECT_BINARY_DIR}/tmp/
    $ENV{NTLDIR}
    PATH_SUFFIXES include/NTL
    PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
    )

FIND_LIBRARY(NTL_LIBRARY
    NAMES ntl
	HINTS ${PROJECT_BINARY_DIR}/tmp
    PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64)

# handle the QUIETLY and REQUIRED arguments and set NTL_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NTL DEFAULT_MSG NTL_LIBRARY NTL_INCLUDE_DIR)
if (${NTL_FOUND})
MESSAGE(STATUS "NTL libs: " ${NTL_LIBRARY} " " ${NTL_INCLUDE_DIR})
MARK_AS_ADVANCED(NTL_LIBRARY NTL_INCLUDE_DIR)
endif(${NTL_FOUND})
