#ifndef CODA_FILESYSTEM_BASE
#define CODA_FILESYSTEM_BASE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include "constants.hpp"

class FileSystemBase {
private:
    int dir_clear(const std::string filepath);
    int recursive_dir_clear(const std::string filepath);
    int delete_regular_files(const std::string filepath);
protected:
    std::string analyst_name_;
    std::string session_name_;
    std::string user_name_;
public:
    int make_directory(const std::string dir_path);
    int remove_directory(const std::string dir_path);
    std::vector<std::string> get_file_list(const std::string dir_path);
    int copy_file(const std::string src_file_path, const std::string dst_file_path);
    // filepath
    std::string path_coda_config();
    // keyword filepath
    std::string kpath_meta_dir();
    std::string kpath_meta_file();
    std::string kpath_pkey_file();
    std::string kpath_ckey_file();
    std::string kpath_schema_file();
    std::string kpath_data_root_dir();
    std::string kpath_data_type_dir(std::string data_type);
    std::string kpath_user_dir(std::string data_type, std::string user_name);
    std::string kpath_data_file(std::string data_type, std::string user_name, std::string file_name);
    std::string kpath_result_root_dir();
    std::string kpath_result_dir(std::string dir_name);
    std::string kpath_result_file(std::string dir_name, std::string file_name);
    // constructor
    FileSystemBase();
};

#endif


