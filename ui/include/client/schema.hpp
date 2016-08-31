#ifndef CODA_SCHEMA
#define CODA_SCHEMA

#include <fstream>
#include <iostream>
#include <string>
#include <iterator>
#include <vector>
#include <sstream>
#include <algorithm>
#include <regex>
#include <initializer_list>
#include <map>
#include "utils.h"


class Schema {
    private:
        int active_flg_;
        std::vector<std::string> item_name_;
        std::vector<std::string> item_type_;
        std::vector<int> item_size_;
        std::vector<std::string> item_size_str_;
        std::vector< std::map<std::string, std::string> > rule_;
    public:
        Schema();
        Schema(std::string file_path);
        int check();
        int get_schema(std::string file_path);
        std::string convert(std::string key_string, int rule_no);
        int convert_csv(std::string in_file_path, std::string out_file_path);
        void debug_display();
};

#endif

