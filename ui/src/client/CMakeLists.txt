include_directories(../../include/client)
include_directories(../../include/)
set(SRCS constants.cpp driver.cpp filesystem_base.cpp filesystem_client.cpp network_base.cpp network_client.cpp main.cpp)
add_executable(client ${SRCS})
