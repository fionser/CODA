//
// Created by riku on 2016/07/27.
//
#include "HElib/FHE.h"
#include "HElib/NumbTh.h"
#include "HElib/EncryptedArray.h"
#include "NTL/ZZ.h"

long numOfSlots(long m, long p) {
    return phi_N(m) / multOrd(p, m);
}

//long FindM(long k, long L, long c, long p, long d, long s, long chosen_m, bool verbose=false);
int main(int argc, char *argv[]) {
    ArgMapping mapping;
    long slots = 100;
    long security = 80;
    long level = 8;
    long plain = 2;
    mapping.arg("p", plain, "plaintext");
    mapping.arg("L", level, "level");
    mapping.arg("k", security, "security level");
    mapping.arg("S", slots, "how many slots?");
    mapping.parse(argc, argv);

    long m = NTL::NextPrime(phi_N(16384 * 2));
    long p = 65537;//8191;
    long max_slots = 0;
    printf("m(%ld) = %ld\n", m, phi_N(m));
    for (long t = 0; t < 10000; t++) {
        p = NTL::NextPrime(p + 2);
        PAlgebra zMStar(m, p);
        if (zMStar.numOfGens() == 1 && zMStar.getNSlots() > max_slots) {
            max_slots = zMStar.getNSlots();
            plain = p;
            printf("%ld %ld\n", plain, max_slots);
        }
    }
//    long m = 16384;
//    plain = 8191;
    FHEcontext context(m, plain, 1);
    buildModChain(context, level);
    std::cout << "SL " << context.securityLevel() << "\n";
    std::cout << "Slots " << context.ea->size() << "\n";
    std::cout << "Num Gens " << context.zMStar.numOfGens() << "\n";
    printf("m = %ld, p = %ld, level = %ld security = %ld\n", m, plain, level, security);
    return 0;
}

