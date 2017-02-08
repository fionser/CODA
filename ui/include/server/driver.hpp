#include <string>
#include "filesystem_server.hpp"
#include "network_server.hpp"
#include "utest_server.hpp"

#include <string.h> // for Windows
#include <stdio.h> // for Windows

class Driver {
    private:
        void error_usage(char *cmd);
        int start_server(int argc, char *argv[]);
        void debug_m(int argc, char *argv[]);
        void test(int argc, char *argv[]);
    public:
        int drive(int argc, char *argv[]);
};

