#ifndef CODA_UTILS
#define CODA_UTILS

#include <iostream>
#include <fstream>
#include <vector>
#include <regex>

namespace Utils {
    std::string trim(const std::string& string, const char* trimCharacterList);
    std::vector<std::string> argv2strings(int argc, char *argv[]);
    std::vector<std::string> split(const std::string& input, const std::string& regex);
    std::string join(const std::vector<std::string>& input, const std::string& regex);
    int test(int argc, char* argv[]);
    int get_items(std::string file_path, std::string keyword, std::vector<std::string>* items);
}

#endif

