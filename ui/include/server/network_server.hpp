#ifndef CODA_NETWORK_SERVER
#define CODA_NETWORK_SERVER
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include "network_base.hpp"
#include "filesystem_server.hpp"

class NetworkServer : private NetworkBase {
    private:
        int connect_client(const in_port_t port_no);
        int control_msg(int connected_socket);
        int init_session(int socket);
        int receive_pk(int socket);
        int join_session(int socket);
        int receive_enc_data(int socket);
        int send_result_data(int socket);
        int file_trans(int socket, FileSystemServer fs);
    public:
        int start_service(const int port_no);
};

#endif

