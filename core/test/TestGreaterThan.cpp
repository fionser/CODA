//
// Created by riku on 2016/07/04.
//
#include <stdio.h>
#include <HElib/FHE.h>
#include <HElib/EncryptedArray.h>
#include <core/greaterthan.h>
#include <core/coda.hpp>
long __a[] = {1, 22, 23, 40, 5};
long __b[] = {10, 4, 23, 11, 76};

int test(core::sk_ptr sk, core::context_ptr &context) {
    size_t slots = context->ea->size();
    const FHEPubKey &pk = *sk;
    Ctxt ca(pk), cb(pk);
    greaterthan gt(100, context);
    NTL::ZZX poly;
    for (int i = 0; i < 5; i++) {
        std::vector<long> poly_a(slots, __a[i]);
        std::vector<long> poly_b(slots, __b[i]);
        context->ea->encrypt(ca, pk, poly_a);
        context->ea->encrypt(cb, pk, poly_b);
        context->ea->encode(poly, poly_b);
        auto ciphers = gt.compare(&ca, &cb);
        auto ciphers2 = gt.compare(&ca, poly);
        auto res = gt.open_result(ciphers, sk);
        auto res2 = gt.open_result(ciphers2, sk);
        assert(res == res2);
        if (res == greaterthan::Result::ERROR) {
            printf("greaterthan::Result::ERROR\n");
            return -1;
        }
        if (__a[i] >= __b[i] && res != greaterthan::Result::GREATER_OR_EQUAL) {
            printf("__a[i]: %ld > __b[i] %ld\n", __a[i], __b[i]);
            return -1;
        }
        if (__a[i] < __b[i] && res != greaterthan::Result::LESS) {
            printf("__a[i]: %ld < __b[i] %ld\n", __a[i], __b[i]);
            return -1;
        }
    }
    return 0;
}

int main() {
    long m = 5227;
    long p = 67499;
    core::context_ptr context = std::make_shared<FHEcontext>(m, p, 1);
    buildModChain(*context, 5);
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    addSome1DMatrices(*sk);
    return test(sk, context);
}