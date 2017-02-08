#include "filesystem_client.hpp"


int FileSystemClient::make_session_directory()
{
    struct stat stat_buf;
    if(session_name_ == "") {
        L_ERROR(_console, "session_name invalid.");
        return -1;
    } else if(stat(session_name_.c_str(), &stat_buf) == 0) {
        L_ERROR(_console, "session_name directory or file {} exist.", session_name_);
        return -1;
    } else if (make_directory(path_session_dir()) != 0) {
        L_ERROR(_console, "make {} directory.", session_name_);
        return -1;
    }
    std::vector<std::string> d;
    d.push_back(path_meta_dir());
    d.push_back(path_data_dir());
    d.push_back(path_plain_dir());
    d.push_back(path_plain_uploading_dir());
    d.push_back(path_plain_result_dir());
    d.push_back(path_enc_dir());
    d.push_back(path_enc_uploading_root_dir());
    d.push_back(path_enc_result_root_dir());
    for(auto itr = d.begin(); itr != d.end(); ++itr) {
        if ( make_directory(*itr) != 0) {
            L_ERROR(_console, "make {} directory.", *itr);
            return -1;
        }
    }
    L_INFO(_console, "make session directory.");
    return 0;
}

int FileSystemClient::copy_schema_file(std::string schema_file_path)
{
    return copy_file(schema_file_path, path_schema_file());
}

int FileSystemClient::make_meta_file(std::string protcol, std::vector<std::string> user_names)
{
    std::ofstream meta_file(path_meta_file().c_str());
    meta_file << (CConst::META_KEY_ANALYST + analyst_name_ + CConst::CH_CRLF);
    meta_file << (CConst::META_KEY_SESSION_NAME + session_name_ + CConst::CH_CRLF);
    meta_file << (CConst::META_KEY_PROTOCOL + protcol + CConst::CH_CRLF);
    std::string user_names_line;
    for(auto itr = user_names.begin(); itr != user_names.end(); ++itr) {
        user_names_line += *itr + std::string(" ");
    }
    meta_file << (CConst::META_KEY_USER_NAMES + user_names_line + CConst::CH_CRLF);
    meta_file.close();
    L_INFO(_console, "make meta file.");
    return 0;
}

std::string FileSystemClient::get_filepath(std::string keyword_filename)
{
    L_DEBUG(_console, "keyword file name = {}.", keyword_filename);
    std::vector<std::string> vs = CConst::split(keyword_filename, CConst::SEP_CH_FILE);
    std::string file_path = "";
    if(vs.size() != 2) {
        L_ERROR(_console, "file keyword illegal.");
    } else if(vs[0] == CConst::KPATH_DATA) {
        if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else {
            file_path = path_meta_dir() + CConst::SEP_CH_FILE + vs[1];
        }
    } else if(vs[0] == CConst::KPATH_META) {
        if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else {
            file_path = path_meta_dir() + CConst::SEP_CH_FILE + vs[1];
        }
    } else if(vs[0] == CConst::KPATH_RESULT) {
        if(session_name_.size() == 0) {
            L_ERROR(_console, "session name null.");
        } else {
            file_path = path_enc_result_root_dir() + CConst::SEP_CH_FILE + vs[1]; // ToDo std::stringnge result dir
        }
    } else if(vs[0] == CConst::KPATH_DEBUG) {
        file_path = vs[1];
    } else {
        L_ERROR(_console, "file keyword illegal.");
    }
    return file_path;
}



// file directory paths
std::string FileSystemClient::path_session_dir() { return session_name_; }
std::string FileSystemClient::path_meta_dir() { return path_session_dir() + CConst::SEP_CH_FILE + CConst::META_DIR_NAME; }
std::string FileSystemClient::path_meta_file() { return path_meta_dir() + CConst::SEP_CH_FILE + CConst::META_FILE_NAME; }
std::string FileSystemClient::path_skey_file() { return path_meta_dir() + CConst::SEP_CH_FILE + CConst::SECRET_KEY_FILE_NAME; }
std::string FileSystemClient::path_pkey_file() { return path_meta_dir() + CConst::SEP_CH_FILE + CConst::PUBLIC_KEY_FILE_NAME; }
std::string FileSystemClient::path_ckey_file() { return path_meta_dir() + CConst::SEP_CH_FILE + CConst::CONTEXT_KEY_FILE_NAME; }
std::string FileSystemClient::path_schema_file() { return path_meta_dir() + CConst::SEP_CH_FILE + CConst::SCHEMA_FILE_NAME; }
std::string FileSystemClient::path_data_dir() { return path_session_dir() + CConst::SEP_CH_FILE + CConst::DATA_DIR_NAME; }
std::string FileSystemClient::path_plain_dir() { return path_data_dir() + CConst::SEP_CH_FILE + CConst::PLAIN_DIR_NAME; }
std::string FileSystemClient::path_plain_uploading_dir() { return path_plain_dir() + CConst::SEP_CH_FILE + CConst::UPLOADING_DIR_NAME; }
Schema_Output_Filepath FileSystemClient::path_plain_uploading_files() {
    Schema_Output_Filepath sof = {
            path_plain_uploading_dir() + CConst::SEP_CH_FILE + CConst::CATEGORICAL,
            path_plain_uploading_dir() + CConst::SEP_CH_FILE + CConst::ORDINAL,
            path_plain_uploading_dir() + CConst::SEP_CH_FILE + CConst::NUMERICAL
    };
    return sof;
}
std::string FileSystemClient::path_plain_result_dir() { return path_plain_dir() + CConst::SEP_CH_FILE + CConst::RESULT_DIR_NAME; }
std::string FileSystemClient::path_plain_result_file(std::string file_name) { return path_plain_result_dir() + CConst::SEP_CH_FILE + file_name; }
std::string FileSystemClient::path_enc_dir() { return path_data_dir() + CConst::SEP_CH_FILE + CConst::ENC_DIR_NAME; }
std::string FileSystemClient::path_enc_uploading_root_dir() { return path_enc_dir() + CConst::SEP_CH_FILE + CConst::UPLOADING_DIR_NAME; }
std::string FileSystemClient::path_enc_uploading_dir(std::string dir_name) { return path_enc_uploading_root_dir() + CConst::SEP_CH_FILE + dir_name; }
std::string FileSystemClient::path_enc_uploading_file(std::string dir_name, std::string file_name) { return path_enc_uploading_root_dir() + CConst::SEP_CH_FILE + dir_name + CConst::SEP_CH_FILE + file_name;}
std::string FileSystemClient::path_enc_result_root_dir() { return path_enc_dir() + CConst::SEP_CH_FILE + CConst::RESULT_DIR_NAME; }
std::string FileSystemClient::path_enc_result_dir(std::string dir_name) { return path_enc_result_root_dir() + CConst::SEP_CH_FILE + dir_name; }
std::string FileSystemClient::path_enc_result_file(std::string dir_name, std::string file_name) { return path_enc_result_dir(dir_name) + CConst::SEP_CH_FILE + file_name;}


FileSystemClient::FileSystemClient()
{
    analyst_name_ = "";
    session_name_ = "";
    user_name_ = "";
}

FileSystemClient::FileSystemClient(std::string session_name) : FileSystemClient::FileSystemClient()
{
    session_name_ = session_name;
}

FileSystemClient::FileSystemClient(std::string session_name, std::string user_name) : FileSystemClient::FileSystemClient()
{
    session_name_ = session_name;
    user_name_ = user_name;
}

FileSystemClient::FileSystemClient(std::string session_name, std::string user_name, std::string analyst_name) : FileSystemClient::FileSystemClient()
{
    analyst_name_ = analyst_name;
    session_name_ = session_name;
    user_name_ = user_name;
}


