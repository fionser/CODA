set(LIB_SRCS chi2.cpp coda.cpp core.cpp  file_util.cpp global.cpp
        literal.cpp protocol.cpp greaterthan.cpp rank-k.cpp
        ctxt_util.cpp contingency_table.cpp contingency_table_helper.cpp
        protocol-contingency.cpp protocol-mean.cpp protocol-hybrid-contingency.cpp
        gc_user.cpp)
add_library(core-imp STATIC ${LIB_SRCS})
