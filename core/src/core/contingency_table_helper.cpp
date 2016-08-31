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
#include <fstream>

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
    size_t bsize = block_size();
    auto modified_sizes = coprime(P.size, Q.size);
    auto bit_per = number_bits(ea->getAlMod().getPPowR());
    std::atomic<size_t> counter(0);
    std::mutex tmux;
    auto program = [&]() {
        size_t sze = gamma.size();
        size_t i;
        while ((i= counter.fetch_add(1)) < sze) {
            const auto &part = gamma.at(i);
            std::vector <long> zeros = get_zero_positions(part, sk, ea);
            if (zeros.empty()) continue;
            auto blinding_factors = get_blinding_factor(tilde_gamma, i, zeros, ea, sk);
            for (size_t j = 0; j < zeros.size(); j++) {
                long crtidx = zeros[j] % bsize;
                size_t u = crtidx % modified_sizes.first;
                size_t v = crtidx % modified_sizes.second;
                if (u >= P.size || v >= Q.size) { continue; }
                Publishable info = { .u = u, .v = v, .position = crtidx,
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
get_zero_positions(const ctxt_ptr &ctxt, const sk_ptr &sk, const EncryptedArray *ea) const {
    if (!ctxt->isCorrect())
        printf("Warnning! might be an invalid cipher\n");
    size_t bs = block_size();
    size_t usable_size = ea->size() / bs * bs;
    std::vector<long> slots;
    ea->decrypt(*ctxt, *sk, slots);
    std::vector <long> zeros;
    for (size_t j = 0; j < usable_size; j++) {
        if (slots.at(j) != 0) continue;
        zeros.push_back(static_cast<long>(j));
    }
    return zeros;
}

std::vector<long> PrivateContingencyTableHelper::
get_blinding_factor(const Type_tilde_gamma &tilde_gamma, long part,
                    const std::vector <long> &positions,
                    const EncryptedArray *ea, sk_ptr sk) const {
    NTL::ZZX poly;
    sk->Decrypt(poly, *tilde_gamma.at(part));
    std::vector<long> slots(positions.size());
    ea->decodeSlots(slots, poly, positions);
    std::vector<long> blind_factor;
    for (size_t bf : slots) {
        blind_factor.push_back(bf);
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
    std::vector<long> blinded_table;
    std::vector<long> positions(publishables.size());
    for (size_t i = 0; i < publishables.size(); i++) {
        positions[i] = publishables[i].position;
    }

    ea->decodeSlots(blinded_table, poly, positions);
    std::vector<long> result_table;

    CTable_t ctable(getP().size, std::vector<long>(getQ().size, 0));
    for (size_t i = 0; i < publishables.size(); i++) {
        const auto &p = publishables[i];
        size_t u = p.u;
        size_t v = p.v;
        long value = blinded_table[i] - p.blinding_factor;
        while (value < 0) value += pr;
        ctable.at(u).at(v) = value;
    }
    return ctable;
}

bool PrivateContingencyTableHelper::dump(const PrivateContingencyTable::ResultType &results,
                                         const std::string &file) const {
    std::ofstream fout(file, std::ios::binary);
    if (!fout.is_open())
        return false;
    fout << *(results.n_uv);
    fout << static_cast<int32_t>(results.gamma.size());
    for (auto &g : results.gamma)
        fout << *g;
    fout << static_cast<int32_t>(results.tilde_gamma.size());
    for (auto &g : results.tilde_gamma)
        fout << *g;
    fout.close();
    return true;
}

bool PrivateContingencyTableHelper::restore(PrivateContingencyTable::ResultType &out,
                                            const std::string &result_file,
                                            const core::pk_ptr &pk) const
{
    std::ifstream fin(result_file, std::ios::binary);
    if (!fin.is_open()) return false;
    out.n_uv = std::make_shared<Ctxt>(*pk);
    fin >> *out.n_uv;
    int32_t gamma_size;
    fin >> gamma_size;
    out.gamma.resize(gamma_size);
    for (int32_t i = 0; i < gamma_size; i++) {
        out.gamma[i] = std::make_shared<Ctxt>(*pk);
        fin >> *(out.gamma[i]);
    }

    fin >> gamma_size;
    out.tilde_gamma.resize(gamma_size);
    for (int32_t i = 0; i < gamma_size; i++) {
        out.tilde_gamma[i] = std::make_shared<Ctxt>(*pk);
        fin >> *(out.tilde_gamma[i]);
    }
    return true;
}
} // namespace core
