#include "core/PPE/PubKey.hpp"
#include "core/PPE/SecKey.hpp"
#include "core/PPE/Context.hpp"
#include "HElib/FHE.h"
namespace ppe {
PubKey::PubKey(const SecKey &sk) {
    pks_.resize(sk.partsNum());
#pragma omp parallel for
    for (size_t i = 0; i < pks_.size(); i++) {
        pks_[i] = std::make_shared<FHEPubKey>(*sk.get(i));
    }
}

bool PubKey::dump(std::ostream &ostream) const {
    ostream << pks_.size();
    for (auto pk : pks_)
        ostream << *pk;
    return true;
}

bool PubKey::restore(std::istream &istream, const Context &context) {
    size_t parts;
    istream >> parts;
    if (parts != context.partsNum()) return false;
    pks_.resize(parts);
    for (size_t i = 0; i < parts; i++) {
        pks_[i] = std::make_shared<FHEPubKey>(*(context.get(i)));
        istream >> *(pks_[i]);
    }
    return true;
}
} // name ppe
