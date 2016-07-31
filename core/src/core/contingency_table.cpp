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
#include <algorithm>

#include <NTL/ZZ.h>

#ifdef FHE_THREADS
#define NR_THREADS 8
#warning  "Using 8 threads"
#else
#warning "Using single threads"
#define NR_THREADS 1
#endif

namespace core {

static long toCRTIndex(long i, long p1, long p2) {
    auto u = i / p2;
    auto v = i % p2;
    auto primes = coprime(p1, p2);
    return CRT(u, v, primes.first, primes.second);
}

static long InvMod(long a, long p) {
    long p0(p), t, q;
    long x0(0), x1(1);
    if (p == 1) return 1;
    while (a> 1) {
        q = a / p;
        t = p; p = a % p; a = t;
        t = x0; x0 = x1 - q * x0; x1 = t;
    }
    if (x1 < 0) x1 += p0;
    return x1;
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
        ea->encode(rets.at(c), randomness);
    }

    return rets;
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

Ctxt PrivateContingencyTable::do_compute(const Ctxt &one_record,
                                         const Attribute &p, const Attribute &q,
                                         const EncryptedArray *ea) const {
    auto modified_sizes = coprime(p.size, q.size);
    assert(modified_sizes.first * modified_sizes.second <= ea->size() && "Too large domain!");

    Ctxt attr_i(one_record), attr_j(one_record);
    ea->rotate(attr_i, -static_cast<long>(p.offset));
    ea->rotate(attr_j, -static_cast<long>(q.offset));

    mask_first(attr_i, modified_sizes.first, *ea);
    mask_first(attr_j, modified_sizes.second, *ea);
    attr_i = core::repeat0(attr_i, modified_sizes.first, modified_sizes.second, *ea);
    attr_j = core::repeat0(attr_j, modified_sizes.second, modified_sizes.first, *ea);

    attr_i *= attr_j;
    return attr_i;
}

std::shared_ptr<Ctxt>
PrivateContingencyTable::compute_table(const std::vector <Ctxt> &attributes,
                                       Attribute p, Attribute q,
                                       const EncryptedArray *ea) const {
    std::atomic<size_t> counter(0);
    size_t nr_ctxts = attributes.size();
    const auto &pk = attributes.front().getPubKey();
    std::shared_ptr<Ctxt> ct = std::make_shared<Ctxt>(pk);
    std::vector<Ctxt> workers_space(NR_THREADS, pk);
    auto program = [&](const long worker_id) {
        size_t next;
        while (true) {
            next = counter.fetch_add(1UL);
            if (next>= nr_ctxts) break;
            auto tmp = do_compute(attributes.at(next), p, q, ea);
            workers_space.at(worker_id) += tmp;
        }
    };

    std::vector<std::thread> workers;
    for (auto wr = 0; wr < NR_THREADS; wr++)
        workers.push_back(std::thread(program, wr));
    for (auto &&wr : workers) wr.join();

    for (auto &tmp : workers_space)
        ct->operator+=(tmp);
    ct->reLinearize();
    return ct;
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

std::vector<long> PrivateContingencyTable::
sample_blinding_factor(Attribute p,
                       Attribute q,
                       const EncryptedArray *ea) const
{
    auto modified_sizes = coprime(p.size, q.size);
    size_t bsize = helper->block_size();
    assert(bsize <= ea->size() && "Can't encode the whole contingenct table!");

    long pr = ea->getAlMod().getPPowR();
    std::vector<long> delta(ea->size(), 0);
    for (size_t b = 0; b < bsize; b++)
        delta.at(b) = NTL::RandomBnd(pr);

    return delta;
}

PrivateContingencyTable::ResultType::Type_tilde_gamma
PrivateContingencyTable::blind_GT(const ResultType::Type_gamma &gamma,
                                  const std::vector<long> &blind_factor,
                                  long domain_size,
                                  const EncryptedArray *ea) const
{
    size_t bsize = helper->block_size();
    size_t usable_size = ea->size() / bsize * bsize;

    std::vector<std::vector<long>> parts(gamma.size(), std::vector<long>(ea->size(), 0));
    for (size_t b = 0; b < bsize; b++) {
        for (size_t i = 0; i < domain_size; i++) {
            auto ii = i * bsize + b;
            auto idx_of_part = ii / usable_size;
            auto idx_in_part = ii % usable_size;
            parts.at(idx_of_part).at(idx_in_part) = blind_factor.at(b);
        }
    }

    NTL::ZZX poly;
    ResultType::Type_tilde_gamma tilde_gamma(gamma.size());
    for (size_t i = 0; i < gamma.size(); i++) {
        tilde_gamma[i] = std::make_shared<Ctxt>(*gamma[i]);
        ea->encode(poly, parts[i]);
        tilde_gamma[i]->addConstant(poly);
    }
    return tilde_gamma;
}

PrivateContingencyTable::ResultType::Type_n_uv
PrivateContingencyTable::blind_table(const ctxt_ptr &ct,
                                     const std::vector<long> &blind_factor,
                                     const EncryptedArray *ea) const {
    ctxt_ptr blinded_ct = std::make_shared<Ctxt>(*ct);
    NTL::ZZX poly;
    ea->encode(poly, blind_factor);
    blinded_ct->addConstant(poly);
    return blinded_ct;
}

PrivateContingencyTable::ResultType
PrivateContingencyTable::evaluate(const std::vector <Ctxt> &attributes) const {
    assert(!attributes.empty());
    const auto &pk = attributes.front().getPubKey();
    auto ea = context->ea;

    Attribute P = helper->getP();
    Attribute Q = helper->getQ();
    long domain_size = attributes.size() - helper->getThreshold();
    printf("Block Size %zd; Repeating %zd; Copies %zd; Bit-copies %zd\n",
           helper->block_size(),
           helper->repeats_per_cipher(),
           helper->how_many_copies(domain_size),
           helper->how_many_copies_for_bits(ea));

    FHE_NTIMER_START(Conduction);
    auto contingency_table = compute_table(attributes, P, Q, ea);
    FHE_NTIMER_STOP(Conduction);

    FHE_NTIMER_START(GreaterThan);
    auto gamma = special_greater_than(contingency_table, domain_size, ea);
    FHE_NTIMER_STOP(GreaterThan);

    FHE_NTIMER_START(Blinding);
    auto blinding_factor = sample_blinding_factor(P, Q, ea);
    auto tilde_gamma = blind_GT(gamma, blinding_factor, domain_size, ea);
    auto n_uv = blind_table(contingency_table, blinding_factor, ea);
    FHE_NTIMER_STOP(Blinding);

    return { .n_uv = n_uv, .gamma = gamma, .tilde_gamma = tilde_gamma};
}


//// Some helper functions
/// bit_per must be 8-multiple
PrivateContingencyTable::AESKey_t convKey(const NTL::ZZ &zz,
                                          long bit_per,
                                          long partition) {
    PrivateContingencyTable::AESKey_t key;
    long nr_bytes = NTL::NumBytes(zz);
    assert(partition == ((nr_bytes << 3) / bit_per) && "Bit_per need to be 8-multiple");

    std::vector<uint8_t> bytes(nr_bytes);
    NTL::BytesFromZZ(bytes.data(), zz, nr_bytes);

    long byte_per = bit_per >> 3;
    for (size_t i = 0; i < nr_bytes; i += byte_per) {
        long z = 0;
        for (size_t j = 0; j < byte_per; j++) {
            z = (z << 3) + bytes.at(i + j);
        }
        key.push_back(z);
    }

    assert(key.size() == partition && "Wrong implementation!");
    return key;
}

NTL::ZZX convKey(const std::vector<PrivateContingencyTable::AESKey_t> &keys,
                 long partition,
                 const EncryptedArray *ea) {
    auto bs = keys.size();
    std::vector<long> poly(ea->size(), 0);
    size_t usable_size = ea->size() / bs * bs;
    for (auto b = 0; b < bs; b++) {
        assert(partition < keys.at(b).size());
        long k = keys[b][partition];
        for (auto i = b; i < usable_size; i += bs)
            poly[i] = k;
    }

    NTL::ZZX zzx;
    ea->encode(zzx, poly);
    return zzx;
}

NTL::ZZ convKey(PrivateContingencyTable::AESKey_t aes, long bit_per) {
    long nr_bytes = (bit_per>> 3) * aes.size();
    std::vector<uint8_t> bytes;
    for (auto v : aes) {
        while (v> 0) {
            bytes.push_back(v & 0xFF);
            v = v >> 8;
        }
    }
    return NTL::ZZFromBytes(bytes.data(), nr_bytes);
}

std::pair<size_t, size_t> coprime(size_t i, size_t j) {
    while (NTL::GCD(i, j) != 1) {
        if (i> j) j += 1;
        else i += 1;
    }
    return std::make_pair(i, j);
}

// return x that x = u mod k1, x = v mod k2
long CRT(long u, long v, long k1, long k2) {
    assert(NTL::GCD(k1, k2) == 1);
    auto P = k1 * k2;
    return (u * InvMod(k2, k1) * k2 + v * InvMod(k1, k2) * k1) % P;
}
}
