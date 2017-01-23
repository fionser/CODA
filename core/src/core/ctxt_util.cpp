//
// Created by riku on 2016/07/19.
//
#include "core/ctxt_util.hpp"
#include "core/literal.hpp"
#include "HElib/Ctxt.h"
#include "HElib/EncryptedArray.h"
#include "HElib/PAlgebra.h"
#include <NTL/ZZ.h>
#include <random>
#include <algorithm>
#include <sstream>

namespace core {
size_t number_bits(long a) {
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
            FHE_NTIMER_START(ea_rotate);
            ea.rotate(res, offset);
            FHE_NTIMER_STOP(ea_rotate);
            res += repeated;
        }
        Ctxt tmp(repeated);
        FHE_NTIMER_START(ea_rotate);
        ea.rotate(tmp, offset);
        FHE_NTIMER_STOP(ea_rotate);
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
    std::vector<std::vector<long>> parts(nr, std::vector<long>(ea.size(), 0));
    std::vector<long> I(nr * ea.size(), 0);
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

template<>
std::string conv(const Ctxt &obj) {
    std::stringstream sstream;
    sstream << obj;
    return sstream.str();
}

template<>
void conv(Ctxt &obj, const std::string &str) {
    std::stringstream sstream(str);
    sstream >> obj;
}

bool dumpCtxts(const std::vector<Ctxt> &ctxts,
               const std::string &outputDirPath) {
    auto makePath = [](std::string const& path, const long count) -> std::string {
        return path + literal::separator + "FILE_" + std::to_string(count);
    };

    long file_nr = 1;
    long ctx_dumped = 0;
    std::ofstream fout(makePath(outputDirPath, file_nr), std::ios::binary | std::ios::trunc);
    if (!fout.is_open()) {
        return false;
    }

    for (const Ctxt &ctx :ctxts) {
        if (ctx_dumped < core::core_setting.CTX_PER_FILE) {
            fout << ctx;
            ctx_dumped += 1;
        } else {
            fout.close();
            file_nr += 1;
            fout = std::ofstream(makePath(outputDirPath, file_nr), std::ios::binary);
            if (!fout.is_open())
                return false;
            fout << ctx;
            ctx_dumped = 1;
        }
    }
    fout.close();
    return true;
}

}

