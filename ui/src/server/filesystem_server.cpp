#include "filesystem_server.hpp"

int FileSystemServer::check_private_member()
{
    if((analyst_name_.size() > 0) && (session_name_.size() > 0) && (user_name_.size() > 0)) {
        return 0;
    } else {
        return -1;
    }
}

int FileSystemServer::make_session_directory()
{
    if(check_private_member() != 0) return -1;
    // if analyst directory dosesn't exist. make
    struct stat stat_buf;
    if(stat(path_analyst_dir().c_str(), &stat_buf) != 0) {
        if(make_directory(path_analyst_dir()) != 0) {
            L_ERROR(_console, "make analyst directory.");
            return -1;
        }
    }
    // ToDo check seaquence
    /*
    if(stat(path_session_dir().c_str(), &stat_buf) != 0){
        L_ERROR(_console, "{} session directory exist.", path_session_dir());
        return -1;
    }
     */
    if(make_directory(path_session_dir()) != 0) {
        L_ERROR(_console, "make session directory.");
        return -1;
    }
    if(make_directory(path_meta_dir()) != 0) {
        L_ERROR(_console, "make meta directory.");
        return -1;
    }
    if(make_directory(path_data_dir()) != 0) {
        L_ERROR(_console, "make data directory.");
        return -1;
    }
    if(make_directory(path_result_root_dir()) != 0) {
        L_ERROR(_console, "make result root directory.");
        return -1;
    }
    L_INFO(_console, "make initial session directory.");
    return 0;
}

int FileSystemServer::make_result_directory(std::string result_dir_name)
{
    if(check_private_member() != 0) return -1;
    struct stat stat_buf;
    if(stat(path_result_dir(result_dir_name).c_str(), &stat_buf) == 0){
        if(S_ISDIR(stat_buf.st_mode)){
            return 0;
        } else {
            L_ERROR(_console, "make result directory.");
            return -1;
        }
    } else if ( make_directory(path_result_dir(result_dir_name)) !=0) {
        L_ERROR(_console, "make result directory.");
        return -1;
    }
    return 0;
}

int FileSystemServer::make_data_type_directory(std::string data_type_dir_name)
{
    std::vector<std::string> users;
    if(Utils::get_items(path_meta_file(), std::string(CConst::META_KEY_USER_NAMES), &users) != 0) {
        return -1;
    }
    if(make_directory(path_data_type_dir(data_type_dir_name)) != 0) {
        return -1;
    }
    for(auto itr=users.begin(); itr != users.end(); ++itr) {
        if(make_directory(path_user_dir(data_type_dir_name,*itr)) != 0) {
            return -1;
        }
    }
    return 0;
}

int FileSystemServer::make_user_directory(const std::string user_name)
{
    struct stat stat_buf;
    if(stat(user_name.c_str(), &stat_buf) == 0){
        if(S_ISDIR(stat_buf.st_mode)){
            return 0;
        } else {
            L_ERROR(_console, "make analyst directory.");
            return -1;
        }
    } else if ( make_directory(user_name) !=0) {
        L_ERROR(_console, "make analyst directory.");
        return -1;
    }
    return 0;
}



int FileSystemServer::add_user(const std::string session_user, const std::string session_name, const std::string add_user_name)
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
    L_DEBUG(_console, "keyword file name = {}.", keyword_filename);
    std::vector<std::string> vs = CConst::split(keyword_filename, CConst::SEP_CH_FILE);
    std::string file_path = "";
    if(vs.size() != 2) {
        L_DEBUG(_console, "file path size = [{0}].", vs.size());
        L_ERROR(_console, "file keyword illegal.");
    } else if(vs[0] == CConst::KPATH_DATA) {
        if(analyst_name_.size() == 0){
            L_ERROR(_console, "analyst name null.");
        } else if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else if(user_name_.size() == 0) {
            L_ERROR(_console, "user name null.");
        } else {
            file_path = path_data_file(vs[1], vs[2], vs[3]);
        }
    } else if(vs[0] == CConst::KPATH_META) {
        if(analyst_name_.size() == 0){
            L_ERROR(_console, "analyst name null.");
        } else if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else {
            file_path = path_meta_dir() + CConst::SEP_CH_FILE + vs[1];
        }
    } else if(vs[0] == CConst::KPATH_RESULT) {
        if(analyst_name_.size() == 0){
            L_ERROR(_console, "analyst name null.");
        } else if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else {
            file_path = path_result_file("cate", vs[1]);
        }
    } else if(vs[0] == CConst::KPATH_DEBUG) {
        file_path = vs[1];
    } else {
        L_ERROR(_console, "file keyword illegal.");
    }
    return file_path;
}


// file directory paths
std::string FileSystemServer::path_analyst_dir() { return analyst_name_; }
std::string FileSystemServer::path_session_dir() { return path_analyst_dir() + CConst::SEP_CH_FILE + session_name_; }
std::string FileSystemServer::path_meta_dir() { return path_session_dir() + CConst::SEP_CH_FILE + CConst::META_DIR_NAME; }
std::string FileSystemServer::path_meta_file() { return path_meta_dir() + CConst::SEP_CH_FILE + CConst::META_FILE_NAME; }
std::string FileSystemServer::path_pkey_file() { return path_meta_dir() + CConst::SEP_CH_FILE + CConst::PUBLIC_KEY_FILE_NAME; }
std::string FileSystemServer::path_ckey_file() { return path_meta_dir() + CConst::SEP_CH_FILE + CConst::CONTEXT_KEY_FILE_NAME; }
std::string FileSystemServer::path_schema_file() { return path_meta_dir() + CConst::SEP_CH_FILE + CConst::SCHEMA_FILE_NAME; }
std::string FileSystemServer::path_data_dir() { return path_session_dir() + CConst::SEP_CH_FILE + CConst::DATA_DIR_NAME; }
std::string FileSystemServer::path_data_type_dir(std::string data_type) { return path_data_dir() + CConst::SEP_CH_FILE + data_type; }
std::string FileSystemServer::path_user_dir(std::string data_type, std::string user_name) { return path_data_type_dir(data_type) + CConst::SEP_CH_FILE + user_name; }
std::string FileSystemServer::path_data_file(std::string data_type, std::string user_name, std::string file_name) { return path_user_dir(data_type, user_name) + CConst::SEP_CH_FILE + file_name; }
std::string FileSystemServer::path_result_root_dir() { return path_session_dir() + CConst::SEP_CH_FILE + CConst::RESULT_DIR_NAME; }
std::string FileSystemServer::path_result_dir(std::string dir_name) { return path_result_root_dir() + CConst::SEP_CH_FILE + dir_name; }
std::string FileSystemServer::path_result_file(std::string dir_name, std::string file_name) { return path_result_dir(dir_name) + CConst::SEP_CH_FILE + file_name;}


FileSystemServer::FileSystemServer()
{
    session_name_ = "";
    user_name_ = "";
    analyst_name_ = "";
}

FileSystemServer::FileSystemServer(std::string session_name) : FileSystemServer::FileSystemServer()
{
    session_name_ = session_name;
}

FileSystemServer::FileSystemServer(std::string session_name, std::string user_name) : FileSystemServer::FileSystemServer()
{
    session_name_ = session_name;
    user_name_ = user_name;
}

FileSystemServer::FileSystemServer(std::string session_name, std::string user_name, std::string analyst_name) : FileSystemServer::FileSystemServer()
{
    session_name_ = session_name;
    user_name_ = user_name;
    analyst_name_ = analyst_name;
}


