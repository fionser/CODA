#include "utest.hpp"

int TestUnitServer::test_filesystem()
{
    FileSystemServer fs;
    std::string name_r1 = CConst::PATH_DEBUG + CConst::SEP_CH_FILE + "abc.txt";
    std::string name_e1 = "abc.txt";
    std::string name_e2 = CConst::PATH_DEBUG + CConst::SEP_CH_FILE + CConst::PATH_DEBUG + CConst::SEP_CH_FILE + "abc.txt";
    fprintf(stdout, "filesystem test : r1 : %s\n", fs.get_filepath(name_r1).c_str());
    fprintf(stdout, "filesystem test : e1 : %s\n", fs.get_filepath(name_e1).c_str());
    fprintf(stdout, "filesystem test : e2 : %s\n", fs.get_filepath(name_e2).c_str());
    return 0;
}

int TestUnitServer::test_all()
{
    test_filesystem();
    return 0;
}


