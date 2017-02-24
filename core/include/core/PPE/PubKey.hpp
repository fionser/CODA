#ifndef CORE_PPE_PUBKEY_HPP
#define CORE_PPE_PUBKEY_HPP
#include <memory>
#include <vector>
#include <iosfwd>
class FHEPubKey;
typedef std::shared_ptr<FHEPubKey> raw_pk_ptr;
namespace ppe {
class SecKey;
class Context;
class PubKey {
public:
    PubKey() {}

    PubKey(const SecKey &sk);

    ~PubKey() {}

    bool operator==(const PubKey &oth) const;

    const raw_pk_ptr get(int idx) const { return pks_.at(idx); }

    size_t partsNum() const { return pks_.size(); }

    bool dump(std::ostream &ostream) const;

    bool restore(std::istream &istream, const Context &context);
private:
    std::vector<raw_pk_ptr> pks_;
};
}
#endif // CORE_PPE_PUBKEY_HPP
