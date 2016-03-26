#ifndef CODA_NETWORK_CLIENT
#define CODA_NETWORK_CLIENT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <errno.h>
#include "network_base.hpp"
#include "filesystem_client.hpp"

class NetworkClient : private NetworkBase {
    private:
        int connect_server(const char *hostname, const in_port_t port_no);
        int control_msg(int client_socket);
        int file_trans(int socket, std::vector<std::string> kpath, FileSystemClient fs, int send_recv_flg);
    public:
        int manage(const char* hostname, const int port_no);
        int service_init(const char* hostname, const int port_no, const std::vector<std::string> info);
        int send_pk(const char* hostname, const int port_no, const std::vector<std::string> info);
        int join_session(const char* hostname, const int port_no, const std::vector<std::string> info);
        int send_enc_data(const char* hostname, const int port_no, const std::vector<std::string> info);
        int receive_result(const char* hostname, const int port_no, const std::vector<std::string> info);
};

#endif

