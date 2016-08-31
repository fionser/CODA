#include "utils.h"

namespace Utils {
    ////////////////////////////////////////////////////////////////////////////////////////
    // trim : delete space etc...
    ////////////////////////////////////////////////////////////////////////////////////////
    std::string trim(const std::string& string, const char* trimCharacterList = " \t\v\r\n")
    {
        std::string result;
        std::string::size_type left = string.find_first_not_of(trimCharacterList);
        if (left != std::string::npos) {
            std::string::size_type right = string.find_last_not_of(trimCharacterList);
            result = string.substr(left, right - left + 1);
        }
        return result;
    }
    ////////////////////////////////////////////////////////////////////////////////////////
    // argv2strings : argv to string vector
    ////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::string> argv2strings(int argc, char *argv[]) {
        std::vector<std::string> arg;
        for(int i=0; i<argc; i++) {
            arg.push_back(argv[i]);
        }
        return arg;
    }
    ////////////////////////////////////////////////////////////////////////////////////////
    // split : string split
    ////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::string> split(const std::string& input, const std::string& regex) {
        std::regex re(regex);
        std::sregex_token_iterator 
            first{input.begin(), input.end(), re, -1}, 
            last;
        return {first, last};
    }
    ////////////////////////////////////////////////////////////////////////////////////////
    // join : string join
    ////////////////////////////////////////////////////////////////////////////////////////
    std::string join(const std::vector<std::string>& input, const std::string& regex)
    {
        std::string str_joined = "";
        for (std::vector<std::string>::const_iterator ii = input.begin(); ii != input.end(); ++ii)
        {
            str_joined += (*ii);
            if ( ii + 1 != input.end() ) {
                str_joined += regex;
            }
        }
        return str_joined;
    }
    ////////////////////////////////////////////////////////////////////////////////////////
    // test : for debug
    ////////////////////////////////////////////////////////////////////////////////////////
    int test(int argc, char* argv[]) {
        return 0;
    }
}


