//
// Created by riku on 2016/07/19.
//

#ifndef CODA_CLION_CTXT_UTIL_HPP
#define CODA_CLION_CTXT_UTIL_HPP
#include <cstddef>
#include <vector>
#include <NTL/ZZX.h>
#include "greaterthan.h"

class Ctxt;
class EncryptedArray;
namespace core {
/// repeat the first n_slots of 'c' for 'rep' times
/// For example, c = [1, 2, 3, 4]; then repeat(c, 3, 2) => [1, 2, 3, 1, 2 3]
Ctxt repeat(const Ctxt &c, const long n_slots, const long rep, const EncryptedArray &ea);
/// For slots except the first n_slots are zeros
Ctxt repeat0(const Ctxt &c, const long n_slots, const long rep, const EncryptedArray &ea);

void mask_first(Ctxt &ctxt, size_t a, const EncryptedArray &ea);

std::vector<std::vector<long>> random_permutation(long D, const EncryptedArray &ea);

std::vector<std::vector<long>> randomness(long D, const EncryptedArray &ea);

size_t number_bits(long a);

bool dumpCtxts(const std::vector<Ctxt> &ctxts, const std::string &outputDirPath);
/// return x s.t. x = a \mod p1, x = b \mod p2
long apply_crt(long a, long b, long p1, long p2);
template<class HELibObj>
std::string conv(const HELibObj &obj);

template<class HElibObj>
void conv(HElibObj &obj, const std::string &message);
} // namespace core
#endif //CODA_CLION_CTXT_UTIL_HPP
