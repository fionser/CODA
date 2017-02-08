#include "utest_server.hpp"

int TestUnitServer::test_filesystem()
{
    FileSystemServer fs("tses", "tusr", "tana");
    if(fs.make_session_directory() == 0) {
        std::cout << "[OK] " << "make session directory." << std::endl;
    } else {
        std::cout << "[NG] " << "make session directory." << std::endl;
    }
    if(fs.make_result_directory("con_1-2") == 0) {
        std::cout << "[OK] " << "make session directory." << std::endl;
    } else {
        std::cout << "[NG] " << "make session directory." << std::endl;
    }
    if(fs.make_data_type_directory("cat") == 0) {
        std::cout << "[OK] " << "make data type directory." << std::endl;
    } else {
        std::cout << "[NG] " << "make data type directory." << std::endl;
    }
    return 0;
}

int TestUnitServer::test_all()
{
    test_filesystem();
    return 0;
}


