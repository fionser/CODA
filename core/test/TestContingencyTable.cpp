//
// Created by riku on 2016/07/19.
//
#include "core/ctxt_util.hpp"
#include "core/contingency_table.hpp"
#include "HElib/Ctxt.h"
#include "HElib/FHE.h"
#include "HElib/EncryptedArray.h"
#include <list>
#include <map>
#ifdef FHE_THREADS
#define NR_THREADS 16
#else
#define NR_THREADS 1
#endif
struct Pair_t {
	long u;
	long v;
	Pair_t(long u, long v) : u(u), v(v) {}
    Pair_t(const Pair_t &oth) {
        u = oth.u;
        v = oth.v;
    }
    Pair_t& operator=(const Pair_t &oth) {
        u = oth.u;
        v = oth.v;
        return *this;
    }

	bool operator==(const Pair_t &b) const {
		return u == b.u && v == b.v;
	}

	bool operator<(const Pair_t &b) const {
        if (u < b.u) return true;
        return v < b.v;
	}
};

static void print(const std::vector<std::vector<int>> &counting) {
    printf("   ");
    auto Q = counting.front().size();
    for (size_t v = 0; v < Q; v++)
        printf("%3ld", v);
    printf("\n");
    auto P = counting.size();
    for (size_t u = 0; u < P; u++) {
        printf("%3ld", u);
        for (size_t v = 0; v < Q; v++)
            printf("%3d", counting[u][v]);
        printf("\n");
    }
}

std::vector<std::vector<long>>
generate_data(core::Attribute P, core::Attribute Q, long N, long slots) {
	std::vector<std::vector<long>> data;
	std::vector<std::vector<int>> counting(P.size, std::vector<int>(Q.size, 0));
	for (long i = 0; i < N; i++) {
		long u = static_cast<long>(NTL::RandomBnd(P.size));
		long v = static_cast<long>(NTL::RandomBnd(Q.size));

		Pair_t pp(u, v);
		counting.at(u).at(v) += 1;

		std::vector<long> row(slots, 0);
		row.at(P.offset + u) = 1;
		row.at(Q.offset + v) = 1;
		data.push_back(row);
	}
    print(counting);
	return data;
}

void test_CT(const long N) {
    long parameters[][2] = {
            {8219, 77933}, // 80-bits
            {16384, 6143}, // 80-bits
            {16384, 8191}, // 80-bits
            {16384 * 2, 8191}, // 200+ bits
    };

    long m = parameters[1][0];
    long p = parameters[1][1];

    core::context_ptr context = std::make_shared<FHEcontext>(m, p, 1);
    buildModChain(*context, 10);
    std::cout << "SLevel " << context->securityLevel() << "\n";
    std::cout << "Num of Gens " << context->zMStar.numOfGens() << "\n";
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    addSome1DMatrices(*sk);
    core::pk_ptr pk = std::make_shared<FHEPubKey>(*sk);

    auto type = core::Attribute::Type::CATEGORICAL;
    struct core::Attribute P = { .text = "P", .type = type, .size = 5, .offset = 0};
    struct core::Attribute Q = { .text = "Q", .type = type, .size = 11, .offset = P.size};

    auto ea = context->ea;
    auto _data = generate_data(P, Q, N, ea->size());
    std::vector<Ctxt> ctxts(N, *pk);
    std::atomic<size_t> counter(0);
    auto encrypt_program = [&]() {
        size_t sze = ctxts.size();
        size_t next;
        while ((next = counter.fetch_add(1UL)) < sze) {
        	ea->encrypt(ctxts.at(next), *pk, _data.at(next));
        }
    };
    std::vector<std::thread> workers;
    for (size_t wr = 0; wr < NR_THREADS; wr++) workers.push_back(std::thread(encrypt_program));
    for (auto &wr : workers) wr.join();

    long threshold = std::max<long>(2L, N / (P.size * Q.size));
    printf("threshold %ld\n", threshold);
    auto helper = new core::PrivateContingencyTableHelper(P, Q, /*threshold = */threshold, ea);
    core::PrivateContingencyTable CT(context, helper);

    auto encrypted_CT = CT.evaluate(ctxts);
    auto &n_uv = encrypted_CT.n_uv;
    auto &gamma = encrypted_CT.gamma;
    auto &tilde_gamma = encrypted_CT.tilde_gamma;

    FHE_NTIMER_START(Decryption);
    std::vector<core::PrivateContingencyTableHelper::Publishable> publishable;
    helper->open_gamma(publishable, gamma, tilde_gamma, ea, sk);
    auto counts = helper->final_decrypt(n_uv, publishable, sk, ea);
    FHE_NTIMER_STOP(Decryption);

    printf("Evaluated %ld records\n", ctxts.size());
    std::vector<std::vector<int>> ctable(P.size, std::vector<int>(Q.size, 0));
    for (size_t x = 0; x < counts.size(); x++) {
        auto u = x / Q.size;
        auto v = x % Q.size;
        ctable[u][v] = counts.at(x);
    }
    print(ctable);

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
    buildModChain(*context, 9);
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

    const size_t N = 10;
    Ctxt ctxt(*pk);
    ea->encrypt(ctxt, *pk, slots);

    auto type = core::Attribute::Type::CATEGORICAL;
    // struct core::Attribute P = { text : "P", type: type, size : 2, offset : 0};
    // struct core::Attribute Q = { text : "Q", type: type, size : 3, offset : 2};
    struct core::Attribute P = { .text = "P", .type= type, .size = 2, .offset = 0};
    struct core::Attribute Q = { .text = "Q", .type= type, .size = 3, .offset = 2};
    auto helper = new core::PrivateContingencyTableHelper(P, Q, 5, ea);
    helper->setCT(&ctxt);
    auto repeated = helper->repeat(500);
    ea->decrypt(*repeated, *sk, slots);
    for (auto &s : slots)
        printf("%ld ", s);
    printf("\n");
}

int main(int argc, char *argv[]) {
    ArgMapping mapping;
    long N = 10;
    mapping.arg("N", N, "number of ciphers");
    mapping.parse(argc, argv);
    test_CT(N);
//    test_crt();
//    test_repeat();
    return 0;
}
