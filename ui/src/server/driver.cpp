#include "driver.hpp"

void Driver::error_usage(char *cmd)
{
    fprintf(stderr, "usage :\n");
    fprintf(stderr, "      : %s %s <port_no>\n", cmd, CConst::S_MAIN_CMD);
    // fprintf(stderr, "      : %s \n", cmd);
}

int Driver::start_server(int argc, char *argv[])
{
    /*
     * argc = 3
     * argv  : 0   1     2
     * usage : cmd start port_no
     */
    if( argc < 3) {
        error_usage(argv[0]);
        return -1;
    }
    NetworkServer s;
    s.start_service(atoi(argv[2]));
    return 0;
}

void Driver::test(int argc, char *argv[])
{
    TestUnitServer tus;
    tus.test_all();
}

int Driver::drive(int argc, char *argv[])
{
    if( argc < 2) {
        error_usage(argv[0]);
        return -1;
    } else if(std::string(argv[1]) == CConst::S_MAIN_CMD) {
        start_server(argc, argv);
    } else if(std::string(argv[1]) == std::string("test")) {
        test(argc, argv);
    } else if(std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v") {
        std::cout << CConst::VER_SERVER << std::endl;
    } else {
        error_usage(argv[0]);
        return -1;
    }
    return 0;
}

