#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "core/file_util.hpp"
#include "core/literal.hpp"
#include "core/global.hpp"
namespace util {
std::vector<std::string> splitBySpace(const std::string &line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>()};
    return tokens;
}

std::tuple<Meta, bool> readMetaFile(const std::string &metaFilePath) {
    std::tuple<Meta, bool> ret;
    std::get<1>(ret) = false;
    std::fstream fin(metaFilePath);
    if (!fin.is_open())
        return ret;

    Meta meta;
    for (std::string line; std::getline(fin, line); ) {
        if (line.empty()) continue;
        auto tokens = splitBySpace(line);
        auto k = tokens.front();
        tokens.erase(tokens.begin());
        if (!tokens.empty())
            meta.insert({k, tokens});
    }

    std::get<0>(ret) = meta;
    std::get<1>(ret) = true;
    return ret;
}

std::string getDirPath(const std::string &path) {
    struct stat path_stat;
    stat(path.c_str(), &path_stat);
    if (S_ISREG(path_stat.st_mode)) {
        return path.substr(0, path.find_last_of(literal::separator));
    } else {
        return path;
    }
}

std::string concatenate(const std::string &path, const std::string &file) {
    return path + literal::separator + file;
}

FILE *createDoneFile(const std::string &path) {
    auto p = concatenate(path, global::_doneFileName);
    FILE *out = fopen(p.c_str(), "w");
    return out;
}

#ifdef OS_WIN
std::vector<std::string> listDir(const std::string &dir, flag_t flag) {
    std::vector<std::string> names;
    char search_path[200];
    sprintf(search_path, "%s/*.*", dir.c_str());
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(search_path, &fd);

    if(hFind != INVALID_HANDLE_VALUE) {
        auto mask = flag == flag_t::FILE_ONLY ?
                    FILE_ATTRIBUTE_NORMAL : FILE_ATTRIBUTE_DIRECTORY;
        do {
            if(fd.dwFileAttributes & mask)
                names.push_back(fd.cFileName);
        } while(FindNextFile(hFind, &fd));
        FindClose(hFind);
    }
    return names;
}
#else
#include <dirent.h>
std::vector<std::string> listDir(const std::string &path, flag_t flag) {
    DIR *dir;
    struct dirent *ent;
    std::vector<std::string> names;
    auto mask = flag == flag_t::FILE_ONLY ? DT_REG : DT_DIR;
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type & mask)
                names.push_back(std::string(ent->d_name));
        }
        closedir(dir);
    }
    return names;
}
#endif // OS_WIN
} // namespace util
