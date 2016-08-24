#ifndef CORE_CODA_H
#define CORE_CODA_H
#include <iostream>
#include <string>
#include <tuple>
#include <memory>
class FHEcontext;
class FHEPubKey;
class FHESecKey;
class Ctxt;

namespace core {
typedef std::shared_ptr<FHEcontext> context_ptr;
typedef std::shared_ptr<FHEPubKey> pk_ptr;
typedef std::shared_ptr<FHESecKey> sk_ptr;
struct {
    const int CTX_PER_FILE = 50;
} core_setting;

struct ContextPk {
    context_ptr context;
    pk_ptr pk;
};

struct ContextKeys {
    context_ptr context;
    sk_ptr sk;
    pk_ptr pk;
};

struct FHEArg {
    long m;     // cyclotomotic polynomial
    long p, r;  // plaintext space is p^r
    long L;     // levels
};

std::ostream& operator<<(std::ostream &out, const FHEArg &args);
std::istream& operator>>(std::istream &in, FHEArg &args);

enum class Protocol {
    PROT_CI2, // chi-2
    PROT_CON, // contingency table
    PROT_MEAN,
    PROT_MAX,
    PROT_UKN  // unknown
};

std::string toString(Protocol type);

Protocol getProtocol(std::string description);

}; // namespace core
#endif // CORE_CODA_H
