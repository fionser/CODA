#include <set>
#include <fstream>
#include <HElib/FHE.h>
#include <algorithm>
#include <core/core.hpp>
#include <spdlog/spdlog.h>
#include <core/global.hpp>
#include <core/literal.hpp>
#include <core/protocol.hpp>
#include <core/file_util.hpp>

namespace protocol {
namespace chi2 {
const core::FHEArg _fheArgs = {.m = 16384, .p = 1031, .r = 2, .L = 5};
enum class dataType_t { GENOTYPE, PHENOTYPE, UNKNOWN };
const char* GENOTYPE_FILE_HEADER = "genotype";
const char* PHENOTYP_FILE_HEADER = "phenotype";
class DoneFileCreator {
public:
    DoneFileCreator() {}
    ~DoneFileCreator() {}
    struct DoneFile {
        dataType_t type;
        size_t N;
    };

    static std::string generate(const DoneFile &df) {
        switch (df.type) {
        case dataType_t::GENOTYPE:
            return generate_genotype(df);
        case dataType_t::PHENOTYPE:
            return generate_phenotype(df);
        default:
            return "";
        }
    }

    static DoneFile createDoneFile(const util::Meta &meta);

private:
    static std::string generate_genotype(const DoneFile &f);
    static std::string generate_phenotype(const DoneFile &f);
};

class UserEncDataLoader {
public:
    UserEncDataLoader() {};
    DoneFileCreator::DoneFile loadDoneFile(const std::string &dir);
    void loadCiphers(const std::string &dir,
                     core::pk_ptr pk,
                     std::list<Ctxt> &out);
};

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
                             const std::string &outputDirPath,
                             core::pk_ptr pk) {
    const long n = _fheArgs.m >> 1;
    std::vector<long> coeffs(n, 0);
    std::set<long> unique_id;
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

        if (unique_id.find(id) != unique_id.end()) {
            L_WARN(global::_console, "ID {0} exists more than one time", id);
            continue;
        }

        coeffs.at(id - 1) = ph;
        unique_id.insert(id);
    }

    auto poly = backwardPack(coeffs);
    Ctxt cipher(*pk);
    pk->Encrypt(cipher, poly);
    std::fstream fout(util::concatenate(outputDirPath, "FILE_1"),
	 	      std::ios::binary | std::ios::out);
    fout << cipher;
    fout.close();

    DoneFileCreator::DoneFile  doneFile = {.type = dataType_t::PHENOTYPE,
                                           .N = unique_id.size()};
    createDoneFile(outputDirPath, DoneFileCreator::generate(doneFile));

    return true;
}

static bool encryptGenotype(std::fstream &fin,
                            const std::string &outputDirPath,
                            core::pk_ptr pk) {
    const long n = _fheArgs.m >> 1;
    std::vector<long> coeff(n);
    std::set<long> unique_id;
    size_t pos;
    for (std::string line; std::getline(fin, line); ) {
        if (line.empty()) continue;

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

        if (unique_id.find(id) != unique_id.end()) {
            L_WARN(global::_console, "ID {0} exists more than one time", id);
            continue;
        }

        coeff.at(id - 1) = gh;
        unique_id.insert(id);
    }

    auto poly = forwardPack(coeff);
    Ctxt cipher(*pk);
    pk->Encrypt(cipher, poly);
    std::fstream fout(util::concatenate(outputDirPath, "FILE_1"),
		      std::ios::binary | std::ios::out);
    fout << cipher;
    fout.close();

    DoneFileCreator::DoneFile  doneFile = {.type = dataType_t::GENOTYPE,
                                              .N = unique_id.size()};
    createDoneFile(outputDirPath, DoneFileCreator::generate(doneFile));
    return true;
}

bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             core::pk_ptr pk,
             core::context_ptr /*dummy*/) {
    std::fstream fin(inputFilePath);
    if (!fin.is_open()) {
        L_ERROR(global::_console, "Can not open file {0}", inputFilePath);
        return false;
    }

    std::string line;
    std::getline(fin, line);
    if (line.find("#protocol PROT_CI2") == std::string::npos) {
        L_ERROR(global::_console,
                "Invalid file format {0}: no valid protocol type was set",
                inputFilePath);
        return false;
    }

    std::getline(fin, line);
    if (line.find("#type phenotype") != std::string::npos)
        return encryptPhenotype(fin, outputDirPath, pk);
    else if (line.find("#type genotype") != std::string::npos)
        return encryptGenotype(fin, outputDirPath, pk);
    return false;
}

static void __output_chi2(std::fstream &fd,
                          const std::array<long, 4> observations)
{
    std::array<long, 4> Os;
    Os[0] = observations[0];
    Os[1] = observations[2] - Os[0];
    Os[2] = observations[1] - Os[0];
    Os[3] = observations[3] - observations[1] - Os[1];

    double _r = observations[2] * 1.0 / (observations[3] - observations[2]);
    std::array<double, 4> Es;
    Es[0] = observations[1] * observations[2] * 1.0 / observations[3];
    Es[1] = (observations[3] - observations[1]) * observations[2] * 1.0 / observations[3];
    Es[2] = Es[0] / _r;
    Es[3] = Es[1] / _r;

    fd << "\t#A\t#a\n";
    fd << "case\t" << Os[0] << "\t" << Os[1] << "\n";
    fd << "control\t" << Os[2] << "\t" << Os[3] << "\n";

    for (auto e : Es) {
        if (std::abs(e) < 1e-8) {
            L_WARN(global::_console, "With expetection 0");
            fd << "chi2 value NA\n";
            return;
        }
    }

    double chi2 = 0.0;
    for (size_t i = 0; i < 4; i++) {
        auto diff = Os[i] - Es[i];
        chi2 = chi2 + (diff * diff) / Es[i];
    }

    fd << "chi2 value " << chi2 << "\n";
}

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk,
             core::context_ptr /*dummy*/) {
    util::Meta doneFile;
    bool ok;
    auto dir = util::getDirPath(inputFilePath);
    auto doneFilePath = util::concatenate(dir, global::_doneFileName);
    std::tie(doneFile, ok) = util::readMetaFile(doneFilePath);
    if (!ok) {
        L_WARN(global::_console, "No .done.lock under {0}", dir);
        return false;
    }

    auto kv = doneFile.find("N");
    if (kv == doneFile.end()) {
        L_WARN(global::_console, "Invalid .done.lock under {0}", dir);
        return false;
    }

    long nr_patients = literal::stol(kv->second.front());

    std::list<Ctxt> ciphers;
    if (!core::loadCiphers(ciphers, pk, inputFilePath))
        return false;

    if (ciphers.size() != 3) {
        L_ERROR(global::_console, "Invalid input file {0}", inputFilePath);
        return false;
    }

    auto oFile = util::concatenate(outputFilePath, "FILE_1");
    std::fstream ostream(oFile, std::ios::out | std::ios::binary);
    if (!ostream.is_open()) {
        L_WARN(global::_console, "Can not open {0}", oFile);
        return false;
    }

    NTL::ZZX poly;
    std::array<long, 4> obs;
    size_t idx = 0;
    for (auto &c : ciphers) {
        sk->Decrypt(poly, c);
        obs[idx++] = NTL::to_long(poly[0]);
    }
    obs[3] = nr_patients << 1;
    __output_chi2(ostream, obs);

    auto fd = util::createDoneFile(outputFilePath);
    fwrite("DONE\n", 5UL, 1UL, fd);
    fclose(fd);
    return true;
}

static bool __evaluate(const std::list<Ctxt> &gs,
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

static ssize_t __loadCiphers(std::list<Ctxt> &gs, std::list<Ctxt> &ps,
                             core::pk_ptr pk,
                             const std::vector<std::string> &inputDirs) {
    ssize_t nr_geno = 0, nr_pheno = 0;
    UserEncDataLoader loader;
    for (auto dir : inputDirs) {
        auto df = loader.loadDoneFile(dir);
        switch (df.type) {
        case dataType_t::GENOTYPE:
	    nr_geno += static_cast<ssize_t>(df.N);
            loader.loadCiphers(dir, pk, gs);
            break;
        case dataType_t::PHENOTYPE:
	    nr_pheno += static_cast<ssize_t>(df.N);
            loader.loadCiphers(dir, pk, ps);
            break;
        default:
            L_WARN(global::_console, "Invalid .done.lock file in {0}", dir);
            return -1;
        }
    }

    if (nr_pheno != nr_geno) {
        L_WARN(global::_console, "Mismatching the number of patients {0} ≠ {1}",
	       nr_geno, nr_pheno);
        return -1;
    }

    if (gs.empty() || ps.empty()) {
        L_WARN(global::_console, "Empty genotype data or phenotype data");
        return 0;
    }

    return nr_geno;
}

bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              const std::vector<std::string> &,
              core::pk_ptr pk,
              core::context_ptr /*dummy*/) {
    std::list<Ctxt> genotype, phenotype;
    ssize_t nr_patients;
    nr_patients = __loadCiphers(genotype, phenotype, pk, inputDirs);
    if (nr_patients <= 0)
        return false;

    std::list<Ctxt> rets;
    if (!__evaluate(genotype, phenotype, rets))
        return false;

    bool ok = core::dumpCiphers(rets, util::concatenate(outputDir, "File_result"));
    if (ok) {
        auto fd = util::createDoneFile(outputDir);
        char buf[1024];
        snprintf(buf, sizeof(buf), "N %zd\n", nr_patients);
        fwrite(buf, strlen(buf), 1UL, fd);
        fclose(fd);
        return true;
    }
    return false;
}


DoneFileCreator::DoneFile UserEncDataLoader::loadDoneFile(const std::string &dir) {
    auto files = util::listDir(dir, util::flag_t::FILE_ONLY);
    DoneFileCreator::DoneFile df;
    for (auto &f : files) {
        if (f.compare(global::_doneFileName) == 0) {
            util::Meta meta;
            auto doneFile = util::concatenate(dir, f);
            bool ok;
            std::tie(meta, ok) = util::readMetaFile(doneFile);
            if (!ok) {
                L_WARN(global::_console, "Can not load file {0}", doneFile);
                df.type = dataType_t::UNKNOWN;
                return df;
            }
            return DoneFileCreator::createDoneFile(meta);
        }
    }
    return df;
}

#define HAS_PREFIX(a, b) (a.find(b) == 0)

void UserEncDataLoader::loadCiphers(const std::string &dir,
                                    core::pk_ptr pk,
                                    std::list<Ctxt> &out) {
    auto files = util::listDir(dir, util::flag_t::FILE_ONLY);
    for (auto f : files) {
        if (HAS_PREFIX(f, "FILE_"))
            core::loadCiphers(out, pk, util::concatenate(dir, f));
    }
}

DoneFileCreator::DoneFile
DoneFileCreator::createDoneFile(const util::Meta &meta) {
    DoneFile df = {.type = dataType_t::UNKNOWN, .N = 0UL };
    auto kv = meta.find("data_type");
    if (kv == meta.end()) {
        L_WARN(global::_console,
               "Invalid done file without setting the filed [data_type]");
        return df;
    }

    if (kv->second.front().compare(PHENOTYP_FILE_HEADER) == 0)
        df.type = dataType_t::PHENOTYPE;
    else if (kv->second.front().compare(GENOTYPE_FILE_HEADER) == 0)
        df.type = dataType_t::GENOTYPE;

    kv = meta.find("N");
    if (kv != meta.end())
        df.N = static_cast<size_t>(std::stol(kv->second.front()));

    return df;
}

std::string
DoneFileCreator::generate_phenotype(const DoneFileCreator::DoneFile &f) {
    char buf[1024 * 1024];
    snprintf(buf, sizeof(buf), "data_type %s\nN %zd", PHENOTYP_FILE_HEADER, f.N);
    return std::string(buf);
}

std::string
DoneFileCreator::generate_genotype(const DoneFileCreator::DoneFile &f) {
    char buf[1024 * 1024];
    snprintf(buf, sizeof(buf), "data_type %s\nN %zd", GENOTYPE_FILE_HEADER, f.N);
    return std::string(buf);
}

} // namespace chi2
} // namespace protocol
