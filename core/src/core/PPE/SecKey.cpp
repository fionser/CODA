#include "core/PPE/SecKey.hpp"
#include "core/PPE/Context.hpp"
#include "HElib/FHE.h"
namespace ppe {
SecKey::SecKey(const Context &context) {
    sks_.resize(context.partsNum());
#pragma omp parallel for
    for (size_t i = 0; i < context.partsNum(); i++) {
        sks_.at(i) = std::make_shared<FHESecKey>(*context.get(i));
    }
}

void SecKey::GenSecKey(long wt) {
#pragma omp parallel for
    for (auto sk : sks_) {
        sk->GenSecKey(wt);
    }
}

void SecKey::addSome1DMatrices() {
#pragma omp parallel for
    for (auto sk : sks_) {
        ::addSome1DMatrices(*sk);
    }
}

bool SecKey::dump(std::ostream &ostream) const {
    ostream << static_cast<int32_t>(sks_.size());
    for (auto sk : sks_)
        ostream << *sk << "\n";
    return true;
}

bool SecKey::restore(std::istream &istream, const Context &context) {
    int32_t parts;
    istream >> parts;
    if (parts != context.partsNum()) return false;
    sks_.resize(parts);
    for (int32_t i = 0; i < parts; i++) {
        sks_[i] = std::make_shared<FHESecKey>(*(context.get(i)));
        istream >> *(sks_[i]);
    }
    return true;
}
} // namespace ppe
