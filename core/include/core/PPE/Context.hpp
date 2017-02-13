#ifndef CORE_PPE_CONTEXT_HPP
#define CORE_PPE_CONTEXT_HPP

#include <memory>
#include <vector>
#include <NTL/ZZ.h>
class FHEcontext;
typedef std::shared_ptr<FHEcontext> raw_context_ptr;
namespace ppe {
class Context {
public:
    // dummy constructor
    Context() {}

	Context(const std::vector<long> &Ms,
			const std::vector<long> &Ps,
			const std::vector<long> &Rs);

    Context(long m, long p, long r) : Context(std::vector<long>(1, m),
											  std::vector<long>(1, p),
											  std::vector<long>(1, r)) {}

	void buildModChain(long L);

	/// @return the specific FHEcontext
	const raw_context_ptr get(int index) const { return contexts_.at(index); }

	size_t partsNum() const { return contexts_.size(); }

	double precision() const;

    NTL::ZZ plainSpace() const { return plainSpace_; }

    bool dump(std::ostream &ostream) const;

    bool restore(std::istream &istream);

private:
    NTL::ZZ plainSpace_;
	std::vector<raw_context_ptr> contexts_;
};
}
#endif // CORE_PPE_CONTEXT_HPP
