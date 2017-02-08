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
#include "filesystem_client.hpp"
#include "../common/network_base.hpp"

class NetworkClient : private NetworkBase {
private:
    std::string hostname_;
    int port_no_;
    std::string username_;
    int connect_server(const char *hostname, const in_port_t port_no);
    int control_msg(int client_socket);
    int file_trans(int socket, std::vector<std::string> kpath, FileSystemClient fs, int send_recv_flg);
public:
    int manage(const char* hostname, const int port_no);
    int service_init(std::string session_name, std::string prot_name, std::vector<std::string> user_names);
    int send_pk(const std::string session_name);
    int join_session(const char* hostname, const int port_no, const std::vector<std::string> info);
    int send_enc_data(const char* hostname, const int port_no, const std::vector<std::string> info);
    int receive_result(const char* hostname, const int port_no, const std::vector<std::string> info);
    NetworkClient();
    NetworkClient(const std::string hostname, const std::string port_no, const std::string user_name);

};

#endif

