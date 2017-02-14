#include "core/algebra/EncVec.hpp"

#include "HElib/EncryptedArray.h"
#include "HElib/Ctxt.h"
#include "HElib/replicate.h"

namespace core {
typedef std::shared_ptr<Ctxt> ctxt_ptr;
typedef std::vector<long> iVec;
static long inline NUM_PARTS(long slots, long length) {
    assert(slots > 0 && length > 0);
    return (length + slots - 1) / slots;
}

class EncVec::Imp {
public:
    Imp(core::pk_ptr pk) : pk_(pk) {
        ea_ = pk->getContext().ea;
        ptxt_ = pk->getContext().alMod.getPPowR();
    }

    ~Imp() {}

    Imp(const Imp &oth) {
        operator=(oth);
    }

    Imp& operator=(const Imp &oth) {
        length_ = oth.length_;
        ea_ = oth.ea_;
        pk_ = oth.pk_;
        ctxts_.resize(oth.ctxts_.size());
        ptxt_ = oth.ptxt_;
        for (size_t i = 0; i < ctxts_.size(); i++)
            ctxts_.at(i) = std::make_shared<Ctxt>(*oth.ctxts_[i]);
        return *this;
    }

    bool pack(const Vector &vec) {
        long n = NUM_PARTS(ea_->size(), vec.length());
        std::vector<iVec> parts = extend(vec, n);
        ctxts_.resize(n);
        #pragma omp parallel for
        for (size_t i = 0; i < n; i++) {
            ctxts_.at(i) = std::make_shared<Ctxt>(*pk_);
            ea_->encrypt(*ctxts_[i], *pk_, parts[i]);
        }
        length_ = vec.length();
        return true;
    }

    bool unpack(Vector &result,
                core::sk_ptr sk,
                bool  negate) const {
        if (ctxts_.empty()) {
            std::cerr << "core::EncVec. Unpacking empty ciphertext" << std::endl;
            return false;
        }
        std::vector<iVec> parts = extend(length_);
        #pragma omp parallel for
        for (size_t i = 0; i + 1 < ctxts_.size(); i++) {
            check(ctxts_.at(i));
            ea_->decrypt(*(ctxts_[i]), *sk, parts[i]);
        }

        unpackLastPart(parts.back(), sk);
        return merge(result, parts, negate);
    }

    bool add(const std::shared_ptr<Imp> &oth) {
        assert(oth && "nullptr");
        if (length() != oth->length()) {
            std::cerr << "WARN: Mismatch size of EncVec: " << length() << " != " << oth->length() << "\n";
            return false;
        }
        for (size_t i = 0; i < ctxts_.size(); i++)
            ctxts_[i]->operator+=(*(oth->ctxts_[i]));
        return true;
    }

    bool add(const Vector &c) {
        if (c.length() != length()) {
            std::cerr << "WARN: Mismatch size of EncVec: " << length() << " != " << c.length() << "\n";
            return false;
        }
        std::vector<iVec> constants = extend(c, ctxts_.size());
        NTL::ZZX poly;
        for (size_t i = 0; i < constants.size(); i++) {
            ea_->encode(poly, constants[i]);
            ctxts_[i]->addConstant(poly);
        }
        return true;
    }

    bool sub(const std::shared_ptr<Imp> &oth) {
        assert(oth && "nullptr");
        if (length() != oth->length()) {
            std::cerr << "WARN: Mismatch size of EncVec: " << length() << " != " << oth->length() << "\n";
            return false;
        }
        for (size_t i = 0; i < ctxts_.size(); i++)
            ctxts_[i]->operator-=(*(oth->ctxts_[i]));
        return true;
    }

    bool sub(const Vector &c) {
        if (c.length() != length()) {
            std::cerr << "WARN: Mismatch size of EncVec: " << length() << " != " << c.length() << "\n";
            return false;
        }
        std::vector<iVec> constants = extend(c, ctxts_.size());
        negate(constants);
        NTL::ZZX poly;
        for (size_t i = 0; i < constants.size(); i++) {
            ea_->encode(poly, constants[i]);
            ctxts_[i]->addConstant(poly);
        }
        return true;
    }

    bool mul(const std::shared_ptr<Imp> &oth) {
        if (!lowLevelMul(oth)) return false;
        reLinearize();
        return true;
    }

    bool lowLevelMul(const std::shared_ptr<Imp> &oth) {
        assert(oth && "nullptr");
        if (oth->length() != length()) {
            std::cerr << "WARN: Mismatch size of EncVec: " << length() << " != " << oth->length() << "\n";
            return false;
        }
        #pragma omp parallel for
        for (size_t i = 0; i < ctxts_.size(); i++)
            ctxts_[i]->operator*=(*(oth->ctxts_.at(i)));
        return true;
    }

    void reLinearize() {
        for (auto ctx : ctxts_)
            ctx->reLinearize();
    }

    bool mul(const Vector &c) {
        if (c.length() != length_)
            return false;
        std::vector<iVec> constants = extend(c, ctxts_.size());
        NTL::ZZX poly;
        #pragma omp parallel for
        for (size_t i = 0; i < constants.size(); i++) {
            ea_->encode(poly, constants[i]);
            ctxts_[i]->multByConstant(poly);
        }
        return true;
    }

    EncVec replicate(long i) const {
        return replicate(i, length());
    }

    EncVec replicate(long i, long width) const {
        assert(i >= 0 && i < length());
        assert(width >= 0 && width < length());

        long N = NUM_PARTS(ea_->size(), width);
        long idx = i / ea_->size();
        long pos = i % ea_->size();
        long replication_length = N > 1 ? ea_->size() : width;
        assert(idx >= 0 && idx < ctxts_.size());
        auto ctx = std::make_shared<Ctxt>(*ctxts_.at(idx));
        ::replicate(*ea_, *ctx, pos, replication_length);

        std::vector<ctxt_ptr> all_same(N);
        all_same[0] = ctx;
        for (int i = 1; i < all_same.size(); i++)
            all_same.at(i) = std::make_shared<Ctxt>(*ctx);

        EncVec tmp(pk_);
        tmp.imp_->length_ = width;
        tmp.imp_->ctxts_ = all_same;
        return tmp;
    }

    std::vector<EncVec> replicateAll() const {
        return replicateAll(length());
    }

    std::vector<EncVec> replicateAll(long width) const {
        assert(width > 0 && "Invalid replicateAll parameter");
        std::vector<EncVec> replicated(length(), pk_);
        #pragma omp parallel for
        for (size_t i = 0; i < replicated.size(); i++) {
            replicated.at(i) = replicate(i, width);
        }
        return replicated;
    }

    bool dump(std::ostream &out) const {
        out << static_cast<int32_t>(ctxts_.size()) << " " << length_ << std::endl;
        for (const auto &ctx : ctxts_)
            out << *ctx << std::endl;
        return true;
    }

    bool restore(std::istream &in) {
        if (!pk_) {
            std::cerr << "From EncVec::restore(): pk can no be empty." << std::endl;
            return false;
        }

        int32_t parts;
        in >> parts;
        in >> length_;
        ctxts_.resize(parts);
        for (int32_t i = 0; i < parts; i++) {
            ctxts_[i] = std::make_shared<Ctxt>(*pk_);
            in >> *(ctxts_[i]);
        }
        return true;
    }
    friend class EncVec;

private:
    void check(ctxt_ptr ctx) const {
        if (!ctx->isCorrect()) {
            std::cerr << "WARN! the ciphertext might be invalid\n";
        }
    }

    bool merge(Vector &result, const std::vector<iVec> &parts, bool neg) const {
        result.SetLength(length_, NTL::to_ZZ(0));
        long idx = 0;
        long half = ptxt_ >> 1;
        auto halfer = [](const long v, const long p, const long _2p) -> long {
            if (v > p) return v - _2p;
            return v;
        };

        for (const iVec &part : parts) {
            for (long ele : part) {
                result[idx] = neg ? halfer(ele, half, ptxt_) : ele;
                idx += 1;
                if (idx >= length_) break;
            }
            if (idx >= length_) break;
        }
        return true;
    }

    void unpackLastPart(iVec &out, core::sk_ptr sk) const {
        long last_part_size = length_ - (ctxts_.size() - 1) * ea_->size();
        if (last_part_size == 0) return;

        std::vector<long> locations(last_part_size, 0);
        for (long i = 0; i < last_part_size; i++) locations[i] = i;
        NTL::ZZX poly;
        check(ctxts_.back());
        sk->Decrypt(poly, *ctxts_.back());
        ea_->decodeSlots(out, poly, locations);
    }

    std::vector<iVec> extend(size_t n) const {
        size_t nn = (n + ea_->size() - 1) / ea_->size();
        iVec tmp(ea_->size(), 0);
        return std::vector<iVec>(nn, tmp);
    }

    std::vector<iVec> extend(const Vector &vec, size_t n) const {
        std::vector<iVec> parts(n, iVec(ea_->size(), 0));
        auto itr = vec.begin();
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < ea_->size() && itr != vec.end(); j++) {
                parts[i][j] = NTL::to_long(*itr++);
            }
        }
        return parts;
    }

    void negate(std::vector<iVec> &parts) {
        for (iVec& part : parts) {
           for (long &ele : part) {
               ele = (-1 * ele) % ptxt_;
           }
        }
    }

    long length() const { return length_; }
private:
    long length_ = 0;
    long ptxt_ = 0;
    const EncryptedArray *ea_ = nullptr;
    core::pk_ptr pk_;
    std::vector<ctxt_ptr> ctxts_;
};

EncVec::EncVec(core::pk_ptr pk) {
    imp_ = std::make_shared<EncVec::Imp>(pk);
}

EncVec::EncVec(const EncVec &oth) {
    imp_ = std::make_shared<Imp>(*oth.imp_);
}

EncVec& EncVec::operator=(const EncVec &oth) {
    imp_ = std::make_shared<Imp>(*oth.imp_);
    return *this;
}

EncVec& EncVec::mul(const EncVec &oth) {
    imp_->mul(oth.imp_);
    return *this;
}

EncVec& EncVec::lowLevelMul(const EncVec &oth) {
    imp_->lowLevelMul(oth.imp_);
    return *this;
}

EncVec& EncVec::reLinearize() {
    imp_->reLinearize();
    return *this;
}

EncVec& EncVec::add(const EncVec &oth) {
    imp_->add(oth.imp_);
    return *this;
}

EncVec& EncVec::add(const Vector &c) {
    imp_->add(c);
    return *this;
}

EncVec& EncVec::sub(const EncVec &oth) {
    imp_->sub(oth.imp_);
    return *this;
}

EncVec& EncVec::sub(const Vector &c) {
    imp_->sub(c);
    return *this;
}

EncVec& EncVec::mul(const Vector &c) {
    imp_->mul(c);
    return *this;
}

EncVec& EncVec::pack(const Vector &vec) {
    imp_->pack(vec);
    return *this;
}

bool EncVec::unpack(Vector &result,
                    core::sk_ptr sk,
                    bool negate) const {
    return imp_->unpack(result, sk, negate);
}

EncVec EncVec::replicate(long i) const {
    return imp_->replicate(i);
}

EncVec EncVec::replicate(long i, long width) const {
    return imp_->replicate(i, width);
}

std::vector<EncVec> EncVec::replicateAll() const {
    return imp_->replicateAll();
}

std::vector<EncVec> EncVec::replicateAll(long width) const {
    return imp_->replicateAll(width);
}

long EncVec::length() const {
    return imp_->length();
}

bool EncVec::dump(std::ostream &out) const {
    return imp_->dump(out);
}

bool EncVec::restore(std::istream &in) {
    return imp_->restore(in);
}
} // namespace core
