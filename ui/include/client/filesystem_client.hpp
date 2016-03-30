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
#include "common/filesystem_base.hpp"

class FileSystemClient : public FileSystemBase {
    public:
        int make_analyst_info(std::vector<std::string> argv);
        int make_directory(const char* dir_name);
        int make_meta_file(std::vector<std::string> argv);
        std::string get_filepath(std::string keyword_filename);
        FileSystemClient();
        FileSystemClient(std::string analyst_name, std::string session_name, std::string user_name);
};

#endif


