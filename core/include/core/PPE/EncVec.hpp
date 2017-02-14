#ifndef CORE_PPE_ENCVEC_HPP
#define CORE_PPE_ENCVEC_HPP
#include "core/algebra/EncVec.hpp"
namespace ppe {
class SecKey;
class PubKey;
class EncVec {
public:
    EncVec(const PubKey &pk);

    EncVec(const EncVec &oth);

    EncVec& operator=(const EncVec &oth);

    ~EncVec() {}

    EncVec& pack(const Vector &vec);

	bool unpack(Vector &result,
				const SecKey &sk,
				bool negate = false) const;

    EncVec& add(const EncVec &c);

    EncVec& add(const Vector &c);

	EncVec& sub(const EncVec &c);

    EncVec& sub(const Vector &c);

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

} // namespace ppe
#endif // CORE_PPE_ENCVEC_HPP
