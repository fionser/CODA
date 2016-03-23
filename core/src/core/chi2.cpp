#include <fstream>
#include <algorithm>
#include <HElib/FHE.h>
#include <core/core.hpp>
#include <spdlog/spdlog.h>
#include <core/global.hpp>
#include <core/literal.hpp>
#include <core/protocol.hpp>
#include <core/file_util.hpp>

namespace protocol {
namespace chi2 {
const core::FHEArg _fheArgs = {64, 1031, 2, 5};
enum class dataType_t { GENOTYPE, PHENOTYPE, UNKNOWN };
std::string _genotype_data = "data_type genotype_data";
std::string _phenotype_data = "data_type phenotype_data";

class UserEncDataLoader {
public:
    UserEncDataLoader() {};
    dataType_t checkType(const std::string &dir);
    void load(const std::string &dir,
              core::pk_ptr pk,
              std::list<Ctxt> &out);
};

static dataType_t getDataType(const std::string path) {
    util::Meta meta;
    bool ok;
    std::tie(meta, ok) = util::readMetaFile(path);
    if (!ok) {
        L_WARN(global::_console, "Can not open file {0}", path);
        return dataType_t::UNKNOWN;
    }

    auto kv = meta.find("data_type");
    if (kv == meta.end()) {
        L_WARN(global::_console, "Can not get the data type from {0}", path);
        return dataType_t::UNKNOWN;
    }

    if (kv->second.at(0).compare("genotype_data") == 0)
        return dataType_t::GENOTYPE;

    if (kv->second.at(0).compare("phenotype_data") == 0)
        return dataType_t::PHENOTYPE;
    L_WARN(global::_console, "Might be invalid .done.lock file in {0}", path);
    return dataType_t::UNKNOWN;
}

static bool createDoneFile(const std::string &path, const std::string type) {
    auto doneFd = util::createDoneFile(path);
    if (!doneFd) {
        L_WARN(global::_console, "Can not create .done.lock in {0}", path);
        return false;
    }

    fwrite(type.c_str(), type.size(), 1UL, doneFd);
    fclose(doneFd);
    return true;
}

static NTL::ZZX forwardPack(const std::vector<long> coeffs) {
    NTL::ZZX poly;
    poly.SetLength(coeffs.size());
    for (size_t i = 0; i < coeffs.size(); i++)
        NTL::SetCoeff(poly, i, coeffs.at(i));

    return poly;
}

static NTL::ZZX backwardPack(std::vector<long> coeffs) {
    NTL::ZZX poly;
    poly.SetLength(coeffs.size());

    NTL::SetCoeff(poly, 0, coeffs.front());
    auto itr = coeffs.begin();
    std::advance(itr, 1);
    std::reverse(itr, coeffs.end());
    for (long i = 1; i < coeffs.size(); i++)
        NTL::SetCoeff(poly, i, -coeffs.at(i));

    return poly;
}

static NTL::ZZX randomPoly(long n) {
    NTL::ZZX poly;
    poly.SetLength(n);
    NTL::SetCoeff(poly, 0, 0);
    long pR = NTL::power_long(_fheArgs.p, _fheArgs.r);
    for (long i = 1; i < n; i++)
        NTL::SetCoeff(poly, i, NTL::RandomBnd(pR));
    return poly;
}

static bool encryptPhenotype(std::fstream &fin,
                             const std::string &outputFilePath,
                             core::pk_ptr pk) {
    const long n = _fheArgs.m >> 1;
    std::vector<long> coeffs(n, 0);
    size_t pos;
    for (std::string line; std::getline(fin, line); ) {
        long id = literal::stol(line, &pos, 10);
        long ph = literal::stol(line.substr(pos), &pos, 10);
        if (id > n || id <= 0) {
            L_WARN(global::_console, "With ID {0} > n ({1})", id, n);
            continue;
        }
        if (ph != 0 && ph != 1) {
            L_WARN(global::_console, "Invalid line: {0}", line);
            continue;
        }
        coeffs.at(id - 1) = ph;
    }

    auto poly = backwardPack(coeffs);
    Ctxt cipher(*pk);
    pk->Encrypt(cipher, poly);
    std::fstream fout(util::concatenate(outputFilePath, "FILE_1"),
	 	      std::ios::binary | std::ios::out);
    fout << cipher;
    fout.close();

    createDoneFile(outputFilePath, _phenotype_data);
    return true;
}

static bool encryptGenotype(std::fstream &fin,
                            const std::string &outputFilePath,
                            core::pk_ptr pk) {
    const long n = _fheArgs.m >> 1;
    std::vector<long> coeff(n);
    size_t pos;
    for (std::string line; std::getline(fin, line); ) {
        long id = literal::stol(line, &pos, 10);
        long gh = literal::stol(line.substr(pos), &pos, 10);
        if (id > n || id <= 0) {
            L_WARN(global::_console, "With ID {0} > n {1}", id, n);
            continue;
        }
        if (!(gh >= 0 && gh <= 2)) {
            L_WARN(global::_console, "Invalid line {0} in genotype file", line);
            continue;
        }
        coeff.at(id - 1) = gh;
    }

    auto poly = forwardPack(coeff);
    Ctxt cipher(*pk);
    pk->Encrypt(cipher, poly);
    std::fstream fout(util::concatenate(outputFilePath, "FILE_1"),
		      std::ios::binary | std::ios::out);
    fout << cipher;
    fout.close();

    createDoneFile(outputFilePath, _genotype_data);
    return true;
}

bool encrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk) {
    std::fstream fin(inputFilePath);
    if (!fin.is_open()) {
        L_ERROR(global::_console, "Can not open file {0}", inputFilePath);
        return false;
    }

    std::string line;
    std::getline(fin, line);
    if (line.find("#protocol PROT_") == std::string::npos) {
        L_ERROR(global::_console,
                "Invalid file format {0}: no valid protocol type was set",
                inputFilePath);
        return false;
    }

    std::getline(fin, line);
    if (line.find("#type phenotype") != std::string::npos)
        return encryptPhenotype(fin, outputFilePath, pk);
    else if (line.find("#type genotype") != std::string::npos)
        return encryptGenotype(fin, outputFilePath, pk);
    return false;
}

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk) {
    std::list<Ctxt> ciphers;
    if (!core::loadCiphers(ciphers, pk, inputFilePath))
        return false;

    auto oFile = util::concatenate(outputFilePath, "FILE_1");
    std::fstream ostream(oFile, std::ios::out | std::ios::binary);
    if (!ostream.is_open()) {
        L_WARN(global::_console, "Can not open {0}", oFile);
        return false;
    }

    NTL::ZZX poly;
    for (auto &c : ciphers) {
        sk->Decrypt(poly, c);
        ostream << poly[0] << "\n";
    }
    ostream.close();

    auto fd = util::createDoneFile(outputFilePath);
    fwrite("DONE\n", 5UL, 1UL, fd);
    fclose(fd);
    return true;
}

static bool _evaluate(const std::list<Ctxt> &gs,
                      const std::list<Ctxt> &ps,
                      std::list<Ctxt> &rets) {
    Ctxt g(gs.front());
    Ctxt p(ps.front());

    for (auto it = gs.cbegin(); it != gs.cend(); ++it) {
        if (it != gs.cbegin())
            g += *it;
        else
            continue;
    }

    for (auto it = ps.cbegin(); it != ps.cend(); ++it) {
        if (it != ps.cbegin())
            p += *it;
        else
            continue;
    }

    const size_t n = static_cast<size_t>(_fheArgs.m >> 1);
    std::vector<long> ones(n, 1L);
    auto fOne = forwardPack(ones);
    auto bOne = backwardPack(ones);

    Ctxt gp(g);
    gp.multiplyBy(p);
    gp.addConstant(randomPoly(n));

    Ctxt g1(g);
    g1.multByConstant(bOne);
    g1.addConstant(randomPoly(n));

    Ctxt p1(p);
    p1.multByConstant(fOne);
    p1.addConstant(randomPoly(n));

    rets.push_back(gp);
    rets.push_back(g1);
    rets.push_back(p1);

    return true;
}

bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              core::pk_ptr pk) {
    std::list<Ctxt> genotype, phenotype;
    UserEncDataLoader loader;
    for (auto dir : inputDirs) {
        switch (loader.checkType(dir)) {
        case dataType_t::GENOTYPE:
            loader.load(dir, pk, genotype);
            break;
        case dataType_t::PHENOTYPE:
            loader.load(dir, pk, phenotype);
            break;
        default:
            L_WARN(global::_console, "Invalid .done.lock file in {0}", dir);
        }
    }

    if (genotype.empty() || phenotype.empty()) {
        L_WARN(global::_console, "Empty genotype data or phenotype data");
        return false;
    }

    std::list<Ctxt> rets;
    if (!_evaluate(genotype, phenotype, rets))
        return false;

    bool ok = core::dumpCiphers(rets, util::concatenate(outputDir, "FILE_result"));
    if (ok) {
        auto fd = util::createDoneFile(outputDir);
        fwrite("DONE\n", 5UL, 1UL, fd);
        fclose(fd);
        return true;
    }
    return false;
}

dataType_t UserEncDataLoader::checkType(const std::string &dir) {
    auto files = util::listDir(dir, util::flag_t::FILE_ONLY);
    for (auto f : files) {
        if (f.compare(global::_doneFileName) == 0)
            return getDataType(util::concatenate(dir, f));
    }

    return dataType_t::UNKNOWN;
}

#define HAS_PREFIX(a, b) (a.find(b) == 0)

void UserEncDataLoader::load(const std::string &dir,
                             core::pk_ptr pk,
                             std::list<Ctxt> &out) {
    auto files = util::listDir(dir, util::flag_t::FILE_ONLY);
    for (auto f : files) {
        if (HAS_PREFIX(f, "FILE_")) {
            L_INFO(global::_console, "Load {0}", util::concatenate(dir, f));
            core::loadCiphers(out, pk, util::concatenate(dir, f));
        }
    }
}
} // namespace chi2
} // namespace protocol
