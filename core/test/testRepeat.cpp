//
// Created by riku on 2016/07/19.
//
#include <iostream>
#include "core/ctxt_util.hpp"
#include "HElib/Ctxt.h"
#include "HElib/FHE.h"
#include "HElib/EncryptedArray.h"
int main() {
    long m = 5227;
    long p = 67499;
    FHEcontext context(m, p, 1);
    buildModChain(context, 5);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    addSome1DMatrices(sk);
    FHEPubKey pk = sk;
    auto ea = context.ea;
    std::vector<long> slots(ea->size(), 0);
    slots[0] = 1;
    slots[1] = 2;
    slots[2] = 3;
    slots[3] = 4;
    Ctxt ctxt(pk);
    ea->encrypt(ctxt, pk, slots);
    for (long P : {1, 4, 6, 8, 15, 21}) {
        Ctxt repeated = core::repeat(ctxt, 3, P, *ea);
        ea->decrypt(repeated, sk, slots);
        size_t count = 0;
        for (auto pp : slots) if (pp == 3) count += 1;
        if (count != P) return -1;
    }

    return 0;
}

