#ifndef CORE_PPE_SECKEY_HPP
#define CORE_PPE_SECKEY_HPP
#include <memory>
#include <vector>
class FHESecKey;
typedef std::shared_ptr<FHESecKey> raw_sk_ptr;
namespace ppe {
class Context;
class SecKey {
public:
    SecKey() {}

	SecKey(const Context &);

    ~SecKey() {}

	/// @return the specific FHEcontext
	const raw_sk_ptr get(int index) const { return sks_.at(index); }

	size_t partsNum() const { return sks_.size(); }

    void addSome1DMatrices();

    void GenSecKey(long wt);

    bool dump(std::ostream &ostream) const;

    bool restore(std::istream &istream, const Context &);
private:
	std::vector<raw_sk_ptr> sks_;
};
} // namespace ppe

#endif // CORE_PPE_SECKEY_HPP
