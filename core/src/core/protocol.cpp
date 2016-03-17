#include "core/protocol.hpp"
#include "HElib/FHE.h"
#include "HElib/NumbTh.h"
#include "HElib/EncryptedArray.h"
namespace protocol {
bool genKeypair(core::Protocol protocol,
		std::fstream &skStream,
		std::fstream &ctxtStream,
		std::fstream &pkStream) {
    core::FHEArg args;
    switch (protocol) {
    case core::Protocol::PROT_CI2:
	args = chi2::fheArgs;
	break;
    case core::Protocol::PROT_CON:
	args = contingency::fheArgs;
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
