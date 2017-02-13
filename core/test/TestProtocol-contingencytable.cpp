#include "core/protocol-contingency.hpp"
#include "core/coda.hpp"
#include "core/file_util.hpp"
#include "HElib/FHE.h"
#include "HElib/timing.h"

#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/dirent.h>
bool create_dir(const std::string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path.c_str(), 0777) != 0)
            return false;
    } else {
        auto files = util::listDir(path, util::flag_t::FILE_ONLY);
        for (auto file : files) {
            auto full = util::concatenate(path, file);
            if (full.compare(".") == 0 || full.compare("..")) continue;
            unlink(full.c_str());
        }

    }
    return true;
}

bool gen_content(const std::string &path) {
    std::ofstream fout(util::concatenate(path, "data.csv"));
    if (!fout.is_open())
        return false;
    fout << "#3 3 2 2\n";
    fout << "1 2 1\n";
    fout << "2 2 1\n";
    fout << "3 1 2\n";
    return true;
}

int main () {
    if (!create_dir("test-ct-1"))
        return -1;
    if (!create_dir("test-ct-2"))
        return -1;

    core::context_ptr context = std::make_shared<FHEcontext>(8192, 8191, 1);
    buildModChain(*context, 10);
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    addSome1DMatrices(*sk);
    core::pk_ptr pk = std::make_shared<FHEPubKey>(*sk);

    ContingencyTableProtocol ct(1, 2);
    gen_content("test-ct-1");
    ct.encrypt("test-ct-1/data.csv", "test-ct-1/", false,
               { .single = pk, .ppe = nullptr },
               { .single = context, .ppe = nullptr });
    gen_content("test-ct-2");
    ct.encrypt("test-ct-2/data.csv", "test-ct-2/", false,
               { .single = pk, .ppe = nullptr },
               { .single = context, .ppe = nullptr });

    std::vector<std::string> inputDirs;
    inputDirs.push_back("test-ct-1");
    inputDirs.push_back("test-ct-2");
    create_dir("test-out");
    ct.evaluate(inputDirs, "test-out", {"1", "2"},
               { .single = pk, .ppe = nullptr },
               { .single = context, .ppe = nullptr });

    ct.decrypt("test-out/File_result", "./",
               { .single = pk, .ppe = nullptr },
               { .single = sk, .ppe = nullptr},
               { .single = context, .ppe = nullptr });

    std::ifstream fin("File_result");
    if (!fin.is_open())
        return -1;

    std::string line;
    std::getline(fin, line);
    if (line.compare("1 2") != 0) {
        std::cout << line << "\n";
        return -1;
    }

    std::getline(fin, line);
    if (line.compare("0 2 0 2 2 0") != 0) {
        std::cout << line << "*\n";
        return -1;
    }
    std::cout << "Passed\n";
    return 0;
}
