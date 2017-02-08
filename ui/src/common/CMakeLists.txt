include_directories(../../include/common)
include_directories(../../include/)
set(SRCS constants.cpp filesystem_base.cpp network_base.cpp utils.cpp)
add_library(common STATIC ${SRCS})
