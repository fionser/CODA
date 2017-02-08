#include "schema.hpp"

int Schema::cTrue_;
int Schema::cError_;
std::string Schema::sep_coma_;
std::string Schema::sep_coron_;
std::string Schema::sep_sp_;
std::string Schema::meta_ch_;
std::string Schema::default_str_;
std::string Schema::categorical_;
std::string Schema::ordinal_;
std::string Schema::numerical_;

Schema::Schema()
{
    // init static value
    cTrue_ = 0;
    cError_ = -1;
    sep_coma_ = ",";
    sep_coron_ = ":";
    sep_sp_ = " ";
    meta_ch_ = "#";
    default_str_ = "0";
    categorical_ = "categorical";
    ordinal_ = "ordinal";
    numerical_ = "numerical";
    // init flg
    active_flg_ = -1;
}

Schema::Schema(std::string file_path) : Schema()
{
    active_flg_ = get_schema(file_path);
}

int Schema::check() const
{
    return active_flg_;
}

int Schema::get_schema(std::string file_path)
{
    std::ifstream ifs(file_path);
    if(ifs.fail()) return cError_;
    std::string str;
    // 1 : 項目
    std::getline(ifs, str);
    std::vector<std::string> item_name = Utils::split(str, sep_coma_);
    int size_schema = item_name.size();
    // 2 : 型（categorical, numerical, ordinal)
    std::getline(ifs, str);
    std::vector<std::string> item_type = Utils::split(str, sep_coma_);
    // 3 : サイズ
    std::getline(ifs, str);
    std::vector<std::string> item_size_str = Utils::split(str, sep_coma_);
    std::vector<int> item_size;
    for(auto itr = item_size_str.begin(); itr != item_size_str.end(); ++itr) {
        item_size.push_back(std::stoi(*itr));
    }
    // 4~ : 変換ルール
    std::vector< std::map<std::string, std::string> > convert_rule(item_size.size());
    while(std::getline(ifs, str)) {
        std::vector<std::string> vec_string = Utils::split(str, sep_coma_);
        for(auto itr = vec_string.begin(); itr != vec_string.end(); ++itr) {
            if(itr->size() != 0) {
                std::vector<std::string> key_value_set = Utils::split(*itr, sep_coron_);
                convert_rule[itr - vec_string.begin()][key_value_set[0]] = key_value_set[1];
            }
        }
    }
    item_name_ = item_name;
    item_type_ = item_type;
    item_size_ = item_size;
    item_size_str_ = item_size_str;
    rule_ = convert_rule;
    return cTrue_;
}

std::string Schema::convert(std::string key_string, int rule_no) const
{
    if(item_type_[rule_no].compare(numerical_) == 0) {
        return std::to_string(int(std::stof(key_string) * pow(10, item_size_.at(rule_no))));
    } else {
        const std::map<std::string, std::string> &key_value_map = rule_.at(rule_no);
        auto itr = key_value_map.find(key_string);
        return itr != key_value_map.end() ? itr->second : default_str_;
    }
}

int Schema::convert_csv(const std::string in_file_path, const Schema_Output_Filepath opaths) const
{
        // check schema
        if(check() != cTrue_) return cError_;
        // open earch files
        std::ifstream ifs(in_file_path);
        if(ifs.fail()) return cError_;
        std::map<std::string, std::ofstream> ofs;
        if((ofs[categorical_] = std::ofstream(opaths.categorical)).fail()) return cError_;
        if((ofs[ordinal_] = std::ofstream(opaths.ordinal)).fail()) return cError_;
        if((ofs[numerical_] = std::ofstream(opaths.numerical)).fail()) return cError_;
        // write meta sets
        std::map< std::string, std::vector<std::string> > meta_strs;
        for(int i=0; i<item_size_str_.size(); ++i) {
            std::string i_type = item_type_[i];
            if(i_type.find(categorical_) == 0) {
                meta_strs[categorical_].push_back(item_size_str_[i]);
            } else if(i_type.find(ordinal_) == 0) {
                meta_strs[categorical_].push_back(item_size_str_[i]);
                meta_strs[ordinal_].push_back(item_size_str_[i]);
            } else if(i_type.find(numerical_) == 0) {
                meta_strs[numerical_].push_back(item_size_str_[i]);
            }
        }
        ofs[categorical_] << meta_ch_ << Utils::join(meta_strs[categorical_], sep_sp_) << std::endl;
        ofs[ordinal_] << meta_ch_ << Utils::join(meta_strs[ordinal_], sep_sp_) << std::endl;
        ofs[numerical_] << meta_ch_ << Utils::join(meta_strs[numerical_], sep_sp_) << std::endl;
        // write convert data
        std::string line;
        while(std::getline(ifs, line)) {
            std::vector<std::string> vec_str = Utils::split(line, sep_coma_);
            std::map< std::string, std::vector<std::string> > write_strs;
            for(int i=0; i<vec_str.size(); i++) {
                std::string s = vec_str[i];
                std::string i_type = item_type_[i];
                if(i_type.find(categorical_) == 0) {
                    if(s.size() != 0) {
                        write_strs[categorical_].push_back(convert(s, i));
                    } else {
                        write_strs[categorical_].push_back(default_str_);
                    }
                } else if(i_type.find(ordinal_) == 0) {
                    if(s.size() != 0) {
                        write_strs[categorical_].push_back(convert(s, i));
                        write_strs[ordinal_].push_back(convert(s, i));
                    } else {
                        write_strs[ordinal_].push_back(default_str_);
                        write_strs[categorical_].push_back(default_str_);
                    }
                } else if(i_type.find(numerical_) == 0) {
                    if(s.size() != 0) {
                        write_strs[numerical_].push_back(convert(s, i));
                    } else {
                        write_strs[numerical_].push_back(default_str_);
                    }
                }
            }
            ofs[categorical_] << Utils::join(write_strs[categorical_], sep_sp_) << std::endl;
            ofs[ordinal_] << Utils::join(write_strs[ordinal_], sep_sp_) << std::endl;
            ofs[numerical_] << Utils::join(write_strs[numerical_], sep_sp_) << std::endl;
        }
        return cTrue_;
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
    std::vector<std::string> result;
    std::ifstream ifs(file_path);
    if(ifs.fail()) return cError_;
    std::string str;
    // item
    std::getline(ifs, str);
    std::vector<std::string> item_num = Utils::split(str, sep_sp_);
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
    // table
    result.push_back(sep_coma_ + Utils::join(y_label, sep_coma_));
    std::getline(ifs, str);
    std::vector<std::string> values = Utils::split(str, sep_sp_);
    for (int i = 0; i < item_size_[x]; ++i) {
        std::string line = x_label[i];
        for (int j = 0; j < item_size_[y]; ++j) {
            line += sep_coma_ + values[(i*item_size_[y]) + j];
        }
        result.push_back(line);
    }
    // write to output_file
    std::ofstream ofs(output_file_path);
    if(ofs.fail()) return -1;
    for (int i = 0; i < result.size(); ++i) {
        ofs << result[i] << std::endl;
    }
    return cTrue_;
}

void Schema::debug_display()
{
    // debug start
    std::cout << "D@ name : " << item_name_[0] << std::endl;
    std::cout << "D@ type : " << item_type_[0] << std::endl;
    std::cout << "D@ size : " << item_size_[0] << std::endl;
    std::cout << "m : " << convert("max", 0) << std::endl;
    std::cout << "f : " << convert("min", 0) << std::endl;
    std::cout << "D@ name : " << item_name_[1] << std::endl;
    std::cout << "D@ type : " << item_type_[1] << std::endl;
    std::cout << "D@ size : " << item_size_[1] << std::endl;
    std::cout << "m : " << convert("m", 1) << std::endl;
    std::cout << "f : " << convert("f", 1) << std::endl;
    // debug end
}

