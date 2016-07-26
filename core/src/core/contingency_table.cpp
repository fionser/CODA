//
// Created by riku on 2016/07/19.
//
#include "HElib/Ctxt.h"
#include "HElib/EncryptedArray.h"
#include "HElib/timing.h"
#include "core/ctxt_util.hpp"
#include "core/coda.hpp"
#include "core/contingency_table.hpp"
#include "core/greaterthan.h"
#include <vector>
#include <thread>

#include <NTL/ZZ.h>
namespace core {
std::pair<size_t, size_t> coprime(size_t i, size_t j) {
    while (NTL::GCD(i, j) != 1) {
        if (i > j) j += 1;
        else i += 1;
    }
    return std::make_pair(i, j);
};

static Ctxt process(const Ctxt &one_record,
                    const Attribute &I,
                    const Attribute &J,
                    const EncryptedArray &ea) {
    auto modified_sizes = coprime(I.size, J.size);
    assert(modified_sizes.first * modified_sizes.second <= ea.size() && "Too large domain!");

    Ctxt attr_i(one_record), attr_j(one_record);
    ea.rotate(attr_i, -static_cast<long>(I.offset));
    ea.rotate(attr_j, -static_cast<long>(J.offset));

    mask_first(attr_i, modified_sizes.first, ea);
    mask_first(attr_j, modified_sizes.second, ea);
    attr_i = core::repeat0(attr_i, modified_sizes.first, modified_sizes.second, ea);
    attr_j = core::repeat0(attr_j, modified_sizes.second, modified_sizes.first, ea);

    attr_i *= attr_j;
    return attr_i;
}

static long InvMod(long a, long p) {
    long p0(p), t, q;
    long x0(0), x1(1);
    if (p == 1) return 1;
    while (a > 1) {
        q = a / p;
        t = p; p = a % p; a = t;
        t = x0; x0 = x1 - q * x0; x1 = t;
    }
    if (x1 < 0) x1 += p0;
    return x1;
}
// return x that x = u mod k1, x = v mod k2
long CRT(long u, long v, long k1, long k2) {
    assert(NTL::GCD(k1, k2) == 1);
    auto P = k1 * k2;
    return (u * InvMod(k2, k1) * k2 + v * InvMod(k1, k2) * k1) % P;
}

static NTL::ZZX make_I_x(long u, long v, long k1, long k2,
                         const EncryptedArray *ea) {
    std::vector<long> X(ea->size(), 0);
    X.at(CRT(u, v, k1, k2)) = 1;
    NTL::ZZX poly;
    ea->encode(poly, X);
    return poly;
}

PrivateContingencyTable
::PrivateContingencyTable(context_ptr context,
                          PrivateContingencyTableHelper *helper)
        : context(context), helper(helper) { }

std::shared_ptr<Ctxt>
PrivateContingencyTable::compute_table(const std::vector <Ctxt> &attributes,
                                       Attribute p, Attribute q,
                                       const EncryptedArray *ea) const {
    std::atomic<size_t> counter(0);
    std::mutex lock;
    size_t nr_ctxts = attributes.size();
    const auto &pk = attributes.front().getPubKey();
    std::shared_ptr<Ctxt> ct = std::make_shared<Ctxt>(pk);
    auto program = [&]() {
        size_t next;
        while (true) {
            next = counter.fetch_add(1UL);
            if (next >= nr_ctxts) break;
            auto tmp = process(attributes.at(next), p, q, *ea);
            lock.lock();
            ct->operator+=(tmp);
            lock.unlock();
        }
    };

    std::vector<std::thread> workers;
    for (auto wr = 0; wr < 1; wr++)
        workers.push_back(std::thread(program));
    for (auto &&wr : workers) wr.join();
    ct->reLinearize();
    return ct;
}

static std::vector<NTL::ZZX>
random_permutation_for_GT(size_t block_size, size_t copies,
                          long threshold, long domain_size,
                          const EncryptedArray *ea) {
    size_t usable_size = ea->size() / block_size * block_size;
    std::vector<long> I(domain_size);
    for (long i = 0; i < I.size(); i++) I.at(i) = -threshold - i;
    auto pr = ea->getAlMod().getPPowR();
    long dummy_random;
    do { dummy_random = NTL::RandomBnd(pr); } while(dummy_random == 0);

    std::vector<std::vector<long>> parts(copies, std::vector<long>(ea->size(), dummy_random));

    for (size_t b = 0; b < block_size; b++) {
        std::random_shuffle(I.begin(), I.end());
        for (size_t i = 0; i < domain_size; i++) {
            auto ii = i * block_size + b;
            auto idx_of_part = ii / usable_size;
            auto idx_in_part = ii % usable_size;
            parts.at(idx_of_part).at(idx_in_part) = I.at(i);
        }
    }

    std::vector<NTL::ZZX> rets(copies);
    for (size_t c = 0; c < copies; c++)
        ea->encode(rets.at(c), parts.at(c));
    return rets;
}

static std::vector<long>
random_hash_key(size_t block_size, size_t repeats, const EncryptedArray *ea) {
    size_t usable_size = ea->size() / block_size * block_size;
    std::vector<long> keys(ea->size());
    long pr = ea->getAlMod().getPPowR();
    for (size_t b = 0; b < block_size; b++) {
        keys.at(b) = NTL::RandomBnd(pr);
        for (size_t r = 1; r < repeats; r++)
            keys.at(b + r * block_size) = keys.at(b);
    }
    return keys;
}

static std::vector<NTL::ZZX>
randomness_for_GT(size_t copies, const EncryptedArray *ea) {
    auto random_sample = [](std::vector<long> &r, long pr) {
        for (size_t i = 0; i < r.size(); i++) {
            do {
                r.at(i) = NTL::RandomBnd(pr);
            } while (r.at(i) == 0);
        }
    };

    auto pr = ea->getAlMod().getPPowR();
    std::vector<long> randomness(ea->size());
    std::vector<NTL::ZZX> rets(copies);
    for (size_t c = 0; c < copies; c++) {
        random_sample(randomness, pr);
        random_sample(randomness, pr);
        ea->encode(rets.at(c), randomness);
    }

    return rets;
}

PrivateContingencyTable::ResultType::Type_gamma
PrivateContingencyTable::special_greater_than(ctxt_ptr CT, long domain_size,
                                              const EncryptedArray *ea) const {
    helper->setCT(CT.get());
    ctxt_ptr repeated = helper->repeat(domain_size);
    auto repeats = helper->repeats_per_cipher();
    auto copies = helper->how_many_copies(domain_size);
    auto parts = random_permutation_for_GT(helper->block_size(), copies,
                                           helper->getThreshold(), domain_size, ea);
    auto randomness = randomness_for_GT(copies, ea);

    ResultType::Type_gamma gamma;
    for (size_t i = 0; i < parts.size(); i++) {
        auto tmp = std::make_shared<Ctxt>(*repeated);
        tmp->addConstant(parts.at(i));
        tmp->multByConstant(randomness.at(i));
        gamma.push_back(tmp);
    }
    return gamma;
}

PrivateContingencyTable::ResultType::Type_gamma
PrivateContingencyTable::add_key_to_gamma(const ResultType::Type_gamma &gamma,
                                          const std::vector<long> &keys,
                                          long domain_size,
                                          const EncryptedArray *ea) const {
    ResultType::Type_gamma  tilde_gamma(gamma.size());
    NTL::ZZX poly;
    ea->encode(poly, keys);
    for (size_t i = 0; i < gamma.size(); i++) {
        tilde_gamma.at(i) = std::make_shared<Ctxt>(*gamma.at(i));
        tilde_gamma.at(i)->addConstant(poly);
    }
    return tilde_gamma;
}

PrivateContingencyTable::ResultType
PrivateContingencyTable::evaluate(const std::vector <Ctxt> &attributes) const {
    assert(!attributes.empty());
    const auto &pk = attributes.front().getPubKey();
    auto ea = context->ea;

    Attribute P = helper->getP();
    Attribute Q = helper->getQ();
    long domain_size = attributes.size();
    printf("repeat block size %zd for %zd times with %zd copies\n",
           helper->block_size(),
           helper->repeats_per_cipher(),
           helper->how_many_copies(domain_size));
    FHE_NTIMER_START(Conduction);
    auto contingency_table = compute_table(attributes, P, Q, ea);
    FHE_NTIMER_STOP(Conduction);

    FHE_NTIMER_START(GreaterThan);
    auto gamma = special_greater_than(contingency_table, domain_size, ea);
    FHE_NTIMER_STOP(GreaterThan);

    FHE_NTIMER_START(Blinding);
    auto keys = random_hash_key(helper->block_size(),
                                helper->repeats_per_cipher(), ea);
    auto tilde_gamma = add_key_to_gamma(gamma, keys, domain_size, ea);
    FHE_NTIMER_STOP(Blinding);

    return { .n_uv = std::vector<ctxt_ptr>(1, contingency_table), .gamma = gamma, .tilde_gamma = tilde_gamma };
}

size_t PrivateContingencyTableHelper::repeats_per_cipher() const {
    auto cont_table_size = block_size();
    assert(cont_table_size <= ea->size() && "One cipher can't contain the whole contingency table");
    return ea->size() / cont_table_size;
}

size_t PrivateContingencyTableHelper::how_many_copies(long domain_size) const {
    size_t repeats = repeats_per_cipher();
    return (domain_size + repeats - 1UL) / repeats;
}

size_t PrivateContingencyTableHelper::block_size() const {
    auto modified_size = coprime(P.size, Q.size);
    return modified_size.first * modified_size.second;
}

std::shared_ptr<Ctxt> PrivateContingencyTableHelper::repeat(size_t R) const {
    assert(CT != nullptr && "the CT haven't been set");
    size_t repeats = std::min(R, repeats_per_cipher());
    if (repeats != R)
        printf("Warning! hope to repeat %zd times but only get %zd\n", R, repeats);
    return std::make_shared<Ctxt>(core::repeat0(*CT, block_size(), repeats, *ea));
}

void PrivateContingencyTableHelper::open_gamma(std::vector<Publishable> &unsuppression,
                                               const Type_gamma &gamma,
                                               const Type_gamma &tilde_gamma,
                                               const EncryptedArray *ea,
                                               sk_ptr sk) {
    size_t bs = block_size();
    auto modified_sizes = coprime(P.size, Q.size);
    size_t usable_size = ea->size() / bs * bs;
    std::vector<long> decrypted(ea->size());
    std::vector<long> keys(ea->size());

    for (size_t i = 0; i < gamma.size(); i++) {
        if (!gamma.at(i)->isCorrect())
            printf("Warnning! might be an invalid cipher\n");
        ea->decrypt(*gamma.at(i), *sk, decrypted);
        ea->decrypt(*tilde_gamma.at(i), *sk, keys);
        for (size_t j = 0; j < usable_size; j++) {
            if (decrypted.at(j) != 0) continue;

            size_t u = j % modified_sizes.first;
            size_t v = j % modified_sizes.second;
            long hash_key = keys.at(j);
            Publishable info = {.u = u, .v = v, .hash_key = hash_key};
            unsuppression.push_back(info);
        }
    }
}
}

