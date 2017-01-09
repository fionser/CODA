#include "core/protocol-hybrid-contingency.hpp"
#include "core/global.hpp"
#include "core/core.hpp"
#include "core/ctxt_util.hpp"
#include "core/file_util.hpp"
#include "HElib/EncryptedArray.h"

#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>

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

namespace contingency_table {
typedef std::vector<long> CTable;

class HybridProtocolImp {
public:
    HybridProtocolImp() {
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
            L_ERROR(global::_console, "Can not convert file {1}.", inputFilePath);
            return false;
        }

        std::vector<Ctxt> ctxts;
        if (!encryptTableAsPoly(ctxts, table, pk, context)) {
            L_ERROR(global::_console, "Error happend when to encrypt {1}.", inputFilePath);
            return false;
        }

        if (!core::dumpCtxts(ctxts, outputDirPath)) {
            L_ERROR(global::_console, "Error happended when to output to {1}.", outputDirPath);
            return false;
        }

        return true;
    }

    bool decrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 core::pk_ptr pk,
                 core::sk_ptr sk,
                 core::context_ptr context) {
        std::vector<Ctxt> ctxts;
        if (!core::loadCiphers(ctxts, pk, inputFilePath)) {
            L_ERROR(global::_console, "Error happened when to load file {1}.", inputFilePath);
            return false;
        }

        std::string resultFile = util::concatenate(outputDirPath, core::core_setting.decrypted_file);
        std::ostream* out;
        std::ofstream fout(resultFile);
        if (!fout.is_open()) {
            L_WARN(global::_console, "Can not create file {1}.", resultFile);
            out = &std::cout;
        } else {
            out = &fout;
        }

        NTL::ZZX poly;
        for (const Ctxt &ctxt : ctxts) {
            sk->Decrypt(poly, ctxt);
            *out << poly;
        }

        if (out != &std::cout)
            static_cast<std::ofstream *>(out)->close();
        return true;
    }

    bool evaluate(const std::vector<std::string> &inputDirs,
                  const std::string &outputDir,
                  const std::vector<std::string> &params,
                  core::pk_ptr pk,
                  core::context_ptr context) {
        auto checker = [](const std::string &fileName) -> bool { return fileName.find("FILE_") == 0; };
        UserCipherLoader loader(checker);

        std::vector<Ctxt> summation;
        for (const auto &dir : inputDirs) {
            std::vector<Ctxt> ctxts;
            loader.loadCiphers(ctxts, dir, pk);
            if (!aggregateTable(summation, ctxts)) {
                L_ERROR(global::_console, "Error happend when to aggregate ciphertexts in {1}", dir);
                return false;
            }
        }

        auto resultFile = util::concatenate(outputDir, core::core_setting.resulting_file);
        if (!core::dumpCiphers(summation, resultFile)) {
            L_ERROR(global::_console, "Error happend when to dump ciphertexts in {1}", resultFile);
            return false;
        }

        return true;
    }

private:
    bool convertDataToTable(CTable &table, const std::string &inputFilePath) const;

    bool encryptTableAsPoly(std::vector<Ctxt> &ctxts,
                            const CTable &table,
                            core::pk_ptr pk,
                            core::context_ptr context) const;

    bool aggregateTable(std::vector<Ctxt> &aggregatedTable,
                        const std::vector<Ctxt> &ctxts) const;

}; // class HybridProtocolImp

bool HybridProtocolImp::encryptTableAsPoly(std::vector<Ctxt> &ctxts,
                                           const CTable &table,
                                           core::pk_ptr pk,
                                           core::context_ptr context) const {
    if (!pk)
        return false;
    long phim = context->alMod.getZMStar().getPhiM();
    size_t table_size = table.size();
    size_t nr_of_parts = (table_size + phim - 1) / phim;

    size_t itr = 0;
    for (size_t i = 0; i < nr_of_parts; i++) {
        NTL::ZZX poly;
        poly.SetMaxLength(phim);
        for (long j = 0; j < phim && itr < table_size; j++, itr++)
            NTL::SetCoeff(poly, j, table[itr]);
        Ctxt ctxt(*pk);
        pk->Encrypt(ctxt, poly);
        ctxts.emplace_back(ctxt);
    }

    return true;
}

//TODO(riku) This is still a stub to be finalized.
bool HybridProtocolImp::convertDataToTable(CTable &table,
                                           const std::string &inputFilePath) const {
    table.reserve(40000);
    for (long i = 0; i < 40000; i++)
        table.push_back(10);
    return true;
}

bool HybridProtocolImp::aggregateTable(std::vector<Ctxt> &aggregatedTable,
                                       const std::vector<Ctxt> &ctxts) const {
    if (aggregatedTable.empty()) {
        aggregatedTable = ctxts;
    } else {
        if (aggregatedTable.size() != ctxts.size())
            return false;
        for (size_t i = 0; i < ctxts.size(); i++)
            aggregatedTable[i] += ctxts[i];
    }
    return true;
}
} // namespace contingency_table

HybridContingencyTable::HybridContingencyTable()
    : Protocol("HybridContingencyTableProtocol") {
    isNeedKeySwitching = false;
    imp = std::make_shared<contingency_table::HybridProtocolImp>();
}

bool HybridContingencyTable::encrypt(const std::string &inputFilePath,
                                     const std::string &outputDirPath,
                                     bool local_compute,
                                     core::pk_ptr pk,
                                     core::context_ptr context) {
    return imp->encrypt(inputFilePath, outputDirPath, local_compute, pk, context);
}

bool HybridContingencyTable::decrypt(const std::string &inputFilePath,
                                     const std::string &outputDirPath,
                                     core::pk_ptr pk,
                                     core::sk_ptr sk,
                                     core::context_ptr context) {
    return imp->decrypt(inputFilePath, outputDirPath, pk, sk, context);
}

bool HybridContingencyTable::evaluate(const std::vector<std::string> &inputDirs,
                                      const std::string &outputDir,
                                      const std::vector<std::string> &params,
                                      core::pk_ptr pk,
                                      core::context_ptr context) {
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

