//
// Created by riku on 2016/07/19.
//
#include "core/ctxt_util.hpp"
#include "core/contingency_table.hpp"
#include "HElib/Ctxt.h"
#include "HElib/FHE.h"
#include "HElib/EncryptedArray.h"
#ifdef FHE_THREADS
#define NR_THREADS 8
#else
#define NR_THREADS 1
#endif
void test_CT() {
    long m = 5227;
//    long m = 2110;
    long p = 67499;
    core::context_ptr context = std::make_shared<FHEcontext>(m, p, 1);
    buildModChain(*context, 8);
    std::cout << "SLevel " << context->securityLevel() << "\n";
    std::cout << "Num of Gens " << context->zMStar.numOfGens() << "\n";
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    addSome1DMatrices(*sk);
    core::pk_ptr pk = std::make_shared<FHEPubKey>(*sk);

    auto ea = context->ea;
    std::vector<long> slots(ea->size(), 0);
    // Attribute P with size = 2
    slots[0] = 1; slots[1] = 1;
    // Attribute Q with size = 4
    slots[2] = 0; slots[3] = 1; slots[4] = 1; slots[5] = 0;
    // Attribute R with size = 4
    slots[6] = 0; slots[7] = 1; slots[8] = 0; slots[9] = 0;

    Ctxt _tmp(*pk);
    ea->encrypt(_tmp, *pk, slots);
    const size_t N = 1000;
    std::vector<Ctxt> ctxts(N, _tmp);

    auto type = core::Attribute::Type::CATEGORICAL;
    struct core::Attribute P = { text : "P", type: type, size : 2, offset : 0};
    struct core::Attribute Q = { text : "Q", type: type, size : 4, offset : 2};
    struct core::Attribute R = { text : "R", type: type, size : 4, offset : 6};

    auto helper = new core::PrivateContingencyTableHelper(P, Q, 5, ea);
    core::PrivateContingencyTable CT(context, helper);
    auto encrypted_CT = CT.evaluate(ctxts);
    auto &gamma = encrypted_CT.gamma;
    auto &tilde_gamma = encrypted_CT.tilde_gamma;

    FHE_NTIMER_START(Decryption);
    std::vector<core::PrivateContingencyTableHelper::Publishable> publishable;
    helper->open_gamma(publishable, gamma, tilde_gamma, ea, sk);
    FHE_NTIMER_STOP(Decryption);
    printf("%zd publishable counting\n", publishable.size());

    //printAllTimers(std::cout);
    printNamedTimer(std::cout, "Conduction");
    printNamedTimer(std::cout, "GreaterThan");
    printNamedTimer(std::cout, "Blinding");
    printNamedTimer(std::cout, "Decryption");
    delete helper;
}

void test_crt() {
    size_t a = 1, b = 2;
    size_t A = 3, B = 4;
    auto x = core::CRT(a, b, A, B);
    assert(x == 10);
}

void test_repeat() {
     long m = 5227;
//    long m = 2110;
    long p = 67499;
    core::context_ptr context = std::make_shared<FHEcontext>(m, p, 1);
    buildModChain(*context, 8);
    std::cout << context->securityLevel() << "\n";
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    addSome1DMatrices(*sk);
    core::pk_ptr pk = std::make_shared<FHEPubKey>(*sk);

    auto ea = context->ea;
    std::vector<long> slots(ea->size(), 0);
    // Attribute P with size = 2
    slots[0] = 1; slots[1] = 2;
    // Attribute Q with size = 4
    slots[2] = 3; slots[3] = 4; slots[4] = 5; slots[5] = 6;
//    slots[6] = 7; slots[7] = 7; slots[8] = 7; slots[9] = 7;

    const size_t N = 100;
    Ctxt ctxt(*pk);
    ea->encrypt(ctxt, *pk, slots);

    auto type = core::Attribute::Type::CATEGORICAL;
    struct core::Attribute P = { text : "P", type: type, size : 2, offset : 0};
    struct core::Attribute Q = { text : "Q", type: type, size : 3, offset : 2};
    auto helper = new core::PrivateContingencyTableHelper(P, Q, 5, ea);
    helper->setCT(&ctxt);
    auto repeated = helper->repeat(500);
    ea->decrypt(*repeated, *sk, slots);
    for (auto &s : slots)
        printf("%ld ", s);
    printf("\n");
}

int main() {
    test_CT();
//    test_crt();
//    test_repeat();
    return 0;
}
