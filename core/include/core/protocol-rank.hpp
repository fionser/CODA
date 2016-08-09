//
// Created by riku on 2016/07/04.
//
#ifndef CODA_CLION_PROTOCOL_RANK_HPP
#define CODA_CLION_PROTOCOL_RANK_HPP
#include <vector>
class Ctxt;
namespace rank {
/// Given a collection of encrypted records,
/// to evaluate the rank-K of the specific attribute.
/// E.g.,
bool evaluate(const std::vector<Ctxt> &ciphers,
              size_t attribute,
              long K);
} // name rank
#endif //CODA_CLION_PROTOCOL_RANK_HPP
