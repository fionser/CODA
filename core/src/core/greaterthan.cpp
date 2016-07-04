//
// Created by riku on 2016/07/04.
//

#include <core/greaterthan.h>
#include <spdlog/spdlog.h>
#include <core/global.hpp>
#include <HElib/Ctxt.h>
#include <HElib/EncryptedArray.h>

greaterthan::ResultCiphers
greaterthan::compare(const Ctxt *x, const Ctxt *y) const {
    if (!x || !y || !context) {
        L_ERROR(global::_console, "Invalid arguments for greaterthan::compare");
        return std::vector<ctxt_ptr>();
    }

    long nr_slots = context->ea->size();
    printf("NR slots %ld\n", nr_slots);
    return std::vector<ctxt_ptr>();
}

greaterthan::Result
greaterthan::open_result(const greaterthan::ResultCiphers &ciphers, core::sk_ptr sk) const {
    if (ciphers.empty() || !context || !sk) {
        L_ERROR(global::_console, "Invalid arguments for greaterthan::open_result");
        return Result::ERROR;
    }

    std::vector<long> slots;
    for (auto c : ciphers) {
        context->ea->decrypt(*c, *sk, slots);
        for (auto s : slots) {
            if (s == 0) return Result::GREATER;
        }
    }

    return Result::LESS_OR_EQUAL;
}
