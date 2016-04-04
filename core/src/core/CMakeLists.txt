include_directories(../../include)
include_directories(${PROJECT_BINARY_DIR}/tmp/include)
set(LIB_SRCS chi2.cpp coda.cpp core.cpp contingency.cpp file_util.cpp global.cpp literal.cpp protocol.cpp contingency-eval.cpp.cpp)
add_library(core-imp STATIC ${LIB_SRCS})
