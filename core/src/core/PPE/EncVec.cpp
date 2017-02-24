#include "core/PPE/EncVec.hpp"
#include "core/PPE/PubKey.hpp"
#include "core/PPE/SecKey.hpp"
#include "core/algebra/util.hpp"
#include "HElib/EncryptedArray.h"
namespace ppe {
class EncVec::Imp {
public:
    Imp(const PubKey &pk) : pk_(pk) {
        getEncryptedArrays(pk_);
        getPrimes(pk_);
        for (int i = 0; i < pk_.partsNum(); i++) {
            crtParts_.push_back(core::EncVec(pk_.get(i)));
        }
    }

    Imp(const Imp &oth) {
        operator =(oth);
    }

    Imp &operator=(const Imp &oth) {
        ea_ = oth.ea_;
        primes_ = oth.primes_;
        pk_ = oth.pk_;
        crtParts_ = oth.crtParts_;
        return *this;
    }

    bool setUpParts(const std::vector <core::EncVec> &parts) {
        if (pk_.partsNum()!=parts.size())
            return false;
        crtParts_ = parts;
        return true;
    }

    bool pack(const Vector &vec) {
        #pragma omp parallel for
        for (size_t i = 0; i < crtParts_.size(); i++)
            crtParts_.at(i).pack(vec);
        return true;
    }

    bool unpack(Vector &result, const SecKey &sk, bool negate) const {
        if (crtParts_.empty())
            return false;
        std::vector <Vector> alphas(crtParts_.size());
        #pragma omp parallel for
        for (size_t i = 0; i < sk.partsNum(); i++)
            crtParts_.at(i).unpack(alphas.at(i), sk.get(i), false);
        return apply_crt(result, alphas, negate);
    }

    bool add(const std::shared_ptr <Imp> &oth) {
        assert(pk_ == oth->pk_);
        for (size_t i = 0; i < crtParts_.size(); i++)
            crtParts_.at(i).add(oth->crtParts_.at(i));
        return true;
    }

    bool add(const Vector &c) {
        for (auto &crtPart : crtParts_)
            crtPart.add(c);
        return true;
    }

    bool sub(const std::shared_ptr <Imp> &oth) {
        #pragma omp parallel for
        for (size_t i = 0; i < crtParts_.size(); i++)
            crtParts_.at(i).sub(oth->crtParts_.at(i));
        return true;
    }

    bool sub(const Vector &c) {
        for (auto &crtPart : crtParts_)
            crtPart.sub(c);
        return true;
    }

    bool mul(const Vector &c) {
        #pragma omp parallel for
        for (size_t i = 0; i < crtParts_.size(); i++)
            crtParts_[i].mul(c);
        return true;
    }

    // equals to lowLevelMul + reLinearize
    bool mul(const std::shared_ptr <Imp> &oth) {
        if (!lowLevelMul(oth)) return false;
        return reLinearize();
    }

    bool lowLevelMul(const std::shared_ptr <Imp> &oth) {
        #pragma omp parallel for
        for (size_t i = 0; i < crtParts_.size(); i++)
            crtParts_.at(i).lowLevelMul(oth->crtParts_.at(i));
        return true;
    }

    bool reLinearize() {
        #pragma omp parallel for
        for (size_t i = 0; i < crtParts_.size(); i++)
            crtParts_[i].reLinearize();
        return true;
    }

    EncVec replicate(long i) const {
        return replicate(i, length());
    }

    EncVec replicate(long i, long width) const {
        EncVec tmp(pk_);
        for (auto &crtPart : crtParts_) {
            tmp.imp_->crtParts_.emplace_back(crtPart.replicate(i, width));
        }
        return tmp;
    }

    std::vector <EncVec> replicateAll() const {
        return replicateAll(length());
    }

    std::vector<EncVec> replicateAll(long width) const {
        std::vector <EncVec> replicated(length(), pk_);
        #pragma omp parallel for
        for (size_t i = 0; i < replicated.size(); i++) {
            replicated.at(i) = replicate(i, width);
        }
        return replicated;
    }

    long length() const {
        return crtParts_.empty() ? 0 : crtParts_.front().length();
    }

    const core::EncVec &partAt(long index) {
        return crtParts_.at(index);
    }

    bool dump(std::ostream &out) const {
        out << static_cast<int32_t>(crtParts_.size()) << std::endl;
        for (const auto &part : crtParts_)
            part.dump(out);
        return true;
    }

    bool restore(std::istream &in) {
        int32_t part_num;
        in >> part_num;
        if (part_num != crtParts_.size()) {
            std::cerr << "Restore mismatch type ppe::EncVec.";
            std::cerr << "Need " << crtParts_.size() << " but get " << part_num << std::endl;
            return false;
        }
        bool ok = true;
        for (auto &part : crtParts_)
            ok &= part.restore(in);
        return ok;
    }
private:
    void getEncryptedArrays(const PubKey &pk) {
        ea_.resize(pk.partsNum());
        for (size_t i = 0; i < ea_.size(); i++) {
            ea_[i] = pk.get(i)->getContext().ea;
        }
    }

    void getPrimes(const PubKey &pk) {
        primes_.resize(pk.partsNum());
        for (size_t i = 0; i < primes_.size(); i++) {
            primes_[i] = NTL::to_ZZ(pk.get(i)->getContext().alMod.getPPowR());
        }
    }

    bool apply_crt(Vector &result, const std::vector <Vector> &plains, const bool negate) const {
        if (plains.empty() || plains.size() > primes_.size()) {
            std::cerr << "Invalid parameter for apply_crt\n";
            return false;
        }
        Vector tmp;
        tmp.SetLength(plains.front().length());
        for (long pos = 0; pos < tmp.length(); pos++) {
            std::vector <NTL::ZZ> alphas;
            alphas.reserve(plains.size());
            for (auto &plain : plains) {
                if (plain.length() != tmp.length()) {
                    std::cerr << "Mismath length for apply_crt\n";
                    return false;
                }
                alphas.push_back(plain[pos]);
            }
            tmp[pos] = algebra::apply_crt(alphas, primes_, negate);
        }
        result = tmp;
        return true;
    }

private:
    PubKey pk_;
    std::vector<NTL::ZZ> primes_;
    std::vector<const EncryptedArray *> ea_;
    std::vector<core::EncVec> crtParts_;
};

EncVec::EncVec(const PubKey &pk) {
    imp_ = std::make_shared <EncVec::Imp>(pk);
}

EncVec::EncVec(const EncVec &oth) {
    imp_ = std::make_shared <Imp>(*oth.imp_);
}

bool EncVec::directSetup(const std::vector <core::EncVec> &crt_parts) {
    return imp_->setUpParts(crt_parts);
}

EncVec &EncVec::operator =(const EncVec &oth) {
    imp_ = std::make_shared <Imp>(*oth.imp_);
    return *this;
}

EncVec &EncVec::mul(const EncVec &oth) {
    imp_->mul(oth.imp_);
    return *this;
}

EncVec &EncVec::lowLevelMul(const EncVec &oth) {
    imp_->lowLevelMul(oth.imp_);
    return *this;
}

EncVec &EncVec::reLinearize() {
    imp_->reLinearize();
    return *this;
}

EncVec &EncVec::add(const EncVec &oth) {
    imp_->add(oth.imp_);
    return *this;
}

EncVec &EncVec::add(const Vector &c) {
    imp_->add(c);
    return *this;
}

EncVec &EncVec::sub(const EncVec &oth) {
    imp_->sub(oth.imp_);
    return *this;
}

EncVec &EncVec::sub(const Vector &c) {
    imp_->sub(c);
    return *this;
}

EncVec &EncVec::mul(const Vector &c) {
    imp_->mul(c);
    return *this;
}

EncVec &EncVec::pack(const Vector &vec) {
    imp_->pack(vec);
    return *this;
}

bool EncVec::unpack(Vector &result,
                    const SecKey &sk,
                    bool negate) const {
    return imp_->unpack(result, sk, negate);
}

EncVec EncVec::replicate(long i) const {
    return imp_->replicate(i);
}

EncVec EncVec::replicate(long i, long width) const {
    return imp_->replicate(i, width);
}

std::vector <EncVec> EncVec::replicateAll() const {
    return imp_->replicateAll();
}

std::vector <EncVec> EncVec::replicateAll(long width) const {
    return imp_->replicateAll(width);
}

long EncVec::length() const {
    return imp_->length();
}

const core::EncVec &EncVec::getCRTPartAt(long index) const {
    return imp_->partAt(index);
}

bool EncVec::dump(std::ostream &out) const {
    return imp_->dump(out);
}

bool EncVec::restore(std::istream &in) {
    return imp_->restore(in);
}
}
