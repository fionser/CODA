#include "constants.hpp"

namespace CConst {
    // Server cmd arg
    const char* S_MAIN_CMD = "start";
    const char* S_SUB_CMD_INIT = "d_init";
    const char* S_SUB_CMD_UADD = "d_uadd";
    // Client cmd arg
    const char* C_MAIN_CMD_INIT = "init"; // analyst
    const char* C_MAIN_CMD_SEND_KEY = "send_key"; // analyst
    const char* C_MAIN_CMD_JOIN = "join"; // uploader
    const char* C_MAIN_CMD_SEND_DATA = "send_data"; // uploader
    const char* C_MAIN_CMD_RECEIVE_RESULT = "receive_result"; // analyst
    const char* C_SUB_CMD_NET = "d_net"; // any
    const char* C_SUB_CMD_DEBUG = "d_debug";
    const int D_SIZE = 8;
    const char *D_NAMES[] = {
        "Meta", // 0
        "Data", // 1
        "Data/Plaine", // 2
        "Data/Plaine/private", // 3
        "Data/Plaine/result", // 4
        "Data/Enc", // 5
        "Data/Enc/Send", // 6
        "Data/Enc/Reciev" // 7
    };
    const char* META_FILE_NAME = "meta.ini";
    const char* FLAG_FILE_NAME = ".done.lock";
    const std::string RESULT_FILE_NAME = "File_result";
    const std::string PUBLIC_KEY = "fhe_key.pk";
    const std::string PUBLIC_KEY_CTEXT = "fhe_key.ctxt";
    const std::string SERVER_DIR_META = "Meta";
    const std::string SERVER_DIR_RESULT = "result";
    const std::string META_KEY_ANALYST = "analyst_name ";
    const std::string META_KEY_SESSION_ID = "session_id ";
    const std::string META_KEY_SESSION_NAME = "session_name ";
    const std::string META_KEY_PROTOCOL = "protocol ";
    const std::string META_KEY_USER_NAMES = "user_names ";
    // MSG
    const char* MSG_OK = "OK";
    const char* MSG_NG = "NG";
    const char* MSG_REQUEST_CMD = "request_cmd : ";
    const char* MSG_ANALYST_NAME = "analyst_name : ";
    const char* MSG_SESSION_NAME = "session_name : ";
    const char* MSG_PROTOCOL = "protocol : ";
    const char* MSG_USER_NUM = "num_of_users : ";
    const char* MSG_USER_NAME = "user_name : ";
    const char* MSG_FILE_TRANS = "file_trans";
    const char* MSG_CLIENT_SERVER = "client_server";
    const char* MSG_SERVER_CLIENT = "server_client";
    // CMD
    const char* CMD_INIT = "init";
    const char* CMD_SEND_PK = "send_pk";
    const char* CMD_JOIN = "join";
    const char* CMD_SEND_DATA = "send_data";
    const char* CMD_RECV_RESULT = "receive_result";
    const char* CMD_DEBUG = "debug";
    const char* CMD_REQ_NUMBER_FILES = "number of files : ";
    const char* CMD_REQ_FILE_NAME = "file name : ";
    const char* CMD_REQ_FILE_SIZE = "file size : ";
    const char* CMD_SEND_FILES = "send_files";
    const char* CMD_RECEIVE_FILES= "receive_files";
    // FILE KEYWORD
    const std::string PATH_DATA = "FDATA";
    const std::string PATH_META = "FMETA";
    const std::string PATH_RESULT = "FREST";
    const std::string PATH_DEBUG = "FDEBUG";
    // KEYWORD
    const std::string KEYWORD_FSIZE = "FSIZE:";
    // FILE KEYWORD
    const char SEP_CH_FILE = '/';
    const std::string CH_CRLF = "\n";
    std::vector<std::string> split(const std::string str, char sep)
    {
        std::vector<std::string> v;
        std::stringstream ss(str);
        std::string buffer;
        while( std::getline(ss, buffer, sep) ) {
            v.push_back(buffer);
        }
        return v;
    }
}

std::shared_ptr<spdlog::logger> _console = spdlog::stdout_logger_mt("console");
// std::shared_ptr<spdlog::logger> _console = spdlog::rotating_logger_mt("file_logger", "coda.log", 1048576 * 5, 5);


