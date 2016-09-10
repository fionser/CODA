#include "../../include/core/protocol-mean.hpp"
#include "../../include/core/core.hpp"
#include "../../include/core/global.hpp"
#include "../../include/core/file_util.hpp"
#include "../../include/core/literal.hpp"

#include "../../include/HElib/EncryptedArray.h"

#include <fstream>
#include <sstream>

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
                 core::context_ptr context) const {
        return false;
    } 

    bool evaluate(const std::vector<std::string> &inputDirs,
                  const std::string &outputDir,
                  core::pk_ptr pk,
                  core::context_ptr context) const {
        return false;
    }

private:
    bool doEncrypt(const std::string &inputFilePath,
                   const std::string &outputDirPath,
                   core::pk_ptr pk,
                   core::context_ptr context) const;

    bool localEncrypt(const std::string &inputFilePath,
                      const std::string &outputDirPath,
                      core::pk_ptr pk,
                      core::context_ptr context) const { return false; }

    int parseHeader(std::ifstream &fin) const;

    void encode(NTL::ZZX &poly, const std::string &row, 
                const int attr_nr, const EncryptedArray *ea) const;

    bool createDoneFile(const std::string &dir, bool local_computed, 
                        int nr_records) const;
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

    auto makePath = [](std::string const& path, size_t count) -> std::string {
        return path + literal::separator + "FILE_" + std::to_string(count);
    };

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
        encode(poly, line, number_of_attributes, ea);
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
        nr_records += 1;
    }

    fout.close();
    return createDoneFile(outputDirPath, false, nr_records);
}

void MeanProtocol::ProtocolImp::
encode(NTL::ZZX &poly, const std::string &row, 
       const int attr_nr, const EncryptedArray *ea) const {
    auto fields = util::splitBySpace(row);
    if (fields.size() != attr_nr) {
        L_WARN(global::_console, "Invalid input file to encrypt");
        return;
    }
    assert(attr_nr <= ea->size() && "Not enough spaces for packing");
    std::vector<long> numbers(ea->size(), 0);
    for (int i = 0; i < attr_nr; i++) {
        numbers[i] = literal::stol(fields[i]);
    }
    ea->encode(poly, numbers);
}

bool MeanProtocol::ProtocolImp::
createDoneFile(const std::string &dir, bool local_computed, int nr_records) const {
    auto fd = util::createDoneFile(dir);
    if (!fd) {
        L_WARN(global::_console, "Can not create done file under {0}", dir);
        return false;
    }

    std::stringstream sstream;
    sstream << '#' << std::to_string(nr_records) << '\n' << local_computed;
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
                           core::pk_ptr pk,
                           core::context_ptr context) const {
    return imp->encrypt(inputFilePath, outputDirPath, local_compute, pk, context);
}

bool MeanProtocol::decrypt(const std::string &inputFilePath,
                           const std::string &outputDirPath,
                           core::pk_ptr pk,
                           core::sk_ptr sk,
                           core::context_ptr context) const {
    return imp->decrypt(inputFilePath, outputDirPath, pk, sk, context);
}

bool MeanProtocol::evaluate(const std::vector<std::string> &inputDirs,
                            const std::string &outputDir,
                            core::pk_ptr pk,
                            core::context_ptr context) const {
    return imp->evaluate(inputDirs, outputDir, pk, context);
}

core::FHEArg MeanProtocol::parameters() const {
    core::FHEArg params = {.m = 16384, .p = 1031, .r = 2, .L = 5};
    return params;
}

