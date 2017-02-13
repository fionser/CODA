#include "core/protocol-PCA.hpp"
#include "core/coda_wrapper.hpp"
#include "core/literal.hpp"
#include "core/global.hpp"
#include "core/file_util.hpp"
#include "core/PPE/PubKey.hpp"
#include "core/PPE/SecKey.hpp"
#include "core/PPE/Context.hpp"

#include <list>
#include <vector>
#include <fstream>
#include <NTL/mat_ZZ.h>
typedef NTL::mat_ZZ Matrix;
class PCAProtocol::Imp {
public:
    Imp() {}
    ~Imp() {}

    bool encrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 bool local_compute,
                 ppe::pk_ptr pk,
                 ppe::context_ptr context) {
        std::ifstream in(inputFilePath, std::ios::binary);
        if (!in.is_open()) {
            L_WARN(global::_console, "Can not open {0}", inputFilePath);
            return false;
        }

        std::vector<long> magnifications = parseHeader(in);
        if (magnifications.empty()) {
            L_WARN(global::_console, "Invalid header of {0}", inputFilePath);
            in.close();
            return false;
        }
        Matrix X = readData(in, magnifications.size());
        if (X.NumCols() == 0)
            return false;
        long max_m = *std::max_element(magnifications.begin(), magnifications.end());
        scale(X, magnifications);
        Matrix XTX;
        NTL::mul(XTX, NTL::transpose(X), X);
        return true;
    }

    bool decrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 ppe::pk_ptr pk,
                 ppe::sk_ptr sk,
                 ppe::context_ptr context) {
        return false;
    }

    bool evaluate(const StringList &inputDirs,
                  const std::string &outputDir,
                  const StringList &params,
                  ppe::pk_ptr pk,
                  ppe::context_ptr context) {
        return false;
    }

private:
    std::vector<long> parseIntegers(const std::string &line) {
        std::vector<long> ret;
        auto fileds = util::splitBySpace(line);
        for (const std::string &field : fileds) {
            size_t pos;
            long val = literal::stol(field, &pos,10);
            if (pos != field.size())
                return std::vector<long>();
            else
                ret.push_back(val);
        }
        return ret;
    }

    std::vector<long> parseHeader(std::ifstream &in) {
        if (in.eof())
            return std::vector<long>();

        std::string line;
        std::getline(in, line);
        if (line.find("#") != 0)
            return std::vector<long>();

        return parseIntegers(line.substr(1));
    }

    Matrix readData(std::ifstream &in, const long num_features) {
        std::list<std::vector<long>> rows;
        for (std::string line; std::getline(in, line); ) {
            std::vector<long> row = parseIntegers(line);
            if (row.size() != num_features) {
                L_WARN(global::_console, "Invalid line {0}: requires {1} features", line, num_features);
                return Matrix();
            }
            rows.push_back(row);
        }

        Matrix ret;
        ret.SetDims(rows.size(), num_features);
        long row_index = 0;
        for (const auto &row : rows) {
            for (long j = 0; j < num_features; j++)
                ret[row_index][j] = row[j];
            row_index += 1;
        }
        return ret;
    }

    void scale(Matrix &m, const std::vector<long> &magnifications) {
        long max_m = *std::max_element(magnifications.begin(), magnifications.end());
        std::vector<NTL::ZZ> factors;
        for (long c = 0; c < m.NumCols(); c++) {
            long exp = max_m - magnifications[c];
            factors.push_back(NTL::power(NTL::to_ZZ(10), exp));
        }

        for (long r = 0; r < m.NumRows(); r++) {
            for (long c = 0; c < m.NumCols(); c++) {
                m[r][c] *= factors[c];
            }
        }
    }
};

PCAProtocol::PCAProtocol() : Protocol("PCA") {
    imp_ = std::make_shared<PCAProtocol::Imp>();
}

bool PCAProtocol::encrypt(const std::string &inputFilePath,
                          const std::string &outputDirPath,
                          bool local_compute,
                          const core::PubKeyWrapper &pk,
                          const core::ContextWrapper &context)
{
    if (!imp_) return false;
    return imp_->encrypt(inputFilePath, outputDirPath,
                         local_compute, pk.ppe, context.ppe);
}

bool PCAProtocol::decrypt(const std::string &inputFilePath,
                          const std::string &outputDirPath,
                          const core::PubKeyWrapper &pk,
                          const core::SecKeyWrapper &sk,
                          const core::ContextWrapper &context)
{
    if (!imp_) return false;
    return imp_->decrypt(inputFilePath, outputDirPath,
                         pk.ppe, sk.ppe, context.ppe);
}

bool PCAProtocol::evaluate(const StringList &inputDirs,
                           const std::string &outputDir,
                           const StringList &params,
                           const core::PubKeyWrapper &pk,
                           const core::ContextWrapper &context)
{

    if (imp_) return false;
    return imp_->evaluate(inputDirs, outputDir,
                          params, pk.ppe, context.ppe);
}

core::FHEArg PCAProtocol::parameters() const {
    assert(0 && "Should not use this function");
}

bool PCAProtocol::genKeypair() const {
    std::string dirPath = util::getDirPath(metaPath_);
    std::ofstream skStream(util::concatenate(dirPath, "fhe_key.sk"), std::ios::binary);
    std::ofstream ctxtStream(util::concatenate(dirPath, "fhe_key.ctxt"), std::ios::binary);
    std::ofstream pkStream(util::concatenate(dirPath, "fhe_key.pk"), std::ios::binary);
    if (!skStream.is_open() || !ctxtStream.is_open() || !pkStream.is_open()) {
        L_WARN(global::_console, "Can not create files under {0}", dirPath);
        return false;
    }

    const std::vector<long> Ms = {16384};
    const std::vector<long> Ps = {8191};
    const std::vector<long> Rs = {1};
    const long L = 10;

    ppe::Context context(Ms, Ps, Rs);
    context.buildModChain(L);
    ppe::SecKey sk(context);
    sk.GenSecKey(64);
    if (isNeedKeySwitching)
        sk.addSome1DMatrices();
    ppe::PubKey pk(sk);

    skStream << TYPE_PPE;
    sk.dump(skStream);

    ctxtStream << TYPE_PPE;
    context.dump(ctxtStream);

    pkStream << TYPE_PPE;
    pk.dump(pkStream);

    skStream.close();
    ctxtStream.close();
    pkStream.close();
    return true;
}

