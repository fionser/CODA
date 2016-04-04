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
    int64_t domain_product;
    std::string protocol;
    std::vector<int64_t> attrDomain;
};

struct __Plain {
    int64_t uid;
    std::vector<int64_t> attributes;
};
typedef std::list<__Plain> __Body;

struct __Ctxt {
    std::vector<Ctxt *> c; // From HElib/Ctxt.h
    int64_t uid;
    size_t used; // show how many valid Ctxt
};

static void __ctxt_free(struct __Ctxt *ctxt) {
    if (!ctxt) return;
    for (Ctxt *c : ctxt->c)
        if (c) delete c;
}

static void __ctxt_create(__Ctxt *_ctxt,
                          const __Plain &data,
                          const ProtocolHeader &header,
                          const EncryptedArray &ea,
                          core::pk_ptr pk) {
    if (!_ctxt) {
        L_DEBUG(global::_console, "Null pointer of _ctxt in __ctxt_create");
        return;
    }

    size_t sze = _ctxt->c.size();
    for (size_t i = sze; i < data.attributes.size(); i++)
        _ctxt->c.push_back(new Ctxt(*pk));

    std::vector<long> coeff(static_cast<size_t>(ea.size()), 0);
    for (size_t i = 0; i < data.attributes.size(); i++) {
        coeff.at(data.attributes.at(i)) = 1;
        ea.encrypt(*_ctxt->c.at(i), *pk, coeff);
        coeff.at(data.attributes.at(i)) = 0;
    }

    _ctxt->uid = data.uid;
    _ctxt->used = data.attributes.size();
}

static bool __createDoneFile(const std::string &outputDirPath) {
    auto doneFile = util::concatenate(outputDirPath, global::_doneFileName);
    FILE *fd = fopen(doneFile.c_str(), "w+");
    if (!fd) {
        L_WARN(global::_console, "Can not create {0} under {1}",
               global::_doneFileName, outputDirPath);
        return false;
    }

    char buf[1024];
    snprintf(buf, sizeof(buf), "DONE");
    size_t len = strlen(buf);
    if (fwrite(buf, 1UL, len, fd) != len) {
        L_WARN(global::_console, "Error happened when to write {0}", doneFile);
        fclose(fd);
        return false;
    }

    fclose(fd);
    return true;
}

static bool __dumpCtxt(const __Ctxt &ctxt,
                       const ProtocolHeader &header,
                       const std::string &outputDirPath) {
    char buf[1024];
    snprintf(buf, sizeof(buf), "FILE_%" PRId64, ctxt.uid);
    auto outputFile = util::concatenate(outputDirPath, buf);
    std::fstream ostream(outputFile, std::ios::binary | std::ios::out);
    if (!ostream.is_open()) {
        L_WARN(global::_console, "Can not create {0}", outputFile);
        return false;
    }

    ostream << ctxt.uid;
    for (auto D : header.attrDomain)
        ostream << " " << D;
    ostream << std::endl;

    for (size_t i = 0; i < ctxt.used; i++) {
        ostream << *ctxt.c.at(i);
        if (i + 1 < ctxt.used)
            ostream << std::endl;
    }
    ostream.close();

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
    int64_t dp = 1;
    for (auto &f : fields) {
        int64_t v = static_cast<int64_t>(literal::stol(f, &pos, 10));
        if (pos != f.size()) {
            L_WARN(global::_console, "Invalid meta field {0} in header", f);
            return false;
        }
        header->attrDomain.push_back(v);
        dp *= v;
    }
    header->domain_product = dp;

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
        /// the first field is uid
        if (values.size() != header->attrDomain.size() + 1) {
            L_WARN(global::_console, "The number of attributes in the file body mismatches the header");
            return false;
        }

        __Plain plain;
        size_t pos;
        int64_t v = static_cast<int64_t>(literal::stol(values[0], &pos, 10));
        if (pos != values[0].size()) {
            L_WARN(global::_console, "Invalid uid {0} in the file body",
                   values[0]);
            return false;
        }
        plain.uid = v;

        for (size_t i = 0; i < header->attrDomain.size(); i++) {
            v = static_cast<int64_t>(literal::stol(values[i + 1], &pos, 10));
            if (pos != values[i + 1].size()) {
                L_WARN(global::_console, "Invalid value {0} in the file body",
                       values[i + 1]);
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

    auto G = context->alMod.getFactorsOverZZ()[0];
    EncryptedArray ea(*context, G);

    __Ctxt _ctxt;
    for (auto &plain : body) {
        __ctxt_create(&_ctxt, plain, header, ea, pk);
        __dumpCtxt(_ctxt, header, outputDirPath);
    }

    __ctxt_free(&_ctxt);
    return __createDoneFile(outputDirPath);
}


} // namespace contingency
} // namespace protocol
