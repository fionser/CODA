//
// Created by riku on 2016/07/27.
//

#include <core/AES.hpp>
#include <core/ctxt_util.hpp>
#include <HElib/FHE.h>
#include <string>
#include <iostream>
#include <NTL/ZZ.h>
#include <NTL/ZZX.h>

int main() {
    NTL::ZZ key;
    NTL::RandomBits(key, 128);
    core::AES128 aes(key);

    FHEcontext context(1024, 1031, 1);
    buildModChain(context, 3);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    const FHEPubKey &pk = sk;
    Ctxt ctxt(pk);
    NTL::ZZX plain = NTL::to_ZZX(10);
    pk.Encrypt(ctxt, plain);

    std::string message(core::conv(ctxt));
    auto aes_ctx = aes.encrypt(message);

    Ctxt decrypted(pk);
    core::conv(decrypted, aes.decrypt(aes_ctx));

    plain[0] = 0;
    sk.Decrypt(plain, decrypted);
    return plain[0] == 10? 0 : 1;
}