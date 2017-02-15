//
// Created by Lu WJ on 14/02/2017.
//
#include "core/algebra/util.hpp"
#include <cassert>
namespace algebra {

NTL::ZZ _InvMod(NTL::ZZ a, NTL::ZZ p)
{
    NTL::ZZ p0(p), t, q;
    NTL::ZZ x0(0), x1(1);
    if (p == 1) return NTL::to_ZZ(1);
    while (a > 1) {
        q = a / p;
        t = p; p = a % p; a = t;
        t = x0; x0 = x1 - q * x0; x1 = t;
    }
    if (x1 < 0) x1 += p0;
    return  x1;
}

NTL::ZZ apply_crt(const std::vector<NTL::ZZ> &alphas,
                  const std::vector<NTL::ZZ> &primes,
                  const bool negate) {
    NTL::ZZ product(1), sum(0);
    size_t size = alphas.size();
    assert(size <= primes.size());
    for (auto &prime : primes) {
        product *= prime;
    }

    NTL::ZZ half = product >> 1;
    for (size_t i = 0; i < size; i++) {
        NTL::ZZ p = product / primes[i];
        sum += (alphas[i] * _InvMod(p, primes[i]) * p);
    }

    NTL::ZZ ret = sum % product;
    if (negate)
        return ret > half ? ret - product : ret;
    else
        return ret;
}
}

