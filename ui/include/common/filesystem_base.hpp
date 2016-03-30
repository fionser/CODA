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
#include "constants.hpp"

class FileSystemBase {
    private:
        int dir_clear(const char *filepath);
        int recursive_dir_clear(const char *filepath);
        int delete_regular_files(const char *filepath);
    protected:
        std::string analyst_name_;
        std::string session_name_;
        std::string user_name_;
    public:
        int remove_directory(const char* dir_name);
        std::vector<std::string> get_file_list(const char *dir_path);
        FileSystemBase();
        FileSystemBase(std::string analyst_name, std::string session_name, std::string user_name);
};

#endif


