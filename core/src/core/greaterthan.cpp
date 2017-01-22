//
// Created by riku on 2016/07/04.
//

#include <core/greaterthan.h>
#include "core/ctxt_util.hpp"
#include <spdlog/spdlog.h>
#include <core/global.hpp>
#include <HElib/Ctxt.h>
#include "../../include/HElib/FHEContext.h"
#include <HElib/EncryptedArray.h>

greaterthan::ResultType
greaterthan::compare(const Ctxt *x, const Ctxt *y) const {
    if (!x || !y || !context) {
        L_ERROR(global::_console, "Invalid arguments for greaterthan::compare");
        return std::vector<ctxt_ptr>();
    }

    long nr_slots = context->ea->size();
    auto parts = core::random_permutation(this->domain_size, *context->ea);
    auto randomness = core::randomness(this->domain_size, *context->ea);
    std::vector<ctxt_ptr> ctxt_parts(parts.size());
    auto y_x(*y);
    y_x.addCtxt(*x, true);
    // y - x + i, then if 0 exists, <=> x >= y.
    for (size_t i = 0; i < parts.size(); i++) {
        ctxt_parts.at(i) = std::make_shared<Ctxt>(y_x);
        NTL::ZZX poly;
        context->ea->encode(poly, parts.at(i));
        ctxt_parts.at(i)->addConstant(poly);
        context->ea->encode(poly, randomness.at(i));
        ctxt_parts.at(i)->multByConstant(poly);
    }
    return ctxt_parts;
}

greaterthan::ResultType
greaterthan::compare(const Ctxt *x, const NTL::ZZX &y) const {
    if (!x || !context) {
        L_ERROR(global::_console, "Invalid arguments for greaterthan::compare");
        return std::vector<ctxt_ptr>();
    }

    long nr_slots = context->ea->size();
    auto parts = core::random_permutation(this->domain_size, *context->ea);
    auto randomness = core::randomness(this->domain_size, *context->ea);
    std::vector<ctxt_ptr> ctxt_parts(parts.size());
    auto y_x(*x); y_x.negate(); // -x
    y_x.addConstant(y, true); // y - x
    // y - x + i, then if 0 exists, <=> x >= y.
    for (size_t i = 0; i < parts.size(); i++) {
        ctxt_parts.at(i) = std::make_shared<Ctxt>(y_x);
        NTL::ZZX poly;
        context->ea->encode(poly, parts.at(i));
        ctxt_parts.at(i)->addConstant(poly);
        context->ea->encode(poly, randomness.at(i));
        ctxt_parts.at(i)->multByConstant(poly);
    }
    return ctxt_parts;
}

greaterthan::ResultType
greaterthan::compare(const Ctxt *x, const long y) const {
    std::vector<long> slots(context->ea->size(), 0);
    for (int i = 0; i < domain_size && i < slots.size(); i++)
        slots[i] = y;
    NTL::ZZX poly;
    context->ea->encode(poly, slots);
    return compare(x, poly);
}

greaterthan::Result
greaterthan::open_result(const greaterthan::ResultType &ciphers, core::sk_ptr sk) const {
    return open_result(ciphers, sk, nullptr);
}

greaterthan::Result
greaterthan::open_result(const greaterthan::ResultType &ciphers, core::sk_ptr sk,
                         greaterthan::ZeroPosition *zp) const {
    if ( !context || !sk) {
        L_ERROR(global::_console, "Invalid arguments for greaterthan::open_result");
        if (zp) zp->valid = false;
        return Result::ERROR;
    }
    if (ciphers.empty()) {
        L_ERROR(global::_console, "empty ciphertexts!");
        if (zp) zp->valid = false;
        return Result::ERROR;
    }

    size_t nr_parts = ciphers.size();
    long nr_slots = context->ea->size();
    for (size_t idx = 0; idx < nr_parts; idx++) {
        std::vector<long> slots;
        context->ea->decrypt(*ciphers.at(idx), *sk, slots);
        for (long pos = 0; pos < nr_slots; pos++) {
            if (slots.at(pos) == 0) {
                if (zp) {
                    zp->index = idx;
                    zp->pos = pos;
                    zp->valid = true;
                }
                return Result::GREATER_OR_EQUAL;
            }
        }
    }

    if (zp) zp->valid = false;
    return Result::LESS;
}

std::ostream& operator<<(std::ostream&out, greaterthan::Result r) {
    switch (r) {
    case greaterthan::Result::LESS:
        out << "< "; return out;
    case greaterthan::Result::GREATER_OR_EQUAL:
        out << ">="; return out;
    default:
        out << "error"; return out;
    }
}

greaterthan::ResultType copy(const greaterthan::ResultType &oth) {
    greaterthan::ResultType copied(oth.size());
    for (size_t i = 0; i < oth.size(); i++)
        copied.at(i) = std::make_shared<Ctxt>(*oth.at(i));
    return copied;
}
