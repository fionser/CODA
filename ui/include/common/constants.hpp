#ifndef CODA_UI_CONST
#define CODA_UI_CONST
#include <cstring>
#include <string.h>
#include <vector>
#include <sstream>
#include <spdlog/spdlog.h>

namespace CConst {
    extern const std::string VER_SERVER;
    extern const std::string VER_CLIENT;
    /////////////////////////////////////////////////
    // Server cmd arg
    /////////////////////////////////////////////////
    extern const char* S_MAIN_CMD;
    /////////////////////////////////////////////////
    // Client cmd arg
    /////////////////////////////////////////////////
    extern const char* C_MAIN_CMD_INIT;
    extern const char* C_MAIN_CMD_SEND_KEY;
    extern const char* C_MAIN_CMD_JOIN;
    extern const char* C_MAIN_CMD_SEND_DATA;
    extern const char* C_MAIN_CMD_RECEIVE_RESULT;
    extern const char* C_SUB_CMD_NET;
    /////////////////////////////////////////////////
    // filesystem
    /////////////////////////////////////////////////
    extern const char SEP_CH_FILE;
    extern const std::string CH_CRLF;
    extern const std::string CODA_CONFIG_FILE_NAME;
    extern const std::string META_DIR_NAME;
    extern const std::string META_FILE_NAME;
    extern const std::string SECRET_KEY_FILE_NAME;
    extern const std::string PUBLIC_KEY_FILE_NAME;
    extern const std::string CONTEXT_KEY_FILE_NAME;
    extern const std::string SCHEMA_FILE_NAME;
    extern const std::string DATA_DIR_NAME;
    extern const std::string PLAIN_DIR_NAME;
    extern const std::string UPLOADING_DIR_NAME;
    extern const std::string CATEGORICAL;
    extern const std::string ORDINAL;
    extern const std::string NUMERICAL;
    extern const std::string RESULT_DIR_NAME;
    extern const std::string ENC_DIR_NAME;
    extern const std::string FLAG_FILE_NAME;
    extern const std::string RESULT_FILE_NAME;
    extern const std::string CONFIG_FILE_NAME;
    /////////////////////////////////////////////////
    // META keywords
    /////////////////////////////////////////////////
    extern const std::string CFG_KEY_CORE_BIN;
    extern const std::string CFG_KEY_HOST;
    extern const std::string CFG_KEY_PORT;
    extern const std::string CFG_KEY_UNAME;
    extern const std::string CFG_KEY_DIRECTORY;
    extern const std::string META_KEY_ANALYST;
    extern const std::string META_KEY_SESSION_NAME;
    extern const std::string META_KEY_PROTOCOL;
    extern const std::string META_KEY_USER_NAMES;
    /////////////////////////////////////////////////
    // Network MSGs
    /////////////////////////////////////////////////
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
    /////////////////////////////////////////////////
    // local CMDs
    /////////////////////////////////////////////////
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
    /////////////////////////////////////////////////
    // FILE KEYWORD
    /////////////////////////////////////////////////
    extern const std::string KPATH_DATA;
    extern const std::string KPATH_META;
    extern const std::string KPATH_RESULT;
    extern const std::string KPATH_DEBUG;
    /////////////////////////////////////////////////
    // KEYWORD
    /////////////////////////////////////////////////
    extern const std::string KEYWORD_FSIZE;
    /////////////////////////////////////////////////
    // for windows
    /////////////////////////////////////////////////
    extern std::vector<std::string> split(std::string str, char sep);
};

extern std::shared_ptr<spdlog::logger> _console;

/////////////////////////////////////////////////
// for windows
/////////////////////////////////////////////////
template<class T> std::string to_string(const T&v) {
	std::ostringstream out;
	out << v;
	return out.str();
}

#endif

