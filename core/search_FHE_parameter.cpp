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
    long level = 10;
    long plain = 2;
    mapping.arg("p", plain, "plaintext");
    mapping.arg("L", level, "level");
    mapping.arg("k", security, "security level");
    mapping.arg("S", slots, "how many slots?");
    mapping.parse(argc, argv);

//    long m = 1024;
//    for (;;) {
//        do {
//            m = m << 1;
//        } while (phi_N(m) < 8515);
//
//        long trial = 0;
//        long p = NTL::NextPrime(plain, 15);
//        while (numOfSlots(m, p) < (slots << 1)) {
//            p = NTL::NextPrime(p + 1);
//            if (p == m) p = NTL::NextPrime(p + 1, 15);
//            trial += 1;
//            if (trial > 10000) break;
//        }
//
//        if (numOfSlots(m, p) >= slots) {
//            plain = p;
//            break;
//        }
//        printf("trial failed with %ld\n", m);
//    }

    long m = 16384;
    plain = 8191;
    FHEcontext context(m, plain, 1);
    buildModChain(context, level);
    std::cout << "SL " << context.securityLevel() << "\n";
    std::cout << "Slots " << context.ea->size() << "\n";
    std::cout << "Num Gens " << context.zMStar.numOfGens() << "\n";
    printf("m = %ld, p = %ld, level = %ld security = %ld\n", m, plain, level, security);
    return 0;
}

