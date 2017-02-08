#include "network_client.hpp"

int NetworkClient::connect_server(const char *hostname, const in_port_t port_no)
{
    int client_socket = -1;
    hostent *host_info;
    sockaddr_in socket_info;
    if ((host_info = gethostbyname(hostname)) == NULL) {
        L_ERROR(_console, "hostname is incorrect.");
        return client_socket;
    }
    memset((char *)&socket_info, 0, sizeof(socket_info));
    socket_info.sin_family = AF_INET;
    socket_info.sin_port = htons(port_no);
    memcpy((char *)&socket_info.sin_addr.s_addr, host_info->h_addr, host_info->h_length);
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        L_ERROR(_console, "make socket.");
        return client_socket;
    }
    if (connect(client_socket, (sockaddr *)&socket_info, sizeof(socket_info)) == -1) {
        L_ERROR(_console, "connect error.");
        client_socket = -1;
    }
    return client_socket;
}


int NetworkClient::control_msg(int client_socket)
{
    int msg_size;
    char buf[BUFF_LEN];
    if((msg_size = read(client_socket, buf, BUFF_LEN)) < 1){
        L_ERROR(_console, "server connection close.");
        return 1;
    }
    fprintf(stdout, "%s\n", buf);
    std::string str_stdin;
    std::getline(std::cin, str_stdin);
    write(client_socket, str_stdin.c_str(), strlen(str_stdin.c_str())+1);
    if(str_stdin == CConst::CMD_SEND_FILES){
        std::vector<std::string> kpath;
        std::string str_stdin;
        fprintf(stdout, "FILE NUM : ");
        std::getline(std::cin, str_stdin);
        int f_num = atoi(str_stdin.c_str());
        for(int i=0; i<f_num; i++) {
            fprintf(stdout, "KEY_WORD FILE [%d] PATH : ", i+1);
            std::string kpath_tmp;
            std::getline(std::cin, kpath_tmp);
            kpath.push_back(std::string(CConst::KPATH_DEBUG + CConst::SEP_CH_FILE +  kpath_tmp));
        }
        FileSystemClient fs;
        file_trans(client_socket, kpath, fs, 0);
        // L_INFO(_console, "send file KEY_PATH:[{0}] LOCAL_PATH:[{1}].", keyword_file_path, local_file_path);
    } else if(str_stdin == CConst::CMD_RECEIVE_FILES){
        std::vector<std::string> kpath;
        std::string str_stdin;
        fprintf(stdout, "FILE NUM : ");
        std::getline(std::cin, str_stdin);
        int f_num = std::atoi(str_stdin.c_str());
        for(int i=0; i<f_num; i++) {
            fprintf(stdout, "KEY_WORD FILE [%d] PATH : ", i+1);
            std::string kpath_tmp;
            std::getline(std::cin, kpath_tmp);
            kpath.push_back(std::string(CConst::KPATH_DEBUG + CConst::SEP_CH_FILE +  kpath_tmp));
        }
        FileSystemClient fs;
        file_trans(client_socket, kpath, fs, 1);
    } else if(str_stdin == CConst::CMD_DEBUG){
        while(true) {
            if(recv_check(client_socket, CConst::MSG_OK) != 0) {
                L_INFO(_console, "end");
                break;
            } else {
                fprintf(stdout, "input : ");
                std::getline(std::cin, str_stdin);
                if(send_msg(client_socket, str_stdin) != 0) {
                    L_ERROR(_console, "send msg error.");
                    return -1;
                }
            }
        }
    }
    return 0;
}

int NetworkClient::file_trans(int socket, std::vector<std::string> kpath, FileSystemClient fs, int send_recv_flg)
{
    if(recv_check(socket, std::string(CConst::MSG_FILE_TRANS)) != 0) {
        L_ERROR(_console, "receive msg error");
        return -1;
    }
    switch(send_recv_flg) {
        case 0:
            if(send_msg(socket, std::string(CConst::MSG_CLIENT_SERVER)) != 0) {
                L_ERROR(_console, "send msg error.");
                return -1;
            }
            break;
        case 1:
            if(send_msg(socket, std::string(CConst::MSG_SERVER_CLIENT)) != 0) {
                L_ERROR(_console, "send msg error.");
                return -1;
            }
            break;
        default:
            return -1;
    }
    if(recv_check(socket, std::string(CConst::CMD_REQ_NUMBER_FILES)) != 0) {
        L_ERROR(_console, "receive msg error.");
        return -1;
    }
    int file_num = kpath.size();
    if(send_msg(socket, std::string(std::string(CConst::CMD_REQ_NUMBER_FILES) + to_string(file_num))) != 0) {
        L_ERROR(_console, "send msg error.");
        return -1;
    }
    std::vector<std::string> lpath;
    for(int i=0; i<file_num; i++) {
        if(recv_check(socket, std::string(CConst::CMD_REQ_FILE_NAME)) != 0) {
            L_ERROR(_console, "receive msg error.");
            return -1;
        }
        std::string lpath_tmp;
        if((lpath_tmp = fs.get_filepath(kpath[i])).size() == 0) {
            L_ERROR(_console, "get file path error.");
            return -1;
        }
        lpath.push_back(std::string(lpath_tmp).c_str());
        if(send_msg(socket, kpath[i]) != 0) {
            L_ERROR(_console, "send msg error.");
            return -1;
        }
    }
    if(recv_check(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "NG");
        return 1;
    }
    if(send_recv_flg == 1) {
        if(send_msg(socket, CConst::MSG_OK) != 0) {
            L_ERROR(_console, "send msg error.");
            return -1;
        }
    }
    // send or receive file 
    switch(send_recv_flg) {
        case 0:
            for(int i=0; i<file_num; i++) {
                if(snd_file(socket, lpath[i].c_str(), kpath[i].c_str()) != 0) {
                    L_ERROR(_console, "send file error.");
                    return 1;
                }
            }
            break;
        case 1:
            for(int i=0; i<file_num; i++) {
                if(recv_file(socket, lpath[i].c_str(), kpath[i].c_str()) != 0) {
                    L_ERROR(_console, "receive file error.");
                    return 1;
                }
            }
            break;
        default:
            return -1;
    }
    return 0;
}

int NetworkClient::manage(const char* hostname, const int port_no)
{
    int client_socket;
    if ((client_socket = connect_server(hostname, port_no)) == -1) {
        L_ERROR(_console, "make socket error.");
        return -1;
    }
    control_msg(client_socket);
    close(client_socket);
    return 0;
}

/*
 * info 0:session_name, 1:analyst_name, 2:protocol_name, 3:user_name
 */
int NetworkClient::service_init(std::string session_name, std::string prot_name, std::vector<std::string> user_names)
{
    L_INFO(_console, "service init");
    int socket;
    if ((socket = connect_server(hostname_.c_str(), port_no_)) == -1) {
        L_ERROR(_console, "error : make socket error.");
        return -1;
    }
    // cmd sent
    if(recv_check(socket, CConst::MSG_REQUEST_CMD) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, CConst::CMD_INIT) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    // send analyst name
    if(recv_check(socket, CConst::MSG_ANALYST_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, username_) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send analyst name [{}].", username_);
    // send session name
    if(recv_check(socket, CConst::MSG_SESSION_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, session_name) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send session name [{}].", session_name);
    // send protocol
    if(recv_check(socket, CConst::MSG_PROTOCOL) != 0) {
        L_ERROR(_console, "receive msg check error.");
        L_ERROR(_console, "server: analyst or session directory cannot be made.");
        return -1;
    }
    if(send_msg(socket, prot_name) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send protocol name [{}].", prot_name);
    // send number of users
    if(recv_check(socket, CConst::MSG_USER_NUM) != 0) {
        L_ERROR(_console, "receive msg check error.");
        L_ERROR(_console, "server: analyst or session directory cannot be made.");
        return -1;
    }
    int num_user = user_names.size();
    if(send_msg(socket, to_string(num_user)) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send number of users [{:d}].", num_user);
    // send users name
    for(int i=0;i<num_user;i++){
        if(recv_check(socket, CConst::MSG_USER_NAME) != 0) {
            L_ERROR(_console, "receive msg check error.");
            return -1;
        }
        if(send_msg(socket, user_names[i]) != 0) {
            L_ERROR(_console, "send msg error.");
            return 1;
        }
        L_DEBUG(_console, "send user name [{}].", user_names[i]);
    }
    if(recv_check(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    L_INFO(_console, "add user to session.");
    // ready for sending
    if(send_msg(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    FileSystemClient fs(session_name, username_, username_);
    std::vector<std::string> key_file_path;
    key_file_path.push_back(fs.kpath_meta_file());
    key_file_path.push_back(fs.kpath_schema_file());
    // send meta file
    if(file_trans(socket, key_file_path, fs, 0) != 0) {
        L_ERROR(_console, "send meta file error.");
        return -1;
    }
    L_INFO(_console, "send meta file.");
    L_INFO(_console, "init session succeed.");
    close(socket);
    return 0;
}

/*
 * info 0:session_name, 1:analyst_name
 */
int NetworkClient::send_pk(const std::string session_name)
{
    L_INFO(_console, "send pk");
    int socket;
    if ((socket = connect_server(hostname_.c_str(), port_no_)) == -1) {
        L_ERROR(_console, "error : make socket error.");
        return -1;
    }
    // cmd sent
    if(recv_check(socket, CConst::MSG_REQUEST_CMD) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, CConst::CMD_SEND_PK) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    // send analyst name
    if(recv_check(socket, CConst::MSG_ANALYST_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, username_) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send analyst name [{}].", username_);
    // send session name
    if(recv_check(socket, CConst::MSG_SESSION_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, session_name) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send session name [{}].", session_name);
    // ready for sending
    FileSystemClient fs(session_name, username_, username_);
    std::vector<std::string> key_file_path;
    key_file_path.push_back(fs.kpath_pkey_file());
    key_file_path.push_back(fs.kpath_ckey_file());
    // send meta file
    if(file_trans(socket, key_file_path, fs, 0) != 0) {
        L_ERROR(_console, "send pk file error.");
        return -1;
    }
    L_INFO(_console, "send pk successfully.");
    close(socket);
    return 0;
}

/*
 * info 0:session_name, 1:analyst_name, 2:user_name
 */
int NetworkClient::join_session(const char* hostname, const int port_no, const std::vector<std::string> info)
{
    L_INFO(_console, "service join");
    int socket;
    if ((socket = connect_server(hostname, port_no)) == -1) {
        L_ERROR(_console, "error : make socket error.");
        return -1;
    }
    // cmd sent
    if(recv_check(socket, CConst::MSG_REQUEST_CMD) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, CConst::CMD_JOIN) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    // send analyst name
    if(recv_check(socket, CConst::MSG_ANALYST_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, info[1]) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send analyst name [{}].", info[1]);
    // send session name
    if(recv_check(socket, CConst::MSG_SESSION_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, info[0]) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send session name [{}].", info[0]);
    // send user name
    if(recv_check(socket, CConst::MSG_USER_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, info[2]) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send user name [{}].", info[2]);
    // ready for receiving
    FileSystemClient fs(info[0], info[1], info[2]);
    std::vector<std::string> key_file_path;
    key_file_path.push_back(fs.kpath_meta_file());
    key_file_path.push_back(fs.kpath_schema_file());
    key_file_path.push_back(fs.kpath_pkey_file());
    key_file_path.push_back(fs.kpath_ckey_file());
    // send meta file
    if(file_trans(socket, key_file_path, fs, 1) != 0) {
        L_ERROR(_console, "receive meta and pk files error.");
        return -1;
    }
    L_INFO(_console, "receive meta and pk files.");
    L_INFO(_console, "join session succeed.");
    close(socket);
    return 0;
}

int NetworkClient::send_enc_data(const char* hostname, const int port_no, const std::vector<std::string> info)
{
    L_INFO(_console, "send enc data");
    int socket;
    if ((socket = connect_server(hostname, port_no)) == -1) {
        L_ERROR(_console, "error : make socket error.");
        return -1;
    }
    // cmd sent
    if(recv_check(socket, CConst::MSG_REQUEST_CMD) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, CConst::C_MAIN_CMD_SEND_DATA) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    // send analyst name
    if(recv_check(socket, CConst::MSG_ANALYST_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, info[1]) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send analyst name [{}].", info[1]);
    // send session name
    if(recv_check(socket, CConst::MSG_SESSION_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, info[0]) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send session name [{}].", info[0]);
    // send user name
    if(recv_check(socket, CConst::MSG_USER_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, info[2]) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send user name [{}].", info[2]);
    // ready for sending
    FileSystemClient fs(info[0], info[2], info[1]);
    std::vector<std::string> key_file_path = fs.get_file_list(fs.path_enc_uploading_dir(CConst::CATEGORICAL).c_str());
    if(key_file_path.size() == 0) {
        L_ERROR(_console, "enc file nothing.");
        return -1;
    }
    for(int i=0; i<key_file_path.size(); i++) {
        key_file_path[i] = CConst::KPATH_DATA + CConst::SEP_CH_FILE + key_file_path[i];
    }
    // send meta file
    if(file_trans(socket, key_file_path, fs, 0) != 0) {
        L_ERROR(_console, "send enc data file error.");
        return -1;
    }
    L_INFO(_console, "send enc data file.");
    L_INFO(_console, "send enc data succeed.");
    close(socket);
    return 0;
}

int NetworkClient::receive_result(const char* hostname, const int port_no, const std::vector<std::string> info)
{
    L_INFO(_console, "service receive result");
    int socket;
    if ((socket = connect_server(hostname, port_no)) == -1) {
        L_ERROR(_console, "error : make socket error.");
        return -1;
    }
    // cmd sent
    if(recv_check(socket, CConst::MSG_REQUEST_CMD) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, CConst::C_MAIN_CMD_RECEIVE_RESULT) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    // send analyst name
    if(recv_check(socket, CConst::MSG_ANALYST_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, info[1]) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send analyst name [{}].", info[1]);
    // send session name
    if(recv_check(socket, CConst::MSG_SESSION_NAME) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    if(send_msg(socket, info[0]) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_DEBUG(_console, "send session name [{}].", info[0]);
    // ready for receiving
    FileSystemClient fs(info[0], info[1], info[1]);
    std::vector<std::string> key_file_path;
    key_file_path.push_back(fs.kpath_result_file("type", CConst::RESULT_FILE_NAME)); // ToDo make type
    key_file_path.push_back(fs.kpath_result_file("type", CConst::FLAG_FILE_NAME));
    // send meta file
    if(file_trans(socket, key_file_path, fs, 1) != 0) {
        L_ERROR(_console, "receive result file error.");
        return -1;
    }
    L_INFO(_console, "receive result file.");
    L_INFO(_console, "receive result succeed.");
    close(socket);
    return 0;
}


NetworkClient::NetworkClient()
{
    hostname_ = "";
    port_no_ = 0;
    username_ = "";
}

NetworkClient::NetworkClient(const std::string hostname, const std::string port_no, const std::string username)
{
    hostname_ = hostname;
    port_no_ = atoi(port_no.c_str());
    username_ = username;
}


