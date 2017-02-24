//
// Created by Lu WJ on 14/02/2017.
//
#include "core/PPE/EncMat.hpp"
#include "core/PPE/EncVec.hpp"
#include "core/PPE/PubKey.hpp"
#include "core/PPE/SecKey.hpp"
#include "core/algebra/util.hpp"

#include "HElib/EncryptedArray.h"

#include <memory>
#include <NTL/vec_ZZ.h>

typedef NTL::vec_ZZ Vector;
namespace ppe {
class EncMat::Imp {
public:
    Imp(const ppe::PubKey &pk) : colNum_(0), rowNum_(0), pk_(pk) {
        getPrimes(pk_);
        getEncryptedArrays(pk_);
        for (int i = 0; i < pk_.partsNum(); i++) {
            crtParts_.push_back(core::EncMat(pk_.get(i)));
        }
    }

    Imp(const Imp &oth) {
        operator=(oth);
    }

    std::shared_ptr<Imp> copyAsEmpty() const {
        auto dummy = std::make_shared<Imp>(pk_);
        dummy->colNum_ = colNum_;
        dummy->ea_ = ea_;
        dummy->pk_ = pk_;
        dummy->primes_ = primes_;
        dummy->colNum_ = colNum_;
        dummy->rowNum_ = rowNum_;
        return dummy;
    }

    Imp& operator=(const Imp &oth) {
        ea_ = oth.ea_;
        pk_ = oth.pk_;
        primes_ = oth.primes_;
        colNum_ = oth.colNum_;
        rowNum_ = oth.rowNum_;
        crtParts_ = oth.crtParts_;
        return *this;
    }

    bool pack(const Matrix &mat) {
        #pragma omp parallel for
        for (size_t i = 0; i < crtParts_.size(); i++)
            crtParts_.at(i).pack(mat);

        rowNum_ = mat.NumRows();
        colNum_ = mat.NumCols();
        return true;
    }

    bool unpack(Matrix &result, const SecKey &sk, const bool negate) const {
        if (crtParts_.empty())
            return false;
        std::vector<Matrix> alphas(crtParts_.size());
        #pragma omp parallel for
        for (size_t i = 0; i < sk.partsNum(); i++)
            crtParts_.at(i).unpack(alphas.at(i), sk.get(i), false);
        return apply_crt(result, alphas, negate);
    }

    bool add(const std::shared_ptr<Imp> &oth) {
        assert(pk_ == oth->pk_);
        if (colNums() == 0 && rowNums() == 0) {
            colNum_ = oth->colNums();
            rowNum_ = oth->rowNums();
        } else if(colNums() != oth->colNums() || rowNums() != oth->rowNums()) {
            std::cerr << "Mismatch matrix size in EncMat::add()" << std::endl;
            return false;
        }
        for (size_t i = 0; i < this->crtParts_.size(); i++)
            this->crtParts_[i].add(oth->crtParts_.at(i));
        return true;
    }

    bool add(const Matrix &mat) {
        if (colNums() == 0 && rowNums() == 0) {
            colNum_ = mat.NumCols();
            rowNum_ = mat.NumRows();
        } else if (mat.NumRows() != rowNums() || mat.NumRows() != rowNums()) {
            std::cerr << "Mismatch matrix size in EncMat::add()" << std::endl;
            return false;
        }
        for (size_t i = 0; i < this->crtParts_.size(); i++)
            this->crtParts_[i].add(mat);
        return true;
    }

    bool sub(const std::shared_ptr<Imp> &oth) {
        assert(pk_ == oth->pk_);
        if (colNums() == 0 && rowNums() == 0) {
            colNum_ = oth->colNums();
            rowNum_ = oth->rowNums();
        } else if(colNums() != oth->colNums() || rowNums() != oth->rowNums()) {
            std::cerr << "Mismatch matrix size in EncMat::sub()" << std::endl;
            return false;
        }
        for (size_t i = 0; i < this->crtParts_.size(); i++)
            this->crtParts_[i].sub(oth->crtParts_.at(i));
        return true;
    }

    bool sub(const Matrix &mat) {
        if (colNums() == 0 && rowNums() == 0) {
            colNum_ = mat.NumCols();
            rowNum_ = mat.NumRows();
        } else if (mat.NumRows() != rowNums() || mat.NumRows() != rowNums()) {
            std::cerr << "Mismatch matrix size in EncMat::sub()" << std::endl;
            return false;
        }
        for (size_t i = 0; i < this->crtParts_.size(); i++)
            this->crtParts_[i].sub(mat);
        return true;
    }

    bool mul(const Matrix &mat) {
        if (colNums() == 0 && rowNums() == 0) {
            colNum_ = mat.NumCols();
            rowNum_ = mat.NumRows();
        } else if (mat.NumRows() != rowNums() || mat.NumRows() != rowNums()) {
            std::cerr << "Mismatch matrix size in EncMat::mul()" << std::endl;
            return false;
        }
        for (size_t i = 0; i < this->crtParts_.size(); i++)
            this->crtParts_[i].mul(mat);
        return true;
    }

    bool dot(const std::shared_ptr<Imp> &oth) {
        assert(pk_ == oth->pk_);
        if (colNums() == 0 && rowNums() == 0) {
            colNum_ = oth->colNums();
            rowNum_ = oth->rowNums();
        } else if(colNums() != oth->colNums() || rowNums() != oth->rowNums()) {
            std::cerr << "Mismatch matrix size in EncMat::dot()" << std::endl;
            return false;
        }
        #pragma omp parallel for
        for (size_t i = 0; i < this->crtParts_.size(); i++)
            this->crtParts_[i].dot(oth->crtParts_[i]);
        return true;
    }

    ppe::EncVec sym_dot(const ppe::EncVec &vec) {
        assert(colNums() == vec.length());
        ppe::EncVec ret(pk_);
        std::vector<core::EncVec> parts(crtParts_.size());
        #pragma omp parallel for
        for (size_t i = 0; i < parts.size(); i++)
            parts[i] = crtParts_[i].sym_dot(vec.getCRTPartAt(i));
        ret.directSetup(parts);
        return ret;
    }

    void negate() {
        for (size_t i = 0; i < crtParts_.size(); i++)
            crtParts_[i].negate();
    }

    bool dump(std::ostream &out) const {
        out << rowNum_ << " " << colNum_ << std::endl;
        for (const auto &part : crtParts_)
            part.dump(out);
        return true;
    }

    bool restore(std::istream &in) {
        in >> rowNum_ >> colNum_;
        for (auto &part : crtParts_)
            part.restore(in);
        return true;
    }

    long colNums() const { return colNum_; }

    long rowNums() const { return rowNum_; }

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

    bool apply_crt(Matrix &result, const std::vector<Matrix> &plains, const bool negate) const {
        if (plains.empty() || plains.size() > primes_.size()) {
            std::cerr << "Invalid parameter for apply_crt\n";
            return false;
        }

        const long rowNum = plains.front().NumRows();
        const long colNum = plains.front().NumCols();
        for (const auto &m : plains) {
           if (m.NumRows() != rowNum || m.NumCols() != colNum)
               return false;
        }
        std::vector<NTL::ZZ> alphas(plains.size());
        result.SetDims(rowNum, colNum);

        for (long rr = 0; rr < rowNum; rr++) {
            for (long cc = 0; cc < colNum; cc++) {
                for (size_t i = 0; i < plains.size(); i++) {
                    alphas.at(i) = plains.at(i).get(rr, cc);
                }
                result[rr][cc] = algebra::apply_crt(alphas, primes_, negate);
            }
        }
        return true;
    }

private:
    long rowNum_, colNum_;
    ppe::PubKey pk_;
    std::vector<NTL::ZZ> primes_;
    std::vector<const EncryptedArray *> ea_;
    std::vector<core::EncMat> crtParts_;
};

EncMat::EncMat(const PubKey &pk) {
    imp_ = std::make_shared<Imp>(pk);
}

EncMat::EncMat(const EncMat &oth) {
    imp_ = std::make_shared<Imp>(*oth.imp_);
}

EncMat EncMat::copyAsEmpty() const {
    EncMat dummy;
    dummy.imp_ = imp_->copyAsEmpty();
    return dummy;
}

EncMat& EncMat::operator=(const EncMat &oth) {
    imp_ = std::make_shared<Imp>(*oth.imp_);
    return *this;
}

long EncMat::rowNums() const {
    return imp_->rowNums();
}

long EncMat::colNums() const {
    return imp_->colNums();
}

EncMat &EncMat::pack(const Matrix &mat) {
    imp_->pack(mat);
    return *this;
}

bool EncMat::unpack(Matrix &result, const SecKey &sk, bool negate) const {
    return imp_->unpack(result, sk, negate);
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

EncMat& EncMat::dot(const EncMat &oth) {
    imp_->dot(oth.imp_);
    return *this;
}

EncVec EncMat::sym_dot(const EncVec &oth) const {
    return imp_->sym_dot(oth);
}

EncMat& EncMat::mul(const Matrix &oth) {
    imp_->mul(oth);
    return *this;
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

