#include "constants.hpp"

namespace CConst {
    const std::string VER_CLIENT = "2.0.4";
    const std::string VER_SERVER = "2.0.4";
    /////////////////////////////////////////////////
    // Server cmd arg
    /////////////////////////////////////////////////
    const char* S_MAIN_CMD = "start";
    /////////////////////////////////////////////////
    // Client cmd arg
    /////////////////////////////////////////////////
    const char* C_MAIN_CMD_INIT = "init"; // analyst
    const char* C_MAIN_CMD_SEND_KEY = "send_key"; // analyst
    const char* C_MAIN_CMD_JOIN = "join"; // uploader
    const char* C_MAIN_CMD_SEND_DATA = "send_data"; // uploader
    const char* C_MAIN_CMD_RECEIVE_RESULT = "receive_result"; // analyst
    const char* C_SUB_CMD_NET = "d_net"; // any
    /////////////////////////////////////////////////
    // filesystem
    /////////////////////////////////////////////////
    const char SEP_CH_FILE = '/';
    const std::string CH_CRLF = "\n";
    const std::string CODA_CONFIG_FILE_NAME = "coda.cfg";
    const std::string META_DIR_NAME = "meta";
    const std::string META_FILE_NAME = "meta.ini";
    const std::string SECRET_KEY_FILE_NAME = "fhe_key.sk";
    const std::string PUBLIC_KEY_FILE_NAME = "fhe_key.pk";
    const std::string CONTEXT_KEY_FILE_NAME = "fhe_key.ctxt";
    const std::string SCHEMA_FILE_NAME = "schema.csv";
    const std::string DATA_DIR_NAME = "data";
    const std::string PLAIN_DIR_NAME = "plain";
    const std::string UPLOADING_DIR_NAME = "uploading";
    const std::string CATEGORICAL = "categorical";
    const std::string ORDINAL = "ordinal";
    const std::string NUMERICAL = "numerical";
    const std::string RESULT_DIR_NAME = "result";
    const std::string ENC_DIR_NAME = "enc";
    const std::string FLAG_FILE_NAME = ".done.lock";
    const std::string RESULT_FILE_NAME = "File_result";
    const std::string CONFIG_FILE_NAME = "coda.cfg";
    /////////////////////////////////////////////////
    // META keywords
    /////////////////////////////////////////////////
    const std::string CFG_KEY_CORE_BIN = "core_bin";
    const std::string CFG_KEY_HOST = "host";
    const std::string CFG_KEY_PORT = "port";
    const std::string CFG_KEY_UNAME = "user_name";
    const std::string CFG_KEY_DIRECTORY = "workspace";
    const std::string META_KEY_ANALYST = "analyst_name ";
    const std::string META_KEY_SESSION_NAME = "session_name ";
    const std::string META_KEY_PROTOCOL = "protocol ";
    const std::string META_KEY_USER_NAMES = "user_names ";
    /////////////////////////////////////////////////
    // Network MSGs
    /////////////////////////////////////////////////
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
    /////////////////////////////////////////////////
    // local CMDs
    /////////////////////////////////////////////////
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
    /////////////////////////////////////////////////
    // FILE KEYWORD
    /////////////////////////////////////////////////
    const std::string KPATH_DATA = "FDATA";
    const std::string KPATH_META = "FMETA";
    const std::string KPATH_RESULT = "FREST";
    const std::string KPATH_DEBUG = "FDEBUG";
    const std::string KEYWORD_FSIZE = "FSIZE:";
    /////////////////////////////////////////////////
    // for windows
    /////////////////////////////////////////////////
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


