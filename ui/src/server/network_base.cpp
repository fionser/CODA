#include "network_base.hpp"

const int BUFF_LEN  = 65536;

int NetworkBase::send_msg(int socket, const std::string msg)
{
    int len = strlen(msg.c_str())+1;
    int status = send(socket, msg.c_str(), len, 0);
    if(status == len) {
        L_DEBUG(_console, "send msg size = {:d}", len);
        L_TRACE(_console, "send msg = [{0}]", std::string(msg).substr(0,status-1));
        return 0;
    } else if(status < 1) {
        L_ERROR(_console, "{0}", strerror(errno));
        return 1;
    } else {
        L_ERROR(_console, "send error.");
        return 1;
    }
}

int NetworkBase::recv_msg(int socket, char *msg, size_t msg_size)
{
    int status = recv(socket, msg, msg_size-1, 0);
    if(status > 0) {
        msg[msg_size-1] = '\0';
        L_DEBUG(_console, "receive msg size = {:d}", status);
        L_TRACE(_console, "receive msg = [{0}]", std::string(msg).substr(0,status-1));
        return 0;
    } else if(status == 0) {
        L_ERROR(_console, "close socket.");
        return 1;
    } else {
        L_ERROR(_console, "{0}", strerror(errno));
        return 1;
    }
}

int NetworkBase::recv_check(int socket, const std::string msg)
{
    if(strlen(msg.c_str()) > BUFF_LEN) {
    }
    char buf[(strlen(msg.c_str()) > BUFF_LEN ? strlen(msg.c_str())+1 : BUFF_LEN)];
    if(recv_msg(socket, buf, sizeof(buf)) == 0) {
        if(std::string(buf) == msg) {
            return 0;
        } else {
            L_ERROR(_console, "expect msg different.");
            L_TRACE(_console, "expect msg:[{0}] but receive msg:[{1}]", msg.c_str(), std::string(buf).c_str());
            return 1;
        }
    } else {
        return 1;
    }
}

int NetworkBase::snd_file(int socket, const std::string local_file_path, const std::string keyword_file_path)
{
    char buf[BUFF_LEN];
    if(send_msg(socket, keyword_file_path) != 0) {
        L_ERROR(_console, "send file name error.");
        return 1;
    }
    if(recv_check(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "send file name error..");
        return 1;
    }
    int file = open(local_file_path.c_str(), O_RDONLY);
    if (file < 0) {
        if(send_msg(socket, std::to_string(-1)) != 0) {
            L_ERROR(_console, "send file size error.");
            return 1;
        }
        L_ERROR(_console, "file [{0}] open error.", local_file_path);
        return 1;
    }
    // send file size
    std::ifstream ifs(local_file_path.c_str(), std::ios_base::binary);
    std::streamsize size = ifs.seekg(0, std::ios::end).tellg();
    ifs.seekg(0, std::ios::beg);
    int file_size = size;
    if(send_msg(socket, std::string(CConst::KEYWORD_FSIZE + to_string(file_size))) != 0) {
        L_ERROR(_console, "send file size error.");
        return 1;
    }
    if(recv_check(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "send file size error..");
        return 1;
    }
    if(file_size == 0) {
        close(file);
        return 0;
    }
    // send file
    int sum_size = 0;
    while (sum_size < file_size) {
        int msg_size = 0;
        if ((msg_size = read(file, buf, sizeof(buf)-1)) > 0) {
            buf[msg_size] = '\0';
            if(send_msg(socket, std::string(buf)) != 0) {
                L_ERROR(_console, "send file contents error.");
                close(file);
                return 1;
            }
            sum_size += std::string(buf).size();
            L_DEBUG(_console, "send file size = {0}/{1}", sum_size, file_size);
        } else {
            L_ERROR(_console, "send file contents error..");
            close(file);
            return 1;
        }
    }
    close(file);
    if(recv_check(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "receive file end msg error.");
        return 1;
    }
    return 0;
}

int NetworkBase::recv_file(int socket, const std::string local_file_path, const std::string keyword_file_path)
{
    char buf[BUFF_LEN];
    if(recv_check(socket, keyword_file_path) != 0) {
        L_ERROR(_console, "receive file name error.");
        if(send_msg(socket, CConst::MSG_NG) != 0) {
            L_ERROR(_console, "send error.");
            return 1;
        }
        return 1;
    }
    if(send_msg(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "send msg error.");
        return 1;
    }
    if(recv_msg(socket, buf, sizeof(buf)) != 0) {
        L_ERROR(_console, "recv file size error.");
        return 1;
    } else if(std::string(buf).substr(0, CConst::KEYWORD_FSIZE.size()) != CConst::KEYWORD_FSIZE) {
        L_ERROR(_console, "recv file size illegal.");
        return 1;
    }
    int file_size = atoi(std::string(buf).substr(CConst::KEYWORD_FSIZE.size()).c_str());
    int file = open(local_file_path.c_str(), O_WRONLY | O_CREAT, S_IREAD | S_IWRITE);
    if (file < 0) {
        L_ERROR(_console, "open file [{0}] error.", local_file_path);
        if(send_msg(socket, CConst::MSG_NG) != 0) {
            L_ERROR(_console, "send error.");
            return 1;
        }
        return 1;
    }
    if(send_msg(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "send msg error.");
        close(file);
        return 1;
    }
    if(file_size == 0) {
        close(file);
        return 0;
    }
    int sum_size = 0;
    // receive file
    while (sum_size < file_size) {
        int msg_size = 0;
        if (recv_msg(socket, buf, sizeof(buf)) == 0) {
            msg_size = strlen(buf);
            write(file, buf, msg_size);
            sum_size += msg_size;
            L_DEBUG(_console, "write file size = {0}/{1}", sum_size, file_size);
        } else {
            L_ERROR(_console, "write file error.");
            close(file);
            return 1;
        }
    }
    close(file);
    if(send_msg(socket, CConst::MSG_OK) != 0) {
        L_ERROR(_console, "send file end msg error.");
        return 1;
    }
    return 0;
}


