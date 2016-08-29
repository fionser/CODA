#include "filesystem_client.hpp"

int FileSystemClient::make_analyst_info(std::vector<std::string> argv)
{
    /*
     * argc >= 5
     * argv  : 0              1              2          3                   4...
     * usage : <session_name> <analyst_name> <protocol> <schema_file_path> <user_name ...>
     */
    if(argv.size() < 5){
        L_ERROR(_console, "analyst info incorrect.");
        return -1;
    } else if(int flg=make_directory(argv[0].c_str())!=0) {
        return flg;
    } else if(int flg=copy_file(argv[3].c_str(), std::string(argv[0] + CConst::SEP_CH_FILE + CConst::D_NAMES[0] + CConst::SEP_CH_FILE + CConst::SCHEMA_FILE_NAME).c_str())!=0) {
        return flg;
    } else if(int flg=make_meta_file(argv)!=0) {
        return flg;
    }
    return 0;
}

int FileSystemClient::make_directory(const char* dir_name)
{
    struct stat stat_buf;
    if(stat(dir_name, &stat_buf) == 0){
        L_ERROR(_console, "session_name directory or file exist.");
        return -1;
    }
    if (mkdir(dir_name, S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
        L_ERROR(_console, "make session directory.");
        return -1;
    }
    std::string dir_n = dir_name;
    for(int i=0; i<CConst::D_SIZE; i++){
        mkdir((dir_n + CConst::SEP_CH_FILE + CConst::D_NAMES[i]).c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    }
    L_INFO(_console, "make session directory.");
    return 0;
}

int FileSystemClient::make_meta_file(std::vector<std::string> argv)
{
    std::string meta_file_name = argv[0];
    meta_file_name = meta_file_name + CConst::SEP_CH_FILE + CConst::D_NAMES[0] + CConst::SEP_CH_FILE + CConst::META_FILE_NAME;
    std::ofstream meta_file(meta_file_name.c_str());
    meta_file << (CConst::META_KEY_ANALYST + argv[1] + CConst::CH_CRLF);
    meta_file << (CConst::META_KEY_SESSION_ID + CConst::CH_CRLF);
    meta_file << (CConst::META_KEY_SESSION_NAME + argv[0] + CConst::CH_CRLF);
    meta_file << (CConst::META_KEY_PROTOCOL + argv[2] + CConst::CH_CRLF);
    std::string user_names;
    for(int i=4; i<argv.size(); i++) {
        user_names += argv[i] + std::string(" ");
    }
    meta_file << (CConst::META_KEY_USER_NAMES + user_names + CConst::CH_CRLF);
    meta_file.close();
    return 0;
}

int FileSystemClient::store_schema_file(std::string file_path)
{
    
    return 0;
}

std::string FileSystemClient::get_filepath(std::string keyword_filename)
{
    std::vector<std::string> vs = CConst::split(keyword_filename, CConst::SEP_CH_FILE);
    std::string file_path = "";
    if(vs.size() != 2) {
        L_ERROR(_console, "file keyword illegal.");
    } else if(vs[0] == CConst::PATH_DATA) {
        if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else {
            file_path = session_name_ + CConst::SEP_CH_FILE + CConst::D_NAMES[6] + CConst::SEP_CH_FILE + vs[1];
        }
    } else if(vs[0] == CConst::PATH_META) {
        if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else {
            file_path = session_name_ + CConst::SEP_CH_FILE + CConst::D_NAMES[0] + CConst::SEP_CH_FILE + vs[1];
        }
    } else if(vs[0] == CConst::PATH_RESULT) {
        if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else {
            file_path = session_name_ + CConst::SEP_CH_FILE + CConst::D_NAMES[7] + CConst::SEP_CH_FILE + vs[1];
        }
    } else if(vs[0] == CConst::PATH_DEBUG) {
        file_path = vs[1];
    } else {
        L_ERROR(_console, "file keyword illegal.");
    }
    return file_path;
}


FileSystemClient::FileSystemClient()
{
    analyst_name_ = "";
    session_name_ = "";
    user_name_ = "";
}

FileSystemClient::FileSystemClient(std::string analyst_name, std::string session_name, std::string user_name)
{
    analyst_name_ = analyst_name;
    session_name_ = session_name;
    user_name_ = user_name;
}



