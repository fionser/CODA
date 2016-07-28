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

            auto aesKeys = decryptToGetAESKeys(tilde_gamma, i, zeros, ea, sk);

            for (size_t j = 0; j < zeros.size(); j++) {
                auto crtidx = zeros[j] % bs;
                size_t u = crtidx % modified_sizes.first;
                size_t v = crtidx % modified_sizes.second;
                Publishable info = {.u = u, .v = v, .j = crtidx,
                        .aes_key = convKey(aesKeys.at(j), bit_per)};
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

std::vector <PrivateContingencyTable::AESKey_t>
PrivateContingencyTableHelper::decryptToGetAESKeys(
        const Type_tilde_gamma &tilde_gammas,
        const size_t loc,
        const std::vector <size_t> &zeros,
        const EncryptedArray *ea, sk_ptr sk) const {
    size_t bs = block_size();
    std::vector<PrivateContingencyTable::AESKey_t> keys(zeros.size());
    size_t usable_size = ea->size() / bs * bs;
    std::vector<long> slots;

    for (auto &parts : tilde_gammas) {
        ea->decrypt(*(parts.at(loc)), *sk, slots);
        for (size_t j = 0; j < zeros.size(); j++) {
            keys.at(j).push_back(slots.at(zeros[j]));
        }
    }
    return keys;
}


std::vector<long> PrivateContingencyTableHelper::
final_decrypt(const Type_n_uv &cells,
              const std::vector<struct PrivateContingencyTableHelper::Publishable> &publishable,
              const sk_ptr sk, const EncryptedArray *ea) const {
    assert(cells.size() >= publishable.size() && "Mismatch size!");
    std::vector<long> plain_cells(cells.size(), 0);
    auto modified = coprime(P.size, Q.size);
    const FHEPubKey &pk = *sk;
    Ctxt ctxt(pk);
    std::vector<long> slots(ea->size(), 0);
    ea->encrypt(ctxt, pk, slots);
    for (auto &p : publishable) {
        AES128 aes(p.aes_key);
        auto idx = p.u * Q.size + p.v;
        std::string decrypt_aes = aes.decrypt(cells.at(idx));
        conv(ctxt, decrypt_aes);
        ea->decrypt(ctxt, *sk, slots);

        long count = 0;
        for (auto ss : slots) {
            if (ss != 0) {
                assert(count == 0 && "Should only one zero!");
                count = ss;
            }
        }
        plain_cells.at(idx) = count;
    }

    return plain_cells;
}

} // namespace core
