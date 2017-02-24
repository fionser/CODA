#include "core/protocol-LR.hpp"
#include "core/coda_wrapper.hpp"
#include "core/literal.hpp"
#include "core/global.hpp"
#include "core/file_util.hpp"
#include "core/PPE/PPE.hpp"

#include <NTL/vec_ZZ.h>
#include <NTL/matrix.h>
#include <list>
#include <fstream>
#include <sstream>
#include <core/coda.hpp>

typedef NTL::mat_ZZ Matrix;
typedef NTL::vec_ZZ Vector;
template<typename T>
static std::pair<double, double> _mean_std(const NTL::Mat<T> &mat, const long colNum);
class LRProtocol::Imp {
public:
    Imp() {}
    ~Imp() {}

    bool encrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 bool /*local_compute*/,
                 ppe::pk_ptr pk,
                 ppe::context_ptr context) {
        std::ifstream in(inputFilePath, std::ios::binary);
        if (!in.is_open()) {
            L_WARN(global::_console, "Can not open {0}", inputFilePath);
            return false;
        }

        std::string outputFile = util::concatenate(outputDirPath, "FILE_1");
        std::ofstream out(outputFile, std::ios::binary);
        if (!out.is_open()) {
            L_WARN(global::_console, "Can not create {0}", outputFile);
            in.close();
            return false;
        }

        std::vector<long> magnifications = parseHeader(in);
        if (magnifications.empty()) {
            L_WARN(global::_console, "Invalid header of {0}", inputFilePath);
            in.close();
            return false;
        }

        Matrix X;
        Vector y;
        bool ok = readData(&X, &y, in, magnifications.size());
        if (!ok) {
            L_WARN(global::_console, "Some went wrong when to read file {0}", inputFilePath);
            return false;
        }

        auto mag_for_X = magnifications;
        mag_for_X.pop_back();
        Matrix scaledX;
        scale(&scaledX, X, mag_for_X);

        Matrix XtX; // X.T * X
        NTL::mul(XtX, NTL::transpose(scaledX), scaledX);
        ppe::EncMat encMat(*pk);
        encMat.pack(XtX);
        ok = encMat.dump(out);

        Vector Xty; // X.T * y
        NTL::mul(Xty, NTL::transpose(scaledX), y);
        ppe::EncVec encVec(*pk);
        encVec.pack(Xty);
        ok &= encVec.dump(out);

        if (ok)
            createDoneFileForEncrypt(outputDirPath, magnifications);
        in.close();
        out.close();
        return ok;
    }

    bool decrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 ppe::pk_ptr pk,
                 ppe::sk_ptr sk,
                 ppe::context_ptr context) {
        std::ifstream in(inputFilePath, std::ios::binary);
        if (!in.is_open()) {
            L_WARN(global::_console, "Can not open {0}.", inputFilePath);
            return false;
        }

        std::string outputFilePath = util::concatenate(outputDirPath, core::core_setting.decrypted_file);
        std::ofstream out(outputFilePath, std::ios::binary);
        if (!out.is_open()) {
            L_WARN(global::_console, "Can not open {0}.", outputFilePath);
            return false;
        }

        ppe::EncVec vec(*pk);
        vec.restore(in);

        Vector pVec;
        vec.unpack(pVec, *sk, /*negate=*/true);
        out << pVec << std::endl;
        FILE *done = util::createDoneFile(outputDirPath);
        fclose(done);
        in.close();
        out.close();
        return true;
    }

    // sum the client ciphertext and apply Powermethod to calculate the largest eigvalue.
    bool evaluate(const StringList &inputDirs,
                  const std::string &outputDir,
                  const StringList &params,
                  ppe::pk_ptr pk,
                  ppe::context_ptr context) {
        if (params.empty()) {
            L_WARN(global::_console, "Need the largest eigvalue to evaluate.");
            return false;
        }

        std::string saveTo = util::concatenate(outputDir, core::core_setting.evaluated_file);
        std::ofstream out(saveTo, std::ios::binary);
        if (!out.is_open()) {
            L_WARN(global::_console, "Can not open {0}.", saveTo);
            return false;
        }

        ppe::EncMat XtX(*pk);
        ppe::EncVec Xty(*pk);
        const std::string specificFile = "FILE_1";
        for (const std::string &clientDir : inputDirs) {
            auto files = util::listDir(clientDir, util::flag_t::FILE_ONLY);
            for (const auto &file : files) {
                if (file.compare(specificFile) != 0) continue;
                auto ctxt_file = util::concatenate(clientDir, file);
                std::ifstream in(ctxt_file, std::ios::binary);
                if (!in.is_open()) {
                    L_WARN(global::_console, "Can not open {0}", ctxt_file);
                    continue;
                }

                ppe::EncMat tmpMat(*pk);
                ppe::EncVec tmpVec(*pk);
                bool ok = tmpMat.restore(in);
                if (!ok) L_WARN(global::_console, "Can not load matrix from {0}", ctxt_file);
                XtX.add(tmpMat);

                ok = tmpVec.restore(in);
                if (!ok) L_WARN(global::_console, "Can not load vector from {0}", ctxt_file);
                Xty.add(tmpVec);
                in.close();
            }
        }

        long lambda = literal::stol(params.front(), NULL, 10);

        ppe::EncMat inv(*pk);
        if (!inverse(&inv, XtX, NTL::to_ZZ(lambda))) {
            L_WARN(global::_console, "Some went wrong when to inverse the matrix");
            return false;
        }
        ppe::EncVec w = inv.sym_dot(Xty);
        w.dump(out);
        out.close();
        FILE *fd = util::createDoneFile(outputDir);
        fclose(fd);
        return true;
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

    bool readData(Matrix *X, Vector *y, std::ifstream &in, const long num_features) {
        assert(num_features > 1 && "Need more than 1 dimension for regression");
        std::list<std::vector<long>> rows;
        std::vector<long> last_column;
        for (std::string line; std::getline(in, line); ) {
            std::vector<long> row = parseIntegers(line);
            if (row.size() != num_features) {
                L_WARN(global::_console, "Invalid line {0}: requires {1} features", line, num_features);
                return false;
            }
            last_column.push_back(row.back());
            row.pop_back();
            rows.push_back(row);
        }

        X->SetDims(rows.size(), num_features - 1);
        y->SetLength(rows.size());
        long row_index = 0;
        for (const auto &row : rows) {
            for (size_t j = 0; j < row.size(); j++)
                (*X)[row_index][j] = row[j];
            (*y)[row_index] = last_column[row_index];
            row_index += 1;
        }
        return true;
    }

    void scale(Matrix *out, const Matrix &mat, const std::vector<long> &mags) {
        const long DIGIT_PERSERVE = NTL::power_long(10, 3);
        std::vector<long> factors;
        for (long c = 0; c < mat.NumCols(); c++) {
            factors.push_back(NTL::power_long(10, mags[c]));
        }

        NTL::Mat<double> dMat;
        dMat.SetDims(mat.NumRows(), mat.NumCols());
        for (long r = 0; r < mat.NumRows(); r++) {
            for (long c = 0; c < mat.NumCols(); c++) {
                dMat[r][c] = NTL::to_double(mat[r][c]) / factors[c];
            }
        }

        std::vector<std::pair<double, double>> mean_stds;
        for (long c = 0; c < mat.NumCols(); c++) {
            mean_stds.push_back(_mean_std(dMat, c));
        }
        out->SetDims(mat.NumRows(), mat.NumCols());
        for (long r = 0; r < mat.NumRows(); r++) {
            for (long c = 0; c < mat.NumCols(); c++) {
                double normalized = DIGIT_PERSERVE * (dMat[r][c] - mean_stds[c].first) / mean_stds[c].second;
                (*out)[r][c] = NTL::to_ZZ(static_cast<long>(normalized));
            }
        }
    }

    bool inverse(ppe::EncMat *R, const ppe::EncMat &Q, NTL::ZZ alpha) const {
        ppe::EncMat A(Q);
        (*R) = Q.copyAsEmpty();
        Matrix I;
        I.SetDims(Q.rowNums(), Q.colNums());
        for (long r = 0; r < I.NumRows(); r++) I[r][r] = 2;

        for (long T = 0; T < 2; T++) {
            Matrix Alpha = I * alpha;
            auto inner(A);
            inner.negate();
            inner.add(Alpha); // inner^(t) = 2 * alpha * I - A^(t)

            if (T > 0) {
                R->dot(inner);
            } else {
                (*R) = inner;
            }
            A.dot(inner);
            alpha *= alpha;
        }

        return true;
    }

    void createDoneFileForEncrypt(const std::string outDir, const std::vector<long> &mags) const {
        FILE *fd = util::createDoneFile(outDir);
        if (!fd)
            return;
        std::stringstream sstream("#");
        for (size_t i = 0; i + 1 < mags.size(); i++)
            sstream << mags[i] << " ";
        if (!mags.empty())
            sstream << mags.back();
        std::string header = sstream.str();
        fwrite(header.c_str(), header.size(), 1UL, fd);
        fclose(fd);
    }
};

LRProtocol::LRProtocol() : Protocol("PCA") {
    imp_ = std::make_shared<LRProtocol::Imp>();
}

bool LRProtocol::encrypt(const std::string &inputFilePath,
                         const std::string &outputDirPath,
                         bool local_compute,
                         const core::PubKeyWrapper &pk,
                         const core::ContextWrapper &context)
{
    if (!imp_) return false;
    return imp_->encrypt(inputFilePath, outputDirPath,
                         local_compute, pk.ppe, context.ppe);
}

bool LRProtocol::decrypt(const std::string &inputFilePath,
                         const std::string &outputDirPath,
                         const core::PubKeyWrapper &pk,
                         const core::SecKeyWrapper &sk,
                         const core::ContextWrapper &context)
{
    if (!imp_) return false;
    return imp_->decrypt(inputFilePath, outputDirPath,
                         pk.ppe, sk.ppe, context.ppe);
}

bool LRProtocol::evaluate(const StringList &inputDirs,
                          const std::string &outputDir,
                          const StringList &params,
                          const core::PubKeyWrapper &pk,
                          const core::ContextWrapper &context)
{

    if (!imp_) return false;
    return imp_->evaluate(inputDirs, outputDir, params, pk.ppe, context.ppe);
}

core::FHEArg LRProtocol::parameters() const {
    assert(0 && "Should not use this function");
}

bool LRProtocol::genKeypair() const {
    std::string dirPath = util::getDirPath(metaPath_);
    std::ofstream skStream(util::concatenate(dirPath, "fhe_key.sk"), std::ios::binary);
    std::ofstream ctxtStream(util::concatenate(dirPath, "fhe_key.ctxt"), std::ios::binary);
    std::ofstream pkStream(util::concatenate(dirPath, "fhe_key.pk"), std::ios::binary);
    if (!skStream.is_open() || !ctxtStream.is_open() || !pkStream.is_open()) {
        L_WARN(global::_console, "Can not create files under {0}", dirPath);
        return false;
    }

    /// NOTE:
    /// hard-coding with the ppe pararmeters. Should be better to chose by clients.
    const std::vector<long> Ms = {16384};
    const std::vector<long> Ps = { 4139};
    const std::vector<long> Rs = {    3};
    const long L = 10;

    // const std::vector<long> Ms = {27893, 27893, 27893, 27893, 27893, 27893, 27893, 27893};
    // const std::vector<long> Ps = { 4139,  7321,  5381,  5783,  4231,  4937,  5279, 6679};
    // const std::vector<long> Rs = {    3,     3,     3,     3,     3,     3,     3,    3};
    // const long L = 32;

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

template<typename T>
std::pair<double, double> _mean_std(const NTL::Mat<T> &mat, const long colNum) {
    T sum(0);
    const long N = mat.NumRows();
    assert(N >= 1);
    for (long r = 0; r < mat.NumRows(); r++)
        sum += mat[r][colNum];
    double mean = NTL::to_double(sum) / N;
    double square_sum(0.0);
    for (long r = 0; r < mat.NumRows(); r++) {
        double diff = NTL::to_double(mat[r][colNum]) - mean;
        square_sum += (diff * diff);
    }
    double std = std::sqrt((square_sum) / N);
    return std::make_pair(mean, std);
}
