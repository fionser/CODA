#ifndef CODA_FILESYSTEM_CLIENT
#define CODA_FILESYSTEM_CLIENT
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include "../common/filesystem_base.hpp"
#include "../common/utils.h"

class FileSystemServer : public FileSystemBase {
private:
    int check_private_member();
public:
    int make_session_directory();
    int make_result_directory(std::string result_dir_name);
    int make_data_type_directory(std::string data_type_dir_name);
    int make_user_directory(const std::string user_name);
    int add_user(const std::string session_user, const std::string session_name, const std::string add_user_name);
    std::string get_filepath(std::string keyword_filename);

    // filepath
    std::string path_analyst_dir();
    std::string path_session_dir();
    std::string path_meta_dir();
    std::string path_meta_file();
    std::string path_pkey_file();
    std::string path_ckey_file();
    std::string path_schema_file();
    std::string path_data_dir();
    std::string path_data_type_dir(std::string data_type);
    std::string path_user_dir(std::string data_type, std::string user_name);
    std::string path_data_file(std::string data_type, std::string user_name, std::string file_name);
    std::string path_result_root_dir();
    std::string path_result_dir(std::string dir_name);
    std::string path_result_file(std::string dir_name, std::string file_name);

    // constructor
    FileSystemServer();
    FileSystemServer(std::string session_name);
    FileSystemServer(std::string session_name, std::string user_name);
    FileSystemServer(std::string session_name, std::string user_name, std::string analyst_name);
};

#endif

