#include "core/algebra/EncMat.hpp"
#include "core/algebra/EncVec.hpp"

#include "HElib/EncryptedArray.h"
#include "HElib/replicate.h"
#include "HElib/Ctxt.h"
namespace core {
class EncMat::Imp {
public:
    Imp(core::pk_ptr pk) : pk_(pk) {
        ea_ = pk->getContext().ea;
    }

    ~Imp() {}

    Imp& operator=(const Imp &oth) {
        pk_ = oth.pk_;
        ea_ = oth.ea_;
        rowCnt_ = oth.rowCnt_;
        ctxts_ = oth.ctxts_;
        return *this;
    }

    Imp(const Imp &oth) {
        operator=(oth);
    }

    long colNums() const {
        return ctxts_.empty() ? 0 : ctxts_.front().length();
    }

    long rowNums() const {
        return rowCnt_;
    }

    Imp& pack(const Matrix &mat) {
        rowCnt_ = mat.NumRows();
        ctxts_.resize(rowCnt_, pk_);
        for (long r = 0; r < rowCnt_; r++) {
            ctxts_.at(r).pack(mat[r]);
        }
        return *this;
    }

    const EncVec& rowAt(int r) const {
        return ctxts_.at(r);
    }

    bool unpack(Matrix &mat, core::sk_ptr sk, bool negate) const {
        if (ctxts_.empty()) return false;
        mat.SetDims(rowCnt_, ctxts_.front().length());
        for (long r = 0; r < rowCnt_; r++) {
            ctxts_.at(r).unpack(mat[r], sk, negate);
        }
        return true;
    }

    bool dot(const std::shared_ptr<Imp> &oth) {
        if (ctxts_.empty()) {
            std::cerr << "WARN: empty EncMat\n";
            return false;
        }

        if (colNums() != oth->rowNums()) {
            std::cerr << "WARN: mismatch matrix size\n";
            return false;
        }

        std::vector<EncVec> results(rowCnt_, pk_);
        for (size_t r = 0; r < rowCnt_; r++) {
            std::vector<EncVec> replicated = ctxts_[r].replicateAll(oth->colNums());
            for (size_t c = 0; c < colNums(); c++) {
                replicated.at(c).mul(oth->rowAt(c));
            }

            for (size_t c = 1; c < colNums(); c++) {
                replicated[0].add(replicated[c]);
            }
            results[r] = replicated[0];
        }
        ctxts_ = results;
        return true;
    }

    EncVec sym_dot(const EncVec &oth) const {
        assert(rowNums() == colNums() && colNums() == oth.length() && "Only for symmetric matrix");
        EncVec ret(rowAt(0));
        ret.mul(oth.replicate(0, colNums()));
        for (long r = 1; r < rowNums(); r++) {
            EncVec tmp(rowAt(r));
            tmp.mul(oth.replicate(r, colNums()));
            ret.add(tmp);
        }
        return ret;
    }

    bool mul(const Matrix &mat) {
        if (rowCnt_ != mat.NumRows()) return false;
        for (size_t r = 0; r < rowCnt_; r++) {
            ctxts_[r].mul(mat[r]);
        }
        return true;
    }

    bool add(const std::shared_ptr<Imp> &oth) {
        if (rowCnt_ != oth->rowCnt_) return false;
        for (size_t r = 0; r < rowCnt_; r++) {
            ctxts_[r].add(oth->rowAt(r));
        }
        return true;
    }

    bool add(const Matrix &mat) {
        if (rowCnt_ != mat.NumRows()) return false;
        for (size_t r = 0; r < rowCnt_; r++) {
            ctxts_[r].add(mat[r]);
        }
        return true;
    }

    bool sub(const std::shared_ptr<Imp> &oth) {
        if (rowCnt_ != oth->rowCnt_) return false;
        for (size_t r = 0; r < rowCnt_; r++) {
            ctxts_[r].sub(oth->rowAt(r));
        }
        return true;
    }

    bool sub(const Matrix &mat) {
        if (rowCnt_ != mat.NumRows()) return false;
        for (size_t r = 0; r < rowCnt_; r++) {
            ctxts_[r].sub(mat[r]);
        }
        return true;
    }
    friend class EncMat;
private:
    long rowCnt_;
    core::pk_ptr pk_;
    std::vector<EncVec> ctxts_;
    const EncryptedArray *ea_ = nullptr;
};

EncMat::EncMat(core::pk_ptr pk) {
    imp_ = std::make_shared<Imp>(pk);
}

EncMat::EncMat(const EncMat &oth) {
    imp_ = std::make_shared<Imp>(*oth.imp_);
}

EncMat& EncMat::operator=(const EncMat &oth) {
    imp_ = std::make_shared<Imp>(*oth.imp_);
    return *this;
}

long EncMat::colNums() const {
    return imp_->colNums();
}

long EncMat::rowNums() const {
    return imp_->rowNums();
}

EncMat& EncMat::pack(const Matrix &mat) {
    imp_->pack(mat);
    return *this;
}

EncMat& EncMat::dot(const EncMat &oth) {
    imp_->dot(oth.imp_);
    return *this;
}

EncVec EncMat::sym_dot(const EncVec &oth) const {
    return imp_->sym_dot(oth);
}

EncMat& EncMat::mul(const Matrix &c) {
    imp_->mul(c);
    return *this;
}

EncMat& EncMat::add(const EncMat &oth) {
    imp_->add(oth.imp_);
    return *this;
}

EncMat& EncMat::add(const Matrix &c) {
    imp_->add(c);
    return *this;
}

EncMat& EncMat::sub(const EncMat &oth) {
    imp_->sub(oth.imp_);
    return *this;
}

EncMat& EncMat::sub(const Matrix &c) {
    imp_->sub(c);
    return *this;
}

bool EncMat::unpack(Matrix &result,
                    core::sk_ptr sk,
                    bool negate) const {
    return imp_->unpack(result, sk, negate);
}

const EncVec& EncMat::rowAt(int r) const {
    return imp_->rowAt(r);
}
}
