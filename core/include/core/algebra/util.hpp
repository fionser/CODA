#ifndef CORE_ALGEBRA_UTIL_HPP
#define CORE_ALGEBRA_UTIL_HPP
#include <vector>
#include <NTL/ZZ.h>
namespace algebra {
NTL::ZZ _InvMod(NTL::ZZ a, NTL::ZZ p);
NTL::ZZ apply_crt(const std::vector<NTL::ZZ> &alphas, const std::vector<NTL::ZZ> &primes, const bool negate);
}
#endif // CORE_ALGEBRA_UTIL_HPP
