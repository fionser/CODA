#ifndef CORE_ALGEBRA_ENCVEC_HPP
#define CORE_ALGEBRA_ENCVEC_HPP
#include "core/coda.hpp"
#include <NTL/vec_ZZ.h>
#include <vector>
#include <memory>
typedef NTL::vec_ZZ Vector;
namespace core {
class EncVec {
public:
    EncVec(core::pk_ptr pk);

    EncVec(const EncVec &oth);

    EncVec& operator=(const EncVec &oth);

    ~EncVec() {}

    EncVec& pack(const Vector &vec);

	bool unpack(Vector &result,
				core::sk_ptr sk,
				bool negate = false) const;

    EncVec& add(const EncVec &c);

    EncVec& add(const Vector &c);

    EncVec& mul(const Vector &c);
    // equals to lowLevelMul + reLinearize
    EncVec& mul(const EncVec &oth);

    EncVec& lowLevelMul(const EncVec &oth);

    EncVec& reLinearize();

    EncVec replicate(long i) const;

    EncVec replicate(long i, long width) const;

    std::vector<EncVec> replicateAll() const;

    std::vector<EncVec> replicateAll(long width) const;

    long length() const;
private:
	class Imp;
    friend class Imp;
	std::shared_ptr<Imp> imp_;
};
} // namespace core
#endif // CORE_ALGEBRA_ENCVEC_HPP
