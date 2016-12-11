#include "core/protocol.hpp"
#include "core/file_util.hpp"
#include "HElib/FHE.h"
#include "HElib/FHEContext.h"

#include <fstream>

std::shared_ptr<Protocol> CurrentProtocol::instance_ = nullptr;

bool Protocol::genKeypair() const {
    auto args = parameters();
    FHEcontext context(args.m, args.p, args.r);
    buildModChain(context, args.L);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    addSome1DMatrices(sk);
    const FHEPubKey &pk = sk;

    std::string dirPath = util::getDirPath(metaPath_);
    std::ofstream skStream(util::concatenate(dirPath, "fhe_key.sk"), std::ios::binary);
    std::ofstream ctxtStream(util::concatenate(dirPath, "fhe_key.ctxt"), std::ios::binary);
    std::ofstream pkStream(util::concatenate(dirPath, "fhe_key.pk"), std::ios::binary);
    if (!skStream.is_open() || !ctxtStream.is_open() || !pkStream.is_open())
        return false;

    skStream << sk;
    writeContextBase(ctxtStream, context);
    ctxtStream << context;
    pkStream << pk;

    skStream.close();
    ctxtStream.close();
    pkStream.close();
    return true;
}

