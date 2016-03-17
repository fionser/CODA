include_directories(../../include/docopt.cpp)
set(LIB_SRCS docopt.cpp)
add_library(docopt STATIC ${LIB_SRCS})