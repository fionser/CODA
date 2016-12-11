#ifndef CORE_FILE_UTILITY_H
#define CORE_FILE_UTILITY_H
#include <map>
#include <string>
#include <vector>
#include <tuple>
namespace util {
typedef std::vector<std::string> strVec_t;
typedef std::map<std::string, strVec_t> Meta;

std::tuple<Meta, bool> readMetaFile(const std::string &metaFilePath);

std::string trim(const std::string &file);

std::vector<std::string> splitBySpace(const std::string &line);

std::string getDirPath(const std::string &path);

std::string concatenate(const std::string &path, const std::string &file);

FILE *createDoneFile(const std::string &dirpath);

enum class flag_t {
    FILE_ONLY,
    DIR_ONLY
};

std::vector<std::string> listDir(const std::string &dir, flag_t f);

} // namespace util
#endif // CORE_FILE_UTILITY_H
