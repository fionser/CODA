#include "HElib/FHE.h"
#include "HElib/NumbTh.h"
#include "HElib/EncryptedArray.h"
#include <iostream>

void Test(long m, long p, long r, long L)
{
    FHEcontext context(m, p, r);
    buildModChain(context, L);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    FHEPubKey pk = sk;

    const auto &G = context.alMod.getFactorsOverZZ()[0];
    EncryptedArray ea(context, G);
}

int main(int argc, char *argv[]) {
    ArgMapping amap;
    long p = 2;
    amap.arg("p", p);
    long r = 1;
    amap.arg("r", r);
    long m = 1031;
    amap.arg("m", m);
    long L = 3;
    amap.arg("L", L);
    long k = 80;
    amap.arg("k", k);
    amap.parse(argc, argv);
    printf("FindM = %ld\n", FindM(80, L, 3, p, 1, 100, 0, true));
    printf("#slot %ld\n", phi_N(m) / multOrd(p, m));
    Test(m, p, r, L);
    return 0;
}
