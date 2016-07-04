//
// Created by riku on 2016/07/04.
//

#ifndef CODA_CLION_GREATERTHAN_H
#define CODA_CLION_GREATERTHAN_H
#include <core/coda.hpp>
#include <vector>
class Ctxt;
typedef std::shared_ptr<Ctxt> ctxt_ptr;
/// The greater-than protocol
/// We need to specify the domain size
class greaterthan {
public:
    typedef std::vector<ctxt_ptr> ResultCiphers;
    enum class Result {
        GREATER,
        LESS_OR_EQUAL,
        ERROR
    };

    greaterthan(long domain_size, core::context_ptr context)
    : context(context), domain_size(domain_size) {}

    ~greaterthan() {}

    ResultCiphers compare(const Ctxt *x, const Ctxt *y) const;

    Result open_result(const ResultCiphers &, core::sk_ptr) const;

private:
    core::context_ptr context = nullptr;
    long domain_size = -1;
};


#endif //CODA_CLION_GREATERTHAN_H
