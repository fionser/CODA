//
// Created by riku on 2016/07/19.
//
#include "core/ctxt_util.hpp"
#include "HElib/Ctxt.h"
#include "HElib/EncryptedArray.h"
#include "HElib/PAlgebra.h"
#include <NTL/ZZ.h>
#include <random>
#include <algorithm>

namespace core {
static size_t number_bits(long a) {
    size_t bits = 0;
    while (a > 0) {
        bits += 1;
        a = a >> 1;
    }
    return bits;
}

static bool test_bit(long a, size_t i) {
    return (a & (1 << i)) != 0;
}

Ctxt repeat0(const Ctxt &c,
            const long n_slots, const long rep,
            const EncryptedArray &ea) {
    Ctxt repeated(c);
    Ctxt res(c.getPubKey()); /// zero-like
    long offset = n_slots;
    for (size_t i = 0; i < number_bits(rep); i++) {
        if (test_bit(rep, i)) {
            ea.rotate(res, offset);
            res += repeated;
        }
        Ctxt tmp(repeated);
        ea.rotate(tmp, offset);
        repeated += tmp;
        offset = offset << 1;
    }

    return res;
}

Ctxt repeat(const Ctxt &c,
            const long n_slots, const long rep,
            const EncryptedArray &ea) {
    assert(n_slots * rep <= ea.size() && "n_slots * rep > ea.size()");
    assert(n_slots >= 0 && rep >= 0 && "n_slots < 0 || rep < 0");
    Ctxt repeated(c);
    mask_first(repeated, n_slots, ea);
    return repeat0(repeated, n_slots, rep, ea);
}

void mask_first(Ctxt &ctxt, size_t n, const EncryptedArray &ea) {
    std::vector<long> mask(ea.size(), 0);
    for (long i = 0; i < n; i++) mask[i] = 1;
    NTL::ZZX poly;
    ea.encode(poly, mask);
    ctxt.multByConstant(poly);
}

std::vector<std::vector<long>> random_permutation(long D, const EncryptedArray &ea) {
    size_t nr = (D + ea.size() - 1) / ea.size();
    std::vector<std::vector<long>> parts(nr, std::vector<long>(ea.size()));
    std::vector<long> I(nr * ea.size());
    for (size_t i = 0; i < I.size(); i++) I.at(i) = i;

    std::random_shuffle(I.begin(), I.end());

    auto itr = I.begin();
    for (auto &part : parts) {
        for (auto &value : part) {
            value = *itr;
            itr++;
        }
    }
    return parts;
}

std::vector<std::vector<long>> randomness(long D, const EncryptedArray &ea) {
    size_t nr = (D + ea.size() - 1) / ea.size();
    std::vector<std::vector<long>> parts(nr, std::vector<long>(ea.size()));
    std::vector<long> I(nr * ea.size());
    long pr = ea.getAlMod().getPPowR();
    for (size_t i = 0; i < I.size(); i++) {
        do{
            I.at(i) = NTL::RandomBnd(pr);
        } while (I.at(i) == 0);
    }
    auto itr = I.begin();
    for (auto &part : parts) {
        for (auto &value : part) {
            value = *itr;
            itr++;
        }
    }
    return parts;
}

NTL::ZZX random_key(const EncryptedArray &ea) {
    long pr = ea.getAlMod().getPPowR();
    std::vector<long> poly(ea.size(), NTL::RandomBnd(pr));
    NTL::ZZX k;
    ea.encode(k, poly);
    return k;
}

void add_key(greaterthan::ResultType &gamma, NTL::ZZX key) {
   for (auto &c : gamma) {
       c->addConstant(key);
   }
}
}

