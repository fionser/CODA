include_directories(../../include/server)
include_directories(../../include/)
set(SRCS constants.cpp driver.cpp filesystem_base.cpp filesystem_server.cpp network_base.cpp network_server.cpp utest.cpp main.cpp)
add_executable(server ${SRCS})
