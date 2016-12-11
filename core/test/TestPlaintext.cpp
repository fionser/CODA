#include "HElib/Ctxt.h"
#include "HElib/FHE.h"
#include "HElib/EncryptedArray.h"
#include "HElib/NumbTh.h"

#include <iostream>
int test_comparison(int argc, char *argv[]) {
    ArgMapping mapping;
    long m, p, L;
    mapping.arg("m", m, "m");
    mapping.arg("p", p, "p");
    mapping.arg("L", L, "L");
    mapping.parse(argc, argv);

    FHEcontext context(m, p, 1);
    buildModChain(context, L);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    addSome1DMatrices(sk);
    const FHEPubKey &pk = sk;
    const EncryptedArray *ea = context.ea;
    printf("kappa %f ell %ld\n", context.securityLevel(), ea->size());

    std::vector<Ctxt> Xs;
    for (long x : {1, 1, 0, 1, 1, 1, 1, 1}) { //(0111)_2
        Ctxt ctx(pk);
        pk.Encrypt(ctx, NTL::to_ZZX(x));
        Xs.push_back(ctx);
    }

    std::vector<long> Ys = {1, 0, 1, 1, 1, 1, 1, 1};// (1101)_2
    std::vector<Ctxt> As, Bs;
    NTL::ZZX ONE = NTL::to_ZZX(1);
    for (long i = Ys.size() - 1; i >= 0; i--) {
        if (i + 1 == Ys.size()) {
            Ctxt a(Xs[i]), b(pk);
            NTL::ZZX tmp = NTL::to_ZZX(1 - Ys[i]);
            a.multByConstant(tmp); // ai = xi * (1 - yi)

            if (Ys[i] == 0) {
            } else {
                b = Xs[i];
                b.negate();
                b.addConstant(ONE);
            } // bi = yi * (1 - xi)

            As.push_back(a);
            Bs.push_back(b);
        } else {
            Ctxt one_minus_a(As.back());
            one_minus_a.negate();
            one_minus_a.addConstant(ONE);
            if (Ys[i] == 0) {
                Ctxt next_b(Bs.back());
                next_b.multiplyBy(one_minus_a); // b^{i} = (1 - a^{i+1}) * b^{i+1}

                Ctxt tmp(Bs.back());
                tmp.negate();
                tmp.addConstant(ONE); // (1 - b^{i+1})
                Ctxt next_a(Xs[i]); // x^i
                next_a *= one_minus_a; // (1 - a^{i+1}) * x^i
                next_a += As.back(); // a^{i+1} + (1 - a^{i+1}) * x^i
                next_a *= tmp;
                next_a.reLinearize(); // a^{i} = (1 - b^{i+1}) * (a^{i+1} + (1 - a^{i+1}) * x^i)

                Bs.push_back(next_b);
                As.push_back(next_a);
            } else {
                Ctxt next_a(Bs.back());
                next_a.negate();
                next_a.addConstant(ONE);
                next_a.multiplyBy(As.back());
                As.push_back(next_a);

                Ctxt next_b(Xs[i]);
                next_b.negate();
                next_b.addConstant(ONE);
                next_b *= one_minus_a;
                next_b += Bs.back();
                next_b *= one_minus_a;
                next_b.reLinearize();
                Bs.push_back(next_b);
            }
        }
    }

    NTL::ZZX a1, b1;
    sk.Decrypt(a1, As.back());
    sk.Decrypt(b1, Bs.back());
    std::cout << a1 << " " << b1 << "\n";
    return 0;
}

int main(int argc, char *argv[]) {
    ArgMapping mapping;
    long m, p, L;
    mapping.arg("m", m, "m");
    mapping.arg("p", p, "p");
    mapping.arg("L", L, "L");
    mapping.parse(argc, argv);
//for (long ii = 1; ii < 20; ++ii) {
    {
        p = NTL::NextPrime(1024);
        FHEcontext context(m, p, 1);
        if (p > 2) {
            // context.bitsPerLevel = static_cast<long>(14 + std::ceil(std::log(m) / std::log(2.0) / 2.0 + std::log(p) / std::log(2)));
            printf("%ld ", context.bitsPerLevel);
        }
        buildModChain(context, L);
        FHESecKey sk(context);
        sk.GenSecKey(64);
        addSome1DMatrices(sk);
        const FHEPubKey &pk = sk;
        const EncryptedArray *ea = context.ea;
        printf("p %ld kappa %f ell %ld\n", p, context.securityLevel(), ea->size());
        Ctxt ctx(pk);
        pk.Encrypt(ctx, NTL::to_ZZX(1));
        printf("%ld %f ", p, ctx.log_of_ratio());
        Ctxt tmp(ctx);
        for (long t = 0; t < L - 1; t++) {
            tmp.multiplyBy(ctx);
            double lr = tmp.log_of_ratio();
            if (lr < 0)
                printf("%f ", lr);
            else
                break;
        }
        printf("\n");
    }
    return 0;
}
