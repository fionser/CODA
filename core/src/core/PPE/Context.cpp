#include "core/PPE/Context.hpp"
#include "HElib/FHEContext.h"
#include <cmath>
#include <vector>
#include <cassert>
namespace ppe {

Context::Context(const std::vector<long> &Ms,
				 const std::vector<long> &Ps,
				 const std::vector<long> &Rs) {
	assert(Ms.size() == Ps.size() && Ps.size() == Rs.size());
	contexts_.resize(Ms.size());
#pragma omp parallel for
	for (size_t i = 0; i < Ms.size(); i++) {
		contexts_.at(i) = std::make_shared<FHEcontext>(Ms[i], Ps[i], Rs[i]);
	}

	plainSpace_ = NTL::to_ZZ(1);
	for (size_t i = 0; i < Ps.size(); i++) {
		NTL::ZZ p = NTL::to_ZZ(Ps[i]);
		NTL::power(p, p, Rs[i]);
		plainSpace_ *= p;
	}
}

void Context::buildModChain(long L)
{
#pragma omp parallel for
	for (size_t i = 0; i < contexts_.size(); i++)
		::buildModChain(*contexts_.at(i), L);
}

double Context::precision() const
{
	return NTL::log(plainSpace()) / NTL::log(NTL::to_ZZ(2));
}

bool Context::dump(std::ostream &ostream) const {
    ostream << static_cast<int32_t>(contexts_.size());
    for (auto &context : contexts_) {
        writeContextBase(ostream, *context);
        ostream << "\n" << *context;
    }
    return true;
}

bool Context::restore(std::istream &in) {
    int32_t parts;
    in >> parts;

    unsigned long m, p, r;
    std::vector<long> gens, ords;
    NTL::ZZ P, Pr;

    contexts_.resize(parts);
    plainSpace_ = NTL::to_ZZ(1);
    for (int32_t i = 0; i < parts; i++) {
        readContextBase(in, m, p, r, gens, ords);
        contexts_[i] = std::make_shared<FHEcontext>(m, p, r, gens, ords);
        P = NTL::to_ZZ(p);
        NTL::power(Pr, P, r);
        plainSpace_ *= Pr;
        in >> *contexts_[i];
    }
    return true;
}
} // namespace ppe
