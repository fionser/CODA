#include "core/protocol-LR.hpp"
#include "core/file_util.hpp"
#include "core/PPE/PPE.hpp"

#include "HElib/FHE.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <core/coda.hpp>

bool create_dir(const std::string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        /* Directory does not exist. EXIST for race condition */
        if (mkdir(path.c_str(), 0777) != 0)
            return false;
    } else {
        auto files = util::listDir(path, util::flag_t::FILE_ONLY);
        for (auto file : files) {
            if (file.compare(".") == 0 || file.compare("..")) continue;
            std::string full = util::concatenate(path, file);
            std::cout << full << std::endl;
            unlink(full.c_str());
        }

    }
    return true;
}

std::vector<long> mags = {1, 0, 0};
struct Cell {
    long p, q, v;
};

// test cases
NTL::mat_ZZ matrix;
NTL::Mat<double> dMatrix;

bool gen_content(const std::string &path) {
    std::ofstream fout(util::concatenate(path, "data.csv"), std::ios::out | std::ios::trunc);
    if (!fout.is_open())
        return false;

    fout << "#" << mags[0] << " " << mags[1] << " " << mags[2] << std::endl;
    matrix.SetDims(2, 3);
    matrix[0][0] = 1; matrix[0][1] = 2; matrix[0][2] = 3;
    matrix[1][0] = 6; matrix[1][1] = 5; matrix[1][2] = 4;
    for (long r = 0; r < matrix.NumRows(); r++) {
        for (long c = 0; c < matrix.NumCols(); c++)
            fout << matrix[r][c] << " ";
        fout << std::endl;
    }
    fout.close();
    return true;
}

bool check(const std::string &dir) {
    auto max_m = std::max_element(mags.begin(), mags.end());
    Matrix matrix2;
    matrix2.SetDims(matrix.NumRows(), matrix.NumCols() - 1);
    Vector vec2;
    vec2.SetLength(matrix2.NumCols());
    for (long r = 0; r < matrix.NumRows(); r++) {
        for (long c = 0; c < matrix2.NumCols(); c++) {
            long f = *max_m - mags[c];
            matrix2[r][c] = matrix[r][c] * NTL::power_long(10, f);
        }
        vec2[r] = matrix[r][matrix.NumCols() - 1];
    }
    NTL::mat_ZZ Xt;
    NTL::transpose(Xt, matrix2);
    NTL::mat_ZZ ground_mat;
    NTL::mul(ground_mat, Xt, matrix2);
    ground_mat *= 2; // two clients

    Vector ground_v;
    NTL::mul(ground_v, Xt, vec2);
    ground_v *= 2;

    std::cout << ground_mat << "\n" << ground_v << std::endl;
    return true;
}

int main () {
    if (!create_dir("test-ct-1"))
        return -1;
    if (!create_dir("test-ct-2"))
        return -1;
    if (!create_dir("test-out"))
        return -1;

    std::vector <long> Ms = {256, 512, 512};
    std::vector <long> Ps = {8191, 8209, 1031};
    std::vector <long> Rs = {1, 1, 2};
    core::ContextWrapper context = {.ppe = std::make_shared <ppe::Context>(Ms, Ps, Rs), .single = nullptr};
    context.ppe->buildModChain(25);
    core::SecKeyWrapper sk = {.ppe = std::make_shared <ppe::SecKey>(*context.ppe), .single = nullptr};
    sk.ppe->GenSecKey(64);
    sk.ppe->addSome1DMatrices();
    core::PubKeyWrapper pk = {.ppe = std::make_shared <ppe::PubKey>(*sk.ppe), .single = nullptr};

    LRProtocol protocol;
    gen_content("test-ct-1");
    if (!protocol.encrypt("test-ct-1/data.csv", "test-ct-1/", true, pk, context))
        return -1;
    gen_content("test-ct-2");
    if (!protocol.encrypt("test-ct-2/data.csv", "test-ct-2/", true, pk, context))
        return -1;

    std::vector <std::string> inputDirs;
    inputDirs.push_back("test-ct-1");
    inputDirs.push_back("test-ct-2");
    if (!protocol.evaluate(inputDirs, "test-out", {"30"}, pk, context)) {
        std::cerr << "Failed in evaluate" << std::endl;
        return -1;
    }
    if (!protocol.decrypt("test-out/" + core::core_setting.evaluated_file, ".", pk, sk, context)) {
        std::cerr << "Failed in decrypt" << std::endl;
        return -1;
    }
    if (!check(".")) {
        std::cerr << "Failed in check" << std::endl;
        return -1;
    }
    return 0;
}
