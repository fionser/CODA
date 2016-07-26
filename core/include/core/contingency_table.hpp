//
// Created by riku on 2016/07/19.
//

#ifndef CODA_CLION_CONTINGENCY_TABLE_HPP
#define CODA_CLION_CONTINGENCY_TABLE_HPP
#include <vector>
#include <string>
#include <memory>
#include "coda.hpp"
#include "../HElib/Ctxt.h"
#include "greaterthan.h"

namespace core {
struct Attribute {
    enum class Type {
        NUMERICAL,
        CATEGORICAL,
        ORDINAL
    };

    std::string text;
    Type type;
    size_t size;
    size_t offset;
};

class PrivateContingencyTableHelper;


class PrivateContingencyTable {
public:
    typedef std::shared_ptr<Ctxt> ctxt_ptr;
    struct ResultType {
        typedef std::vector<ctxt_ptr> Type_n_uv;
        typedef std::vector<ctxt_ptr> Type_gamma;
        Type_n_uv n_uv;
        Type_gamma gamma;
        Type_gamma tilde_gamma;
    };

    PrivateContingencyTable(context_ptr context,
                            PrivateContingencyTableHelper *helper);

    ~PrivateContingencyTable() {}

    ResultType evaluate(const std::vector<Ctxt> &attributes) const;

private:
    ctxt_ptr compute_table(const std::vector<Ctxt> &attributes,
                           Attribute p, Attribute q,
                           const EncryptedArray *ea) const;

    ResultType::Type_gamma special_greater_than(ctxt_ptr CT, long domain_size,
                                                const EncryptedArray *ea) const;

    ResultType::Type_gamma add_key_to_gamma(const ResultType::Type_gamma &gamma,
                                            const std::vector<long> &keys,
                                            long domain_size,
                                            const EncryptedArray *ea) const;

private:
    context_ptr context = nullptr;
    PrivateContingencyTableHelper *helper = nullptr;
};

class PrivateContingencyTableHelper {
public:
    typedef typename PrivateContingencyTable::ResultType::Type_gamma Type_gamma;
    struct Publishable {
        size_t u, v;
        long hash_key;
    };

    PrivateContingencyTableHelper(const Attribute P, const Attribute Q,
                                 const long threshold, const EncryptedArray *ea)
    : P(P), Q(Q), threshold(threshold), ea(ea) {}

    virtual ~PrivateContingencyTableHelper() {}

    std::shared_ptr<Ctxt> repeat(size_t R) const;

    size_t how_many_copies(long domain_size) const;

    size_t repeats_per_cipher() const;

    size_t block_size() const;

    void setCT(const Ctxt *ctxt) { this->CT = ctxt; }

    Attribute getP() const { return P; }

    Attribute getQ() const { return Q; }

    long getThreshold() const { return threshold; }

    void open_gamma(std::vector<Publishable> &no_suppression,
                    const Type_gamma  &gamma,
                    const Type_gamma  &tilde_gamma,
                    const EncryptedArray *ea, sk_ptr sk);
private:
    Attribute P, Q;
    const long threshold;
    const Ctxt *CT = nullptr;
    const EncryptedArray *ea;
};
long CRT(long u, long v, long k1, long k2);
} // namespace core
#endif //CODA_CLION_CONTINGENCY_TABLE_HPP
