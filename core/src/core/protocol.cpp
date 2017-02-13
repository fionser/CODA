#include "core/protocol.hpp"
#include "core/coda_wrapper.hpp"
#include "core/file_util.hpp"
#include "core/global.hpp"
#include "HElib/FHE.h"
#include "HElib/FHEContext.h"

#include <fstream>

std::shared_ptr<Protocol> CurrentProtocol::instance_ = nullptr;

bool Protocol::genKeypair() const {
    std::string dirPath = util::getDirPath(metaPath_);
    std::ofstream skStream(util::concatenate(dirPath, "fhe_key.sk"), std::ios::binary);
    std::ofstream ctxtStream(util::concatenate(dirPath, "fhe_key.ctxt"), std::ios::binary);
    std::ofstream pkStream(util::concatenate(dirPath, "fhe_key.pk"), std::ios::binary);
    if (!skStream.is_open() || !ctxtStream.is_open() || !pkStream.is_open()) {
        L_WARN(global::_console, "Can not create files under {0}", dirPath);
        return false;
    }


    auto args = parameters();
    FHEcontext context(args.m, args.p, args.r);
    buildModChain(context, args.L);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    if (isNeedKeySwitching)
        addSome1DMatrices(sk);
    hookForKeyGen(sk);
    const FHEPubKey &pk = sk;

    skStream << TYPE_SINGLE;
    skStream << sk;
    ctxtStream << TYPE_SINGLE;
    writeContextBase(ctxtStream, context);
    ctxtStream << context;
    pkStream << TYPE_SINGLE;
    pkStream << pk;

    skStream.close();
    ctxtStream.close();
    pkStream.close();
    return true;
}

