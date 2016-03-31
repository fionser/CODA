#include <sstream>
#include <iostream>
#include <core/global.hpp>

#include "HElib/FHE.h"
#include "core/core.hpp"
#include "core/file_util.hpp"
#include "core/protocol.hpp"
namespace core {
bool genKeypair(Protocol protocol, const std::string &metaFilePath) {
    std::string dirPath = util::getDirPath(metaFilePath);
    std::fstream skStream(util::concatenate(dirPath, "fhe_key.sk"),
                          std::ios::binary | std::ios::out);
    std::fstream ctxtStream(util::concatenate(dirPath, "fhe_key.ctxt"),
                            std::ios::binary | std::ios::out);
    std::fstream pkStream(util::concatenate(dirPath, "fhe_key.pk"),
                          std::ios::binary | std::ios::out);
    if (!skStream.is_open() || !ctxtStream.is_open() || !pkStream.is_open())
        return false;
    bool ok = protocol::genKeypair(protocol, skStream, ctxtStream, pkStream);
    skStream.close();
    ctxtStream.close();
    pkStream.close();
    return ok;
}

context_ptr loadContext(bool *ok, const std::string &contextFile) {
    std::fstream in(contextFile, std::ios::binary | std::ios::in);
    if (!in.is_open()) {
        if (ok) *ok = false;
        return nullptr;
    }

    FHEArg args;
    in >> args;
    auto context = std::make_shared<FHEcontext>(args.m, args.p, args.r);
    if (context == nullptr) {
        if (ok) *ok = false;
        return nullptr;
    }
    buildModChain(*context, args.L);
    return context;
}

pk_ptr loadPK(bool *ok, const context_ptr &context, const std::string &pkFile) {
    std::fstream in(pkFile, std::ios::binary | std::ios::in);
    if (!in.is_open()) {
        if (ok) *ok = false;
        return nullptr;
    }

    auto pk = std::make_shared<FHEPubKey>(*context);
    if (pk == nullptr) {
        if (ok) *ok = false;
        return nullptr;
    }

    in >> *pk;
    in.close();

    if (ok) *ok = true;
    return pk;
}

sk_ptr loadSK(bool *ok, const context_ptr &context, const std::string &skFile) {
    if (context == nullptr) {
        if (ok) *ok = false;
        return nullptr;
    }

    std::fstream in(skFile, std::ios::binary | std::ios::in);
    if (!in.is_open()) {
        L_WARN(global::_console, "Can not open {0}", skFile);
        if (ok) *ok = false;
        return nullptr;
    }

    auto sk = std::make_shared<FHESecKey>(*context);
    if (sk == nullptr) {
        if (ok) *ok = false;
        return nullptr;
    }

    in >> *sk;
    in.close();
    if (ok) *ok = true;
    return sk;
}

static bool loadFromMetaFile(const std::string &metaFilePath,
                             core::context_ptr &context,
                             core::pk_ptr &pk,
                             util::Meta &meta) {
    bool ok;
    std::tie(meta, ok) = util::readMetaFile(metaFilePath);

    if (!ok) {
        L_ERROR(global::_console, "Can not find meta file {0}", metaFilePath);
        return false;
    }

    if (meta.find("protocol") == meta.end() || meta["protocol"].empty()) {
        L_ERROR(global::_console, "No protocol was set in {0}", metaFilePath);
        return false;
    }

    auto dirPath = util::getDirPath(metaFilePath);
    auto contextPath = util::concatenate(dirPath, "fhe_key.ctxt");
    context = loadContext(&ok, contextPath);
    if (!ok) {
        L_ERROR(global::_console, "Can not load fhe_key.ctxt in {0}", dirPath);
        return false;
    }

    auto pkPath = util::concatenate(dirPath, "fhe_key.pk");
    pk = loadPK(&ok, context, pkPath);
    if (!ok) {
        context = nullptr;
        L_ERROR(global::_console, "Can not load fhe_key.pk in {0}", dirPath);
        return false;
    }

    return true;
}

static bool loadFromMetaFile(const std::string &metaFilePath,
                             core::context_ptr &context,
                             core::pk_ptr &pk,
                             core::sk_ptr &sk,
                             util::Meta &meta) {
    if (!loadFromMetaFile(metaFilePath, context, pk, meta))
        return false;

    auto dirPath = util::getDirPath(metaFilePath);
    auto skPath = util::concatenate(dirPath, "fhe_key.sk");
    bool ok;
    sk = loadSK(&ok, context, skPath);
    if (!ok) {
        L_ERROR(global::_console, "Can not load fhe_key.sk in {0}", dirPath);
        return false;
    }

    return true;
}

bool encrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             const std::string &metaFilePath) {
    util::Meta meta;
    core::context_ptr context = nullptr;
    core::pk_ptr pk = nullptr;
    if(!loadFromMetaFile(metaFilePath, context, pk, meta))
        return false;

    auto protocol = core::getProtocol(meta["protocol"].front());
    switch (protocol) {
    case core::Protocol::PROT_CI2:
        return protocol::chi2::encrypt(inputFilePath, outputFilePath, pk);
    case core::Protocol::PROT_CON:
        return protocol::contingency::encrypt(inputFilePath, outputFilePath, pk, context);
    default:
        L_ERROR(global::_console, "Unkonwn protocol was set in {0}", metaFilePath);
        return false;
    }
}

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             const std::string &metaFilePath) {
    util::Meta meta;
    core::context_ptr context = nullptr;
    core::pk_ptr pk = nullptr;
    core::sk_ptr sk = nullptr;
    if (!loadFromMetaFile(metaFilePath, context, pk, sk, meta))
        return false;
    auto protocol = getProtocol(meta["protocol"].front());
    switch (protocol) {
    case core::Protocol::PROT_CI2:
        return protocol::chi2::decrypt(inputFilePath, outputFilePath, pk, sk);
    case core::Protocol::PROT_CON:
        return protocol::contingency::decrypt(inputFilePath, outputFilePath, pk, sk, context);
    default:
        L_ERROR(global::_console, "Unkonwn protocol was set in {0}", metaFilePath);
        return false;
    }
}

bool loadCiphers(std::vector<Ctxt> &out, const pk_ptr &pk, const std::string &file) {
    std::ifstream in(file);
    if (!in.is_open()) {
        L_ERROR(global::_console, "Can not open file {0}", file);
        return false;
    }

    Ctxt cipher(*pk);
    while (!in.eof() && in.peek() != -1) {
        in >> cipher;
        out.push_back(cipher);
    }
    in.close();
    return true;
}

bool loadCiphers(std::list<Ctxt> &out, const pk_ptr &pk, const std::string &file) {
    std::ifstream in(file);
    if (!in.is_open()) {
        L_ERROR(global::_console, "Can not open file {0}", file);
        return false;
    }

    Ctxt cipher(*pk);
    while (!in.eof() && in.peek() != -1) {
        in >> cipher;
        out.push_back(cipher);
    }
    in.close();
    return true;
}

bool dumpCiphers(const std::list<Ctxt> &ciphers, const std::string &file) {
    std::ofstream out(file);
    if (!out.is_open()) {
        L_ERROR(global::_console, "Can not dump into file {0}", file);
        return false;
    }

    for (auto &c : ciphers) {
        out << c;
    }
    out.close();
    return true;
}

bool evaluate(const std::string &sessionDirPath,
              const std::string &outputDirPath,
              const std::string &metaFilePath) {
    auto dirs = util::listDir(sessionDirPath, util::flag_t::DIR_ONLY);
    std::vector<std::string> userDirs;
    for (auto dir : dirs) {
        if (dir.compare(".") == 0 || dir.compare("..") == 0)
            continue;
        if (dir.compare("Meta") == 0 || dir.compare("result") == 0)
            continue;
        userDirs.push_back(util::concatenate(sessionDirPath, dir));
    }

    util::Meta meta;
    core::context_ptr context;
    core::pk_ptr pk;
    if (!loadFromMetaFile(metaFilePath, context, pk, meta))
        return false;

    auto protocol = core::getProtocol(meta["protocol"].front());
    switch (protocol) {
    case core::Protocol::PROT_CI2:
        return protocol::chi2::evaluate(userDirs, outputDirPath, pk);
    case core::Protocol::PROT_CON:
        return protocol::contingency::evaluate(userDirs, outputDirPath, pk);
    default:
        L_ERROR(global::_console, "Unknown protocol was set in {0}", metaFilePath);
        return false;
    }
}
} // nammespae core
