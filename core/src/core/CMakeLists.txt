include_directories(../../include)
set(LIB_SRCS chi2.cpp coda.cpp core.cpp contingency.cpp file_util.cpp global.cpp literal.cpp protocol.cpp)
add_library(core-imp STATIC ${LIB_SRCS})