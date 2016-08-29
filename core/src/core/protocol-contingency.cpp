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
#include <fstream>
#include <sstream>
#include <functional>
class UserCipherLoader {
public:
    typedef std::function<bool(const std::string &dir)> doneFileChecker;

    UserCipherLoader(doneFileChecker checker) : checker(checker) {}
    ~UserCipherLoader() {}

    template<class Container>
        bool loadCiphers(Container &out, const std::string &path, const core::pk_ptr &pk);
private:
    doneFileChecker checker;
};

template<class Container>
bool UserCipherLoader::loadCiphers(Container &out,
                                   const std::string &path,
                                   const core::pk_ptr &pk)
{
    if (!checker(path)) return false;
    auto files = util::listDir(path, util::flag_t::FILE_ONLY);
    for (const std::string &file : files) {
        if (file.find("FILE_") != 0) continue;
        std::string cipher_file = util::concatenate(path, file);
        core::loadCiphers(out, pk, cipher_file);
    }
    return true;
}

namespace contingency_table {
/// the index of attributes starts from 1.
#define ATTR_INDEX(d) (d - 1)
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

    bool createDoneFile(const std::string &path,
                        const std::vector<core::Attribute> &attributes);

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
    
    bool ok = doEncrypt(fin, outputDirPath, attributes, pk, ea);
    fin.close();
    if (ok)
        ok &= createDoneFile(outputDirPath, attributes);
    return ok;
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
    return true;
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
                                 const std::vector<core::Attribute> &attributes) {
    auto fd = util::createDoneFile(path);
    if (!fd) {
        L_WARN(global::_console, "Can not create done file under {0}", path);
        return false;
    }

    std::stringstream sstream;
    sstream << "#";
    auto nr = attributes.size();
    for (size_t i = 0; i + 1 < nr; i++)
        sstream << std::to_string(attributes[i].size) << " ";
    sstream << std::to_string(attributes.back().size);
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
    auto attributes = parseHeader(doneFile);
    if (attributes.size() != 2) {
        L_WARN(global::_console, "Invalid done file under {0}", inputFilePath);
        return false;
    }

    core::PrivateContingencyTable::ResultType results;
    const EncryptedArray *ea = context->ea;
    core::PrivateContingencyTableHelper helper(attributes[0], attributes[1], 10, ea);
    helper.restore(results, inputFilePath, pk);

    std::vector<core::PrivateContingencyTableHelper::Publishable> publishables;
    helper.open_gamma(publishables, results.gamma, results.tilde_gamma, ea, sk);
    auto table = helper.final_decrypt(results.n_uv, publishables, sk, ea);

    std::ofstream fout(util::concatenate(outputDirPath, "FILE_1"), std::ios::binary);
    if (!fout.is_open()) {
        L_WARN(global::_console, "Can not create new file under {0}", outputDirPath);
        return false;
    }
    fout << "#" << attributes[0].size << " " << attributes[1].size << "\n";
    for (auto &row : table)
        for (auto c : row)
            fout << c << " ";
    fout << "\n";
    fout.close();
    return true;
}

bool ProtocolImp::evaluate(const std::vector<std::string> &inputDirs,
                           const std::string &outputDir, core::pk_ptr pk,
                           core::context_ptr context)
{
    //TODO(riku) might check the done file
    auto checker = [](const std::string &) -> bool { return true; };
    auto attributes = parseHeader(util::concatenate(inputDirs.front(), global::_doneFileName));

    size_t nr_attribute = attributes.size();
    long pIndex = ATTR_INDEX(_p);
    long qIndex = ATTR_INDEX(_q);
    if (pIndex >= nr_attribute || pIndex < 0 || qIndex >= nr_attribute || qIndex < 0) {
        L_ERROR(global::_console, "Invalid attribute index {0} and {1} (total {2} attri.)", _p, _q, nr_attribute);
        return false;
    }

    core::Attribute P = attributes[pIndex];
    core::Attribute Q = attributes[qIndex];

    FHE_NTIMER_START(LOAD_FILES);
    std::vector<Ctxt> ctxts;
    UserCipherLoader loader(checker);
    for (auto dir : inputDirs)
        loader.loadCiphers(ctxts, dir, pk);
    FHE_NTIMER_STOP(LOAD_FILES);
    printNamedTimer(std::cout, "LOAD_FILES");

    const EncryptedArray *ea = context->ea;
    core::PrivateContingencyTableHelper helper(P, Q, 2, ea);
    core::PrivateContingencyTable privateContingencyTable(context, &helper);
    auto results = privateContingencyTable.evaluate(ctxts);

    auto outputFile = util::concatenate(outputDir, "File_result");
    bool ok = helper.dump(results, outputFile);
    if (ok) {
        std::vector<core::Attribute> attrs;
        attrs.push_back(P);
        attrs.push_back(Q);
        createDoneFile(outputDir, attrs);
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
extern const core::FHEArg _fheArgs = {.m = 5227, .p = 67499, .r = 1, .L = 10};

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
    printf("Evaluating %d %d attributes\n", p, q);
    ContingencyTableProtocol ct(p, q);
    return ct.evaluate(inputDirs, outputDir, pk, context);
}
} // namespace protocol
} // namespace core
