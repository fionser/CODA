#ifndef CODA_NETWORK_BASE
#define CODA_NETWORK_BASE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <vector>
#include <fstream>
#include <errno.h>
#include "constants.hpp"

extern const int BUFF_LEN;

class NetworkBase {
    public:
        int send_msg(int socket, const std::string msg);
        int recv_msg(int socket, char *msg, size_t len);
        int recv_check(int socket, const std::string msg);
        int snd_file(int socket, const std::string local_file_path, const std::string keyword_file_path);
        int recv_file(int socket, const std::string local_file_path, const std::string keyword_file_path);
};

#endif

