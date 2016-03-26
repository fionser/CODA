#ifndef CODA_FILESYSTEM_CLIENT
#define CODA_FILESYSTEM_CLIENT
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include "filesystem_base.hpp"

class FileSystemServer : public FileSystemBase {
    public:
        int make_user_directory(const char* user_name);
        int make_session_directory(const char* session_user, const char* session_name);
        int add_user(const char* session_user, const char* session_name, const char* add_user_name);
        std::string get_filepath(std::string keyword_filename);
        FileSystemServer();
        FileSystemServer(std::string analyst_name, std::string session_name, std::string user_name);
};

#endif

