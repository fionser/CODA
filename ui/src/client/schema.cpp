#include "schema.hpp"


Schema::Schema()
{
    active_flg_ = -1;
}

Schema::Schema(std::string file_path)
{
    active_flg_ = get_schema(file_path);
}

int Schema::check()
{
    return active_flg_;
}

int Schema::get_schema(std::string file_path)
{
    int code_normal = 0;
    int code_error = -1;
    std::string sep_coma = ",";
    std::string sep_coron = ":";
    std::ifstream ifs(file_path);
    if(ifs.fail()) return code_error;
    std::string str;
    // 1 : 項目
    std::getline(ifs, str);
    std::vector<std::string> item_name = Utils::split(str, sep_coma);
    int size_schema = item_name.size();
    // 2 : 型（categorical, numerical, ordinal)
    std::getline(ifs, str);
    std::vector<std::string> item_type = Utils::split(str, sep_coma);
    // 3 : サイズ
    std::getline(ifs, str);
    std::vector<std::string> item_size_str = Utils::split(str, sep_coma);
    std::vector<int> item_size;
    for(auto itr = item_size_str.begin(); itr != item_size_str.end(); ++itr) {
        item_size.push_back(std::stoi(*itr));
    }
    // 4~ : 変換ルール
    std::vector< std::map<std::string, std::string> > convert_rule(item_size.size());
    while(std::getline(ifs, str)) {
        std::vector<std::string> vec_string = Utils::split(str, sep_coma);
        for(auto itr = vec_string.begin(); itr != vec_string.end(); ++itr) {
            if(itr->size() != 0) {
                std::vector<std::string> key_value_set = Utils::split(*itr, sep_coron);
                convert_rule[itr - vec_string.begin()][key_value_set[0]] = key_value_set[1];
            }
        }
    }
    item_name_ = item_name;
    item_type_ = item_type;
    item_size_ = item_size;
    item_size_str_ = item_size_str;
    rule_ = convert_rule;
    return 0;
}

std::string Schema::convert(std::string key_string, int rule_no)
{
    return rule_[rule_no][key_string];
}

int Schema::convert_csv(std::string in_file_path, std::string out_file_path)
{
    std::string sep_coma = ",";
    std::string default_str = "0";
    std::ifstream ifs(in_file_path);
    if(ifs.fail()) return -1;
    std::ofstream ofs(out_file_path);
    if(ofs.fail()) return -1;
    std::string str;
    ofs << "#" << Utils::join(item_size_str_, " ") << std::endl;
    while(std::getline(ifs, str)) {
        std::vector<std::string> vec_string = Utils::split(str, sep_coma);
        std::vector<std::string> rline;
        for(auto itr = vec_string.begin(); itr != vec_string.end(); ++itr) {
            if(itr->size() != 0) {
                rline.push_back(convert(*itr, itr - vec_string.begin()));
            } else {
                rline.push_back("");
            }
        }
        ofs << Utils::join(rline, " ") << std::endl;
    }
    return 0;
}

std::string Schema::search_key(int rule_num, std::string value)
{
    std::string key = "";
    std::map<std::string, std::string> mp = rule_[rule_num];
    for(auto itr = mp.begin(); itr != mp.end(); ++itr) {
        if(itr->second == value) {
            key = itr->first;
            break;
        }
    }
    return key;
}

std::vector<std::string> Schema::get_label(int num)
{
    std::vector<std::string> labels;
    int size = item_size_[num];
    for (int i = 1; i < item_size_[num]+1; ++i) {
        std::string a = std::to_string(i);
    }
    return labels;
}

int Schema::deconvert(std::string file_path, std::string output_file_path)
{
    std::string sep_coma = ",";
    std::string sep_space = " ";
    std::vector<std::string> result;
    std::ifstream ifs(file_path);
    if(ifs.fail()) return -1;
    std::string str;
    // item
    std::getline(ifs, str);
    std::vector<std::string> item_num = Utils::split(str, sep_space);
    int x = std::stoi(item_num[0])-1;
    int y = std::stoi(item_num[1])-1;
    result.push_back(item_name_[x] + "-" + item_name_[y]);
    // label
    std::vector<std::string> x_label;
    for (int i = 1; i < item_size_[x]+1; ++i) {
        x_label.push_back(search_key(x,std::to_string(i)));
    }
    std::vector<std::string> y_label;
    for (int i = 1; i < item_size_[y]+1; ++i) {
        y_label.push_back(search_key(y,std::to_string(i)));
    }
    // hyo
    result.push_back(sep_coma + Utils::join(y_label, sep_coma));
    std::getline(ifs, str);
    std::vector<std::string> values = Utils::split(str, sep_space);
    for (int i = 0; i < item_size_[x]; ++i) {
        std::string line = x_label[i];
        for (int j = 0; j < item_size_[y]; ++j) {
            line += sep_coma + values[(i*item_size_[y]) + j];
        }
        result.push_back(line);
    }
    // write to output_file
    std::ofstream ofs(output_file_path);
    if(ofs.fail()) return -1;
    for (int i = 0; i < result.size(); ++i) {
        ofs << result[i] << std::endl;
    }
    return 0;
}

void Schema::debug_display()
{
    // debug start
    std::cout << "D@ name : " << item_name_[0] << std::endl;
    std::cout << "D@ type : " << item_type_[0] << std::endl;
    std::cout << "D@ size : " << item_size_[0] << std::endl;
    std::cout << "m : " << convert("m", 0) << std::endl;
    std::cout << "f : " << convert("f", 0) << std::endl;
    // debug end
}

