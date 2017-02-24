//
// Created by Lu WJ on 14/02/2017.
//

#ifndef CORE_PPE_ENCMAT_HPP
#define CORE_PPE_ENCMAT_HPP
#include "core/algebra/EncMat.hpp"

#include <iosfwd>
namespace ppe {
class SecKey;
class PubKey;
class EncVec;
class EncMat {
public:
	EncMat(const PubKey &pk);

	~EncMat() {}

	EncMat(const EncMat &oth);

    EncMat copyAsEmpty() const;

	EncMat& operator=(const EncMat &oth);

    EncMat& negate();

	long rowNums() const;

	long colNums() const;

	EncMat& pack(const Matrix &mat);

	EncMat& add(const EncMat &oth);

	EncMat& add(const Matrix &c);

	EncMat& sub(const EncMat &oth);

	EncMat& sub(const Matrix &c);

	EncMat& dot(const EncMat &oth);

	EncVec sym_dot(const EncVec &oth) const;

	EncMat& mul(const Matrix &oth);

	bool unpack(Matrix &result,
				const SecKey &sk,
				bool negate = false) const;

	bool dump(std::ostream &out) const;

	bool restore(std::istream &in);

private:
    EncMat() : imp_(nullptr) {}

private:
	class Imp;
    friend class Imp;
	std::shared_ptr<Imp> imp_;
};

} // namespace ppe
#
#endif //CORE_PPE_ENCMAT_HPP
