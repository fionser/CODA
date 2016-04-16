//
// Created by fionser on 4/12/16.
//
#include <fstream>
#include <NTL/ZZX.h>

#include "core/coda.hpp"
#include "core/core.hpp"
#include "core/global.hpp"
#include "core/literal.hpp"
#include "core/file_util.hpp"
#include "HElib/FHEContext.h"
#include "HElib/EncryptedArray.h"

namespace protocol {
namespace mean {
extern const core::FHEArg _fheArgs = {.m = 16384, .p = 1031, .r = 2, .L = 5};
NTL::ZZX __pack(std::vector<std::string> const& fields, EncryptedArray const& ea) {
    if (ea.size() < fields.size())
        assert(0 && "ea.size() < coeff.size() in __pack");
    std::vector<long> coeff(ea.size(), 0);
    size_t pos;
    for (size_t i = 0; i < fields.size(); i++) {
        long v = literal::stol(fields[i], &pos, 10);
        coeff[i] = 0L;
        if (pos != fields[i].size()) {
            L_WARN(global::_console, "Invalid field {0}", fields[i]);
            continue;
        }
        coeff[i] = v;
    }

    NTL::ZZX poly;
    ea.encode(poly, coeff);
    return poly;
}

static bool __dumpCtxt(std::vector<Ctxt *> const& ctxts, std::string const& outputPath) {
    return core::dumpCiphers(ctxts, outputPath);
}

static bool __encrypt(std::fstream &fin, core::pk_ptr pk, core::context_ptr context,
                      std::string const &outputFilePath) {
    const size_t BATCH_CTXT_SIZE = 50;
    auto &G = context->alMod.getFactorsOverZZ()[0];
    EncryptedArray ea(*context, G);
    std::vector<Ctxt *> ctxts(BATCH_CTXT_SIZE, nullptr);
    size_t outputFileCount = 1;
    ssize_t ctxtUsed = 0;
    auto makePath = [](std::string const& path, size_t count) -> std::string {
        return path + literal::separator + "FILE_" + std::to_string(count);
    };

    size_t nr_line = 0;
    for (std::string line; std::getline(fin, line); ) {
        if (ctxtUsed >= BATCH_CTXT_SIZE) {
            __dumpCtxt(ctxts, makePath(outputFilePath, outputFileCount++));
            ctxtUsed = 0;
        }
        if (ctxts.at(ctxtUsed) == nullptr)
            ctxts[ctxtUsed] = new Ctxt(*pk);
        auto packed = __pack(util::splitBySpace(line) , ea);
        pk->Encrypt(*ctxts[ctxtUsed], packed);
        ctxtUsed += 1;
        nr_line += 1;
    }

    if (ctxtUsed > 0) {
        std::vector<Ctxt *> tmp(ctxts.begin(), ctxts.begin() + ctxtUsed);
        __dumpCtxt(tmp, makePath(outputFilePath, outputFileCount++));
    }

    auto fd = util::createDoneFile(outputFilePath);
    char buf[1024];
    snprintf(buf, sizeof(buf), "%zd", nr_line);
    fwrite(buf, strlen(buf), 1, fd);
    fclose(fd);

    for (auto c : ctxts)
        delete c;

    return true;
}

bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             core::pk_ptr pk,
             core::context_ptr context) {
    std::fstream fin(inputFilePath);
    if (!fin.is_open()) {
        L_ERROR(global::_console, "Can not open file {0}", inputFilePath);
        return false;
    }

    std::string line;
    std::getline(fin, line);
    if (line.find("#protocol PROT_MEAN") == std::string::npos) {
        L_ERROR(global::_console,
                "Invalid file format {0}: no valid protocol type was set",
                inputFilePath);
        return false;
    }

    return __encrypt(fin, pk, context, outputDirPath);
}


static size_t __loadDoneFile(std::string const& dir) {
    std::fstream fin(util::concatenate(dir, global::_doneFileName));
    if (!fin.is_open()) {
        L_WARN(global::_console, "No .done.lock under {0}", dir);
        return 0;
    }
    size_t record = 0;
    fin >> record;
    return record;
}

bool decrypt(const std::string &inputFilePath,
             const std::string &outputDir,
             core::pk_ptr pk,
             core::sk_ptr sk,
             core::context_ptr context) {

    std::string dir = util::getDirPath(inputFilePath);
    size_t nr_record = __loadDoneFile(dir);
    if (nr_record == 0) {
        L_WARN(global::_console, "Something went wrong in {0}/.done.lock", dir);
        return true;
    }

    std::fstream fout(util::concatenate(outputDir, "FILE_1"),
                      std::ios::binary | std::ios::out);
    if (!fout.is_open()) {
        L_WARN(global::_console, "Can create file under {0}", outputDir);
        return false;
    }

    std::list<Ctxt> result;
    if (!core::loadCiphers(result, pk, inputFilePath))
        return false;
    if (result.empty()) {
        L_WARN(global::_console, "no ciphers in {0}", inputFilePath);
        return false;
    }

    EncryptedArray ea(*context, context->alMod.getFactorsOverZZ()[0]);
    std::vector<long> unpack(ea.size());
    ea.decrypt(result.front(), *sk, unpack);

    for (auto &mean : unpack)
        fout << mean *1.0 / nr_record << " ";
    fout << "\n";

    auto fd = util::createDoneFile(outputDir);
    if (!fd) {
        L_WARN(global::_console, "Can create .done.lock under {0}", outputDir);
        return false;
    }
    fwrite("DONE\n", 5, 1, fd);
    fclose(fd);
    return true;
}

static void __doSum(Ctxt *result, std::list<Ctxt> const& ciphers) {
    for (auto &c : ciphers)
        result->addCtxt(c);
}

static void __sumDir(std::string const &dir,
                     core::pk_ptr pk, core::context_ptr context,
                     Ctxt **result, size_t *record_nr) {
    size_t record = __loadDoneFile(dir);
    if (record == 0)
        return;
    *record_nr += record;

    if (*result == nullptr)
        *result= new Ctxt(*pk);

    std::list<Ctxt> ciphers;
    auto files = util::listDir(dir, util::flag_t::FILE_ONLY);
    for (auto &file : files) {
        if (file.find("FILE_") != 0)
           continue;
        core::loadCiphers(ciphers, pk, util::concatenate(dir, file));
    }
    __doSum(*result, ciphers);
}

bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              core::pk_ptr pk,
              core::context_ptr context) {
    Ctxt* result = nullptr;
    size_t record_nr = 0;

    for (auto &inputDir : inputDirs)
       __sumDir(inputDir, pk, context, &result, &record_nr);

    std::fstream fout(util::concatenate(outputDir, "File_result"), std::ios::out | std::ios::binary);
    if (!fout.is_open()) {
        L_WARN(global::_console, "Can not create File_result under {0}", outputDir);
        delete result;
        return false;
    }

    fout << *result;
    delete result;

    /// create .done.lock after evaluation
    auto fd = util::createDoneFile(outputDir);
    char buf[1024];
    snprintf(buf, sizeof(buf), "%zd", record_nr);
    fwrite(buf, strlen(buf), 1, fd);
    fclose(fd);
    return true;
}
} // namespace mean
} // namespace protocol

