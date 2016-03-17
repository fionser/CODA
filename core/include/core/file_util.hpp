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

std::vector<std::string> splitBySpace(const std::string &line);

std::string getDirPath(const std::string &path);

std::string concatenate(const std::string &path, const std::string &file);

bool createDoneFile(const std::string &path);

std::vector<std::string> listDir(const std::string &dir);
} // namespace util
#endif // CORE_FILE_UTILITY_H
