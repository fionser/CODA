#include "core/gc_user.hpp"
#include "core/global.hpp"
#include "core/coda.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
namespace plugin {
namespace gc {
class Alice::Imp {
public:
    Imp(const std::string &inputFile) : inputFile_(inputFile) {
    }

    bool run(const std::string &outputFile) const {
        int sockid = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockid < 0) {
            L_WARN(global::_console, "Can't not create the network socket.");
            return false;
        }

        struct sockaddr_in addrport;
        addrport.sin_family = AF_INET;
        addrport.sin_port = core::core_setting.gc_user_port;
        addrport.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(sockid, (struct sockaddr *) &addrport, sizeof(addrport)) < 0) {
            L_WARN(global::_console, "Can't not bind the address.");
            return false;
        }

        if (listen(sockid, 1000) < 0) {
            L_WARN(global::_console, "Can't not listen()");
            shutdown(sockid, 2);
            return false;
        }

        for (;;) {
            struct sockaddr addr;
            socklen_t addr_len;
            int client = accept(sockid, (struct sockaddr *)&addr, &addr_len);
            if (client < 0)
                continue;
            // int pid = fork();
            // if (pid != 0) {
            if (handleConnection(client))
                break;
            // }
        }
        shutdown(sockid, 1);
        // list request
        // fork subchild to call GC
        return true;
    }
private:
    bool handleConnection(int client) const {
        char cmd[1024];
        const char *format = "java -cp to-run:lib/* com.oblivm.backend.lang.inter.Cmd\
                        -t gen -i %s --config /Users/fionser/Documents/codes/coda/build/Config.conf\
                        -c com.oblivm.backend.generated.Comparison.NoClass";
        std::snprintf(cmd, sizeof(cmd), format, "/Users/fionser/Documents/codes/coda/build/alice.bin");
        FILE *pipe = popen(cmd, "r");
        if (!pipe)
            return false;
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            printf("%s", buffer);
        }
        fclose(pipe);
        shutdown(client, 1);
        return true;
    }

private:
    std::string inputFile_;
}; // Alice::Imp

Alice::Alice(const std::string &inputFile) : GCUser(inputFile) {
    imp_ = std::make_shared<Imp>(inputFile);
}

bool Alice::run(const std::string &outputFile) const {
    return imp_->run(outputFile);
}

class Bob::Imp {
public:
    Imp(const std::string &aliceIpAddr,
        const long port,
        const std::string &inputFile)
        : aliceIpAddr_(aliceIpAddr),
          port_(port),
          inputFile_(inputFile) {

    }

    bool run(const std::string &outputFile) const {
        // send request
        // then fork child to call GC
        int sockid = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in addrport;
        addrport.sin_family = AF_INET;
        addrport.sin_addr.s_addr = inet_addr(aliceIpAddr_.c_str());
        addrport.sin_port = core::core_setting.gc_user_port;
        if (connect(sockid, (struct sockaddr *) &addrport, sizeof(addrport)) < 0) {
            L_WARN(global::_console, "Can not connect to Alice");
            return false;
        }

        char cmd[1024];
        const char *format = "java -cp to-run:lib/* com.oblivm.backend.lang.inter.Cmd\
                        -t eva -i %s --config /Users/fionser/Documents/codes/coda/build/Config.conf\
                        -c com.oblivm.backend.generated.Comparison.NoClass";
        std::snprintf(cmd, sizeof(cmd), format, "/Users/fionser/Documents/codes/coda/build/bob.bin");
        FILE *pipe = popen(cmd, "r");
        if (!pipe)
            return false;
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            printf("%s", buffer);
        }
        fclose(pipe);
        shutdown(sockid, 1);
        return true;
    }

private:
    std::string aliceIpAddr_;
    long port_;
    std::string inputFile_;
}; // Bob::Imp

Bob::Bob(const std::string &aliceIpAddr,
         const long port,
         const std::string &inputFile): GCUser(inputFile) {
    imp_ = std::make_shared<Imp>(aliceIpAddr, port, inputFile);
}

bool Bob::run(const std::string &outputFile) const {
    return imp_->run(outputFile);
}
} // namespace gc
} // namespace plugin
