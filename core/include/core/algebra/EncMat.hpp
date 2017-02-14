#ifndef CORE_ALGEBRA_ENCMAT_HPP
#define CORE_ALGEBRA_ENCMAT_HPP
#include "core/coda.hpp"
#include <vector>
#include <memory>
#include <NTL/mat_ZZ.h>
typedef NTL::mat_ZZ Matrix;
namespace core {
class EncVec;
class EncMat {
public:
    EncMat(core::pk_ptr pk);

    ~EncMat() {}

    EncMat(const EncMat &oth);

    EncMat& operator=(const EncMat &oth);

    long rowNums() const;

    long colNums() const;

    EncMat& pack(const Matrix &mat);

    const EncVec& rowAt(int r) const;

    EncMat& add(const EncMat &oth);

    EncMat& add(const Matrix &c);

    EncMat& sub(const EncMat &oth);

    EncMat& sub(const Matrix &c);

    EncMat& dot(const EncMat &oth);

    EncVec sym_dot(const EncVec &oth) const;

    EncMat& mul(const Matrix &oth);

    bool unpack(Matrix &result,
                core::sk_ptr sk,
                bool negate = false) const;
private:
	class Imp;
	std::shared_ptr<Imp> imp_;
};
} // namespace core
#endif // CORE_ALGEBRA_ENCMAT_HPP
