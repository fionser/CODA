#include <sstream>
#include <iostream>
#include <core/global.hpp>

#include "HElib/FHE.h"
#include "HElib/FHEContext.h"
#include "core/core.hpp"
#include "core/file_util.hpp"
#include "core/protocol.hpp"
#include "core/protocol-mean.hpp"
#include "core/protocol-contingency.hpp"
namespace core {
static bool setProtocol(const std::string &metaPath) {

    switch (getProtocol(metaPath)) {
    case core::Protocol::PROT_CI2:
        //__currentProtocol = nullptr;
        break;
    case core::Protocol::PROT_CON:
        CurrentProtocol::set(std::make_shared<ContingencyTableProtocol>());
        break;
    case core::Protocol::PROT_MEAN:
        CurrentProtocol::set(std::make_shared<MeanProtocol>());
        break;
    default:
        break;
    }

    if (CurrentProtocol::get() != nullptr) {
        CurrentProtocol::get()->setMeta(metaPath);
        return true;
    } else {
        return false;
    }
}


context_ptr loadContext(bool *ok, const std::string &contextFile) {
    std::ifstream in(contextFile, std::ios::binary);
    if (!in.is_open()) {
        if (ok) *ok = false;
        return nullptr;
    }
    unsigned long m, p, r;
    std::vector<long> gens, ords;
    readContextBase(in, m, p, r, gens, ords);
    auto context = std::make_shared<FHEcontext>(m, p, r, gens, ords);
    if (context == nullptr) {
        if (ok) *ok = false;
        return nullptr;
    }
    in >> *context;
    return context;
}

pk_ptr loadPK(bool *ok, const context_ptr &context, const std::string &pkFile) {
    std::ifstream in(pkFile, std::ios::binary);
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

bool dumpCiphers(std::vector<Ctxt *> const& ciphers, std::string const& file) {
    std::ofstream out(file);
    if (!out.is_open()) {
        L_ERROR(global::_console, "Can not dump into file {0}", file);
        return false;
    }

    for (auto &c : ciphers)
        out << *c;
    out.close();
    return true;
}

bool genKeypair(const std::string &metaFilePath) {
    if (CurrentProtocol::get() == nullptr) {
        if (!core::setProtocol(metaFilePath))
            return false;
    }
    return CurrentProtocol::get()->genKeypair();
}

bool encrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             const std::string &metaFilePath,
             bool local_compute) {
    if (CurrentProtocol::get() == nullptr) {
        if (!core::setProtocol(metaFilePath))
            return false;
    }

    util::Meta meta;
    core::context_ptr context = nullptr;
    core::pk_ptr pk = nullptr;
    if(!loadFromMetaFile(metaFilePath, context, pk, meta))
        return false;
    return CurrentProtocol::get()->encrypt(inputFilePath, outputFilePath, local_compute, pk, context);
}

bool decrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             const std::string &metaFilePath) {
    if (CurrentProtocol::get() == nullptr) {
        if (!core::setProtocol(metaFilePath))
            return false;
    }

    util::Meta meta;
    core::context_ptr context = nullptr;
    core::pk_ptr pk = nullptr;
    core::sk_ptr sk = nullptr;
    if (!loadFromMetaFile(metaFilePath, context, pk, sk, meta))
        return false;
    return CurrentProtocol::get()->decrypt(inputFilePath, outputDirPath, pk, sk, context);
}

bool evaluate(const std::string &sessionDirPath,
              const std::string &outputDirPath,
              const std::string &metaFilePath,
              const std::vector<std::string> &params) {
    if (CurrentProtocol::get() == nullptr) {
        if (!core::setProtocol(metaFilePath))
            return false;
    }

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

    return CurrentProtocol::get()->evaluate(userDirs, outputDirPath, params, pk, context);
}

long apply_crt(long a, long b, long p1, long p2) {
    auto InvMod = [](long a, long p) -> long {
        long x0, x1, p0, t, q;
        p0 = p;
        x0 = 0; x1 = 1;
        if (p == 1) return 1;
        while (a > 1) {
            q = a / p;
            t = p; p = a % p; a = t;
            t = x0; x0 = x1 - q * x0; x1 = t;
        }
        if (x1 < 0) x1 += p0;
        return  x1;
    };
    long P = p1 * p2;
    long x = 0;
    x += a * InvMod(p2, p1) * p2;
    x += b * InvMod(p1, p2) * p1;
    return x % P;
}

} // nammespae core
