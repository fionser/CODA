//
// Created by riku on 2016/08/09.
//
#include "../../include/core/protocol-contingency.hpp"
#include "../../include/core/contingency_table.hpp"
#include "../../include/core/global.hpp"
#include "../../include/core/file_util.hpp"
#include "../../include/core/literal.hpp"
#include "../../include/HElib/EncryptedArray.h"
#include "../../include/core/core.hpp"
#include "../../include/core/ctxt_util.hpp"
#include <fstream>
#include <memory>
#include <sstream>
#include <functional>
long __THRESHOLD = 2;
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

bool UserCipherLoader::loadLocalComputedCipher(Ctxt &out, int32_t &nr,
                                               const std::string &path, const core::pk_ptr &pk)
{
    auto files = util::listDir(path, util::flag_t::FILE_ONLY);
    for (const std::string &file : files) {
        if (checker(file)) {
            std::string cipher_file = util::concatenate(path, file);
            std::ifstream fin(cipher_file, std::ios::binary);
            fin >> nr;
            fin >> out;
            fin.close();
            return true;
        }
    }
    return false;
}

namespace contingency_table {
/// the index of attributes starts from 1.
#define ATTR_INDEX(d) (d - 1)

class CTable {
public:
    CTable() {}

    ~CTable() {}

    void setSize(size_t p, size_t q) {
        assert(p >= 0 && q >= 0);
        std::vector<std::vector<long>> (p, std::vector<long>(q, 0)).swap(_data);

    }

    void add(int pp, int qq) {
       if (pp == 0 || qq == 0) return;
        _data[ATTR_INDEX(pp)][ATTR_INDEX(qq)] += 1;
    }

    void print() const {
        for (auto &row : _data) {
            for (auto d : row)
                std::cout << d << " ";
            std::cout << "\n";
        }
    }

    NTL::ZZX encode(const EncryptedArray *ea) const {
        size_t p = _data.size();
        size_t q = _data.front().size();
        auto modified = core::coprime(p, q);
        std::vector<long> poly(ea->size(), 0);
        for (size_t i = 0; i < p; i++) {
            for (size_t j = 0; j < q; j++) {
                auto crtIdx = core::apply_crt(i, j, modified.first, modified.second);
                poly.at(crtIdx) = _data[i][j];
            }
        }

        NTL::ZZX zzx;
        ea->encode(zzx, poly);
        return zzx;
    }

private:
    std::vector<std::vector<long>> _data;
};

class ProtocolImp {
public:
    ProtocolImp(int p, int q) : _p(p), _q(q) {
        assert(_p >= 0 && _q >= 0 && "Invaild attributes index");
    }

    ~ProtocolImp() {}

    bool encrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 bool local_compute,
                 core::pk_ptr pk,
                 core::context_ptr context);

    bool decrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 core::pk_ptr pk,
                 core::sk_ptr sk,
                 core::context_ptr context);

    bool evaluate(const std::vector<std::string> &inputDirs,
                  const std::string &outputDir,
                  core::pk_ptr pk,
                  core::context_ptr context);
private:
    bool doEncrypt(std::ifstream &fin,
                   const std::string &outputDirPath,
                   const std::vector<core::Attribute> &attributes,
                   core::pk_ptr pk,
                   const EncryptedArray *ea);

    bool localEncrypt(std::ifstream &fin,
                      const std::string &outputDirPath,
                      const std::vector<core::Attribute> &attributes,
                      core::pk_ptr pk,
                      const EncryptedArray *ea);

    bool doEvaluate(const std::vector<std::string> &inputDirs,
                    const std::string &outputDir,
                    const std::vector<core::Attribute> &attributes,
                    core::pk_ptr pk, core::context_ptr context);

    bool localEvaluate(const std::vector<std::string> &inputDirs,
                       const std::string &outputDir,
                       const std::vector<core::Attribute> &attributes,
                       core::pk_ptr pk, core::context_ptr context);

    bool createDoneFile(const std::string &path,
                        bool local_computed,
                        const std::vector<core::Attribute> &attributes,
                        const std::vector<std::pair<int32_t, int32_t>> &pairs);

    void encode(NTL::ZZX &poy,
                const std::vector<std::string> &values,
                const std::vector<core::Attribute> &attributes,
                const EncryptedArray *ea);

    std::vector<core::Attribute> parseHeader(const std::string &file_path);
    std::vector<core::Attribute> parseHeader(std::istream &in);
    const int _p, _q;
};

bool ProtocolImp::encrypt(const std::string &inputFilePath,
                          const std::string &outputDirPath,
                          bool local_compute,
                          core::pk_ptr pk,
                          core::context_ptr context)
{
    std::ifstream fin(inputFilePath, std::ios::binary);
    if (!fin.is_open()) {
        L_DEBUG(global::_console, "Can not open file {0}", inputFilePath);
        return false;
    }

    std::vector<core::Attribute> attributes = parseHeader(fin);
    size_t nr_attributes = attributes.size();
    if (nr_attributes == 0) {
        L_DEBUG(global::_console, "Some went wrong when parsing the header of {0}", inputFilePath);
        return false;
    }

    const EncryptedArray *ea = context->ea;
    if (ea->size() < attributes.back().offset + attributes.back().size) {
        L_ERROR(global::_console, "Not enough CRT-packing slots for those attributes");
        return false;
    }

    bool ok;
    if (!local_compute)
        ok = doEncrypt(fin, outputDirPath, attributes, pk, ea);
    else
        ok = localEncrypt(fin, outputDirPath, attributes, pk, ea);
    fin.close();
    return ok;
}

static std::vector<long> parseRow(const std::string &line,
                                  const std::vector<core::Attribute> &attributes)
{
    std::vector<long> numbers;
    auto fields = util::splitBySpace(line);
    if (fields.size() != attributes.size()) {
        L_WARN(global::_console, "Mismatch the number of attributes: {0} and {1}",
               fields.size(), attributes.size());
        return numbers;
    }
    numbers.resize(attributes.size());
    size_t pos = 0;
    for (size_t i = 0; i < attributes.size(); i++) {
        numbers[i] = literal::stol(fields[i], &pos, 10);
        if (pos != fields[i].size()) {
            L_WARN(global::_console, "Invalid field: {0}", fields[i]);
            return std::vector<long>();
        }
    }
    return numbers;
}

bool ProtocolImp::localEncrypt(std::ifstream &fin,
                               const std::string &outputDirPath,
                               const std::vector <core::Attribute> &attributes,
                               core::pk_ptr pk,
                               const EncryptedArray *ea)
{
    size_t nr_attributes = attributes.size();


    std::vector<CTable> ctables((nr_attributes * (nr_attributes - 1)) / 2);
    size_t idx = 0;
    for (size_t i  = 0; i < nr_attributes; i++) {
        for (size_t j = i + 1; j < nr_attributes; j++) {
            ctables.at(idx).setSize(attributes[i].size, attributes[j].size);
            idx += 1;
        }
    }

    long nr_records = 0;
    for (std::string line; std::getline(fin, line); ) {
        std::vector<long> numbers = parseRow(line, attributes);
        if (numbers.size() != nr_attributes) continue;
        nr_records += 1;
        size_t idx = 0;
        for (size_t i  = 0; i < nr_attributes; i++) {
            for (size_t j = i + 1; j < nr_attributes; j++) {
                ctables.at(idx).add(numbers.at(i), numbers.at(j));
                idx += 1;
            }
        }
    }

    auto makePath = [](std::string const& path, const long count) -> std::string {
        return path + literal::separator + "FILE_" + std::to_string(count);
    };

    long file_nr = 1;
    Ctxt ctx(*pk);
    for (auto &ctable : ctables) {
        auto poly = ctable.encode(ea);
        std::ofstream fout(makePath(outputDirPath, file_nr), std::ios::binary);
        if (!fout.is_open()) {
            L_WARN(global::_console, "Can not create new files under {0}", outputDirPath);
            continue;
        }

        fout << nr_records;
        pk->Encrypt(ctx, poly);
        fout << ctx;
        file_nr += 1;
    }

    return createDoneFile(outputDirPath, true, attributes, {});
}

bool ProtocolImp::doEncrypt(std::ifstream &fin,
                            const std::string &outputDirPath,
                            const std::vector<core::Attribute> &attributes,
                            core::pk_ptr pk,
                            const EncryptedArray *ea)
{
    size_t nr_attributes = attributes.size();
    auto makePath = [](std::string const& path, const long count) -> std::string {
        return path + literal::separator + "FILE_" + std::to_string(count);
    }; 

    long file_nr = 1;
    long ctx_dumped = 0;
    std::ofstream fout(makePath(outputDirPath, file_nr), std::ios::binary | std::ios::out);
    if (!fout.is_open()) {
        L_WARN(global::_console, "Can not create new files under {0}", outputDirPath);
        return false;
    }

    Ctxt ctx(*pk);
    NTL::ZZX poly;
    for (std::string line; std::getline(fin, line); ) {
        auto row = util::splitBySpace(line);
        if (row.size() != nr_attributes) {
            L_WARN(global::_console, "A row might be wrong:\n {0}", line);
            continue;
        }
        encode(poly, row, attributes, ea);
        pk->Encrypt(ctx, poly);

        if (ctx_dumped < core::core_setting.CTX_PER_FILE) {
            fout << ctx;
            ctx_dumped += 1;
        } else {
            fout.close();
            file_nr += 1;
            fout = std::ofstream(makePath(outputDirPath, file_nr), std::ios::binary);
            assert(fout.is_open() && "Can not create new file");
            fout << ctx;
            ctx_dumped = 1;
        }
    }
    fout.close();
    return createDoneFile(outputDirPath, false, attributes, {});
}

void ProtocolImp::encode(NTL::ZZX &poly,
                         const std::vector<std::string> &values,
                         const std::vector<core::Attribute> &attributes,
                         const EncryptedArray *ea) {
    size_t nr_attributes = attributes.size();
    std::vector<long> slots(ea->size(), 0);   
    size_t offset = 0;
    for (size_t i = 0; i < nr_attributes; i++) {
        long attribute_value = literal::stol(values[i]);
        if (attribute_value != 0) {
            slots.at(offset + ATTR_INDEX(attribute_value)) = 1;
        }
        offset += attributes[i].size;
    }
    ea->encode(poly, slots);
}

std::vector<core::Attribute> ProtocolImp::parseHeader(const std::string &in) {
    std::ifstream fin(in);
    auto res = parseHeader(fin);
    fin.close();
    return res;
}

std::vector<core::Attribute> ProtocolImp::parseHeader(std::istream &in) {
    std::vector<core::Attribute> attributes;
    std::string line;
    std::getline(in, line); 
    if (line.c_str()[0] == '#') {
        auto attr_sizes = util::splitBySpace(line.substr(1));
        attributes.reserve(attr_sizes.size());
        size_t offset = 0;
        core::Attribute attribute;
        for (auto s : attr_sizes) {
            attribute.size = literal::stol(s);
            attribute.offset = offset;
            attribute.type = core::Attribute::Type::CATEGORICAL;
            attribute.text = "Unknown";
            attributes.push_back(attribute);
            offset += attribute.size;
        }
    }
    return attributes;
}

bool ProtocolImp::createDoneFile(const std::string &path,
                                 bool local_computed,
                                 const std::vector<core::Attribute> &attributes,
                                 const std::vector<std::pair<int32_t, int32_t>> &pairs) {
    auto fd = util::createDoneFile(path);
    if (!fd) {
        L_WARN(global::_console, "Can not create done file under {0}", path);
        return false;
    }

    std::stringstream sstream;
    sstream << '#';
    auto nr = attributes.size();
    for (size_t i = 0; i + 1 < nr; i++)
        sstream << std::to_string(attributes[i].size) << " ";
    sstream << std::to_string(attributes.back().size) << '\n';
    sstream << local_computed;

    for (auto &pair : pairs)
        sstream << '\n' << pair.first << " " << pair.second;

    auto header = sstream.str();
    fwrite(header.c_str(), header.size(), 1UL, fd);
    fclose(fd);
    return true;
}

bool ProtocolImp::decrypt(const std::string &inputFilePath,
                          const std::string &outputDirPath, 
                          core::pk_ptr pk, core::sk_ptr sk,
                          core::context_ptr context)
{
    std::string doneFile = util::concatenate(util::getDirPath(inputFilePath), global::_doneFileName);
    std::ifstream fin(doneFile, std::ios::binary);
    if (!fin.is_open()) {
        L_WARN(global::_console, "Can not open {0}", doneFile);
        return false;
    }
    auto attributes = parseHeader(fin);
    bool localComputed;
    fin >> localComputed;
    int32_t p, q;
    fin >> p >> q;

    core::PrivateContingencyTable::ResultType results;
    const EncryptedArray *ea = context->ea;
    core::PrivateContingencyTableHelper helper(attributes.at(ATTR_INDEX(p)),
                                               attributes.at(ATTR_INDEX(q)), __THRESHOLD, ea);
    helper.restore(results, inputFilePath, pk);

    std::vector<core::PrivateContingencyTableHelper::Publishable> publishables;
    helper.open_gamma(publishables, results.gamma, results.tilde_gamma, ea, sk);
    auto table = helper.final_decrypt(results.n_uv, publishables, sk, ea);
    std::ofstream fout(util::concatenate(outputDirPath, "FILE_1"), std::ios::binary);
    if (!fout.is_open()) {
        L_WARN(global::_console, "Can not create new file under {0}", outputDirPath);
        return false;
    }

    fout << p << " " << q << '\n';
    for (auto &row : table)
        for (auto c : row)
            fout << c << " ";
    fout << "\n";
    fout.close();
    return true;
}

bool ProtocolImp::evaluate(const std::vector<std::string> &inputDirs,
                           const std::string &outputDir, core::pk_ptr pk,
                           core::context_ptr context) {
    std::ifstream fin(util::concatenate(inputDirs.front(), global::_doneFileName));
    if (!fin.is_open()) {
        L_WARN(global::_console, "Can not open {0}", global::_doneFileName);
        return false;
    }
    auto attributes = parseHeader(fin);
    bool local_compute;
    fin >> local_compute;
    fin.close();

    if (local_compute)
        return localEvaluate(inputDirs, outputDir, attributes, pk, context);
    else
        return doEvaluate(inputDirs, outputDir, attributes, pk, context);
}

static size_t get_index(int p, int q, int nr_attr) {
    size_t idx = 1;
    for (size_t i = 0; i < nr_attr; i++) {
        for (size_t j = i + 1; j < nr_attr; j++) {
            if (i == p && j == q) return idx;
            idx += 1;
        }
    }
    assert(0);
}

bool ProtocolImp::localEvaluate(const std::vector<std::string> &inputDirs,
                                const std::string &outputDir,
                                const std::vector<core::Attribute> &attributes,
                                core::pk_ptr pk, core::context_ptr context) {
    size_t nr_attribute = attributes.size();
    long pIndex = ATTR_INDEX(_p);
    long qIndex = ATTR_INDEX(_q);
    if (pIndex >= nr_attribute || pIndex < 0 || qIndex >= nr_attribute || qIndex < 0) {
        L_ERROR(global::_console, "Invalid attribute index {0} and {1} (total {2} attri.)", _p, _q, nr_attribute);
        return false;
    }

    if (pIndex > qIndex) std::swap(pIndex, qIndex);

    long specific_file_nr = get_index(pIndex, qIndex, attributes.size());

    std::string specific_file = "FILE_" + std::to_string(specific_file_nr);

    // load specific file
    auto checker = [&specific_file](const std::string &fileName) -> bool {
        return fileName.compare(specific_file) == 0;
    };

    std::shared_ptr<Ctxt> ct = std::make_shared<Ctxt>(*pk);
    Ctxt ctxt(*pk);
    UserCipherLoader loader(checker);
    int32_t nr_records = 0;
    for (auto dir : inputDirs) {
        int32_t records;
        if (loader.loadLocalComputedCipher(ctxt, records, dir, pk)) {
            ct->operator+=(ctxt);
            nr_records += records;
        }
    }

    core::Attribute P = attributes[ATTR_INDEX(_p)];
    core::Attribute Q = attributes[ATTR_INDEX(_q)];
    const EncryptedArray *ea = context->ea;
    core::PrivateContingencyTableHelper helper(P, Q, __THRESHOLD, ea);
    core::PrivateContingencyTable privateContingencyTable(context, &helper);
    auto results = privateContingencyTable.evaluate(ct, static_cast<long>(nr_records));

    auto outputFile = util::concatenate(outputDir, "File_result");
    bool ok = helper.dump(results, outputFile);
    if (ok) {
        int32_t pp = static_cast<int32_t>(_p);
        int32_t qq = static_cast<int32_t>(_q);
        auto pair = std::make_pair(pp, qq);
        std::vector<std::pair<int32_t, int32_t>> pairs(1, pair);
        ok &= createDoneFile(outputDir, true, attributes, pairs);
    } else {
        L_WARN(global::_console, "Some went wrong when to dump ciphers to {0}", outputFile);
    }
    return ok;
}

bool ProtocolImp::doEvaluate(const std::vector<std::string> &inputDirs,
                             const std::string &outputDir,
                             const std::vector<core::Attribute> &attributes,
                             core::pk_ptr pk, core::context_ptr context) {
    size_t nr_attribute = attributes.size();
    long pIndex = ATTR_INDEX(_p);
    long qIndex = ATTR_INDEX(_q);
    if (pIndex >= nr_attribute || pIndex < 0 || qIndex >= nr_attribute || qIndex < 0) {
        L_ERROR(global::_console, "Invalid attribute index {0} and {1} (total {2} attri.)", _p, _q, nr_attribute);
        return false;
    }

    core::Attribute P = attributes[pIndex];
    core::Attribute Q = attributes[qIndex];

    // Load all FILE_* files
    auto checker = [](const std::string &fileName) -> bool { return fileName.find("FILE_") == 0; };
    std::vector<Ctxt> ctxts;
    UserCipherLoader loader(checker);
    for (auto dir : inputDirs)
        loader.loadCiphers(ctxts, dir, pk);

    const EncryptedArray *ea = context->ea;
    core::PrivateContingencyTableHelper helper(P, Q, __THRESHOLD, ea);
    core::PrivateContingencyTable privateContingencyTable(context, &helper);
    auto results = privateContingencyTable.evaluate(ctxts);

    auto outputFile = util::concatenate(outputDir, "File_result");
    bool ok = helper.dump(results, outputFile);
    if (ok) {
        int32_t pp = static_cast<int32_t>(_p);
        int32_t qq = static_cast<int32_t>(_q);
        auto pair = std::make_pair(pp, qq);
        std::vector<std::pair<int32_t, int32_t>> pairs(1, pair);
        ok = createDoneFile(outputDir, false, attributes, pairs);
    } else {
        L_WARN(global::_console, "Some went wrong when to dump ciphers to {0}", outputFile);
    }
    return ok;
}

} // namespace contingency_table

ContingencyTableProtocol::ContingencyTableProtocol(int p, int q)
        : Protocol(std::string("Contingency Table"))
{
    imp = std::make_shared<contingency_table::ProtocolImp>(p, q);
}

bool ContingencyTableProtocol::encrypt(const std::string &inputFilePath,
                                       const std::string &outputDirPath,
                                       bool local_compute,
                                       core::pk_ptr pk,
                                       core::context_ptr context)
{
    return imp->encrypt(inputFilePath, outputDirPath, local_compute, pk, context);
}

bool ContingencyTableProtocol::decrypt(const std::string &inputFilePath,
                                       const std::string &outputDirPath,
                                       core::pk_ptr pk, core::sk_ptr sk,
                                       core::context_ptr context)
{
    return imp->decrypt(inputFilePath, outputDirPath, pk, sk, context);
}

bool ContingencyTableProtocol::evaluate(const std::vector <std::string> &inputDirs,
                                        const std::string &outputDir, core::pk_ptr pk,
                                        core::context_ptr context)
{
    return imp->evaluate(inputDirs, outputDir, pk, context);
}

core::FHEArg ContingencyTableProtocol::parameters() const {
    core::FHEArg args;
    args.L = 10;
    args.m = 16384;
    args.p = 8191;
    args.r = 1;
    return args;
}

namespace protocol {
namespace contingency {
extern const core::FHEArg _fheArgs = {.m = 16384, .p = 8191, .r = 1, .L = 10};

bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             bool local_compute,
             core::pk_ptr pk,
             core::context_ptr context) {
    ContingencyTableProtocol ct;
    return ct.encrypt(inputFilePath, outputDirPath, local_compute, pk, context);
}

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk,
             core::context_ptr context) {
    ContingencyTableProtocol ct;
    return ct.decrypt(inputFilePath, outputFilePath, pk, sk, context);
}

bool evaluate(const std::vector <std::string> &inputDirs,
              const std::string &outputDir,
              const std::vector<std::string> &params,
              core::pk_ptr pk,
              core::context_ptr context) {
    if (params.size() != 2) return false;
    int p = literal::stol(params[0]);
    int q = literal::stol(params[1]);
    ContingencyTableProtocol ct(p, q);
    return ct.evaluate(inputDirs, outputDir, pk, context);
}
} // namespace protocol
} // namespace core
