#ifndef CODA_UI_CONST
#define CODA_UI_CONST
#include <cstring>
#include <string.h>
#include <vector>
#include <sstream>
#include <spdlog/spdlog.h>

namespace CConst {
    // Server cmd arg
    extern const char* S_MAIN_CMD;
    extern const char* S_SUB_CMD_INIT;
    extern const char* S_SUB_CMD_UADD;
    // Client cmd arg
    extern const char* C_MAIN_CMD_INIT;
    extern const char* C_MAIN_CMD_SEND_KEY;
    extern const char* C_MAIN_CMD_JOIN;
    extern const char* C_MAIN_CMD_SEND_DATA;
    extern const char* C_MAIN_CMD_RECEIVE_RESULT;
    extern const char* C_SUB_CMD_NET;
    extern const char* C_SUB_CMD_DEBUG;
    extern const int D_SIZE;
    extern const char* D_NAMES[];
    extern const char* META_FILE_NAME;
    extern const char* SCHEMA_FILE_NAME;
    extern const char* FLAG_FILE_NAME;
    extern const std::string RESULT_FILE_NAME;
    extern const std::string PUBLIC_KEY;
    extern const std::string PUBLIC_KEY_CTEXT;
    extern const std::string SERVER_DIR_META;
    extern const std::string SERVER_DIR_RESULT;
    extern const std::string META_KEY_ANALYST;
    extern const std::string META_KEY_SESSION_ID;
    extern const std::string META_KEY_SESSION_NAME;
    extern const std::string META_KEY_PROTOCOL;
    extern const std::string META_KEY_USER_NAMES;
    // MSG
    extern const char* MSG_OK;
    extern const char* MSG_NG;
    extern const char* MSG_REQUEST_CMD;
    extern const char* MSG_ANALYST_NAME;
    extern const char* MSG_SESSION_NAME;
    extern const char* MSG_PROTOCOL;
    extern const char* MSG_USER_NUM;
    extern const char* MSG_USER_NAME;
    extern const char* MSG_FILE_TRANS;
    extern const char* MSG_CLIENT_SERVER;
    extern const char* MSG_SERVER_CLIENT;
    // CMD
    extern const char* CMD_INIT;
    extern const char* CMD_SEND_PK;
    extern const char* CMD_JOIN;
    extern const char* CMD_SEND_DATA;
    extern const char* CMD_RECV_RESULT;
    extern const char* CMD_DEBUG;
    extern const char* CMD_REQ_NUMBER_FILES;
    extern const char* CMD_REQ_FILE_NAME;
    extern const char* CMD_REQ_FILE_SIZE;
    extern const char* CMD_SEND_FILES;
    extern const char* CMD_RECEIVE_FILES;
    // FILE KEYWORD
    extern const std::string PATH_DATA;
    extern const std::string PATH_META;
    extern const std::string PATH_RESULT;
    extern const std::string PATH_DEBUG;
    // KEYWORD
    extern const std::string KEYWORD_FSIZE;
    // etc
    extern const char SEP_CH_FILE;
    extern const std::string CH_CRLF;
    extern std::vector<std::string> split(std::string str, char sep);
};

extern std::shared_ptr<spdlog::logger> _console;

template<class T> std::string to_string(const T&v) {
	std::ostringstream out;
	out << v;
	return out.str();
}

#endif

