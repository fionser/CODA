#ifndef UI_CLIENT_DRIVER_HPP
#define UI_CLIENT_DRIVER_HPP
#include <string>
#include "filesystem_client.hpp"
#include "network_client.hpp"
#include <cstring> // for Windows
#include "utest_client.hpp"
#include "../common/utils.h"
#include "schema.hpp"

class Driver {
private:
    void error_usage(char *cmd);
    int init_session(int argc, char *argv[]);
    int send_key(int argc, char *argv[]);
    int join_session(int argc, char *argv[]);
    int send_data(int argc, char *argv[]);
    int convert(int argc, char *argv[]);
    int receive_result(int argc, char *argv[]);
    int net_start(int argc, char *argv[]);
    int d_rm_session_dir(char *dir_name);
    std::map<std::string, std::string> get_config_info(std::string file_path);
public:
    Driver() {}
    ~Driver() {}
    int drive(int argc, char *argv[]);
};
#endif
