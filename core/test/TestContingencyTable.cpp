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
#include <thread>
#ifdef FHE_THREADS
#define NR_THREADS 36
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

static void print(const std::vector<std::vector<long>> &counting) {
    printf("   ");
    auto Q = counting.front().size();
    for (size_t v = 0; v < Q; v++)
        printf("%3ld", v);
    printf("\n");
    auto P = counting.size();
    for (size_t u = 0; u < P; u++) {
        printf("%3ld", u);
        for (size_t v = 0; v < Q; v++)
            printf("%3ld", counting[u][v]);
        printf("\n");
    }
}

typedef std::vector<std::vector<long>> CTable_t;
std::pair<CTable_t, CTable_t>
generate_data(core::Attribute P, core::Attribute Q, long N, long slots) {
	CTable_t data;
	CTable_t counting(P.size, std::vector<long>(Q.size, 0));
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
	return std::make_pair(data, counting);
}

bool check(const std::vector<std::vector<long>> &g,
           const std::vector<std::vector<long>> &s, long t) {
    auto Q = g.front().size();
    auto P = g.size();
    for (size_t u = 0; u < P; u++) {
    	for (size_t v = 0; v < Q; v++) {
	    if (g[u][v] < t || s[u][v] != 0) return false;
            if (g[u][v] != s[u][v]) return false;
	}
    }
    return true;
}

void test_CT(const long N, size_t sizeP, size_t sizeQ, long which) {
    long parameters[][3] = {
            // m, p, #slots
            {8209, 15217, 513}, // 80-bits
            {8219, 9719, 1174}, // 80-bits
            {8221, 9127, 1644}, // 80+ bits
            {8221, 24247, 2740}, // 80+ bits
            {16384, 8191, 4096}, // 80+ bits
    };

    long m = parameters[which][0];
    long p = parameters[which][1];

    core::context_ptr context = std::make_shared<FHEcontext>(m, p, 1);
    buildModChain(*context, 10);
    std::cout << "SLevel " << context->securityLevel() << "\n";
    std::cout << "Num of Gens " << context->zMStar.numOfGens() << "\n";
    std::cout << "Same order " << context->zMStar.SameOrd(0) << "\n";
    std::cout << "Num of Slots " << context->ea->size() << "\n";
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    addSome1DMatrices(*sk);
    core::pk_ptr pk = std::make_shared<FHEPubKey>(*sk);

    auto type = core::Attribute::Type::CATEGORICAL;
    struct core::Attribute P = { .text = "P", .type = type, .size = sizeP, .offset = 0};
    struct core::Attribute Q = { .text = "Q", .type = type, .size = sizeQ, .offset = P.size};

    auto ea = context->ea;
    auto generated = generate_data(P, Q, N, ea->size());
    CTable_t &_data = generated.first;
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

    FHE_NTIMER_START(open_gamma);
    std::vector<core::PrivateContingencyTableHelper::Publishable> publishable;
    helper->open_gamma(publishable, gamma, tilde_gamma, ea, sk);
    FHE_NTIMER_STOP(open_gamma);

    FHE_NTIMER_START(final_decrypt);
    auto ctable = helper->final_decrypt(n_uv, publishable, sk, ea);
    FHE_NTIMER_STOP(final_decrypt);

    print(ctable);

//    printAllTimers(std::cout);
    printNamedTimer(std::cout, "ea_rotate");
    printNamedTimer(std::cout, "Conduction");
    printNamedTimer(std::cout, "GreaterThan");
    printNamedTimer(std::cout, "Blinding");
    printNamedTimer(std::cout, "open_gamma");
    printNamedTimer(std::cout, "final_decrypt");
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
    long P = 5;
    long Q = 11;
    long which = 1;
    mapping.arg("N", N, "number of ciphers");
    mapping.arg("P", P, "Size of C_p");
    mapping.arg("Q", Q, "Size of C_q");
    mapping.arg("w", which, "which FHE parameter");
    mapping.parse(argc, argv);
    test_CT(N, P, Q, which);
//    test_crt();
//    test_repeat();
    return 0;
}
