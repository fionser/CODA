#include "driver.hpp"

void Driver::error_usage(char *cmd)
{
    fprintf(stderr, "usage :\n");
    fprintf(stderr, "      : %s %s <session_name> <protocol> <schema_file_path> <user_name ...>\n", cmd, CConst::C_MAIN_CMD_INIT);
    fprintf(stderr, "      : %s %s <session_name>\n", cmd, CConst::C_MAIN_CMD_SEND_KEY);
    fprintf(stderr, "      : %s %s <hostname> <portno> <session_name> <analyst_name> <user_name>\n", cmd, CConst::C_MAIN_CMD_JOIN);
    fprintf(stderr, "      : %s %s <hostname> <portno> <session_name> <analyst_name> <user_name>\n", cmd, CConst::C_MAIN_CMD_SEND_DATA);
    fprintf(stderr, "      : %s %s <hostname> <portno> <session_name> <analyst_name>\n", cmd, CConst::C_MAIN_CMD_RECEIVE_RESULT);
    fprintf(stderr, "      : %s %s [-e] <session_name> <csv_data_file_path>\n", cmd, "conv");
    fprintf(stderr, "      : %s %s [-d] <session_name> <result_file_path>\n", cmd, "conv");
    fprintf(stderr, "      : %s %s <hostname> <port_no>\n", cmd, CConst::C_SUB_CMD_NET);
}

int Driver::init_session(int argc, char *argv[])
{
    /*
     * argc = 6
     * argv  : 0  1     2              3          4                  5
     * usage : ui init <session_name> <protocol> <schema_file_path> <user_name ...>
     */
    // check arg
    if( argc < 6) {
        error_usage(argv[0]);
        return -1;
    }
    // check config
    std::map<std::string, std::string> cnf_info = get_config_info(CConst::CONFIG_FILE_NAME);
    if(cnf_info.size() == 0) {
        L_ERROR(_console, "coda config error.");
        return -1;
    }
    std::string session_name = argv[2];
    std::string plot_name = argv[3];
    std::string schema_file_path = argv[4];
    std::string analyst_name = cnf_info[CConst::CFG_KEY_UNAME];
    std::string user_name = cnf_info[CConst::CFG_KEY_UNAME];
    // store user names
    std::vector<std::string> user_names;
    for(int i=5; i < argc; i++){
        user_names.push_back(argv[i]);
    }
    // create filesystem
    FileSystemClient fs(session_name, analyst_name, user_name);
    if(fs.make_session_directory() != 0){
        L_ERROR(_console, "init session directory.");
        if(fs.remove_directory(fs.path_session_dir()) != 0) {
            L_ERROR(_console, "remove directory NG.");
        }
        return -1;
    }
    // copy schema file
    if(fs.copy_schema_file(schema_file_path) != 0) {
        L_ERROR(_console, "copy schema file.");
        if(fs.remove_directory(fs.path_session_dir()) != 0) {
            L_ERROR(_console, "remove directory NG.");
        }
        return -1;
    }
    L_INFO(_console, "copy schema file.");
    // create meta file
    if(fs.make_meta_file(plot_name, user_names) != 0) {
        L_ERROR(_console, "copy schema file.");
        if(fs.remove_directory(fs.path_session_dir()) != 0) {
            L_ERROR(_console, "remove directory NG.");
        }
        return -1;
    }
    // connect server and register session.
    NetworkClient nc(cnf_info[CConst::CFG_KEY_HOST], cnf_info[CConst::CFG_KEY_PORT], cnf_info[CConst::CFG_KEY_UNAME]);
    if(nc.service_init(session_name, plot_name, user_names) == 0){
        L_INFO(_console, "init session OK.");
        return 0;
    } else {
        L_ERROR(_console, "init session NG.");
        if(fs.remove_directory(fs.path_session_dir()) != 0) {
            L_ERROR(_console, "remove directory NG.");
        }
        return -1;
    }
}

int Driver::send_key(int argc, char *argv[])
{
    /*
     * argc = 3
     * argv  : 0  1         2
     * usage : ui send_key <session_name>
     */
    if( argc < 3) {
        error_usage(argv[0]);
        return -1;
    }
    // check config
    std::map<std::string, std::string> cnf_info = get_config_info(CConst::CONFIG_FILE_NAME);
    if(cnf_info.size() == 0) {
        L_ERROR(_console, "coda config error.");
        return -1;
    }
    std::string session_name = argv[2];
    // send key
    NetworkClient nc(cnf_info[CConst::CFG_KEY_HOST], cnf_info[CConst::CFG_KEY_PORT], cnf_info[CConst::CFG_KEY_UNAME]);
    if(nc.send_pk(session_name) == 0){
        L_INFO(_console, "send public key successfully.");
        return 0;
    } else {
        L_ERROR(_console, "send public key failure.");
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
    FileSystemClient d(argv[4]);
    if(d.make_session_directory() != 0){
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

int Driver::convert(int argc, char *argv[])
{
    /*
     * argc = 5
     * argv  : 0  1    2   3              4
     * usage : ui conv -e <session_name> <data_xxx.csv>
     */
    /*
     * argc = 5
     * argv  : 0  1    2   3              4
     * usage : ui conv -d <session_name> <result_data_file_path>
     */
    if( argc < 3) {
        error_usage(argv[0]);
        return -1;
    }
    if(std::string(argv[2]) == "-e") {
        if( argc < 5) {
            error_usage(argv[0]);
            return -1;
        }
        FileSystemClient fc(argv[3]);
        Schema schema = Schema(fc.path_schema_file());
        schema.convert_csv(argv[4], fc.path_plain_uploading_files());
    } else if(std::string(argv[2]) == "-d") {
        if( argc < 5) {
            error_usage(argv[0]);
            return -1;
        }
        FileSystemClient fc(argv[3]);
        Schema schema = Schema(fc.path_schema_file());
        schema.deconvert(argv[4], "result.csv");
    } else {
        error_usage(argv[0]);
        return -1;
    }
    return 0;
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

int Driver::d_rm_session_dir(char *dir_name)
{
    FileSystemClient d;
    if(d.remove_directory(dir_name) == 0){
        return 0;
    } else {
        return -1;
    }
}

std::map<std::string, std::string> Driver::get_config_info(std::string file_path)
{
    std::map<std::string, std::string> cnf_info;
    // file check
    struct stat stat_buf;
    if(stat(file_path.c_str(), &stat_buf) != 0) {
        L_ERROR(_console, "config file {} doesn't exist.", file_path);
        return cnf_info;
    }
    // create map
    std::vector<std::string> v,keys;
    keys.push_back(CConst::CFG_KEY_CORE_BIN);
    keys.push_back(CConst::CFG_KEY_HOST);
    keys.push_back(CConst::CFG_KEY_PORT);
    keys.push_back(CConst::CFG_KEY_UNAME);
    for(auto itr = keys.begin(); itr != keys.end(); ++itr) {
        Utils::get_items(file_path, *itr, &v);
        cnf_info[*itr] = v[0];
        v.clear();
        v.shrink_to_fit();
    }
    L_DEBUG(_console, "read config file. {}", file_path);
    return cnf_info;
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
    } else if (std::string(argv[1]) == "conv") {
        convert(argc, argv);
    } else if (std::string(argv[1]) == CConst::C_MAIN_CMD_RECEIVE_RESULT) {
        receive_result(argc, argv);
    } else if (std::string(argv[1]) == CConst::C_SUB_CMD_NET) {
        net_start(argc, argv);
    } else if (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v") {
        std::cout << CConst::VER_CLIENT << std::endl;
    } else if (std::string(argv[1]) == "test") {
        UTC::test(argc, argv);
    } else {
        error_usage(argv[0]);
        return -1;
    }
    return 0;
}


