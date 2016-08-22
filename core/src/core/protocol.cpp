#include "core/protocol.hpp"
#include "HElib/FHE.h"
#include <thread>
namespace core {
ProtocolFactory* ProtocolFactory::__instance = nullptr;

ProtocolFactory * ProtocolFactory::instance() {
    static std::mutex lock;
    lock.lock();
    if (!__instance)
        __instance = new ProtocolFactory();
    lock.unlock();
    return __instance;
}

void ProtocolFactory::register_builder(const std::string &type,
                                       const Builder &builder)
{

}
} // namespace core
namespace protocol {
bool genKeypair(core::Protocol protocol,
		std::fstream &skStream,
		std::fstream &ctxtStream,
		std::fstream &pkStream) {
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
    FHESecKey sk(context);
    sk.GenSecKey(64);
    addSome1DMatrices(sk);
    const FHEPubKey &pk = sk;
    skStream << sk;
    ctxtStream << args;
    pkStream << pk;

    return true;
}
} // namespace protocol
