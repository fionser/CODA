//
// Created by riku on 2016/07/19.
//

#ifndef CODA_CLION_CONTINGENCY_TABLE_HPP
#define CODA_CLION_CONTINGENCY_TABLE_HPP
#include <vector>
#include <string>
#include <memory>
#include "core/coda.hpp"
#include "HElib/Ctxt.h"
#include "core/greaterthan.h"

namespace NTL { class ZZX; }
class EncryptedArray;

namespace core {
struct Attribute {
    enum class Type {
        NUMERICAL = 0,
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
        typedef ctxt_ptr Type_n_uv;
        typedef std::vector<ctxt_ptr> Type_gamma;
        typedef Type_gamma Type_tilde_gamma;
        Type_n_uv n_uv;
        Type_gamma gamma;
        Type_tilde_gamma tilde_gamma;
    };

    PrivateContingencyTable(context_ptr context,
                            PrivateContingencyTableHelper *helper);

    ~PrivateContingencyTable() {}

    ResultType evaluate(const std::vector<Ctxt> &attributes) const;

    ResultType evaluate(const ctxt_ptr &ct, long nr_records) const;
private:
    ctxt_ptr compute_table(const std::vector<Ctxt> &attributes,
                           Attribute p, Attribute q,
                           const EncryptedArray *ea) const;

    std::vector<long> sample_blinding_factor(Attribute p, Attribute q,
                                             const EncryptedArray *ea) const;

    Ctxt do_compute(const Ctxt &one_record, const Attribute &p,
                    const Attribute &q, const EncryptedArray *ea) const;

    ResultType::Type_gamma special_greater_than(ctxt_ptr CT, long domain_size,
                                                const EncryptedArray *ea) const;

    ResultType::Type_tilde_gamma blind_GT(const ResultType::Type_gamma &gamma,
                                          const std::vector<long> &blind_factor,
                                          long domain_size,
                                          const EncryptedArray *ea) const;

    ResultType::Type_n_uv blind_table(const ctxt_ptr &ct,
                                      const std::vector<long> &blind_factor,
                                      const EncryptedArray *ea) const;

private:
    context_ptr context = nullptr;
    PrivateContingencyTableHelper *helper = nullptr;
};

class PrivateContingencyTableHelper {
public:
    typedef typename PrivateContingencyTable::ResultType::Type_n_uv Type_n_uv;
    typedef typename PrivateContingencyTable::ResultType::Type_gamma Type_gamma;
    typedef typename PrivateContingencyTable::ResultType::Type_tilde_gamma Type_tilde_gamma;
    typedef std::vector<std::vector<long>> CTable_t;
    struct Publishable {
        size_t u, v; // (u, v)
        long position; // unzero position in ciphertext
        long blinding_factor;
    };

    PrivateContingencyTableHelper(const Attribute P,
                                  const Attribute Q,
                                  const long threshold,
                                  const EncryptedArray *ea)
            : P(P), Q(Q), threshold(threshold), ea(ea) {}

    virtual ~PrivateContingencyTableHelper() {}

    bool dump(const PrivateContingencyTable::ResultType &ct_result,
              const std::string &file_path) const;

    bool restore(PrivateContingencyTable::ResultType &out,
                 const std::string &result_file,
                 const core::pk_ptr &pk) const;

    std::shared_ptr<Ctxt> repeat(size_t R) const;

    size_t how_many_copies(long domain_size) const;

    size_t repeats_per_cipher() const;

    size_t block_size() const;

    void setCT(const Ctxt *ctxt) { this->CT = ctxt; }

    Attribute getP() const { return P; }

    Attribute getQ() const { return Q; }

    long getThreshold() const { return threshold;}

    void open_gamma(std::vector<Publishable> &no_suppression,
                    const Type_gamma  &gamma,
                    const Type_tilde_gamma  &tilde_gamma,
                    const EncryptedArray *ea, sk_ptr sk) const;

    CTable_t final_decrypt(const Type_n_uv &cells,
                           const std::vector<Publishable> &publishable,
                           const sk_ptr sk,
                           const EncryptedArray *ea) const;
private:
    std::vector<long> get_zero_positions(const ctxt_ptr &ctxt,
                                         const sk_ptr &sk,
                                         const EncryptedArray *ea) const;

    std::vector<long> get_blinding_factor(const Type_tilde_gamma &tilde_gamma,
                                          long part,
                                          const std::vector<long> &position,
                                          const EncryptedArray *ea,
                                          sk_ptr sk) const;
private:
    Attribute P, Q;
    const long threshold;
    const Ctxt *CT = nullptr;
    const EncryptedArray *ea;
};

long CRT(long u, long v, long k1, long k2);

std::pair<size_t, size_t> coprime(size_t i, size_t j);

} // namespace core
#endif //CODA_CLION_CONTINGENCY_TABLE_HPP
