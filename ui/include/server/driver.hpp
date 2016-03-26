#include <string>
#include "filesystem_server.hpp"
#include "network_server.hpp"
#include "utest.hpp"

#include <string.h> // for Windows
#include <stdio.h> // for Windows

class Driver {
    private:
        void error_usage(char *cmd);
        int start_server(int argc, char *argv[]);
        int dir_ini(int argc, char *argv[]);
        int dir_useradd(int argc, char *argv[]);
        void debug_m(int argc, char *argv[]);
        void test_m();
    public:
        int drive(int argc, char *argv[]);
};

