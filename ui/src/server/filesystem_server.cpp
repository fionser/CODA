#include "filesystem_server.hpp"

int FileSystemServer::make_user_directory(const char* user_name)
{
    struct stat stat_buf;
    if(stat(user_name, &stat_buf) == 0){
        if(S_ISDIR(stat_buf.st_mode)){
            return 0;
        } else {
            L_ERROR(_console, "make analyst directory.");
            return -1;
        }
    } else if (mkdir(user_name, S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
        L_ERROR(_console, "make analyst directory.");
        return -1;
    }
    return 0;
}

int FileSystemServer::make_session_directory(const char* session_user, const char* session_name)
{
    std::string dir_name = std::string(session_user) + "/" + std::string(session_name);
    struct stat stat_buf;
    if(stat(dir_name.c_str(), &stat_buf) == 0){
        L_ERROR(_console, "session directory exist.");
        return -1;
    } else if (mkdir(dir_name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
        L_ERROR(_console, "make session directory.");
        return -1;
    }
    mkdir((dir_name + CConst::SEP_CH_FILE + CConst::SERVER_DIR_META).c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    mkdir((dir_name + CConst::SEP_CH_FILE + CConst::SERVER_DIR_RESULT).c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    return 0;
}

int FileSystemServer::add_user(const char* session_user, const char* session_name, const char* add_user_name)
{
    std::string s_dir_name = std::string(session_user) + CConst::SEP_CH_FILE + std::string(session_name);
    std::string u_dir_name = s_dir_name + CConst::SEP_CH_FILE + std::string(add_user_name);
    struct stat stat_buf;
    if(stat(s_dir_name.c_str(), &stat_buf) != 0){
        L_ERROR(_console, "analyst directory or session directory doesn't exist.");
        return -1;
    } else if (mkdir(u_dir_name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
        L_ERROR(_console, "make add user directory.");
        return -1;
    }
    return 0;
}

std::string FileSystemServer::get_filepath(std::string keyword_filename)
{
    std::vector<std::string> vs = CConst::split(keyword_filename, CConst::SEP_CH_FILE);
    std::string file_path = "";
    if(vs.size() != 2) {
        L_DEBUG(_console, "file path size = [{0}].", vs.size());
        L_ERROR(_console, "file keyword illegal.");
    } else if(vs[0] == CConst::PATH_DATA) {
        if(analyst_name_.size() == 0){
            L_ERROR(_console, "analyst name null.");
        } else if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else if(user_name_.size() == 0) {
            L_ERROR(_console, "user name null.");
        } else {
            file_path = analyst_name_ + CConst::SEP_CH_FILE + session_name_ + CConst::SEP_CH_FILE + user_name_ + CConst::SEP_CH_FILE + CConst::SEP_CH_FILE + vs[1];
        }
    } else if(vs[0] == CConst::PATH_META) {
        if(analyst_name_.size() == 0){
            L_ERROR(_console, "analyst name null.");
        } else if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else {
            file_path = analyst_name_ + CConst::SEP_CH_FILE + session_name_ + CConst::SEP_CH_FILE + "Meta" + CConst::SEP_CH_FILE + vs[1];
        }
    } else if(vs[0] == CConst::PATH_RESULT) {
        if(analyst_name_.size() == 0){
            L_ERROR(_console, "analyst name null.");
        } else if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else {
            file_path = analyst_name_ + CConst::SEP_CH_FILE + session_name_ + CConst::SEP_CH_FILE + "result" + CConst::SEP_CH_FILE + vs[1];
        }
    } else if(vs[0] == CConst::PATH_DEBUG) {
        file_path = vs[1];
    } else {
        L_ERROR(_console, "file keyword illegal.");
    }
    return file_path;
}

FileSystemServer::FileSystemServer()
{
    analyst_name_ = "";
    session_name_ = "";
    user_name_ = "";
}

FileSystemServer::FileSystemServer(std::string analyst_name, std::string session_name, std::string user_name)
{
    analyst_name_ = analyst_name;
    session_name_ = session_name;
    user_name_ = user_name;
}



