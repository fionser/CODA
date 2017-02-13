#include "core/protocol-percentile.hpp"
#include "core/contingency_table.hpp" // definition of Attribute
#include "core/encoder.hpp" // definition of Attribute
#include "core/global.hpp"
#include "core/core.hpp"
#include "core/ctxt_util.hpp"
#include "core/file_util.hpp"
#include "core/literal.hpp"

#include "HElib/EncryptedArray.h"
#include "HElib/replicate.h"

#include <fstream>
#include <functional>

class UserCipherLoader {
public:
    typedef std::function<bool(const std::string &dir)> doneFileChecker;

    UserCipherLoader(doneFileChecker checker) : checker(checker) {}

    ~UserCipherLoader() {}

    int loadSpecficCtxtFile(std::vector<Ctxt> &out,
                            const std::string &path,
                            const core::pk_ptr &pk) const;

private:
    doneFileChecker checker;
};

class PercentileProtocol::Imp {
public:
    Imp() {}

    ~Imp() {}

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

    bool evaluate(const StringList &inputDirs,
                  const std::string &outputDir,
                  const StringList &params,
                  core::pk_ptr pk,
                  core::context_ptr context);

    friend class UserCipherLoader;
private:
    std::vector<core::Attribute> parseHeader(const std::string &file) const {
        std::ifstream fin(file);
        if (!fin.is_open())
            return std::vector<core::Attribute>();
        auto ret = parseHeader(fin);
        fin.close();
        return ret;
    }

    typedef std::vector<long> Column;
    std::vector<Column> parseContent(std::istream &fin,
                                     const std::vector<core::Attribute> &attrs) const;

    static std::vector<core::Attribute> parseHeader(std::istream &fin);

    int doEncrypt(std::ifstream &fin, const std::string &outputDirPath,
                  const std::vector<core::Attribute> &attrs,
                  core::pk_ptr pk, const EncryptedArray *ea) const;

    int localEncrypt(std::ifstream &fin, const std::string &outputDirPath,
                     const std::vector<core::Attribute> &attrs,
                     core::pk_ptr pk, const EncryptedArray *ea) const;

    /// encrypt the encoding vector to ctxt(s). When the #slot is smaller than
    // the encode.size(), then use multiple ciphertexts.
    std::vector<Ctxt> encryptOneEncoding(const std::vector<long> &encoded,
                                         core::pk_ptr pk,
                                         const EncryptedArray *ea) const;

    bool sumCtxtOfClients(std::vector<Ctxt> &summed, int &total_record,
                          const StringList &inputDirs,
                          UserCipherLoader::doneFileChecker checker,
                          core::pk_ptr pk) const;

    bool createEncDoneFile(const std::string &outputDirPath,
                           const int record_nr,
                           const std::vector<core::Attribute> &attrs) const;

    bool createEvalDoneFile(const std::string &outputDir) const;

    bool createDecDoneFile(const std::string &outputDir) const;
}; // PercentileProtocol::Imp

bool PercentileProtocol::Imp::encrypt(const std::string &inputFilePath,
                                      const std::string &outputDirPath,
                                      bool local_compute,
                                      core::pk_ptr pk,
                                      core::context_ptr context) {
    std::ifstream fin(inputFilePath);
    if (!fin.is_open()) {
        L_WARN(global::_console, "Can not open {0}", inputFilePath);
        return false;
    }
    auto attributes = parseHeader(fin);
    if (attributes.empty()) {
        L_ERROR(global::_console, "Can not parse the file {0}", inputFilePath);
        return false;
    }

    const EncryptedArray *ea = context->ea;
    int record_nr = 0;
    if (local_compute)
        record_nr = localEncrypt(fin, outputDirPath, attributes, pk, ea);
    else
        record_nr = doEncrypt(fin, outputDirPath, attributes, pk, ea);

    if (record_nr > 0)
        createEncDoneFile(outputDirPath, record_nr, attributes);
    fin.close();
    return record_nr > 0;
}

std::vector<PercentileProtocol::Imp::Column>
PercentileProtocol::Imp::parseContent(std::istream &fin,
                                      const std::vector<core::Attribute> &attrs) const {
    std::vector<Column> columns;
    if (attrs.empty())
        return columns;
    columns.resize(attrs.size());
    for (std::string line; std::getline(fin, line); ) {
        auto fields = util::splitBySpace(line);
        if (fields.size() != columns.size()) {
            L_WARN(global::_console, "Invalid row {0}", line);
            continue;
        }

        for (size_t i = 0; i < fields.size(); i++)
            columns[i].push_back(literal::stol(fields[i]));
    }
    return columns;
}

// TODO(riku) the k-percentile might also support non-local-computation.
int PercentileProtocol::Imp::doEncrypt(std::ifstream &fin,
                                       const std::string &outputDirPath,
                                       const std::vector<core::Attribute> &attrs,
                                       core::pk_ptr pk,
                                       const EncryptedArray *ea) const {
    L_WARN(global::_console, "Not implemented yet");
    return 0;
}

int PercentileProtocol::Imp::localEncrypt(std::ifstream &fin,
                                          const std::string &outputDirPath,
                                          const std::vector<core::Attribute> &attrs,
                                          core::pk_ptr pk,
                                          const EncryptedArray *ea) const {
    auto columns = parseContent(fin, attrs);
    if (columns.empty())
        return 0;

    auto makePath = [](std::string const& path, const long count) -> std::string {
        return path + literal::separator + "FILE_" + std::to_string(count);
    };

    for (long i = 0; i < attrs.size(); i++) {
        std::vector<long> summation(attrs[i].size + 1, 0);
        for (long v : columns[i]) {
            std::vector<long> encoding;
            bool ok = encoder::staircase::encode(encoding, attrs[i], v);
            if (!ok) {
                L_WARN(global::_console, "Something went wrong when to encode {0}-th column", i + 1);
                continue;
            }
            for (size_t j = 0; j < summation.size() && j < encoding.size(); j++) {
                summation[j] += encoding[j];
            }
        }
        auto encryptedColumn = encryptOneEncoding(summation, pk, ea);
        std::string savePath = makePath(outputDirPath, i + 1);
        core::dumpCiphers(encryptedColumn, savePath);
    }

    return columns.front().size();
}

std::vector<Ctxt>
PercentileProtocol::Imp::encryptOneEncoding(const std::vector<long> &encoded,
                                            core::pk_ptr pk,
                                            const EncryptedArray *ea) const {
    std::vector<Ctxt> ctxts;
    if (!ea) {
        L_WARN(global::_console, "Null EncryptedArray pointer");
        return ctxts;
    }

    size_t ctx_nr = (encoded.size() + ea->size() - 1) / ea->size();
    auto outer = encoded.cbegin();
    while (outer != encoded.cend()) {
        std::vector<long> poly(ea->size(), 0);
        auto inner = poly.begin();
        while (inner != poly.end() && outer != encoded.cend()) {
            *inner = *outer;
            inner++;
            outer++;
        }
        Ctxt ctx(*pk);
        ea->encrypt(ctx, *pk, poly);
        ctxts.push_back(ctx);
    }

    return ctxts;
}

bool PercentileProtocol::Imp::decrypt(const std::string &inputFilePath,
                                      const std::string &outputDirPath,
                                      core::pk_ptr pk,
                                      core::sk_ptr sk,
                                      core::context_ptr context) {
    std::vector<Ctxt> ctxts;
    const EncryptedArray *ea = context->ea;
    std::ifstream fin(inputFilePath);
    if (!fin.is_open()) {
        L_WARN(global::_console, "Can not open {0}", inputFilePath);
        return false;
    }

    size_t nr_parts;
    greaterthan gt(0, context);
    bool first = true;
    int kPercentile = 1;
    while (!fin.eof()) {
        std::vector<ctxt_ptr> parts;
        fin >> nr_parts;
        if (nr_parts == 0) break;
        for (int index = 0; index < nr_parts; index++) {
            parts.push_back(std::make_shared<Ctxt>(*pk));
            fin >> *parts.back();
        }

        greaterthan::Result result = gt.open_result(parts, sk);
        // the first frequency is larger than the k-percentile
        if (result == greaterthan::Result::GREATER_OR_EQUAL) {
            break;
        } else {
            kPercentile += 1;
        }
    }
    fin.close();

    std::string resultFile = util::concatenate(outputDirPath, core::core_setting.resulting_file);
    std::ofstream fd(resultFile);
    if (!fd.is_open()) {
        L_WARN(global::_console, "Can not create {0}", resultFile);
        std::cout << "the k-percentile is " << kPercentile << "\n";
    } else {
        fd << kPercentile;
        fd.close();
        createDecDoneFile(outputDirPath);
    }
    return true;
}

bool PercentileProtocol::Imp::evaluate(const StringList &inputDirs,
                                       const std::string &outputDir,
                                       const StringList &params,
                                       core::pk_ptr pk,
                                       core::context_ptr context) {
    if (params.size() != 2) {
        L_ERROR(global::_console, "Invalid evaluate parameters. Require [k-percentile] [target attr index]");
        return false;
    }

    std::ifstream fin(util::concatenate(inputDirs.front(),
                                        global::_doneFileName));
    if (!fin.is_open()) {
        L_WARN(global::_console, "Can not open {0}", global::_doneFileName);
        return false;
    }

    auto attributes = parseHeader(fin);
    const int percentile = literal::stol(params[0]);
    const int attrIndex = literal::stol(params[1]);
    if (attrIndex > attributes.size() || attrIndex <= 0) {
        L_ERROR(global::_console, "Invalid evaluate parameter {0}, only get {1} attributes",
                attrIndex, attributes.size());
        return false;
    }

    std::string specificFile = "FILE_" + std::to_string(attrIndex);
    auto checker = [specificFile](const std::string &file) -> bool {
        return file.compare(specificFile)  == 0;
    };
    std::vector<Ctxt> summation;
    int total_record = 0;
    sumCtxtOfClients(summation, total_record, inputDirs, checker, pk);

    greaterthan gt(total_record, context);
    const int threshold = static_cast<int>(std::ceil(percentile * total_record / 100.0));
    core::Attribute targetAttr = attributes.at(attrIndex - 1);
    const EncryptedArray *ea = context->ea;
    std::string saveTo = util::concatenate(outputDir, core::core_setting.resulting_file);
    std::ofstream fout(saveTo);

    for (int index = 1; index <= targetAttr.size; index++) {
        int ctxt_index = (index - 1) / ea->size();
        int slot_index = index % ea->size();
        Ctxt replicated_slot(summation.at(ctxt_index));
        // replicate the slot_index for total_record times
        replicate(*ea, replicated_slot, slot_index);
        auto gt_result = gt.compare(&replicated_slot, threshold);
        fout << gt_result.size();
        for (auto ctx : gt_result) {
            fout << *ctx;
        }
    }
    fout << 0;
    fout.close();
    return createEvalDoneFile(outputDir);
}

bool PercentileProtocol::Imp::sumCtxtOfClients(std::vector<Ctxt> &summation,
                                               int &totoal_record,
                                               const StringList &inputDirs,
                                               UserCipherLoader::doneFileChecker checker,
                                               core::pk_ptr pk) const {
    for (auto &dir : inputDirs) {
        UserCipherLoader loader(checker);
        int record_nr = 0;
        if (summation.empty()) {
            record_nr = loader.loadSpecficCtxtFile(summation, dir, pk);
        } else {
            std::vector<Ctxt> clientCtxts;
            record_nr = loader.loadSpecficCtxtFile(clientCtxts, dir, pk);
            if (clientCtxts.size() != summation.size()) {
                L_WARN(global::_console, "Mismatch number of ctxts in {0}", dir);
                continue;
            }

            for (size_t i = 0; i < clientCtxts.size(); i++)
                summation[i] += clientCtxts[i];
        }

        if (record_nr == 0) {
            L_WARN(global::_console, "Something went wrong when to load {0}", dir);
        } else {
            totoal_record += record_nr;
        }
    }
    return !summation.empty();
}

// header = #<attribute_nr><space><attr1_size><space><attr2_size> ....
static std::vector<core::Attribute> __parseHeader(std::istream &in) {
    std::vector<core::Attribute> attributes;
    std::string line;
    std::getline(in, line);
    if (line.c_str()[0] == '#') {
        auto attr_sizes = util::splitBySpace(line.substr(1));
        core::Attribute attribute;
        long attr_number = literal::stol(attr_sizes[0]);
        for (size_t i = 1; i < attr_sizes.size(); ++i) {
            const std::string &s(attr_sizes[i]);
            // TODO(riku) might be better to check whether `s` is a correct integer
            attribute.size = literal::stol(s);
            attribute.offset = 0; // no use for ordinal data
            attribute.type = core::Attribute::Type::ORDINAL;
            attribute.text = "Unknown";
            attributes.push_back(attribute);
        }

        if (attr_number != attributes.size()) {
            L_WARN(global::_console, "Invalid header: need {0} attributes but got {1}",
                   attr_number, attributes.size());
        }
    } else {
            L_WARN(global::_console, "Invalid header");
    }
    return attributes;
}

std::vector<core::Attribute>
PercentileProtocol::Imp::parseHeader(std::istream &in) {
    return __parseHeader(in);
}

bool PercentileProtocol::Imp::createEncDoneFile(const std::string &outputDirPath,
                                                const int record_nr,
                                                const std::vector<core::Attribute> &attrs) const {
    FILE *fd = util::createDoneFile(outputDirPath);
    if (fd == NULL) {
        L_WARN(global::_console,
               "Can not create the done file under {0}",
               outputDirPath);
        return false;
    }

    std::stringstream sstream;
    sstream << '#' << attrs.size();
    for (auto &attr : attrs)
        sstream << " " << std::to_string(attr.size);
    sstream << "\n" << record_nr;
    std::string header = sstream.str();
    fwrite(header.c_str(), header.size(), 1UL, fd);
    fclose(fd);
    return true;
}

bool PercentileProtocol::Imp::createEvalDoneFile(const std::string &outputDirPath) const {
    FILE *fd = util::createDoneFile(outputDirPath);
    if (fd) {
        fclose(fd);
        return true;
    } else {
        return false;
    }
}

bool PercentileProtocol::Imp::createDecDoneFile(const std::string &outputDirPath) const {
    FILE *fd = util::createDoneFile(outputDirPath);
    if (fd) {
        fclose(fd);
        return true;
    } else {
        return false;
    }
}

PercentileProtocol::PercentileProtocol() : Protocol("k-percentile") {
    imp_ = std::make_shared<PercentileProtocol::Imp>();
}

bool PercentileProtocol::encrypt(const std::string &inputFilePath,
                                 const std::string &outputDirPath,
                                 bool local_compute,
                                 const core::PubKeyWrapper &pk,
                                 const core::ContextWrapper &context)
{
    if (!imp_) return false;
    return imp_->encrypt(inputFilePath, outputDirPath,
                         local_compute, pk.single, context.single);
}

bool PercentileProtocol::decrypt(const std::string &inputFilePath,
                                 const std::string &outputDirPath,
                                 const core::PubKeyWrapper &pk,
                                 const core::SecKeyWrapper &sk,
                                 const core::ContextWrapper &context)
{
    if (!imp_) return false;
    return imp_->decrypt(inputFilePath, outputDirPath,
                         pk.single, sk.single, context.single);
}

bool PercentileProtocol::evaluate(const StringList &inputDirs,
                                  const std::string &outputDir,
                                  const StringList &params,
                                  const core::PubKeyWrapper &pk,
                                  const core::ContextWrapper &context)
{
    if (!imp_) return false;
    return imp_->evaluate(inputDirs, outputDir,
                          params, pk.single, context.single);
}

core::FHEArg PercentileProtocol::parameters() const {
    core::FHEArg arg = { .p = 8191, .m = 16384, .r = 1, .L = 8 };
    return arg;
}

int UserCipherLoader::loadSpecficCtxtFile(std::vector<Ctxt> &out,
                                          const std::string &dir,
                                          const core::pk_ptr &pk) const {
    std::ifstream doneFile(util::concatenate(dir, global::_doneFileName));
    int record_nr = 0;
    if (doneFile.is_open()) {
        __parseHeader(doneFile); // skip the header
        doneFile >> record_nr;
        doneFile.close();
    }
    auto files = util::listDir(dir, util::flag_t::FILE_ONLY);
    for (const std::string &file : files) {
        if (checker(file)) {
            std::string cipher_file = util::concatenate(dir, file);
            core::loadCiphers(out, pk, cipher_file);
            break;
        }
    }
    return record_nr;
}


