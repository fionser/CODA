#ifndef CORE_PPE_HPP
#define CORE_PPE_HPP
#include <memory>
namespace core {
class PPEContext {
public:
    PPEContext();
    ~PPEContext() {};

private:
    class Imp;
    std::shared_ptr<Imp> imp_;
};
typedef std::shared_ptr<PPEContext> context_ptr;

class PPESecKey {
public:
    PPESecKey();
    ~PPESecKey() {}

private:
    class Imp;
    std::shared_ptr<Imp> imp_;
};
typedef std::shared_ptr<PPESecKey> sk_ptr;

class PPEPubKey {
public:
    PPEPubKey();
    ~PPEPubKey() {}

private:
    class Imp;
    std::shared_ptr<Imp> imp_;
};
typedef std::shared_ptr<PPEPubKey> pk_ptr;

class PPEEncrytedArray {
public:
    PPEEncrytedArray();
    ~PPEEncrytedArray() {}

private:
    class Imp;
    std::shared_ptr<Imp> imp_;
};
typedef std::shared_ptr<PPEEncrytedArray> ea_ptr;

} // namespace core
#endif // CORE_PPE_HPP
