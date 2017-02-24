#include "core/algebra/EncMat.hpp"
#include "core/algebra/EncVec.hpp"

#include "HElib/EncryptedArray.h"
#include "HElib/replicate.h"
#include "HElib/Ctxt.h"
namespace core {
class EncMat::Imp {
public:
    Imp(core::pk_ptr pk) : rowCnt_(0), pk_(pk) {
        ea_ = pk->getContext().ea;
        ctxts_.clear();
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
        if (empty()) {
            std::cerr << "core::EncMat. Unpacking empty ciphertext" << std::endl;
            return false;
        }
        mat.SetDims(rowCnt_, ctxts_.front().length());
        for (long r = 0; r < rowCnt_; r++)
            ctxts_.at(r).unpack(mat[r], sk, negate);
        return true;
    }

    bool dot(const std::shared_ptr<Imp> &oth) {
        if (empty()) {
            std::cerr << "WARN: empty EncMat::dot" << std::endl;
            return false;
        }

        if (colNums() != oth->rowNums()) {
            std::cerr << "WARN: mismatch matrix size (EncMat::dot)" << std::endl;
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

    bool sym_dot(const std::shared_ptr<Imp> &oth) {
        if (oth->aux_replicated_.empty()) { // no auxilary data is made
            return dot(oth);
        }
        if (empty()) {
            std::cerr << "WARN: empty object in " << __func__ << std::endl;
            return false;
        }
        if (colNums() != rowNums()) {
            std::cerr << "WARN: sym_dot need square matrix" << std::endl;
            return false;
        }
        if (colNums() != oth->rowNums()) {
            std::cerr << "WARN: mismatch matrix size in " << __func__ << std::endl;
            return false;
        }

        return false;
    }

    bool make_auxilary() {
        if (empty()) {
            std::cerr << "WARN: Can not make auxilary for empty EncMat" << std::endl;
            return false;
        }
        if (!aux_replicated_.empty()) return true;
        aux_replicated_.resize(rowCnt_);
        for (size_t r = 0; r < aux_replicated_.size(); r++) {
            aux_replicated_[r] = ctxts_[r].replicateAll(rowCnt_);
        }
        return true;
    }

    EncVec sym_dot(const EncVec &oth) const {
        assert(rowNums() == colNums() && colNums() == oth.length() && "Only for symmetric matrix");
        assert(!empty() && "EncVec::sym_dot not for empty object");
        EncVec ret(rowAt(0));
        ret.lowLevelMul(oth.replicate(0, colNums()));
        for (long r = 1; r < rowNums(); r++) {
            EncVec tmp(rowAt(r));
            tmp.lowLevelMul(oth.replicate(r, colNums()));
            ret.add(tmp);
        }
        ret.reLinearize();
        return ret;
    }

    bool mul(const Matrix &mat) {
        if (empty()) {
            Matrix zeros;
            zeros.SetDims(mat.NumRows(), mat.NumCols());
            pack(zeros);
        } else if (rowCnt_ == mat.NumRows()) {
            #pragma omp parallel for
            for (size_t r = 0; r < rowCnt_; r++)
                ctxts_[r].mul(mat[r]);
        } else {
            std::cerr << "Mismatch matrix size in EncMat::mul()" << std::endl;
            return false;
        }
        return true;
    }

    bool mul(const NTL::ZZ &c) {
        if(!empty()) {
            for (size_t r = 0; r < rowCnt_; r++)
                ctxts_[r].mul(c);
        }
        return true;
    }

    bool add(const std::shared_ptr<Imp> &oth) {
        assert(pk_ == oth->pk_);
        if (empty()) {
            this->operator=(*oth);
        } else if (rowCnt_ == oth->rowCnt_) {
            for (size_t r = 0; r < rowCnt_; r++)
                ctxts_[r].add(oth->rowAt(r));
        } else {
            std::cerr << "Mismatch matrix size in EncMat::add()" << std::endl;
            return false;
        }
        return true;
    }

    bool add(const Matrix &mat) {
        if (empty()) {
            pack(mat);
        } else if (rowCnt_ == mat.NumRows()) {
            for (size_t r = 0; r < rowCnt_; r++)
                ctxts_[r].add(mat[r]);
        } else {
            std::cerr << "Mismatch matrix size in EncMat::add()" << std::endl;
            return false;
        }
        return true;
    }

    bool sub(const std::shared_ptr<Imp> &oth) {
        if (empty()) {
            this->operator=(*oth);
            this->negate();
        } else if (rowCnt_ == oth->rowCnt_) {
            for (size_t r = 0; r < rowCnt_; r++)
                ctxts_[r].sub(oth->rowAt(r));
        } else {
            std::cerr << "Mismatch matrix size in EncMat::sub()" << std::endl;
            return false;
        }
        return true;
    }

    bool sub(const Matrix &mat) {
        if (empty()) {
            pack(mat);
            negate();
        } else if (rowCnt_ == mat.NumRows()) {
            for (size_t r = 0; r < rowCnt_; r++)
                ctxts_[r].sub(mat[r]);
        } else {
            std::cerr << "Mismatch matrix size in EncMat::sub()" << std::endl;
            return false;
        }
        return true;
    }

    bool negate() {
        for (auto &ctxt : ctxts_)
            ctxt.negate();
        return true;
    }

    bool dump(std::ostream &out) const {
        out << rowCnt_  << " " << static_cast<int32_t>(ctxts_.size()) << std::endl;
        for (const auto &ctxt : ctxts_)
            ctxt.dump(out);
        return true;
    }

    bool restore(std::istream &in) {
        in >> rowCnt_;
        int32_t parts;
        in >> parts;
        ctxts_.resize(parts, pk_);
        for (size_t i = 0; i < parts; i++)
            ctxts_.at(i).restore(in);
        return true;
    }

    friend class EncMat;

private:
    bool empty() const { return ctxts_.empty(); }

private:
    long rowCnt_;
    core::pk_ptr pk_;
    std::vector<EncVec> ctxts_;
    std::vector<std::vector<EncVec>> aux_replicated_;
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

EncMat& EncMat::sym_dot(const EncMat &oth) {
    imp_->sym_dot(oth.imp_);
    return *this;
}

bool EncMat::make_auxilary() {
    return imp_->make_auxilary();
}

EncVec EncMat::sym_dot(const EncVec &oth) const {
    return imp_->sym_dot(oth);
}

EncMat& EncMat::mul(const Matrix &c) {
    imp_->mul(c);
    return *this;
}

EncMat& EncMat::mul(const NTL::ZZ &c) {
    imp_->mul(c);
    return *this;
}

EncMat& EncMat::mul(const long &c) {
    imp_->mul(NTL::to_ZZ(c));
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

EncMat& EncMat::negate() {
    imp_->negate();
    return *this;
}

bool EncMat::dump(std::ostream &out) const {
    return imp_->dump(out);
}

bool EncMat::restore(std::istream &in) {
    return imp_->restore(in);
}
}
