#include "filesystem_base.hpp"

int FileSystemBase::dir_clear(const std::string filepath)
{
    int rc = 0;
    DIR *dp = NULL;
    struct dirent *ent = NULL;
    char buf[BUFSIZ];
    if((dp = opendir(filepath.c_str())) == NULL){
        L_ERROR(_console, "open file [{0}] : {1}", filepath, strerror(errno));
        return -1;
    }
    while((ent = readdir(dp)) != NULL){
        if((strcmp(".", ent->d_name) == 0) || (strcmp("..", ent->d_name) == 0)){
            continue;
        } else {
            snprintf(buf, sizeof(buf), "%s/%s", filepath.c_str(), ent->d_name);
            rc = recursive_dir_clear(buf);
            if(rc != 0){
                break;
            }
        }
    }
    closedir(dp);
    return rc;
}

int FileSystemBase::recursive_dir_clear(const std::string filepath)
{
    int rc = 0;
    rc = delete_regular_files(filepath);
    if(rc == 1){
        return 0;
    } else if(rc != 0){
        return -1;
    }
    // recursive directory
    if(dir_clear(filepath) != 0){
        return -1;
    } else if(rmdir(filepath.c_str()) < 0){
        L_ERROR(_console, "rmdir [{0}] : {1}", filepath, strerror(errno));
        return -1;
    }
    return 0;
}

int FileSystemBase::delete_regular_files(const std::string filepath)
{
    int rc = 0;
    struct stat sb = {0};
    rc = stat(filepath.c_str(), &sb);
    if(rc < 0){
        L_ERROR(_console, "stat [{0}] : {1}", filepath, strerror(errno));
        return -1;
    }
    if(S_ISDIR(sb.st_mode)){
        return 0;
    }
    rc = unlink(filepath.c_str());
    if(rc < 0){
        L_ERROR(_console, "unlink [{0}] : {1}", filepath, strerror(errno));
        return -1;
    }
    return 1;
}

int FileSystemBase::make_directory(const std::string dir_path)
{
    int flg= mkdir(dir_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if(flg != 0) {
        L_ERROR(_console, "make {} directory.", dir_path);
        return flg;
    } else {
        return flg;
    }
}

int FileSystemBase::remove_directory(const std::string dir_path)
{
    struct stat stat_buf;
    if(stat(dir_path.c_str(), &stat_buf) == 0){
        if(S_ISDIR(stat_buf.st_mode)){
            // remove internal files
            if(dir_clear(dir_path) != 0){
                return -1;
            }
            // remove the directory.
            if(rmdir(dir_path.c_str()) < 0){
                L_ERROR(_console, "rmdir [{0}] : {1}", dir_path, strerror(errno));
                return -1;
            }
            L_INFO(_console, "rmdir [{0}]", dir_path);
            return 0;
        }
    }
    L_ERROR(_console, "directory [{0}] doesn't exist.", dir_path);
    return -1;
}

std::vector<std::string> FileSystemBase::get_file_list(const std::string dir_path)
{
    std::vector<std::string> files;
    DIR *dp = NULL;
    struct dirent *ent = NULL;
    if((dp = opendir(dir_path.c_str())) == NULL){
        L_ERROR(_console, "opendir [{0}] {1}", dir_path, strerror(errno));
        return std::vector<std::string>(0);
    }
    while((ent = readdir(dp)) != NULL){
        if((strcmp(".", ent->d_name) == 0) || (strcmp("..", ent->d_name) == 0)){
            continue;
        } else {
            files.push_back(ent->d_name);
        }
    }
    std::sort(files.begin(),files.end());
    /* debug
    std::vector<std::string>::iterator sIter = find(files.begin(), files.end(), std::string("dummy.txt") );
    if( sIter != files.end() ){
      printf("dummy.txt exit\n");
      files.erase(sIter);
    }
    printf("iter = %d\n", int(sIter - files.begin()));
    for(int i=0; i<files.size(); i++){
        printf("%s\n", files[i].c_str());
    }
    */
    return files;
}

int FileSystemBase::copy_file(const std::string src_file_path, const std::string dst_file_path)
{
    // open source file
    std::ifstream ifs(src_file_path.c_str());
    if (!ifs) {
        L_ERROR(_console, "file open error: {0}", src_file_path);
        return -1;
    }
    // open copy file
    std::ofstream ofs(dst_file_path);
    if (!ofs) {
        L_ERROR(_console, "file open error: {0}", dst_file_path);
        return -1;
    }
    // copy
    ofs << ifs.rdbuf() << std::flush;
    return 0;
}

// file directory paths
std::string FileSystemBase::path_coda_config() { return CConst::KPATH_META; }
// keyword file directory paths
std::string FileSystemBase::kpath_meta_dir() { return CConst::KPATH_META; }
std::string FileSystemBase::kpath_meta_file() { return kpath_meta_dir() + CConst::SEP_CH_FILE + CConst::META_FILE_NAME; }
std::string FileSystemBase::kpath_pkey_file() { return kpath_meta_dir() + CConst::SEP_CH_FILE + CConst::PUBLIC_KEY_FILE_NAME; }
std::string FileSystemBase::kpath_ckey_file() { return kpath_meta_dir() + CConst::SEP_CH_FILE + CConst::CONTEXT_KEY_FILE_NAME; }
std::string FileSystemBase::kpath_schema_file() { return kpath_meta_dir() + CConst::SEP_CH_FILE + CConst::SCHEMA_FILE_NAME; }
std::string FileSystemBase::kpath_data_root_dir() { return CConst::KPATH_DATA; }
std::string FileSystemBase::kpath_data_type_dir(std::string data_type) { return kpath_data_root_dir() + CConst::SEP_CH_FILE + data_type; }
std::string FileSystemBase::kpath_user_dir(std::string data_type, std::string user_name) { return kpath_data_type_dir(data_type) + CConst::SEP_CH_FILE + user_name; }
std::string FileSystemBase::kpath_data_file(std::string data_type, std::string user_name, std::string file_name) { return kpath_user_dir(data_type, user_name) + CConst::SEP_CH_FILE + file_name; }
std::string FileSystemBase::kpath_result_root_dir() { return CConst::KPATH_RESULT; }
std::string FileSystemBase::kpath_result_dir(std::string dir_name) { return kpath_result_root_dir() + CConst::SEP_CH_FILE + dir_name; }
std::string FileSystemBase::kpath_result_file(std::string dir_name, std::string file_name) { return kpath_result_dir(dir_name) + CConst::SEP_CH_FILE + file_name;}


// constructor
FileSystemBase::FileSystemBase()
{
    analyst_name_ = "";
    session_name_ = "";
    user_name_ = "";
}

