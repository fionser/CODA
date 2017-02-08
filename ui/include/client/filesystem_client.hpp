#ifndef CODA_FILESYSTEM_CLIENT
#define CODA_FILESYSTEM_CLIENT
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
#include "schema.hpp"
#include "../common/filesystem_base.hpp"

class FileSystemClient : public FileSystemBase {
public:
    int make_session_directory();
    int copy_schema_file(std::string schema_file_path);
    int make_meta_file(std::string protcol, std::vector<std::string> user_names);
    std::string get_filepath(std::string keyword_filename);

    // filepath
    std::string path_session_dir();
    std::string path_meta_dir();
    std::string path_meta_file();
    std::string path_skey_file();
    std::string path_pkey_file();
    std::string path_ckey_file();
    std::string path_schema_file();
    std::string path_data_dir();
    std::string path_plain_dir();
    std::string path_plain_uploading_dir();
    Schema_Output_Filepath path_plain_uploading_files();
    std::string path_plain_result_dir();
    std::string path_plain_result_file(std::string file_name);
    std::string path_enc_dir();
    std::string path_enc_uploading_root_dir();
    std::string path_enc_uploading_dir(std::string dir_name);
    std::string path_enc_uploading_file(std::string dir_name, std::string file_name);
    std::string path_enc_result_root_dir();
    std::string path_enc_result_dir(std::string dir_name);
    std::string path_enc_result_file(std::string dir_name, std::string file_name);

    // constructor
    FileSystemClient();
    FileSystemClient(std::string session_name);
    FileSystemClient(std::string session_name, std::string user_name);
    FileSystemClient(std::string session_name, std::string user_name, std::string analyst_name);
};

#endif


