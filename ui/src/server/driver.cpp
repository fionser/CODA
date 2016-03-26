#include "driver.hpp"

void Driver::error_usage(char *cmd)
{
    fprintf(stderr, "usage :\n");
    fprintf(stderr, "      : %s %s <port_no>\n", cmd, CConst::S_MAIN_CMD);
    fprintf(stderr, "      : %s %s <analyst_name> <session_name>\n", cmd, CConst::S_SUB_CMD_INIT);
    fprintf(stderr, "      : %s %s <analyst_name> <session_name> <add_user_name>\n", cmd, CConst::S_SUB_CMD_UADD);
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

int Driver::dir_ini(int argc, char *argv[])
{
    /*
     * argc = 4
     * argv  : 0   1       2              3
     * usage : cmd d_init <analyst_name> <session_name>
     */
    if( argc < 4) {
        error_usage(argv[0]);
        return -1;
    }
    FileSystemServer d;
    if(d.make_user_directory(argv[2]) == 0){
        d.make_session_directory(argv[2], argv[3]);
    }
    return 0;
}

int Driver::dir_useradd(int argc, char *argv[])
{
    /*
     * argc = 5
     * argv  : 0   1       2              3              4
     * usage : cmd d_uadd <analyst_name> <session_name> <add_user_name>
     */
    if( argc < 5) {
        error_usage(argv[0]);
        return -1;
    }
    FileSystemServer d;
    d.add_user(argv[2], argv[3], argv[4]);
    return 0;
}

void Driver::debug_m(int argc, char *argv[])
{
    /*
     * argv  : 0   1     2...
     * usage : cmd debug arg...
     */
    FileSystemServer d;
    std::vector<std::string> test = d.get_file_list(argv[2]);
    for(int i=0; i<test.size(); i++) {
        printf("list %d : %s\n", i, test[i].c_str());
    }
    printf("result : %d\n", d.remove_directory(argv[2]));
}

void Driver::test_m()
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
    } else if (std::string(argv[1]) == CConst::S_SUB_CMD_INIT) {
        dir_ini(argc, argv);
    } else if(std::string(argv[1]) == CConst::S_SUB_CMD_UADD) {
        dir_useradd(argc, argv);
    } else if(std::string(argv[1]) == std::string("debug")) {
        debug_m(argc, argv);
    } else if(std::string(argv[1]) == std::string("test")) {
        test_m();
    } else {
        error_usage(argv[0]);
        return -1;
    }
    return 0;
}

