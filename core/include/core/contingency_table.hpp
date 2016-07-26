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
};

class ContingencyTableHelper {
public:
    ContingencyTableHelper() {}
    virtual ~ContingencyTableHelper() {}

    virtual size_t nr_attributes() const = 0;
    virtual Attribute get_attribute(size_t index) const = 0;
    virtual size_t offset_of(size_t index) const = 0;
};

class PrivateContingencyTable {
public:
    struct ResultType {
        typedef std::vector<std::shared_ptr<Ctxt>> Type_n_uv;
        Type_n_uv n_uv;
        typedef std::vector<greaterthan::ResultType> Type_gamma;
        Type_gamma gamma;
        Type_gamma gamma_tilde;
    };

    PrivateContingencyTable(long T, context_ptr context);
    ~PrivateContingencyTable() {}
    ResultType evaluate(const std::vector<Ctxt> &attributes,
                        const ContingencyTableHelper *helper,
                        size_t size_of_attr_p,
                        size_t size_of_attr_q) const;
private:
    long threshold = 0;
    context_ptr context = nullptr;
};

long CRT(long u, long v, long k1, long k2);
} // namespace core
#endif //CODA_CLION_CONTINGENCY_TABLE_HPP
