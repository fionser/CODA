//
// Created by riku on 2016/07/28.
//
#include "HElib/Ctxt.h"
#include "HElib/EncryptedArray.h"
#include "core/ctxt_util.hpp"
#include "core/coda.hpp"
#include "core/contingency_table.hpp"
#include "core/greaterthan.h"
#include <vector>
#include <algorithm>
#include <thread>

#ifdef FHE_THREADS
#define NR_THREADS 8
#else
#define NR_THREADS 1
#endif

namespace core {
size_t PrivateContingencyTableHelper::repeats_per_cipher() const {
    auto cont_table_size = block_size();
    assert(cont_table_size <= ea->size() &&
           "One cipher can't contain the whole contingency table");
    return ea->size() / cont_table_size;
}

size_t PrivateContingencyTableHelper::how_many_copies(long domain_size) const {
    size_t repeats = repeats_per_cipher();
    return (domain_size + repeats - 1UL) / repeats;
}

size_t PrivateContingencyTableHelper::
how_many_copies_for_bits(const EncryptedArray *ea) const {
    auto bit_per = (number_bits(ea->getAlMod().getPPowR()) / 8UL) << 3;
    return (aesKeyLength() + bit_per - 1) / bit_per;
}

size_t PrivateContingencyTableHelper::block_size() const {
    auto modified_size = coprime(P.size, Q.size);
    return modified_size.first * modified_size.second;
}

std::shared_ptr <Ctxt> PrivateContingencyTableHelper::repeat(size_t R) const {
    assert(CT != nullptr && "the CT haven't been set");
    size_t repeats = std::min(R, repeats_per_cipher());
    if (repeats != R)
        printf("Warning! hope to repeat %zd times but only get %zd\n", R, repeats);
    return std::make_shared<Ctxt>(
            core::repeat0(*CT, block_size(), repeats, *ea));
}

void PrivateContingencyTableHelper::
open_gamma(std::vector <Publishable> &unsuppression,
           const Type_gamma &gamma,
           const Type_tilde_gamma &tilde_gamma,
           const EncryptedArray *ea, sk_ptr sk) const {
    size_t bs = block_size();
    size_t usable_size = ea->size() / bs * bs;
    auto modified_sizes = coprime(P.size, Q.size);
    auto bit_per = number_bits(ea->getAlMod().getPPowR());
    std::atomic<size_t> counter(0);
    std::mutex tmux;
    auto program = [&]() {
        size_t sze = gamma.size();
        size_t i;
        while ((i= counter.fetch_add(1)) < sze) {
            const auto &part = gamma.at(i);
//            if (!part->isCorrect())
//                printf("Warnning! might be an invalid cipher\n");
            std::vector<long> decrypted(ea->size());
            ea->decrypt(*part, *sk, decrypted);
            std::vector <size_t> zeros;
            for (size_t j = 0; j < usable_size; j++) {
                if (decrypted.at(j) != 0) continue;
                size_t u = j % modified_sizes.first;
                size_t v = j % modified_sizes.second;
                if (u >= P.size || v >= Q.size) {
                    printf("WARN! position %zd impossible be zero!\n", j);
                    continue;
                }
                zeros.push_back(j);
            }

            if (zeros.empty()) continue;

            auto blinding_factors = get_blinding_factor(tilde_gamma, i, zeros, ea, sk);

            for (size_t j = 0; j < zeros.size(); j++) {
                auto crtidx = zeros[j] % bs;
                size_t u = crtidx % modified_sizes.first;
                size_t v = crtidx % modified_sizes.second;
                Publishable info = { .u = u, .v = v, .j = crtidx,
                                     .blinding_factor = blinding_factors.at(j) };
                tmux.lock();
                unsuppression.push_back(info);
                tmux.unlock();
            }
        }
    };

    std::vector<std::thread> workers;
    for (size_t wr = 0; wr < NR_THREADS; wr++)
        workers.push_back(std::thread(program));
    for (auto &wr : workers) wr.join();
}

std::vector<long> PrivateContingencyTableHelper::
get_blinding_factor(const Type_tilde_gamma &tilde_gamma, long part,
                    const std::vector <size_t> &positions,
                    const EncryptedArray *ea, sk_ptr sk) const {
    NTL::ZZX poly;
    sk->Decrypt(poly, *tilde_gamma.at(part));
    // TODO(riku) to implement decode1Slot for efficency
    std::vector<long> slots(ea->size());
    ea->decode(slots, poly);
    std::vector<long> blind_factor;
    for (size_t pos : positions) {
        blind_factor.push_back(slots.at(pos));
    }
    return blind_factor;
}

PrivateContingencyTableHelper::CTable_t
PrivateContingencyTableHelper::final_decrypt(const Type_n_uv &table,
                                             const std::vector <Publishable> &publishables,
                                             const sk_ptr sk,
                                             const EncryptedArray *ea) const
{
    auto pr = ea->getAlMod().getPPowR();
    NTL::ZZX poly;
    sk->Decrypt(poly, *table);
    std::vector<long> blinded_table(ea->size(), 0);
    // TODO(riku) to implement decode1Slot for efficency
    ea->decode(blinded_table, poly);
    std::vector<long> result_table;

    CTable_t ctable(getP().size, std::vector<long>(getQ().size, 0));
    for (const auto &p : publishables) {
        auto u = p.u;
        auto v = p.v;
        ctable.at(u).at(v) = (blinded_table[p.j] - p.blinding_factor) % pr;
    }
    return ctable;
}

} // namespace core