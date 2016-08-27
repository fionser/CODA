#include <string>
#include "filesystem_client.hpp"
#include "network_client.hpp"
#include <cstring> // for Windows
#include "utest_client.hpp"

class Driver {
    private:
        void error_usage(char *cmd);
        int init_session(int argc, char *argv[]);
        int send_key(int argc, char *argv[]);
        int join_session(int argc, char *argv[]);
        int send_data(int argc, char *argv[]);
        int receive_result(int argc, char *argv[]);
        int net_start(int argc, char *argv[]);
        int dir_ini(std::vector<std::string> argv);
        int d_rm_session_dir(char *dir_name);
        void debug_m(int argc, char *argv[]);
    public:
        int drive(int argc, char *argv[]);
};

