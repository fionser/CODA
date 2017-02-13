set(LIB_SRCS Context.cpp SecKey.cpp PubKey.cpp EncVec.cpp coda_ppe_wrapper.cpp)
add_library(PPE STATIC ${LIB_SRCS})
