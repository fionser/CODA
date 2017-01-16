#include "core/protocol-hybrid-contingency.hpp"
#include "core/contingency_table.hpp"
#include "core/global.hpp"
#include "core/core.hpp"
#include "core/ctxt_util.hpp"
#include "core/file_util.hpp"
#include "core/literal.hpp"
#include "HElib/EncryptedArray.h"

#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>

static std::ostream& _toBinary(std::ostream &out, long v) {
    for (int i = 0; i < 32; i++) {
        out << (v & 1);
        v >>= 1;
    }
    return out;
}

class UserCipherLoader {
public:
    typedef std::function<bool(const std::string &dir)> doneFileChecker;

    UserCipherLoader(doneFileChecker checker) : checker(checker) {}
    ~UserCipherLoader() {}

    template<class Container>
        bool loadCiphers(Container &out, const std::string &path, const core::pk_ptr &pk);

    bool loadLocalComputedCipher(Ctxt &out, int32_t &nr, const std::string &path, const core::pk_ptr &pk);
private:
    doneFileChecker checker;
};

/// the index of attributes starts from 1.
#define ATTR_INDEX(d) (d - 1)

class CTable {
private:
    typedef std::pair<int, int> _key;

public:
    CTable() : _pSze(0), _qSze(0) {}

    ~CTable() {}

    void setSize(long pSize, long qSize) {
        _pSze = pSize;
        _qSze = qSize;
        std::map<_key, long> ().swap(_data);
    }

    long size() const {
        return _pSze * _qSze;
    }

    long sizeP() const { return _pSze; }

    long sizeQ() const { return _qSze; }

    void count(int pp, int qq) {
        add(pp, qq, 1);
    }

    void add(int pp, int qq, int value) {
        if (pp < 0 || qq < 0 || value <= 0) return;
        _key k = std::make_pair(pp, qq);
        long *ptr;
        if (contain(k, ptr))
            *ptr += value;
        else
            _data.insert({k, value});
    }

    void print(std::ostream &out) const {
        for (auto &kv : _data)
            out << kv.first.first << "," << kv.first.second << ":" << kv.second << "\n";
    }

    std::vector<NTL::ZZX> encode(const EncryptedArray *ea) const {
        long sze = size();
        long phim = ea->getAlMod().getZMStar().getPhiM();
        long nr_of_parts = (sze + phim - 1) / phim;
        std::vector<NTL::ZZX> polys(nr_of_parts);
        for (long i = 0; i < nr_of_parts; i++)
            polys.at(i).SetLength(phim);

        for (const auto &kv : _data) {
            int i = kv.first.first;
            int j = kv.first.second;
            long v = kv.second;
            long index = ATTR_INDEX(i) * _qSze + ATTR_INDEX(j);
            long part = index / phim;
            long offset = index % phim;
            NTL::SetCoeff(polys.at(part), offset, v);
        }
        return polys;
    }

private:
    bool contain(const _key &k, const long *val) const {
        if (!val) return false;

        auto v1 = _data.find(k);
        if (v1 == _data.end())
            return false;

        val = &(v1->second);
        return true;
    }
private:
    long _pSze, _qSze;
    std::map<_key, long> _data;
};

namespace contingency_table {
// typedef std::vector<long> CTable;

class HybridProtocolImp {
public:
    // the contingency table of the attribute 't'
    // in the view of attribute 'p' and 'q' with cell suppression threshold.
    HybridProtocolImp(int p, int q, int t, long threshold)
        : _p(p), _q(q), _t(t), _threshold(threshold) {
    }

    ~HybridProtocolImp() {
    }

    bool encrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 bool local_compute,
                 core::pk_ptr pk,
                 core::context_ptr context) {
        if (!local_compute) {
            L_ERROR(global::_console, "Works only with the local_compute flag.");
            return false;
        }

        CTable table;
        if (!convertDataToTable(table, inputFilePath)) {
            L_ERROR(global::_console, "Can not convert file {0}.", inputFilePath);
            return false;
        }

        std::vector<Ctxt> ctxts;
        if (!encryptTableAsPoly(ctxts, table, pk, context)) {
            L_ERROR(global::_console, "Error happend when to encrypt {0}.",
                    inputFilePath);
            return false;
        }

        if (!core::dumpCtxts(ctxts, outputDirPath)) {
            L_ERROR(global::_console, "Error happended when to output to {0}.",
                    outputDirPath);
            return false;
        }

        return createDoneFile(table.sizeP(), table.sizeQ(), outputDirPath);
    }

    bool decrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 core::pk_ptr pk,
                 core::sk_ptr sk,
                 core::context_ptr context) {
        std::vector<Ctxt> ctxts;
        if (!core::loadCiphers(ctxts, pk, inputFilePath)) {
            L_ERROR(global::_console, "Error happened when to load file {0}.",
                    inputFilePath);
            return false;
        }
        std::string doneFile = util::concatenate(util::getDirPath(inputFilePath),
                                                 global::_doneFileName);
        std::ifstream fin(doneFile);
        std::vector<core::Attribute> attributes;
        if (!fin.is_open()) {
            L_WARN(global::_console, "Warnning, absent done file under {0}.",
                   util::getDirPath(inputFilePath));
            return false;
        }

        attributes = parseHeader(fin);
        if (attributes.size() != 2) {
            L_WARN(global::_console, "Invalid done file under {0}.",
                   util::getDirPath(inputFilePath));
            return false;
        }

        std::vector<NTL::ZZX> polys(ctxts.size());
        for (long i = 0; i < ctxts.size(); i++)
            sk->Decrypt(polys[i], ctxts[i]);

        std::string resultFile = util::concatenate(outputDirPath,
                                                   core::core_setting.decrypted_file);
        long phiM = context->alMod.getZMStar().getPhiM();
        long tableSize = attributes[0].size * attributes[1].size;
        if (!dumpDecryptedContent(resultFile, polys, phiM, tableSize)) {
            L_ERROR(global::_console, "Error happened when to dump the decrypted content.");
            return false;
        }

        return createDoneFile(attributes[0].size, attributes[1].size, outputDirPath);
    }

    bool evaluate(const std::vector<std::string> &inputDirs,
                  const std::string &outputDir,
                  const std::vector<std::string> &params,
                  core::pk_ptr pk,
                  core::context_ptr context) {
        if (inputDirs.empty())
            return false;
        std::string doneFile = util::concatenate(inputDirs.front(), global::_doneFileName);
        std::ifstream fin(doneFile);
        if (!fin.is_open()) {
            L_ERROR(global::_console, "Can not open the done file {0}.", doneFile);
            return false;
        }

        auto twoAttributes = parseHeader(fin);
        fin.close();
        if (twoAttributes.size() != 2) {
            L_ERROR(global::_console, "Invalid done file {0}.", doneFile);
            return false;
        }

        std::vector<Ctxt> evaluationResult;
        if (!doEvaluate(evaluationResult, pk, inputDirs))
            return false;

        long tableSize = twoAttributes[0].size * twoAttributes[1].size;
        if (!createRandomShares(evaluationResult, tableSize, context, outputDir))
            return false;

        auto resultFile = util::concatenate(outputDir, core::core_setting.resulting_file);
        if (!core::dumpCiphers(evaluationResult, resultFile)) {
            L_ERROR(global::_console, "Error happend when to dump ciphertexts in {0}", resultFile);
            return false;
        }

        return createDoneFile(twoAttributes[0].size, twoAttributes[1].size, outputDir);
    }

private:
    std::vector<core::Attribute> parseHeader(std::istream &in) const;

    bool convertDataToTable(CTable &table, const std::string &inputFilePath) const;

    bool encryptTableAsPoly(std::vector<Ctxt> &ctxts,
                            const CTable &table,
                            core::pk_ptr pk,
                            core::context_ptr context) const;

    bool createDoneFile(size_t sizeP, size_t sizeQ, const std::string &outputDirPath) const;

    bool dumpDecryptedContent(const std::string &resultFile,
                              const std::vector<NTL::ZZX> ctxts,
                              const long phiM,
                              long tableSize) const;

    bool doEvaluate(std::vector<Ctxt> &summation,
                    const core::pk_ptr pk,
                    const std::vector<std::string> &inputDirs) const;

    bool createRandomShares(std::vector<Ctxt> &evaluationResult,
                            long tableSize,
                            const core::context_ptr context,
                            const std::string &outputDir) const;
    int _p, _q, _t;
    long _threshold;

}; // class HybridProtocolImp

std::vector<core::Attribute> HybridProtocolImp::parseHeader(std::istream &in) const {
    std::vector<core::Attribute> attributes;
    std::string line;
    std::getline(in, line);
    if (line.c_str()[0] == '#') {
        auto attr_sizes = util::splitBySpace(line.substr(1));
        attributes.reserve(attr_sizes.size());
        size_t offset = 0;
        core::Attribute attribute;
        long attr_number = literal::stol(attr_sizes[0]);
        for (size_t i = 1; i < attr_sizes.size(); ++i) {
            const std::string& s(attr_sizes[i]);
            attribute.size = literal::stol(s);
            attribute.offset = offset;
            attribute.type = attribute.size > 0 ? core::Attribute::Type::CATEGORICAL : core::Attribute::Type::NUMERICAL;
            attribute.text = "Unknown";
            attributes.push_back(attribute);
            offset += attribute.size;
        }

        if (attr_number != attributes.size()) {
            L_WARN(global::_console,
                   "Invalid header: need {0} attributes but got {1}",
                   attr_number, attributes.size());
        }
    }
    return attributes;
}

bool HybridProtocolImp::encryptTableAsPoly(std::vector<Ctxt> &ctxts,
                                           const CTable &table,
                                           core::pk_ptr pk,
                                           core::context_ptr context) const {
    if (!pk)
        return false;
    auto polys = table.encode(context->ea);
    Ctxt ctxt(*pk);
    for (auto &poly : polys) {
        pk->Encrypt(ctxt, poly);
        ctxts.push_back(ctxt);
    }
    return !ctxts.empty();
}

bool HybridProtocolImp::convertDataToTable(CTable &table,
                                           const std::string &inputFilePath) const {
    std::ifstream fin(inputFilePath);
    if (!fin.is_open()) {
        L_ERROR(global::_console, "Can not open {0}.", inputFilePath);
        return false;
    }

    auto attributes = parseHeader(fin);
    size_t nr_attributes = attributes.size();
    if (_p > nr_attributes || _q > nr_attributes || _t > nr_attributes) {
        L_ERROR(global::_console, "Invalid attribute index.");
        return false;
    }

    auto P = attributes.at(ATTR_INDEX(_p));
    auto Q = attributes.at(ATTR_INDEX(_q));
    auto T = attributes.at(ATTR_INDEX(_t));
    if (P.type != core::Attribute::Type::CATEGORICAL ||
        Q.type != core::Attribute::Type::CATEGORICAL ||
        T.type != core::Attribute::Type::NUMERICAL) {
        L_ERROR(global::_console, "Invalid attribute type, the 1st and 2nd need categorical, and 3rd needs numerical.");
        return false;
    }
    table.setSize(P.size, Q.size);

    long lineNr = 2;
    for (std::string line; std::getline(fin, line); lineNr += 1) {
        auto fields = util::splitBySpace(line);
        if (fields.size() != nr_attributes) {
            L_WARN(global::_console, "Invalid line (#{0}) of {1}.", lineNr, inputFilePath);
            continue;
        }
        long value_p = literal::stol(fields[ATTR_INDEX(_p)]);
        long value_q = literal::stol(fields[ATTR_INDEX(_q)]);
        long value_t = literal::stol(fields[ATTR_INDEX(_t)]);
        // add this value into the table.
        table.add(value_p, value_q, value_t);
    }
    fin.close();
    return true;
}

bool HybridProtocolImp::doEvaluate(std::vector<Ctxt> &summation,
                                   const core::pk_ptr pk,
                                   const std::vector<std::string> &inputDirs) const {
    auto filter = [](const std::string &fileName) -> bool { return fileName.find("FILE_") == 0; };
    UserCipherLoader loader(filter);
    summation.clear();

    for (const auto &dir : inputDirs) {
        std::vector<Ctxt> ctxts;
        loader.loadCiphers(ctxts, dir, pk);
        if (summation.empty()) {
            summation = ctxts;
        } else {
            if (summation.size() != ctxts.size()) {
                L_ERROR(global::_console, "Mismatch sizes of ciphertexts under :{0}", dir);
                return false;
            }

            for (size_t i = 0; i < ctxts.size(); i++)
                summation[i] += ctxts[i];
        }
    }
    return true;
}

bool HybridProtocolImp::createRandomShares(std::vector<Ctxt> &evaluationResult,
                                           long tableSize,
                                           const core::context_ptr context,
                                           const std::string &outputDir) const {
    if (evaluationResult.empty() || !context) {
        L_ERROR(global::_console, "Invalid method parameters.");
        return false;
    }

    std::ofstream fout(util::concatenate(outputDir, core::core_setting.random_share_file));
    if (!fout.is_open()) {
        L_ERROR(global::_console, "Can not create the random share under {0}.", outputDir);
        return false;
    }
    // sample random polynomails from ZZ_p[X] / (x^n + 1).
    auto randomPolyMaker = [](long p, long n) -> NTL::ZZX {
        NTL::ZZX poly;
        poly.SetMaxLength(n);
        for (long i = 0; i < n; i++)
            NTL::SetCoeff(poly, i, NTL::RandomBnd(p));
        return poly;
    };

    std::vector<NTL::ZZX> shares;
    const long ptxt = evaluationResult.front().getPtxtSpace();
    const long phiM = context->alMod.getZMStar().getPhiM();
    for (auto &c : evaluationResult) {
        auto rndPoly = randomPolyMaker(ptxt, phiM);
        c.addConstant(rndPoly);
        shares.push_back(rndPoly);
    }

    for (auto itr = shares.cbegin(); itr != shares.cend(); itr++) {
        long sze = phiM < tableSize ? phiM : tableSize;
        const NTL::ZZX &poly = *itr;
        for (long i = 0; i < sze; i++)
            _toBinary(fout, NTL::to_long(poly[i]));
        tableSize -= sze;
        assert(tableSize >= 0);
    }
    fout.close();
    return true;
}

bool HybridProtocolImp::createDoneFile(size_t sizeP, size_t sizeQ,
                                       const std::string &outputDirPath) const {
    FILE *fd = util::createDoneFile(outputDirPath);
    if (!fd) {
        L_WARN(global::_console, "Can not create done file under {0}",
               outputDirPath);
        return false;
    }

    std::stringstream sstream;
    sstream << "#2 "; // two attrs.
    // follows by the sizes of these two attrs.
    sstream << sizeP << " " << sizeQ << "\n";
    std::string header = sstream.str();
    fwrite(header.c_str(), header.size(), 1UL, fd);
    fclose(fd);
    return true;
}

bool HybridProtocolImp::dumpDecryptedContent(const std::string &resultFile,
                                             const std::vector<NTL::ZZX> polys,
                                             const long phiM,
                                             long tableSize) const {
    std::ostream* out;
    std::ofstream fout(resultFile);
    if (!fout.is_open()) {
        L_WARN(global::_console, "Can not create file {0}.", resultFile);
        out = &std::cout;
    } else {
        out = &fout;
    }

    for (auto itr = polys.cbegin(); itr != polys.cend(); itr++) {
        long sze = phiM < tableSize ? phiM : tableSize;
        const NTL::ZZX &poly = *itr;
        for (long i = 0; i < sze; i++)
            _toBinary(*out, NTL::to_long(poly[i]));
        tableSize -= sze;
        assert(tableSize >= 0);
    }

    if (out != &std::cout)
        static_cast<std::ofstream *>(out)->close();
    return true;
}
    int _p, _q, _t;
} // namespace contingency_table

HybridContingencyTable::HybridContingencyTable()
    : Protocol("HybridContingencyTableProtocol") {
    isNeedKeySwitching = false;
    imp = nullptr;
}

bool HybridContingencyTable::encrypt(const std::string &inputFilePath,
                                     const std::string &outputDirPath,
                                     bool local_compute,
                                     core::pk_ptr pk,
                                     core::context_ptr context) {
    if (!imp)
        imp = std::make_shared<contingency_table::HybridProtocolImp>(1, 2, 3, 0);
    return imp->encrypt(inputFilePath, outputDirPath, local_compute, pk, context);
}

bool HybridContingencyTable::decrypt(const std::string &inputFilePath,
                                     const std::string &outputDirPath,
                                     core::pk_ptr pk,
                                     core::sk_ptr sk,
                                     core::context_ptr context) {
    if (!imp)
        imp = std::make_shared<contingency_table::HybridProtocolImp>(1, 2, 3, 0);
    return imp->decrypt(inputFilePath, outputDirPath, pk, sk, context);
}

bool HybridContingencyTable::evaluate(const std::vector<std::string> &inputDirs,
                                      const std::string &outputDir,
                                      const std::vector<std::string> &params,
                                      core::pk_ptr pk,
                                      core::context_ptr context) {
    if (!imp)
        imp = std::make_shared<contingency_table::HybridProtocolImp>(1, 2, 3, 0);
    return imp->evaluate(inputDirs, outputDir, params, pk, context);
}

core::FHEArg HybridContingencyTable::parameters() const {
    return { .m = 16384, .p = 8191, .r = 3, .L = 5};
}

template<class Container>
bool UserCipherLoader::loadCiphers(Container &out,
                                   const std::string &path,
                                   const core::pk_ptr &pk)
{
    auto files = util::listDir(path, util::flag_t::FILE_ONLY);
    for (const std::string &file : files) {
        if (!checker(file)) continue;
        std::string cipher_file = util::concatenate(path, file);
        core::loadCiphers(out, pk, cipher_file);
    }
    return true;
}

