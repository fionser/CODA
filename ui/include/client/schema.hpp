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
#include <cmath>
#include "../common/utils.h"

struct Schema_Output_Filepath
{
    std::string categorical;
    std::string ordinal;
    std::string numerical;
};

class Schema {
    private:
        static int cTrue_;
        static int cError_;
        static std::string sep_coma_;
        static std::string sep_coron_;
        static std::string sep_sp_;
        static std::string meta_ch_;
        static std::string default_str_;
        static std::string categorical_;
        static std::string ordinal_;
        static std::string numerical_;
        int active_flg_;
        std::vector<std::string> item_name_;
        std::vector<std::string> item_type_;
        std::vector<int> item_size_;
        std::vector<std::string> item_size_str_;
        std::vector< std::map<std::string, std::string> > rule_;
        std::string search_key(int rule_num, std::string value);
        std::vector<std::string> get_label(int num);
    public:
        Schema();
        Schema(std::string file_path);
        int check() const;
        int get_schema(std::string file_path);
        std::string convert(std::string key_string, int rule_no) const;
        int convert_csv(const std::string in_file_path, const Schema_Output_Filepath opaths) const;
        int deconvert(std::string file_path, std::string output_file_path);
        void debug_display();
};

#endif

