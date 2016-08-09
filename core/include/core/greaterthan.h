//
// Created by riku on 2016/07/04.
//

#ifndef CODA_CLION_GREATERTHAN_H
#define CODA_CLION_GREATERTHAN_H
#include "coda.hpp"
#include <vector>
#include <NTL/ZZX.h>
#include <iosfwd>
class Ctxt;
typedef std::shared_ptr<Ctxt> ctxt_ptr;
/// The greater-than protocol
/// We need to specify the domain size
class greaterthan {
public:
    typedef std::vector<ctxt_ptr> ResultType;
    enum class Result {
        GREATER_OR_EQUAL,
        LESS,
        ERROR
    };

    struct ZeroPosition {
        size_t index;
        size_t pos;
        bool valid;
    };

    greaterthan(long domain_size, core::context_ptr context)
    : context(context), domain_size(domain_size) {}

    ~greaterthan() {}

    ResultType compare(const Ctxt *x, const Ctxt *y) const;

    ResultType compare(const Ctxt *x, const NTL::ZZX &y) const;

    Result open_result(const ResultType &, core::sk_ptr) const;

    Result open_result(const ResultType &, core::sk_ptr, ZeroPosition *pos) const;

private:
    core::context_ptr context = nullptr;
    long domain_size = -1;
};

std::ostream& operator<<(std::ostream &out, greaterthan::Result r);

greaterthan::ResultType copy(const greaterthan::ResultType &oth);
#endif //CODA_CLION_GREATERTHAN_H
