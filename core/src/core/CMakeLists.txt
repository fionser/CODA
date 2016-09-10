include_directories(../../include)
include_directories(${NTL_INCLUDE_PATH})
set(LIB_SRCS chi2.cpp coda.cpp core.cpp  file_util.cpp global.cpp
        literal.cpp protocol.cpp mean.cpp greaterthan.cpp rank-k.cpp
        ctxt_util.cpp contingency_table.cpp contingency_table_helper.cpp 
        protocol-contingency.cpp protocol-mean.cpp)
add_library(core-imp STATIC ${LIB_SRCS})
