//
// Created by riku on 2016/07/31.
//
#include "HElib/EncryptedArray.h"
#include "HElib/FHE.h"
int main() {
    FHEcontext context(16384, 8191, 1);
    buildModChain(context, 3);
    std::vector<long> slots(context.ea->size());
    for (long i = 0; i < context.ea->size(); i++)
       slots[i] = i + 1;
    FHESecKey sk(context);
    sk.GenSecKey(64);
    const FHEPubKey &pk = sk;
    Ctxt ctxt(pk);
    context.ea->encrypt(ctxt, pk, slots);

    NTL::ZZX poly;

    FHE_NTIMER_START(decode1Slot);
    sk.Decrypt(poly, ctxt);
    std::vector<long> positions(10);
    for (size_t i = 0; i < 10; i++) positions[i] = i;
    context.ea->decodeSlots(slots, poly, positions);
    for (size_t i = 0; i < 10; i++) assert(slots[i] == i + 1);
    FHE_NTIMER_STOP(decode1Slot);

    FHE_NTIMER_START(ea_decrypt);
    context.ea->decrypt(ctxt, sk, slots);
    FHE_NTIMER_STOP(ea_decrypt);

    printNamedTimer(std::cout, "decode1Slot");
    printNamedTimer(std::cout, "ea_decrypt");
    return 0;
}

