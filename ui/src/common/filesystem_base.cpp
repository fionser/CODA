#include "filesystem_base.hpp"

int FileSystemBase::dir_clear(const char *filepath)
{
    int rc = 0;
    DIR *dp = NULL;
    struct dirent *ent = NULL;
    char buf[BUFSIZ];
    if((dp = opendir(filepath)) == NULL){
        L_ERROR(_console, "open file [{0}] : {1}", filepath, strerror(errno));
        return -1;
    } 
    while((ent = readdir(dp)) != NULL){
        if((strcmp(".", ent->d_name) == 0) || (strcmp("..", ent->d_name) == 0)){
            continue;
        } else {
            snprintf(buf, sizeof(buf), "%s/%s", filepath, ent->d_name);
            rc = recursive_dir_clear(buf);
            if(rc != 0){
                break;
            }
        }
    }
    closedir(dp);
    return rc;
}

int FileSystemBase::recursive_dir_clear(const char *filepath)
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
    } else if(rmdir(filepath) < 0){
        L_ERROR(_console, "rmdir [{0}] : {1}", filepath, strerror(errno));
        return -1;
    }
    return 0;
}

int FileSystemBase::delete_regular_files(const char *filepath)
{
    int rc = 0;
    struct stat sb = {0};
    rc = stat(filepath, &sb);
    if(rc < 0){
        L_ERROR(_console, "stat [{0}] : {1}", filepath, strerror(errno));
        return -1;
    }
    if(S_ISDIR(sb.st_mode)){
        return 0;
    }
    rc = unlink(filepath);
    if(rc < 0){
        L_ERROR(_console, "unlink [{0}] : {1}", filepath, strerror(errno));
        return -1;
    }
    return 1;
}

int FileSystemBase::remove_directory(const char* dir_name)
{
    struct stat stat_buf;
    if(stat(dir_name, &stat_buf) == 0){
        if(S_ISDIR(stat_buf.st_mode)){
            // remove internal files
            if(dir_clear(dir_name) != 0){
                return -1;
            }
            // remove the directory.
            if(rmdir(dir_name) < 0){
                L_ERROR(_console, "rmdir [{0}] : {1}", dir_name, strerror(errno));
                return -1;
            }
            L_INFO(_console, "rmdir [{0}]", dir_name);
            return 0;
        }
    }
    L_ERROR(_console, "directory [{0}] doesn't exist.", dir_name);
    return -1;
}

std::vector<std::string> FileSystemBase::get_file_list(const char *dir_path)
{
    std::vector<std::string> files;
    DIR *dp = NULL;
    struct dirent *ent = NULL;
    if((dp = opendir(dir_path)) == NULL){
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

int FileSystemBase::copy_file(const char* src, const char* dst)
{
    // open source file
    std::ifstream ifs(src);
    if (!ifs) {
        L_ERROR(_console, "file open error: {0}", src);
        return -1;
    }
    // open copy file
    std::ofstream ofs(dst);
    if (!ofs) {
        L_ERROR(_console, "file open error: {0}", dst);
        return -1;
    }
    // copy
    ofs << ifs.rdbuf() << std::flush;
    // check error
    if (!ifs) {
        L_ERROR(_console, "I/O error");
        return -1;
    }
    if (!ofs) {
        L_ERROR(_console, "I/O error");
        return -1;
    }

    return 0;
}

FileSystemBase::FileSystemBase()
{
    analyst_name_ = "";
    session_name_ = "";
    user_name_ = "";
}

FileSystemBase::FileSystemBase(std::string analyst_name, std::string session_name, std::string user_name)
{
    analyst_name_ = analyst_name;
    session_name_ = session_name;
    user_name_ = user_name;
}



