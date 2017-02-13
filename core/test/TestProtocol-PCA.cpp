#include "core/protocol-PCA.hpp"
#include "core/coda.hpp"
#include "core/file_util.hpp"
#include "core/literal.hpp"
#include "HElib/FHE.h"
#include "HElib/timing.h"

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

long MP = 1;
long MQ = 2;
long MV = 0;
struct Cell {
    long p, q, v;
};

// test cases
std::vector<Cell> _cells = {
    {10, 200, 1},
    {20, 100, 1},
    {20, 100, 1},
    {20, 100, 1},
    {30, 100, 2},
    {30, 100, 2},
    {30, 100, 2},
    {30, 100, 2},
    {40, 200, 2}
};

bool gen_content(const std::string &path) {
    std::ofstream fout(util::concatenate(path, "data.csv"), std::ios::out | std::ios::trunc);
    if (!fout.is_open())
        return false;
    fout << "#" << MP << " " << MQ << " " << MV << "\n";
    for (auto &cell : _cells)
        fout << cell.p << " " << cell.q << " " << cell.v << "\n";
    fout.close();
    return true;
}

int main () {
    if (!create_dir("test-ct-1"))
        return -1;
    if (!create_dir("test-ct-2"))
        return -1;

    core::context_ptr context = std::make_shared<FHEcontext>(256, 8191, 3);
    buildModChain(*context, 7);
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    addSome1DMatrices(*sk);
    core::pk_ptr pk = std::make_shared<FHEPubKey>(*sk);

    PCAProtocol protocol;
    gen_content("test-ct-1");
    if (!protocol.encrypt("test-ct-1/data.csv", "test-ct-1/", true, pk, context))
        return -1;
    gen_content("test-ct-2");
    if (!protocol.encrypt("test-ct-2/data.csv", "test-ct-2/", true, pk, context))
        return -1;
    // std::vector<std::string> inputDirs;
    // inputDirs.push_back("test-ct-1");
    // inputDirs.push_back("test-ct-2");
    //
    // if (!create_dir("test-out"))
    //    return -1;
    // // the 50-percentile of the 1-st attr
    // if (!protocol.evaluate(inputDirs, "test-out", {"50", "1"}, pk, context))
    //     return -1;
    // if (!protocol.decrypt("test-out/" + core::core_setting.resulting_file,
    //                 "./", pk, sk, context))
    //     return -1;
    //
    // std::ifstream fin(core::core_setting.decrypted_file);
    // if (!fin.is_open())
    //     return -1;
    // int kPercentile;
    // fin >> kPercentile;
    // if (kPercentile != 3) {
    //     return -1;
    // }
    // fin.close();
    return 0;
}
