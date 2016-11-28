#include "core/protocol.hpp"
#include "core/file_util.hpp"
#include "HElib/FHE.h"
#include "HElib/FHEContext.h"

#include <fstream>
bool Protocol::genKeypair(const std::string &metaPath) const {
    auto args = parameters();
    FHEcontext context(args.m, args.p, args.r);
    buildModChain(context, args.L);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    addSome1DMatrices(sk);
    const FHEPubKey &pk = sk;

    std::string dirPath = util::getDirPath(metaPath);
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

namespace protocol {
bool genKeypair(core::Protocol protocol,
                std::ofstream &skStream,
                std::ofstream &ctxtStream,
                std::ofstream &pkStream) {
    core::FHEArg args;
    switch (protocol) {
    case core::Protocol::PROT_CI2:
        args = chi2::_fheArgs;
        break;
    case core::Protocol::PROT_CON:
        args = contingency::_fheArgs;
        break;
    case core::Protocol::PROT_MEAN:
        args = mean::_fheArgs;
        break;
    default:
        return false;
    }

    FHEcontext context(args.m, args.p, args.r);
    buildModChain(context, args.L);
    writeContextBase(ctxtStream, context);
    ctxtStream << context;

    FHESecKey sk(context);
    sk.GenSecKey(64);
    addSome1DMatrices(sk);
    skStream << sk;

    const FHEPubKey &pk = sk;
    pkStream << pk;

    return true;
}
} // namespace protocol
