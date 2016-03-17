#include <sstream>
#include <iostream>
#include <cassert>

#include "HElib/FHE.h"
#include "HElib/NumbTh.h"
#include "HElib/EncryptedArray.h"
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
	if (!ok) *ok = false;
	return nullptr;
    }

    std::fstream in(skFile, std::ios::binary | std::ios::in);
    if (!in.is_open()) {
	if (!ok) *ok = false;
	return nullptr;
    }

    auto sk = std::make_shared<FHESecKey>(*context);
    if (sk == nullptr) {
	if (!ok) *ok = false;
	return nullptr;
    }

    in >> *sk;
    in.close();
    if (!ok) *ok = true;
    return sk;
}

bool encrypt(const std::string &inputFilePath,
	     const std::string &outputFilePath,
	     const std::string &metaFilePath) {
    auto dirPath = util::getDirPath(metaFilePath);
    auto contextPath = util::concatenate(dirPath, "fhe_key.ctxt");
    auto pkPath = util::concatenate(dirPath, "fhe_key.pk");
    bool ok;
    util::Meta meta;
    std::tie(meta, ok) = util::readMetaFile(metaFilePath);
    if (!ok) {
	std::cerr << "WARN! Can not find meta file: " << metaFilePath << "\n";
	return false;
    }

    if (meta.find("protocol") == meta.end() || meta["protocol"].empty()) {
	std::cerr << "WARN! No protocol was set in the meta file: " << metaFilePath << "\n";
	return false;
    }

    auto protocol = core::getProtocol(meta["protocol"].front());
    if (protocol == core::Protocol::PROT_UKN) {
	std::cerr << "WARN! Unknown protocol was set in the meta file: " << metaFilePath << "\n";
	return false;
    }

    auto context = loadContext(&ok, contextPath);
    if (!ok) {
	std::cerr << "WARN! Can not find the fhe_key.ctxt under directory " << dirPath << "\n";
	return false;
    }

    auto pk = loadPK(&ok, context, pkPath);
    if (!ok) {
	std::cerr << "WARN! Can not find the fhe_key.pk under directory " << dirPath << "\n";
	return false;
    }

    switch (protocol) {
    case core::Protocol::PROT_CI2:
	return protocol::chi2::encrypt(inputFilePath, outputFilePath, pk);
    case core::Protocol::PROT_CON:
	return protocol::contingency::encrypt(inputFilePath, outputFilePath, pk);
    default:
	assert(0 && "Impossible");
	return false;
    }
}

bool decrypt(const std::string &contextFile,
	     const std::string &pkFile,
	     const std::string &dataFile) {
    return true;
}

bool loadCiphers(std::vector<Ctxt> &out, const pk_ptr &pk, const std::string &file) {
    std::ifstream in(file);
    if (!in.is_open()) {
	std::cerr << "WARN! Can't not open file: " << file << "\n";
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
} // nammespae core
