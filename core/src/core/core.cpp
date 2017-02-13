#include <sstream>
#include <iostream>

#include "HElib/FHE.h"
#include "HElib/FHEContext.h"
#include "core/coda_wrapper.hpp"
#include "core/core.hpp"
#include "core/PPE/types.hpp"
#include "core/PPE/Context.hpp"
#include "core/PPE/PubKey.hpp"
#include "core/PPE/SecKey.hpp"
#include "core/global.hpp"
#include "core/file_util.hpp"
#include "core/protocol.hpp"
#include "core/protocol-mean.hpp"
#include "core/protocol-contingency.hpp"
#include "core/protocol-hybrid-contingency.hpp"
#include "core/protocol-percentile.hpp"

namespace core {
static bool setProtocol(const std::string &metaFilePath) {
    util::Meta meta;
    bool ok;
    std::tie(meta, ok) = util::readMetaFile(metaFilePath);
    if (!ok || meta.find("protocol") == meta.end() || meta["protocol"].empty())
        return false;

    switch (getProtocol(meta["protocol"].front())) {
    case core::Protocol::PROT_CI2:
        assert(0 && "Not implemented yet.");
        //__currentProtocol = nullptr;
        break;
    case core::Protocol::PROT_CON:
        CurrentProtocol::set(std::make_shared<ContingencyTableProtocol>());
        break;
    case core::Protocol::PROT_MEAN:
        CurrentProtocol::set(std::make_shared<MeanProtocol>());
        break;
    case core::Protocol::PROT_HYBRID_CON:
        CurrentProtocol::set(std::make_shared<HybridContingencyTable>());
        break;
    case core::Protocol::PROT_PERCENTILE:
        CurrentProtocol::set(std::make_shared<PercentileProtocol>());
        break;
    default:
        break;
    }

    if (CurrentProtocol::get() != nullptr) {
        CurrentProtocol::get()->setMeta(metaFilePath);
        return true;
    } else {
        return false;
    }
}

static core::context_ptr loadSingleContext(std::istream &in) {
    unsigned long m, p, r;
    std::vector<long> gens, ords;
    readContextBase(in, m, p, r, gens, ords);
    auto context = std::make_shared<FHEcontext>(m, p, r, gens, ords);
    if (context == nullptr) {
        return nullptr;
    }
    in >> *context;
    return context;
}

static ppe::context_ptr loadPPEContext(std::istream &in) {
    auto context = std::make_shared<ppe::Context>();
    if (!context) return nullptr;
    context->restore(in);
    return context;
}

ContextWrapper loadContext(bool *ok, const std::string &contextFile) {
    ContextWrapper context = { .single = nullptr, .ppe = nullptr };
    std::ifstream in(contextFile, std::ios::binary);
    if (!in.is_open()) {
        if (ok) *ok = false;
        return context;
    }

    int type;
    in >> type;
    if (type == TYPE_SINGLE) {
        context.single = loadSingleContext(in);
        if (ok) *ok = context.single != nullptr;
    } else if (type == TYPE_PPE) {
        context.ppe = loadPPEContext(in);
        if (ok) *ok = context.ppe != nullptr;
    } else {
        if (ok) *ok = false;
    }

    in.close();
    return context;
}

static core::pk_ptr loadSinglePubKey(std::istream &in, const core::context_ptr &context) {
    auto pk = std::make_shared<FHEPubKey>(*context);
    if (pk == nullptr)
        return nullptr;

    in >> *pk;
    return pk;
}

static ppe::pk_ptr loadPPEPubKey(std::istream &in, const ppe::context_ptr &context) {
    auto pk = std::make_shared<ppe::PubKey>(*context);
    if (pk == nullptr)
        return nullptr;
    pk->restore(in, *context);
    return pk;
}

PubKeyWrapper loadPK(bool *ok, const ContextWrapper &context, const std::string &pkFile) {
    PubKeyWrapper pk = { .single = nullptr, .ppe = nullptr };
    std::ifstream in(pkFile, std::ios::binary);
    if (!in.is_open()) {
        if (ok) *ok = false;
        return pk;
    }

    int type;
    in >> type;
    if (type == TYPE_SINGLE) {
        pk.single = loadSinglePubKey(in, context.single);
        if (ok) *ok = pk.single != nullptr;
    } else if (type == TYPE_PPE) {
        pk.ppe = loadPPEPubKey(in, context.ppe);
        if (ok) *ok = pk.ppe != nullptr;
    } else {
        if (ok) *ok = false;
    }
    in.close();
    return pk;
}

static core::sk_ptr loadSingleSecKey(std::istream &in, const core::context_ptr &context) {
    auto sk = std::make_shared<FHESecKey>(*context);
    if (sk == nullptr)
        return nullptr;
    in >> *sk;
    return sk;
}

static ppe::sk_ptr loadPPESecKey(std::istream &in, const ppe::context_ptr &context) {
    auto sk = std::make_shared<ppe::SecKey>(*context);
    if (sk == nullptr)
        return nullptr;
    sk->restore(in, *context);
    return sk;
}

SecKeyWrapper loadSK(bool *ok, const ContextWrapper &context, const std::string &skFile) {
    SecKeyWrapper sk = { .single = nullptr, .ppe = nullptr };
    std::fstream in(skFile, std::ios::binary | std::ios::in);
    if (!in.is_open()) {
        L_WARN(global::_console, "Can not open {0}", skFile);
        if (ok) *ok = false;
        return sk;
    }

    int type;
    in >> type;
    if (type == TYPE_SINGLE) {
        sk.single = loadSingleSecKey(in, context.single);
        if (ok) *ok = sk.single != nullptr;
    } else if (type == TYPE_PPE) {
        sk.ppe = loadPPESecKey(in, context.ppe);
        if (ok) *ok = sk.ppe != nullptr;
    } else {
        if (ok) *ok = false;
    }
    in.close();
    return sk;
}

static bool loadFromMetaFile(const std::string &metaFilePath,
                             ContextWrapper &context,
                             PubKeyWrapper &pk,
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
        L_ERROR(global::_console, "Can not load fhe_key.pk in {0}", dirPath);
        return false;
    }

    return true;
}

static bool loadFromMetaFile(const std::string &metaFilePath,
                             core::ContextWrapper &context,
                             core::PubKeyWrapper &pk,
                             core::SecKeyWrapper &sk,
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

bool dumpCiphers(const std::vector<Ctxt> &ciphers, const std::string &file) {
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
    core::ContextWrapper context;
    core::PubKeyWrapper pk;
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
    core::ContextWrapper context;
    core::PubKeyWrapper pk;
    core::SecKeyWrapper sk;
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
    core::ContextWrapper context;
    core::PubKeyWrapper pk;
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
