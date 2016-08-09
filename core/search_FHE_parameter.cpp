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

void testIt() {
    long ps[] = {4139, 5657, 6091, 15797, 4231, 6323, 7321, 6791, 7459};
    long m = 27893;
    for (auto p : ps) {
        PAlgebra zMStar(m, p);
        std::cout << p << ": ";
        for (long i = 0; i < zMStar.numOfGens(); i++)
            std::cout << zMStar.SameOrd(i) << " " ;
        std::cout << "\n" << zMStar.getNSlots() << "\n";
    }
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
    
    long m = 27893;
    std::cout << "m\tp\t#slot\tSameOrd\n";
    for (long t = 0; t < 10; t++) {
        long p = NTL::NextPrime(1 << 12);
        while (p < m) {
            PAlgebra zMStar(m, p);
            if (zMStar.numOfGens() == 1 && zMStar.getNSlots() >= 36) {
                printf("%ld\t%ld\t%ld\t%d\n", m, p, zMStar.getNSlots(), zMStar.SameOrd(0));
            }
            p = NTL::NextPrime(p + 2);
        }
        m = NTL::NextPrime(m + 2);
    }

    // FHEcontext context(m, plain, 1);
    // buildModChain(context, level);
    // std::cout << "SL " << context.securityLevel() << "\n";
    // std::cout << "Slots " << context.ea->size() << "\n";
    // std::cout << "Num Gens " << context.zMStar.numOfGens() << "\n";
    // printf("m = %ld, p = %ld, level = %ld security = %ld\n", m, plain, level, security);
    return 0;
}

