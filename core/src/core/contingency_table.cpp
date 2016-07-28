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
    do {dummy_random = NTL::RandomBnd(pr); } while(dummy_random == 0);

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

static std::vector<PrivateContingencyTable::AESKey_t>
random_hash_key(size_t block_size,
                size_t repeats,
                long key_bits,
                const EncryptedArray *ea) {
    size_t usable_size = ea->size() / block_size * block_size;
    long bits = (number_bits(ea->getAlMod().getPPowR()) / 8UL) << 3;
    long partition = (key_bits + bits - 1) / bits;
    std::vector<PrivateContingencyTable::AESKey_t> keys;

    NTL::ZZ randomness;
    for (size_t b = 0; b < block_size; b++) {
        NTL::RandomBits(randomness, key_bits);
//        std::cout << b << " " << randomness <<"\n";
        keys.emplace_back(convKey(randomness, bits, partition));
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

PrivateContingencyTable::ResultType::Type_tilde_gamma
PrivateContingencyTable::add_key_to_gamma(const ResultType::Type_gamma &gamma,
                                          const std::vector<AESKey_t> &keys,
                                          long domain_size,
                                          const EncryptedArray *ea) const {
    size_t partition = keys.front().size();
    size_t repeats = gamma.size();
    /// vector of Type_gamma
    ResultType::Type_tilde_gamma tilde_gammas(partition, ResultType::Type_gamma(repeats));
    for (size_t p = 0; p < partition; p++) {
        auto key_poly = convKey(keys, p, ea);
        for (size_t i = 0; i < repeats; i++) {
            tilde_gammas[p].at(i) = std::make_shared<Ctxt>(*gamma.at(i));
            tilde_gammas[p][i]->addConstant(key_poly);
        }
    }
    return tilde_gammas;
}

PrivateContingencyTable::ResultType::Type_n_uv
PrivateContingencyTable::aes_encrypt_cells(const std::vector<ctxt_ptr> &cells,
                                           const std::vector<AESKey_t> &keys,
                                           const EncryptedArray *ea) const {
    ResultType::Type_n_uv n_uv(cells.size());
    auto bits = number_bits(ea->getAlMod().getPPowR());
    for (size_t i = 0; i < cells.size(); i++) {
        auto raw_str = conv(*cells.at(i));
        auto aes_key = convKey(keys.at(i), bits);
        AES128 aes(aes_key);
        n_uv.at(i) = aes.encrypt(raw_str);
    }
    return n_uv;
}

std::vector<ctxt_ptr>
PrivateContingencyTable::extract_cells_in_table(const ctxt_ptr& CT,
                                                Attribute p, Attribute q,
                                                const EncryptedArray *ea) const {
    std::vector<ctxt_ptr> cells(p.size * q.size);
    auto modified_size = coprime(p.size, q.size);
    for (size_t i = 0; i < cells.size(); i++) {
        cells.at(i) = std::make_shared<Ctxt>(*CT);
    }

    for (size_t u = 0; u < p.size; u++) {
        for (size_t v = 0; v < q.size; v++) {
            auto x = CRT(u, v, modified_size.first, modified_size.second);
            auto Ix = make_I_x(u, v, modified_size.first, modified_size.second, ea);
            cells.at(x)->multByConstant(Ix);
        }
    }

    return cells;
}

PrivateContingencyTable::ResultType
PrivateContingencyTable::evaluate(const std::vector <Ctxt> &attributes) const {
    assert(!attributes.empty());
    const auto &pk = attributes.front().getPubKey();
    auto ea = context->ea;

    Attribute P = helper->getP();
    Attribute Q = helper->getQ();
    long domain_size = attributes.size();
    printf("Block Size %zd; Repeating %zd; Copies %zd; Bit-copies %zd\n",
           helper->block_size(),
           helper->repeats_per_cipher(),
           helper->how_many_copies(domain_size),
           helper->how_many_copies_for_bits(ea));

    FHE_NTIMER_START(Conduction);
    auto contingency_table = compute_table(attributes, P, Q, ea);
    auto cells = extract_cells_in_table(contingency_table, P, Q, ea);
    FHE_NTIMER_STOP(Conduction);

    FHE_NTIMER_START(GreaterThan);
    auto gamma = special_greater_than(contingency_table, domain_size, ea);
    FHE_NTIMER_STOP(GreaterThan);

    FHE_NTIMER_START(Blinding);
    auto keys = random_hash_key(helper->block_size(),
                                helper->repeats_per_cipher(),
                                helper->aesKeyLength(),
                                ea);
    auto n_uv = aes_encrypt_cells(cells, keys, ea);
    auto tilde_gamma = add_key_to_gamma(gamma, keys, domain_size, ea);
    FHE_NTIMER_STOP(Blinding);

    return {.n_uv = n_uv, .gamma = gamma, .tilde_gamma = tilde_gamma};
}

/// bit_per must be 8-multiple
PrivateContingencyTable::AESKey_t convKey(const NTL::ZZ &zz,
                                                 long bit_per,
                                                 long partition) {
    PrivateContingencyTable::AESKey_t key;
    long nr_bytes = NTL::NumBytes(zz);
    std::vector<uint8_t> bytes(nr_bytes);
    NTL::BytesFromZZ(bytes.data(), zz, nr_bytes);
    long z = 0;
    long need = bit_per;
    for (int i = 0; i < nr_bytes; i++)  {
        if (need> 8) {
            z = (z << 8) + bytes.at(i);
            need -= 8;
        } else {
            auto mask = (1 << need) - 1;
            key.push_back((z << need) + (bytes[i] & mask));
            if (need == 0) {
                z = 0;
            } else {
                z = (bytes[i] >> need);
            }
            need = bit_per;
        }
    }
    if (z != 0) key.push_back(z);

    assert(key.size() == partition);
    return key;
}

//// Some helper functions
NTL::ZZX convKey(const std::vector<PrivateContingencyTable::AESKey_t> &keys,
                        long p,
                        const EncryptedArray *ea) {
    auto bs = keys.size();
    std::vector<long> poly(ea->size(), 0);
    size_t usable_size = ea->size() / bs * bs;
    for (auto b = 0; b < bs; b++) {
        assert(p < keys.at(b).size());
        long k = keys[b][p];
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
