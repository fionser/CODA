#ifndef CORE_ALGEBRA_ENCMAT_HPP
#define CORE_ALGEBRA_ENCMAT_HPP
#include "core/coda.hpp"
#include <vector>
#include <memory>
#include <iosfwd>
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

	bool unpack(Matrix &result, core::sk_ptr sk, bool negate = false) const;

    const EncVec& rowAt(int r) const;

    EncMat& add(const EncMat &oth);

    EncMat& add(const Matrix &c);

    EncMat& sub(const EncMat &oth);

    EncMat& sub(const Matrix &c);
    /// this * oth
    EncMat& dot(const EncMat &oth);
    /// this * oth, but this is symmetric matrix
    /// faster when oth is with the auxilary made.
    /// this * oth equals to oth * this since this is symmetric
    EncMat& sym_dot(const EncMat &oth);

    bool make_auxilary();

    EncVec sym_dot(const EncVec &oth) const;

    EncMat& mul(const Matrix &oth);

    EncMat& mul(const NTL::ZZ &c);

    EncMat& mul(const long &c);

    EncMat& negate();

	bool dump(std::ostream &out) const;

	bool restore(std::istream &in);
private:
	class Imp;
	std::shared_ptr<Imp> imp_;
};
} // namespace core
#endif // CORE_ALGEBRA_ENCMAT_HPP
