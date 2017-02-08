#include "network_server.hpp"

int NetworkServer::connect_client(const in_port_t port_no)
{
    int listening_socket = -1;
    sockaddr_in socket_info;
    memset((char *)&socket_info, 0, sizeof(socket_info));
    socket_info.sin_family = AF_INET;
    socket_info.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_info.sin_port = htons(port_no);
    if ((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "error : make socket error.\n");
        return -1;
    }
    if (bind(listening_socket, (sockaddr *)&socket_info, sizeof(socket_info)) < 0){
        fprintf(stderr, "error : bind error.\n");
        return -1;
    }
    if (listen(listening_socket, 1) == -1 ) {
        fprintf(stderr, "error : listen error.\n");
        return -1;
    }
    return listening_socket;
}

int NetworkServer::control_msg(int connected_socket)
{
    int msg_size;
    char buf[BUFF_LEN];
    hostent *host_info;
    sockaddr_in socket_info;
    socklen_t slen;
    // クライアント情報を取得できた場合，それを表示する
    slen = (socklen_t)sizeof(socket_info);
    getpeername(connected_socket, (sockaddr *)&socket_info, &slen);
    host_info = gethostbyaddr((char *)&socket_info.sin_addr.s_addr, sizeof(socket_info.sin_addr), AF_INET);
    if (host_info != NULL) {
        L_INFO(_console, "connectetd {0}. (IP: {1}, port: {2:d})", host_info->h_name, inet_ntoa(socket_info.sin_addr), ntohs(socket_info.sin_port));
    }
    // control start
    write(connected_socket, CConst::MSG_REQUEST_CMD, strlen(CConst::MSG_REQUEST_CMD)+1);
    if((msg_size = read(connected_socket, buf, BUFF_LEN)) < 1) {
        return 1;
    } else if(std::string(buf) == CConst::CMD_INIT){
        return init_session(connected_socket);
    } else if(std::string(buf) == CConst::CMD_SEND_PK){
        return receive_pk(connected_socket);
    } else if(std::string(buf) == CConst::CMD_JOIN){
        return join_session(connected_socket);
    } else if(std::string(buf) == CConst::CMD_SEND_DATA){
        return receive_enc_data(connected_socket);
    } else if(std::string(buf) == CConst::CMD_RECV_RESULT){
        return send_result_data(connected_socket);
    } else if(std::string(buf) == CConst::CMD_SEND_FILES){
        FileSystemServer fs;
        file_trans(connected_socket, fs);
    } else if(std::string(buf) == CConst::CMD_RECEIVE_FILES){
        FileSystemServer fs;
        file_trans(connected_socket, fs);
    } else if(std::string(buf) == CConst::CMD_DEBUG){
        for(int i=0; i<2; i++) {
            if(send_msg(connected_socket, std::string(CConst::MSG_OK)) != 0) {
                L_ERROR(_console, "send msg error.");
                return 1;
            }
            if(recv_msg(connected_socket, buf, sizeof(buf)) != 0) {
                L_ERROR(_console, "receive msg error");
                return 1;
            }
            L_INFO(_console, "client input : {0}", std::string(buf));
        }
        if(send_msg(connected_socket, std::string(CConst::MSG_NG)) != 0) {
            L_ERROR(_console, "send msg error.");
            return 1;
        }
        if(recv_msg(connected_socket, buf, sizeof(buf)) != 0) {
            L_ERROR(_console, "receive msg error");
            return 1;
        }
        L_INFO(_console, "client input : {0}", std::string(buf));
    }
    return 0;
}

int NetworkServer::init_session(int socket)
{
    char buf[BUFF_LEN];
    L_INFO(_console, "service init");
    // get analyst name
    if(send_msg(socket, CConst::MSG_ANALYST_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string analyst_name = buf;
    L_DEBUG(_console, "get analyst name [{}].", analyst_name);
    // get session name
    if(send_msg(socket, CConst::MSG_SESSION_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string session_name = buf;
    L_DEBUG(_console, "get session name [{}].", session_name);
    // get analyst session unique check
    FileSystemServer d(session_name, analyst_name, analyst_name);
    if(d.make_session_directory() != 0){
        L_ERROR(_console, "make analyst directory");
        if(send_msg(socket, CConst::MSG_NG) != 0) {
            L_ERROR(_console, "send msg error.");
            return 1;
        }
        // ToDo rm session dir ?
        close(socket);
        return 1;
    }
    L_INFO(_console, "make session dir [{0}/{1}].", analyst_name, session_name);
    // get protocol
    if(send_msg(socket, CConst::MSG_PROTOCOL) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string prot = buf;
    L_DEBUG(_console, "get protocol name [{}].", prot);
    // get number of users
    if(send_msg(socket, CConst::MSG_USER_NUM) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    int num_user = std::atoi(buf);
    L_DEBUG(_console, "get number of users [{:d}].", num_user);
    // get users name
    for(int i=0; i<num_user; i++){
        if(send_msg(socket, CConst::MSG_USER_NAME) != 0) {
            L_ERROR(_console, "send msg error.");
            return 1;
        }
        if(recv_msg(socket, buf, sizeof(buf)) != 0) {
            L_ERROR(_console, "receive msg error");
            return 1;
        }
        std::string user_name = buf;
        L_DEBUG(_console, "get user name [{}].", user_name);
        if(d.add_user(analyst_name.c_str(), session_name.c_str(), user_name.c_str()) != 0){
            L_ERROR(_console, "make user dir error[{}].", user_name);
            close(socket);
            return 1;
        }
    }
    if(send_msg(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    L_INFO(_console, "add user to session.");
    // ready for receiving
    if(recv_check(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "receive msg check error.");
        return -1;
    }
    FileSystemServer fs(session_name, analyst_name, analyst_name);
    // get meta file
    if(file_trans(socket, fs) != 0) {
        L_ERROR(_console, "receive meta file error.");
        return -1;
    }
    L_INFO(_console, "receive meta file.");
    L_INFO(_console, "init session succeed.");
    return 0;
}

int NetworkServer::receive_pk(int socket)
{
    char buf[BUFF_LEN];
    L_INFO(_console, "service receive pk");
    // get analyst name
    if(send_msg(socket, CConst::MSG_ANALYST_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string analyst_name = buf;
    L_DEBUG(_console, "get analyst name [{}].", analyst_name);
    // get session name
    if(send_msg(socket, CConst::MSG_SESSION_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string session_name = buf;
    L_DEBUG(_console, "get session name [{}].", session_name);
    // ready for receiving
    FileSystemServer fs(session_name, analyst_name, analyst_name);
    // get meta file
    if(file_trans(socket, fs) != 0) {
        L_ERROR(_console, "receive pk files error.");
        return -1;
    }
    L_INFO(_console, "receive pk files.");
    L_INFO(_console, "receive pk succeed.");
    return 0;
}

int NetworkServer::join_session(int socket)
{
    char buf[BUFF_LEN];
    L_INFO(_console, "service join");
    // get analyst name
    if(send_msg(socket, CConst::MSG_ANALYST_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string analyst_name = buf;
    L_DEBUG(_console, "get analyst name [{}].", analyst_name);
    // get session name
    if(send_msg(socket, CConst::MSG_SESSION_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string session_name = buf;
    L_DEBUG(_console, "get session name [{}].", session_name);
    // get user name
    if(send_msg(socket, CConst::MSG_USER_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string user_name = buf;
    L_DEBUG(_console, "get user name [{}].", user_name);
    // ready for send
    FileSystemServer fs(session_name, analyst_name, user_name);
    // send meta and pk file
    if(file_trans(socket, fs) != 0) {
        L_ERROR(_console, "send meta and pk files error.");
        return -1;
    }
    L_INFO(_console, "send meta and pk files.");
    L_INFO(_console, "join session succeed.");
    return 0;
}

int NetworkServer::receive_enc_data(int socket)
{
    char buf[BUFF_LEN];
    L_INFO(_console, "service receive enc data");
    // get analyst name
    if(send_msg(socket, CConst::MSG_ANALYST_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string analyst_name = buf;
    L_DEBUG(_console, "get analyst name [{}].", analyst_name);
    // get session name
    if(send_msg(socket, CConst::MSG_SESSION_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string session_name = buf;
    L_DEBUG(_console, "get session name [{}].", session_name);
    // get user name
    if(send_msg(socket, CConst::MSG_USER_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string user_name = buf;
    L_DEBUG(_console, "get user name [{}].", user_name);
    // ready for receiving
    FileSystemServer fs(session_name, analyst_name, user_name);
    // get meta file
    if(file_trans(socket, fs) != 0) {
        L_ERROR(_console, "receive enc data files error.");
        return -1;
    }
    L_INFO(_console, "receive enc data files.");
    L_INFO(_console, "receive enc data succeed.");
    return 0;
}

int NetworkServer::send_result_data(int socket)
{
    char buf[BUFF_LEN];
    L_INFO(_console, "service send result");
    // get analyst name
    if(send_msg(socket, CConst::MSG_ANALYST_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string analyst_name = buf;
    L_DEBUG(_console, "get analyst name [{}].", analyst_name);
    // get session name
    if(send_msg(socket, CConst::MSG_SESSION_NAME) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    std::string session_name = buf;
    L_DEBUG(_console, "get session name [{}].", session_name);
    // ready for send
    FileSystemServer fs(session_name, analyst_name, analyst_name);
    // send meta and pk file
    if(file_trans(socket, fs) != 0) {
        L_ERROR(_console, "send result file error.");
        return -1;
    }
    L_INFO(_console, "send send result files.");
    L_INFO(_console, "send result succeed.");
    return 0;
}


int NetworkServer::file_trans(int socket, FileSystemServer fs)
{
    int send_receive_flg = 0;
    char buf[BUFF_LEN];
    // send receive check
    if(send_msg(socket, std::string(CConst::MSG_FILE_TRANS)) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    if(std::string(buf) == std::string(CConst::MSG_CLIENT_SERVER)) {
        send_receive_flg = 0;
    } else if(std::string(buf) == std::string(CConst::MSG_SERVER_CLIENT)) {
        send_receive_flg = 1;
    } else {
        L_ERROR(_console, "send receive flg illegal.");
        return 1;
    }
    // file num check
    int file_num = 0;
    if(send_msg(socket, std::string(CConst::CMD_REQ_NUMBER_FILES)) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "receive msg error");
        return 1;
    }
    if(std::string(buf).substr(0, std::string(CConst::CMD_REQ_NUMBER_FILES).size()) != std::string(CConst::CMD_REQ_NUMBER_FILES)) {
        L_ERROR(_console, "receive msg error.");
        return 1;
    } else {
        file_num = std::atoi(std::string(buf).substr(std::string(CConst::CMD_REQ_NUMBER_FILES).size()).c_str());
    }
    // file name check
    std::vector<std::string> kpath, lpath;
    for(int i=0; i<file_num; i++) {
        if(send_msg(socket, std::string(CConst::CMD_REQ_FILE_NAME)) != 0) {
            L_ERROR(_console, "send msg error.");
            return 1;
        }
        if(recv_msg(socket, buf, sizeof(buf)) != 0) {
            L_ERROR(_console, "receive msg error");
            return 1;
        }
        std::string kpath_tmp = buf;
        std::string lpath_tmp;
        if((lpath_tmp = fs.get_filepath(kpath_tmp)).size() == 0) {
            L_ERROR(_console, "get file path error.");
            return 1;
        }
        kpath.push_back(std::string(kpath_tmp));
        lpath.push_back(std::string(lpath_tmp));
    }
    if(send_msg(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(send_receive_flg == 1) {
        if(recv_check(socket, CConst::MSG_OK) != 0) {
            L_ERROR(_console, "NG");
            return 1;
        }
    }
    // send or receive file
    switch(send_receive_flg) {
        case 1:
            for(int i=0; i<file_num; i++) {
                if(snd_file(socket, lpath[i], kpath[i]) != 0) {
                    L_ERROR(_console, "send file error.");
                    return 1;
                }
            }
            break;
        case 0:
            for(int i=0; i<file_num; i++) {
                if(recv_file(socket, lpath[i], kpath[i]) != 0) {
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

int NetworkServer::start_service(const int port_no)
{
    int listening_socket;
    int connected_socket;
    pid_t pid;
    // リスニングソケットの生成
    if ((listening_socket = connect_client(port_no)) != -1) {
        // 接続待機
        while(true) {
            // 接続済みソケット
            if ((connected_socket = accept(listening_socket, NULL, NULL)) != -1) {
                // 子プロセスの生成
                //　子プロセス
                if ((pid = fork()) == 0) {
                    close(listening_socket);
                    control_msg(connected_socket);
                    close(connected_socket);
                    exit(0);
                }
                // 親プロセス
                else {
                    close(connected_socket);
                }
            }
        }
    }
    return 0;
}


