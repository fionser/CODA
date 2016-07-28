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
#include "AES.hpp"
#include "greaterthan.h"

namespace NTL { class ZZX; }

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
        typedef std::vector<AES128::Ctxt> Type_n_uv;
        typedef std::vector<ctxt_ptr> Type_gamma;
        typedef std::vector<Type_gamma> Type_tilde_gamma;
        Type_n_uv n_uv;
        Type_gamma gamma;
        Type_tilde_gamma tilde_gamma;
    };
    typedef std::vector<long> AESKey_t;

    PrivateContingencyTable(context_ptr context,
                            PrivateContingencyTableHelper *helper);

    ~PrivateContingencyTable() {}

    ResultType evaluate(const std::vector<Ctxt> &attributes) const;

private:
    ctxt_ptr compute_table(const std::vector<Ctxt> &attributes,
                           Attribute p, Attribute q,
                           const EncryptedArray *ea) const;

    Ctxt do_compute(const Ctxt &one_record, const Attribute &p,
                    const Attribute &q, const EncryptedArray *ea) const;

    std::vector<ctxt_ptr> extract_cells_in_table(const ctxt_ptr& CT,
                                                 Attribute p, Attribute q,
                                                 const EncryptedArray *ea) const;

    ResultType::Type_gamma special_greater_than(ctxt_ptr CT, long domain_size,
                                                const EncryptedArray *ea) const;

    ResultType::Type_tilde_gamma add_key_to_gamma(const ResultType::Type_gamma &gamma,
                                                  const std::vector<AESKey_t> &keys,
                                                  long domain_size,
                                                  const EncryptedArray *ea) const;

    ResultType::Type_n_uv aes_encrypt_cells(const std::vector<ctxt_ptr> &cells,
                                            const std::vector<AESKey_t> &keys,
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
    struct Publishable {
        size_t u, v;
        NTL::ZZ aes_key;
    };

    PrivateContingencyTableHelper(const Attribute P,
                                  const Attribute Q,
                                  const long threshold,
                                  const EncryptedArray *ea)
            : P(P), Q(Q), threshold(threshold), ea(ea), key_bits(128L) {}

    virtual ~PrivateContingencyTableHelper() {}

    std::shared_ptr<Ctxt> repeat(size_t R) const;

    size_t how_many_copies(long domain_size) const;

    size_t how_many_copies_for_bits(const EncryptedArray *ea) const;

    size_t repeats_per_cipher() const;

    size_t block_size() const;

    void setCT(const Ctxt *ctxt) {this->CT = ctxt; }

    void setKeyLength(long s) {key_bits = s;}

    Attribute getP() const { return P;}

    Attribute getQ() const { return Q;}

    long getThreshold() const { return threshold;}

    long aesKeyLength() const { return key_bits;}

    void open_gamma(std::vector<Publishable> &no_suppression,
                    const Type_gamma  &gamma,
                    const Type_tilde_gamma  &tilde_gamma,
                    const EncryptedArray *ea, sk_ptr sk) const;

    std::vector<long> final_decrypt(const Type_n_uv &cells,
                                    const std::vector<Publishable> &publishable,
                                    const sk_ptr sk,
                                    const EncryptedArray *ea) const;
private:
    std::vector<PrivateContingencyTable::AESKey_t>
        decryptToGetAESKeys(const Type_tilde_gamma &tilde_gamma,
                            const size_t loc,
                            const std::vector<size_t> &zeros,
                            const EncryptedArray *ea, sk_ptr sk) const;

private:
    Attribute P, Q;
    const long threshold;
    long key_bits;
    const Ctxt *CT = nullptr;
    const EncryptedArray *ea;
};

long CRT(long u, long v, long k1, long k2);

std::pair<size_t, size_t> coprime(size_t i, size_t j);


PrivateContingencyTable::AESKey_t convKey(const NTL::ZZ &zz, long bit_per, long partition);

NTL::ZZX convKey(const std::vector<PrivateContingencyTable::AESKey_t> &keys, long p, const EncryptedArray *ea);

NTL::ZZ convKey(PrivateContingencyTable::AESKey_t aes, long bit_per);
} // namespace core
#endif //CODA_CLION_CONTINGENCY_TABLE_HPP
