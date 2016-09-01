#include "../include/HElib/replicate.h"
#include "../include/HElib/FHE.h"
#include "../include/HElib/EncryptedArray.h"
#include "../include/HElib/NumbTh.h"
#include "../include/HElib/timing.h"

#include <iostream>
int main(int argc, char *argv[]) {
    long m, p, L;
    ArgMapping mapping;
    mapping.arg("m", m, "m");
    mapping.arg("p", p, "p");
    mapping.arg("L", L, "L");
    mapping.parse(argc, argv);

    FHEcontext context(m, p, 1);
    buildModChain(context, L);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    const FHEPubKey &pk = sk;
    auto ea = context.ea;
    std::cout << context.securityLevel() << " #slots: " << ea->size() << "\n";

    std::vector<long> slots(ea->size(), 0);
    for (long i = 0; i < ea->size(); i++) slots[i] = i + 1;
    Ctxt ctx(pk);
    ea->encrypt(ctx, pk, slots);
    std::cout << ctx.log_of_ratio() << "\n";
    {
        Ctxt tmp(ctx);
        FHE_NTIMER_START(replicate1);
        replicate(*ea, tmp, 2);
        FHE_NTIMER_STOP(replicate1);
        std::cout << tmp.log_of_ratio() << "\n";
    }

    {
        Ctxt tmp(ctx);
        FHE_NTIMER_START(replicate2);
        replicate(*ea, tmp, 2, 6);
        FHE_NTIMER_STOP(replicate2);
        std::cout << tmp.log_of_ratio() << "\n";
    }

    printNamedTimer(std::cout, "replicate1");
    printNamedTimer(std::cout, "replicate2");
    return 0;
}
