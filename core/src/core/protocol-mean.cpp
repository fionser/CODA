#include "core/protocol-mean.hpp"
#include "core/core.hpp"
#include "core/global.hpp"
#include "core/file_util.hpp"
#include "core/literal.hpp"

#include "HElib/EncryptedArray.h"

#include <fstream>
#include <sstream>

struct DoneFileContent {
    bool local_computed;
    int nr_attrs;
    int nr_records;
};

static std::string makePath(std::string const& path, size_t count) {
    return path + literal::separator + "FILE_" + std::to_string(count);
}

class MeanProtocol::ProtocolImp {
public:
    ProtocolImp() {}

    ~ProtocolImp() {}

    bool encrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 bool local_compute,
                 core::pk_ptr pk,
                 core::context_ptr context) const {
        if (local_compute) {
            return localEncrypt(inputFilePath, outputDirPath, pk, context);
        } else {
            return doEncrypt(inputFilePath, outputDirPath, pk, context);
        }
    }

    bool decrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 core::pk_ptr pk,
                 core::sk_ptr sk,
                 core::context_ptr context) const;

    bool evaluate(const std::vector<std::string> &inputDirs,
                  const std::string &outputDir,
                  core::pk_ptr pk,
                  core::context_ptr context) const;

private:
    bool doEncrypt(const std::string &inputFilePath,
                   const std::string &outputDirPath,
                   core::pk_ptr pk,
                   core::context_ptr context) const;

    bool localEncrypt(const std::string &inputFilePath,
                      const std::string &outputDirPath,
                      core::pk_ptr pk,
                      core::context_ptr context) const;

    bool doEvaluate(const std::vector<std::string> &inputDirs,
                    const std::string &outputDir,
                    const DoneFileContent &content,
                    core::pk_ptr pk,
                    core::context_ptr context) const;

    bool localEvaluate(const std::vector<std::string> &inputDirs,
                       const std::string &outputDir,
                       const DoneFileContent &content,
                       core::pk_ptr pk,
                       core::context_ptr context) const;

    int parseHeader(std::ifstream &fin) const;

    bool encode(NTL::ZZX &poly, const std::string &row,
                const int attr_nr, const EncryptedArray *ea) const;

    bool addOneLine(std::vector<long> &numbers,
                    const std::string &line,
                    const int attr_nr) const;

    bool createDoneFile(const std::string &dir, const DoneFileContent &content) const;
};

int MeanProtocol::ProtocolImp::parseHeader(std::ifstream &fin) const {
    std::string line;
    std::getline(fin, line);
    stringstream ss(line);
    char sharp;
    ss >> sharp;
    if (sharp != '#') return -1;
    int attr_nr = -1;
    ss >> attr_nr;
    return attr_nr;
}

bool MeanProtocol::ProtocolImp::doEncrypt(const std::string &inputFilePath,
                                          const std::string &outputDirPath,
                                          core::pk_ptr pk,
                                          core::context_ptr context) const
{
    std::ifstream fin(inputFilePath, std::ios::binary);
    if (!fin.is_open()) {
        L_ERROR(global::_console, "Can not open file {0}", inputFilePath);
        return false;
    }

    const int number_of_attributes = parseHeader(fin);
    if (number_of_attributes <= 0) {
        L_ERROR(global::_console, "Invalid header of {0}", inputFilePath);
        return false;
    }

    long file_nr = 1;
    std::ofstream fout(makePath(outputDirPath, file_nr), std::ios::binary);
    if (!fout.is_open()) {
        L_WARN(global::_console,
               "Can not create new files under {0}", outputDirPath);
        return false;
    }

    Ctxt ctx(*pk);
    NTL::ZZX poly;
    const EncryptedArray *ea = context->ea;
    long ctx_dumped = 0;
    int nr_records = 0;
    for (std::string line; std::getline(fin, line); ) {
        bool ok = encode(poly, line, number_of_attributes, ea);
        if (!ok) continue;
        pk->Encrypt(ctx, poly);

        if (ctx_dumped < core::core_setting.CTX_PER_FILE) {
            fout << ctx;
            ctx_dumped += 1;
        } else {
            fout.close();
            file_nr += 1;
            fout = std::ofstream(makePath(outputDirPath, file_nr), std::ios::binary);
            assert(fout.is_open() && "Can not create new FILE_* file");
            fout << ctx;
            ctx_dumped = 1;
        }
        nr_records += 1;
    }

    fout.close();
    DoneFileContent content = { .local_computed = false, .nr_attrs = number_of_attributes, .nr_records = nr_records };
    return createDoneFile(outputDirPath, content);
}

bool MeanProtocol::ProtocolImp::localEncrypt(const std::string &inputFilePath,
                                            const std::string &outputDirPath,
                                            core::pk_ptr pk,
                                            core::context_ptr context) const {
    std::ifstream fin(inputFilePath, std::ios::binary);
    if (!fin.is_open()) {
        L_ERROR(global::_console, "Can not open file {0}", inputFilePath);
        return false;
    }

    std::ofstream fout(makePath(outputDirPath, 1), std::ios::binary);
    if (!fout.is_open()) {
        L_ERROR(global::_console, "Can not create new file in {0}", outputDirPath);
        fin.close();
        return false;
    }

    const int number_of_attributes = parseHeader(fin);
    if (number_of_attributes <= 0) {
        L_ERROR(global::_console, "Invalid header of {0}", inputFilePath);
        return false;
    }

    const EncryptedArray *ea = context->ea;
    if (ea->size() < number_of_attributes) {
        L_ERROR(global::_console, "Can not pack {0} numbers", number_of_attributes);
        return false;
    }

    std::vector<long> numbers(ea->size(), 0);
    int nr_records = 0;
    for (std::string line; std::getline(fin, line); ) {
        if (addOneLine(numbers, line, number_of_attributes))
            nr_records += 1;
    }

    Ctxt ctx(*pk);
    ea->encrypt(ctx, *pk, numbers);
    fout << ctx;
    fin.close();
    fout.close();

    DoneFileContent content = { .local_computed = true, .nr_attrs = number_of_attributes, .nr_records = nr_records };
    return createDoneFile(outputDirPath, content);
}

bool MeanProtocol::ProtocolImp::decrypt(const std::string &inputFilePath,
                                        const std::string &outputDirPath,
                                        core::pk_ptr pk,
                                        core::sk_ptr sk,
                                        core::context_ptr context) const {
    std::string dir = util::getDirPath(inputFilePath);
    std::string doneFilePath = util::concatenate(dir, global::_doneFileName);
    std::ifstream doneFile(doneFilePath, std::ios::binary);
    if (!doneFile.is_open()) {
        L_ERROR(global::_console, "Can not found {0}", doneFilePath);
        return false;
    }

    int nr_attrs = parseHeader(doneFile);
    if (nr_attrs <= 0) {
        L_ERROR(global::_console, "Invalid {0}", doneFilePath);
        doneFile.close();
        return false;
    }

    bool local_computed;
    doneFile >> local_computed;
    int nr_records;
    doneFile >> nr_records;
    doneFile.close();

    Ctxt ctx(*pk);
    std::ifstream fin(inputFilePath, std::ios::binary);
    if (!fin.is_open()) {
        L_ERROR(global::_console, "Can not open {0}", inputFilePath);
        return false;
    }
    fin >> ctx;
    fin.close();

    NTL::ZZX poly;
    sk->Decrypt(poly, ctx);
    const EncryptedArray *ea = context->ea;
    if (ea->size() < nr_attrs) {
        L_ERROR(global::_console, "Invalid HElib for unpacking");
        return false;
    }
    std::vector<long> results(nr_attrs, 0);
    std::vector<long> positions(nr_attrs);
    for (int i = 0; i < nr_attrs; i++) positions[i] = i;
    ea->decodeSlots(results, poly, positions);

    std::ofstream fout(util::concatenate(outputDirPath, core::core_setting.decrypted_file),
                       std::ios::binary);
    if (!fout.is_open()) {
        L_ERROR(global::_console, "Can not create new file in {0}", outputDirPath);
    }

    for (auto res : results) {
        fout << res * 1.0 / nr_records << " ";
    }
    fout.close();

    DoneFileContent content = { .local_computed = local_computed, .nr_attrs = nr_attrs, .nr_records = nr_records };
    return createDoneFile(outputDirPath, content);
}

bool MeanProtocol::ProtocolImp::evaluate(const std::vector<std::string> &inputDirs,
                                         const std::string &outputDir,
                                         core::pk_ptr pk,
                                         core::context_ptr context) const
{
    std::ifstream fin(util::concatenate(inputDirs.front(), global::_doneFileName));
    if (!fin.is_open()) {
        L_WARN(global::_console, "Can not open {0}", global::_doneFileName);
        return false;
    }

    DoneFileContent content;
    int nr_attrs = parseHeader(fin);
    if (nr_attrs <= 0) {
        L_WARN(global::_console, "Invalid done file");
        return false;
    }

    content.nr_attrs = nr_attrs;
    fin >> content.local_computed;
    fin >> content.nr_records;
    fin.close();

    if (content.local_computed)
        return localEvaluate(inputDirs, outputDir, content, pk, context);
    else
        return doEvaluate(inputDirs, outputDir, content, pk, context);
}

bool MeanProtocol::ProtocolImp::
doEvaluate(const std::vector<std::string> &inputDirs,
           const std::string &outputDir,
           const DoneFileContent &content,
           core::pk_ptr pk,
           core::context_ptr context) const
{
    auto resultFile = util::concatenate(outputDir, "FILE_1");
    std::ofstream fout(resultFile, std::ios::binary);
    if (!fout.is_open()) {
        L_WARN(global::_console, "Can not create {0}", resultFile);
        return false;
    }

    Ctxt ctx(*pk);
    int nr_records = 0;
    for (const auto &dir : inputDirs) {
        auto files = util::listDir(dir, util::flag_t::FILE_ONLY);
        for (const auto &file : files) {
            if (file.find("FILE_") != 0) continue;
            std::list<Ctxt> ctxs;
            auto fullPath = util::concatenate(dir, file);
            if (!core::loadCiphers(ctxs, pk, fullPath)) return false;
            for (auto &ct : ctxs) {
                ctx += ct;
                nr_records += 1;
            }
        }
    }
    assert(content.nr_records == nr_records && "Seems cipher files are invalid");
    fout.close();

    fout = std::ofstream(makePath(outputDir, 1), std::ios::binary);
    if (!fout.is_open()) {
        L_WARN(global::_console, "Can not create new file in {0}", outputDir);
        return false;
    }
    fout << ctx;
    fout.close();
    return createDoneFile(outputDir, content);
}

bool MeanProtocol::ProtocolImp::
localEvaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              const DoneFileContent &content,
              core::pk_ptr pk,
              core::context_ptr context) const
{
    return false;
}

bool MeanProtocol::ProtocolImp::addOneLine(std::vector<long> &numbers,
                                           const std::string &line,
                                           const int attr_nr) const {
    auto fields = util::splitBySpace(line);
    if (fields.size() != attr_nr) {
        L_WARN(global::_console, "Invalid input file to encrypt");
        return false;
    }

    for (int i = 0; i < attr_nr; i++)
        numbers[i] +=  literal::stol(fields[i]);
    return true;
}

bool MeanProtocol::ProtocolImp::
encode(NTL::ZZX &poly, const std::string &row,
       const int attr_nr, const EncryptedArray *ea) const {
    auto fields = util::splitBySpace(row);
    if (fields.size() != attr_nr) {
        L_WARN(global::_console, "Invalid input file to encrypt");
        return false;
    }

    if (attr_nr > ea->size()) return false;

    std::vector<long> numbers(ea->size(), 0);
    for (int i = 0; i < attr_nr; i++) {
        numbers[i] = literal::stol(fields[i]);
    }
    ea->encode(poly, numbers);
    return true;
}

bool MeanProtocol::ProtocolImp::createDoneFile(const std::string &dir,
                                               const DoneFileContent &content) const {
    auto fd = util::createDoneFile(dir);
    if (!fd) {
        L_WARN(global::_console, "Can not create done file under {0}", dir);
        return false;
    }

    std::stringstream sstream;
    sstream << '#' << std::to_string(content.nr_attrs) << '\n' << content.local_computed
        << '\n' << content.nr_records;
    auto header = sstream.str();
    fwrite(header.c_str(), header.size(), 1UL, fd);
    fclose(fd);
    return true;
}

////////////////////////////////////////////////////////
MeanProtocol::MeanProtocol() : Protocol("MeanProtocol") {
    imp = std::make_shared<MeanProtocol::ProtocolImp>();
}

bool MeanProtocol::encrypt(const std::string &inputFilePath,
                           const std::string &outputDirPath,
                           bool local_compute,
                           const core::PubKeyWrapper &pk,
                           const core::ContextWrapper &context)
{
    return imp->encrypt(inputFilePath, outputDirPath, local_compute,
                        pk.single, context.single);
}

bool MeanProtocol::decrypt(const std::string &inputFilePath,
                           const std::string &outputDirPath,
                           const core::PubKeyWrapper &pk,
                           const core::SecKeyWrapper &sk,
                           const core::ContextWrapper &context)
{
    return imp->decrypt(inputFilePath, outputDirPath,
                        pk.single, sk.single, context.single);
}

bool MeanProtocol::evaluate(const std::vector<std::string> &inputDirs,
                            const std::string &outputDir,
                            const std::vector<std::string> &/*params*/,
                            const core::PubKeyWrapper &pk,
                            const core::ContextWrapper &context)
{
    return imp->evaluate(inputDirs, outputDir, pk.single, context.single);
}

core::FHEArg MeanProtocol::parameters() const {
    core::FHEArg params = {.m = 16384, .p = 1031, .r = 2, .L = 5};
    return params;
}

