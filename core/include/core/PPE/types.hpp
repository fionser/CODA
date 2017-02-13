#ifndef CORE_PPT_TYPES_HPP
#include <memory>
namespace ppe {
class Context;
class PubKey;
class SecKey;
typedef std::shared_ptr<Context> context_ptr;
typedef std::shared_ptr<PubKey> pk_ptr;
typedef std::shared_ptr<SecKey> sk_ptr;
}
#endif
