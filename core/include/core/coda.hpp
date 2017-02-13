#ifndef CORE_CODA_H
#define CORE_CODA_H
#include "core/PPE/types.hpp"
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
struct ContextWrapper {
    context_ptr single;
    ppe::context_ptr ppe;
};

struct SecKeyWrapper {
    sk_ptr single;
    ppe::sk_ptr ppe;
};

struct PubKeyWrapper {
    pk_ptr single;
    ppe::pk_ptr ppe;
};

struct {
    const int CTX_PER_FILE = 50;
    const std::string decrypted_file = "File_result";
    const std::string resulting_file = "File_result";
    const std::string random_share_file = ".random_share";
    const int gc_user_port = 56789;
} core_setting;

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
    PROT_HYBRID_CON, // hybrid protocol for large contingency table
    PROT_MEAN,
    PROT_PERCENTILE, // k-percentile
    PROT_UNKOWN  // unknown
};

std::string toString(Protocol type);

Protocol getProtocol(std::string description);

}; // namespace core
#endif // CORE_CODA_H
