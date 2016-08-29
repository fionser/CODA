#include "driver.hpp"

void Driver::error_usage(char *cmd)
{
    fprintf(stderr, "usage :\n");
    fprintf(stderr, "      : %s %s <hostname> <portno> <session_name> <analyst_name> <protocol> <schema_file_path> <user_name ...>\n", cmd, CConst::C_MAIN_CMD_INIT);
    fprintf(stderr, "      : %s %s <hostname> <portno> <session_name> <analyst_name>\n", cmd, CConst::C_MAIN_CMD_SEND_KEY);
    fprintf(stderr, "      : %s %s <hostname> <portno> <session_name> <analyst_name> <user_name>\n", cmd, CConst::C_MAIN_CMD_JOIN);
    fprintf(stderr, "      : %s %s <hostname> <portno> <session_name> <analyst_name> <user_name>\n", cmd, CConst::C_MAIN_CMD_SEND_DATA);
    fprintf(stderr, "      : %s %s <hostname> <portno> <session_name> <analyst_name>\n", cmd, CConst::C_MAIN_CMD_RECEIVE_RESULT);
    fprintf(stderr, "      : %s %s <hostname> <port_no>\n", cmd, CConst::C_SUB_CMD_NET);
    fprintf(stderr, "      : %s %s \n", cmd, CConst::C_SUB_CMD_DEBUG);
}

int Driver::init_session(int argc, char *argv[])
{
    /*
     * argc = 9
     * argv  : 0  1     2          3        4              5              6          7                  8
     * usage : ui init <hostname> <portno> <session_name> <analyst_name> <protocol> <schema_file_path> <user_name ...>
     */
    if( argc < 9) {
        error_usage(argv[0]);
        return -1;
    }
    std::vector<std::string> args;
    for(int i=4; i < argc; i++){
        args.push_back(argv[i]);
    }
    if(dir_ini(args) != 0){
        L_ERROR(_console, "init session directory.");
        if(d_rm_session_dir(argv[4]) != 0) {
            L_ERROR(_console, "remove directory NG.");
        }
        return -1;
    }
    NetworkClient c;
    if(c.service_init(argv[2], atoi(argv[3]), args) == 0){
        L_INFO(_console, "init session OK.");
        return 0;
    } else {
        L_ERROR(_console, "init session NG.");
        if(d_rm_session_dir(argv[4]) != 0) {
            L_ERROR(_console, "remove directory NG.");
        }
        return -1;
    }
}

int Driver::send_key(int argc, char *argv[])
{
    /*
     * argc = 6
     * argv  : 0  1         2          3        4              5
     * usage : ui send_key <hostname> <portno> <session_name> <analyst_name>
     */
    if( argc < 6) {
        error_usage(argv[0]);
        return -1;
    }
    std::vector<std::string> args;
    for(int i=4; i < argc; i++){
        args.push_back(argv[i]);
    }
    NetworkClient c;
    if(c.send_pk(argv[2], atoi(argv[3]), args) == 0){
        L_INFO(_console, "send public key OK.");
        return 0;
    } else {
        L_ERROR(_console, "send public key NG.");
        return -1;
    }
}

int Driver::join_session(int argc, char *argv[])
{
    /*
     * argc = 7
     * argv  : 0  1     2          3        4              5              6
     * usage : ui join <hostname> <portno> <session_name> <analyst_name> <user_name>
     */
    if( argc < 7) {
        error_usage(argv[0]);
        return -1;
    }
    FileSystemClient d;
    if(d.make_directory(argv[4]) != 0){
        return -1;
    }
    std::vector<std::string> args;
    for(int i=4; i < argc; i++){
        args.push_back(argv[i]);
    }
    NetworkClient c;
    if(c.join_session(argv[2], atoi(argv[3]), args) == 0) {
        L_INFO(_console, "join session OK.");
        return 0;
    } else {
        L_ERROR(_console, "join session NG.");
        if(d_rm_session_dir(argv[4]) != 0) {
            L_ERROR(_console, "remove directory NG.");
        }
        return -1;
    }
}

int Driver::send_data(int argc, char *argv[])
{
    /*
     * argc = 7
     * argv  : 0  1          2          3        4              5              6
     * usage : ui send_data <hostname> <portno> <session_name> <analyst_name> <user_name>
     */
    if( argc < 7) {
        error_usage(argv[0]);
        return -1;
    }
    std::vector<std::string> args;
    for(int i=4; i < argc; i++){
        args.push_back(argv[i]);
    }
    NetworkClient c;
    if(c.send_enc_data(argv[2], atoi(argv[3]), args) == 0){
        L_INFO(_console, "send enc data OK.");
        return 0;
    } else {
        L_ERROR(_console, "send enc data NG.");
        return -1;
    }
}

int Driver::receive_result(int argc, char *argv[])
{
    /*
     * argc = 6
     * argv  : 0  1               2          3        4              5   
     * usage : ui receive_result <hostname> <portno> <session_name> <analyst_name>
     */
    if( argc < 6) {
        error_usage(argv[0]);
        return -1;
    }
    std::vector<std::string> args;
    for(int i=4; i < argc; i++){
        args.push_back(argv[i]);
    }
    NetworkClient c;
    if(c.receive_result(argv[2], atoi(argv[3]), args) == 0){
        L_INFO(_console, "receive result data OK.");
        return 0;
    } else {
        L_ERROR(_console, "receive result data NG.");
        return -1;
    }
    return 0;
}


int Driver::net_start(int argc, char *argv[])
{
    /*
     * argc = 4
     * argv  : 0  1      2          3
     * usage : ui d_net <hostname> <port_no>
     */
    if( argc < 4) {
        error_usage(argv[0]);
        return -1;
    }
    NetworkClient c;
    c.manage(argv[2], atoi(argv[3]));
    return 0;
}

int Driver::dir_ini(std::vector<std::string> argv)
{
    /*
     * argc >= 5
     * argv  : 0              1              2          3                   4...
     * usage : <session_name> <analyst_name> <protocol> <schema_file_path> <user_name ...>
     */
    FileSystemClient d;
    return d.make_analyst_info(argv);
}

int Driver::d_rm_session_dir(char *dir_name)
{
    FileSystemClient d;
    if(d.remove_directory(dir_name) == 0){
        return 0;
    } else {
        return -1;
    }
}

void Driver::debug_m(int argc, char *argv[])
{
    /*
     * argv  : 0   1       2...
     * usage : cmd d_debug arg...
     */
    /////////// init directory by analyst info ////////////////////////////////
    if( argc < 6) {
        error_usage(argv[0]);
        return;
    }
    std::vector<std::string> args;
    for(int i=2; i < argc; i++){
        args.push_back(argv[i]);
    }
    if(dir_ini(args) != 0){
        L_ERROR(_console, "init session directory.");
        return;
    }
    /////////// dir remove debug ////////////////////////////////
    // d_rm_session_dir(argv[2]);
    /////////// file list debug /////////////////////////////////
    // FileSystemClient d;
    // std::vector<std::string> test = d.get_file_list(argv[2]);
    // for(int i=0; i<test.size(); i++) {
    //     printf("list %d : %s\n", i, test[i].c_str());
    // }
    // printf("result : %d\n", d.remove_directory(argv[2]));
}

int Driver::drive(int argc, char *argv[])
{
    if( argc < 2) {
        error_usage(argv[0]);
        return -1;
    } else if (std::string(argv[1]) == CConst::C_MAIN_CMD_INIT) {
        init_session(argc, argv);
    } else if (std::string(argv[1]) == CConst::C_MAIN_CMD_SEND_KEY) {
        send_key(argc, argv);
    } else if (std::string(argv[1]) == CConst::C_MAIN_CMD_JOIN) {
        join_session(argc, argv);
    } else if (std::string(argv[1]) == CConst::C_MAIN_CMD_SEND_DATA) {
        send_data(argc, argv);
    } else if (std::string(argv[1]) == CConst::C_MAIN_CMD_RECEIVE_RESULT) {
        receive_result(argc, argv);
    } else if (std::string(argv[1]) == CConst::C_SUB_CMD_NET) {
        net_start(argc, argv);
    } else if (std::string(argv[1]) == CConst::C_SUB_CMD_DEBUG) {
        debug_m(argc, argv);
    } else if (std::string(argv[1]) == "test") {
        UTC::test();
    } else {
        error_usage(argv[0]);
        return -1;
    }
    return 0;
}


