#include <list>
#include <fstream>

#include <HElib/Ctxt.h>
#include <spdlog/spdlog.h>
#include <core/global.hpp>
#include <core/literal.hpp>
#include <core/protocol.hpp>
#include <core/file_util.hpp>
#include <HElib/EncryptedArray.h>
#include <cinttypes>

namespace protocol {
namespace contingency {
const core::FHEArg _fheArgs = {.m = 5227, .p = 67499, .r = 1, .L = 5};

struct ProtocolHeader {
    std::string protocol;
    std::vector<int64_t> attrDomain;
};

struct __Plain {
    std::vector<int64_t> attributes;
};
typedef std::list<__Plain> __Body;

struct __Ctxt {
    Ctxt *c; // From HElib/Ctxt.h
    int64_t domain_product;
};

static void __ctxt_free(struct __Ctxt *ctxt) {
    if (ctxt && ctxt->c)
        delete ctxt->c;
}

struct __Comb {
    std::vector<int64_t> indices;
};
typedef std::vector<__Comb> __Combs;

static void __get_all_combination(__Combs &combs,
                                  __Comb &current,
                                  const ProtocolHeader &header,
                                  size_t pos) {
    assert(current.indices.size() == header.attrDomain.size() && "Same indices size");

    if (pos >= header.attrDomain.size()) {
        combs.push_back(current);
        return;
    }

    for (int64_t index = 0; index < header.attrDomain[pos]; index++) {
        current.indices.at(pos) = index;
        __get_all_combination(combs, current, header, pos + 1);
    }
}

static bool __equal(const __Comb &c, const __Plain &p) {
    if (c.indices.size() != p.attributes.size()) {
        L_DEBUG(global::_console, "Combination size {0}, body size {1}",
                c.indices.size(), p.attributes.size());
        return false;
    }

    for (size_t i = 0 ; i < c.indices.size(); i++) {
        if (c.indices[i] != p.attributes[i])
            return false;
    }

    return true;
}

/// TODO(riku) use faster algorithm
static NTL::ZZX __count_each_combination(const __Combs &combs,
                                         const __Body &body,
                                         const EncryptedArray &ea) {
    std::vector<long> slots(static_cast<size_t>(ea.size()), 0L);

    for (auto &p : body) {
        ssize_t index = 0;
        bool found = false;
        for (ssize_t i = 0; i < combs.size(); i++) {
            if (__equal(combs[i], p)) {
                found = true;
                index += i;
                break;
            }
        }

        if (!found) {
            L_WARN(global::_console, "Mismatch in body!");
            continue;
        }

        slots.at(index) += 1;
    }

    NTL::ZZX poly;
    ea.encode(poly, slots);
    return poly;
}

static __Ctxt __ctxt_create(const __Body &body,
                            const ProtocolHeader &header,
                            core::pk_ptr pk,
                            core::context_ptr context) {
    int64_t domain_product = 1;
    for (auto d : header.attrDomain)
        domain_product *= d;

    auto G = context->alMod.getFactorsOverZZ()[0];
    EncryptedArray ea(*context, G);
    if (ea.size() < domain_product) {
        L_WARN(global::_console, "Not enough slot {0} for {1}", ea.size(), domain_product);
        return {nullptr, 0};
    }

    __Combs combs;
    combs.reserve(domain_product);
    __Comb current = { .indices = std::vector<int64_t>(header.attrDomain.size()) };
    __get_all_combination(combs, current, header, 0);
    NTL::ZZX poly = __count_each_combination(combs, body, ea);

    Ctxt *ctxt = new Ctxt(*pk);
    pk->Encrypt(*ctxt, poly);
    return {ctxt, domain_product};
}

static bool __dumpCtxt(const __Ctxt &ctxt,
                       const std::string &outputDirPath) {
    auto outputFile = util::concatenate(outputDirPath, "FILE_1");
    std::fstream ostream(outputFile, std::ios::binary | std::ios::out);
    if (!ostream.is_open()) {
        L_WARN(global::_console, "Can not create {0}", outputFile);
        return false;
    }

    ostream << *ctxt.c;
    ostream << ctxt.domain_product;
    ostream.close();

    auto doneFile = util::concatenate(outputDirPath, global::_doneFileName);
    FILE *fd = fopen(doneFile.c_str(), "w+");
    if (!fd) {
        L_WARN(global::_console, "Can not create {0} under {1}",
               global::_doneFileName, outputFile);
        return false;
    }

    char buf[1024];
    snprintf(buf, sizeof(buf), "domain_product %" PRId64 "\n", ctxt.domain_product);
    size_t len = strlen(buf);
    if (fwrite(buf, 1UL, len, fd) != len) {
        L_WARN(global::_console, "Error happened when to write {0}", doneFile);
        fclose(fd);
        return false;
    }

    fclose(fd);
    return true;
}

static bool __parseHeader(ProtocolHeader *header, std::fstream &fin) {
    std::string line;
    std::getline(fin, line);
    if (line.compare("#protocol PROT_CON") != 0)
        return false;
    header->protocol = "PROT_CON";

    std::getline(fin, line);
    size_t pos = line.find("#meta ");
    if (pos != 0UL) {
        L_WARN(global::_console, "No #meta is set in the header");
        return false;
    }

    line = line.substr(5);
    auto fields = util::splitBySpace(line);
    for (auto &f : fields) {
        int64_t v = static_cast<int64_t>(literal::stol(f, &pos, 10));
        if (pos != f.size()) {
            L_WARN(global::_console, "Invalid meta field {0} in header", f);
            return false;
        }
        header->attrDomain.push_back(v);
    }

    return !header->attrDomain.empty();
}

static bool __parseBody(__Body &body, std::fstream &fin,
                        const ProtocolHeader *header) {
    if (!header) return false;
    if (!fin.is_open()) {
        L_WARN(global::_console, "The file handler is ready closed.");
        return false;
    }

    for (std::string line; std::getline(fin, line); ) {
        auto values = util::splitBySpace(line);
        if (values.size() != header->attrDomain.size()) {
            L_WARN(global::_console, "The number of attributes in the file body mismatches the header");
            return false;
        }

        __Plain plain;
        for (size_t i = 0; i < header->attrDomain.size(); i++) {
            size_t pos;
            int64_t v = static_cast<int64_t>(literal::stol(values[i], &pos, 10));
            if (pos != values[i].size()) {
                L_WARN(global::_console, "Invalid value {0} in the file body", values[i]);
                return false;
            }

            if (v < 0 || v >= header->attrDomain[i]) {
                L_WARN(global::_console, "Invalid value {0} ≥ {1} in the file body",
                       v, header->attrDomain[i]);
                return false;
            }

            plain.attributes.push_back(v);
        }
        body.push_back(plain);
    }

    return true;
}

bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             core::pk_ptr pk,
             core::context_ptr context) {
    std::fstream fin(inputFilePath);
    if (!fin.is_open()) {
        L_WARN(global::_console, "Can not open file {0}", inputFilePath);
        return false;
    }

    ProtocolHeader header;
    if (!__parseHeader(&header, fin)) {
        L_ERROR(global::_console, "Error happened in parsing the header of {0}",
                inputFilePath);
        return false;
    }

    __Body body;
    if (!__parseBody(body, fin, &header)) {
        L_ERROR(global::_console, "Error happened in parsing the body of {0}",
                inputFilePath);
        return false;
    }

    int64_t domain_product = 1;
    for (auto d : header.attrDomain)
        domain_product *= d;

    auto _ctxt = __ctxt_create(body, header, pk, context);
    return __dumpCtxt(_ctxt, outputDirPath);
    //return __encrypt(fin, outputDirPath, header, pk);
}

bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              core::pk_ptr pk) {
    L_INFO(global::_console, "Not implemented yet");
    return false;
}

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk,
             core::context_ptr context) {
    L_INFO(global::_console, "Not implemented yet");
    return false;
}
} // namespace contingency
} // namespace protocol
