//
// Created by riku on 4/1/16.
//
#include <list>
#include <unordered_map>

#include <HElib/Ctxt.h>
#include <spdlog/spdlog.h>
#include <core/global.hpp>
#include <core/protocol.hpp>
#include <core/file_util.hpp>
#include <HElib/EncryptedArray.h>
#include <core/literal.hpp>
#include <cinttypes>
#include <core/core.hpp>

namespace protocol {
namespace contingency {
struct UserEncData {
    std::unordered_map<int64_t, std::vector<Ctxt *>> _data;
};

static void __uData_init(UserEncData *uData) {
    if (!uData) return;
}

static void __uData_free(UserEncData *uData) {
    if (!uData) return;
    for (auto &kv : uData->_data) {
        for (auto c : kv.second)
            if (c) delete c;
    }
}

static void __uData_add(UserEncData *uData,
                        int64_t uid,
                        const std::vector<Ctxt *> &ciphers) {
    if (!uData) return;
    uData->_data.insert({uid, ciphers});
}

static Ctxt *__uData_get(const UserEncData *uData, int64_t uid) {
    if (!uData) return nullptr;
    auto kv = uData->_data.find(uid);
    if (kv == uData->_data.end())
        return nullptr;
    if (kv->second.size() != 1) {
        L_DEBUG(global::_console, "No unique in __uData_get")
        return nullptr;
    }
    return kv->second.front();
}

static size_t __uData_size(UserEncData *uData) {
    if (!uData) return 0UL;
    return uData->_data.size();
}

static std::vector<int64_t> __uData_key(const UserEncData *uData) {
    std::vector<int64_t> ret;
    if (!uData) return ret;
    for (auto &kv : uData->_data)
        ret.push_back(kv.first);
    return ret;
}

class UserEncDataLoader {
public:
    UserEncDataLoader() { }

    ~UserEncDataLoader() { }

    bool loadCiphers(const std::string &dir,
                     core::pk_ptr pk,
                     UserEncData &out);

private:
    bool loadOneUser(const std::string &file,
                     core::pk_ptr pk,
                     UserEncData &out);
};

bool UserEncDataLoader::loadCiphers(const std::string &dir,
                                    core::pk_ptr pk,
                                    UserEncData &out) {
    auto files = util::listDir(dir, util::flag_t::FILE_ONLY);

    for (auto &f : files) {
        if (f.compare(global::_doneFileName) == 0)
            continue;
        if (!loadOneUser(util::concatenate(dir, f), pk, out))
            return false;
    }

    return true;
}

bool UserEncDataLoader::loadOneUser(const std::string &file,
                                    core::pk_ptr pk,
                                    UserEncData &out) {
    std::fstream istream(file, std::ios::binary | std::ios::in);
    if (!istream.is_open()) {
        L_WARN(global::_console, "Can not open {0}", file);
        return false;
    }
    /// parse the header of the ciphertext file.
    std::string line;
    if (!getline(istream, line)) {
        L_WARN(global::_console, "Error happened when read {0}", file);
        return false;
    }
    auto fields = util::splitBySpace(line);
    std::vector<int64_t> uid_and_domains;
    size_t pos;
    for (auto &f : fields) {
        int64_t v = literal::stol(f, &pos, 10);
        if (pos != f.size()) {
            L_WARN(global::_console, "Invalid field {0} in file {1}", f, file);
            return false;
        }
        uid_and_domains.push_back(v);
    }

    if (uid_and_domains.size() <= 1) {
        L_WARN(global::_console, "Not enough information in header of {0}",
               file);
        return false;
    }
    /// read Ctxts
    int64_t uid = uid_and_domains.front();
    size_t nr_attributes = uid_and_domains.size() - 1;
    std::vector<Ctxt *> ciphers(nr_attributes);
    for (size_t i = 0; i < nr_attributes; i++) {
        ciphers.at(i) = new Ctxt(*pk);
        istream >> *(ciphers[i]);
    }
    __uData_add(&out, uid, ciphers);

    istream.close();
    return true;
}

struct Selection {
    size_t AttrIdx;
    size_t AttrVal;
};

struct EvalArgs {
    /// one std::vector<size_t> for the selection of one data contributor.
    std::vector<std::vector<Selection>> selections;
};

static void __shift(Ctxt *res,
                    const Ctxt *a,
                    size_t shift,
                    const EncryptedArray &ea) {
    if (!res || !a) return;
    *res = *a;
    ea.rotate(*res, static_cast<long>(shift));
}

static void __merge(UserEncData *merged,
                    const UserEncData u,
                    const std::vector<Selection> &slt,
                    core::pk_ptr pk,
                    const EncryptedArray &ea) {
    if (!merged) return;

    Ctxt *tmp = new Ctxt(*pk);
    Ctxt *res = nullptr;
    for (auto &kv : u._data) {
        res = new Ctxt(*pk);
        bool first = true;
        for (const Selection &s : slt) {
            Ctxt *attr = kv.second.at(s.AttrIdx);
            __shift(tmp, attr, s.AttrVal, ea);
            if (!first) {
                res->operator+=(*tmp);
            } else {
                res->operator=(*tmp);
                first = false;
            }
        }
        __uData_add(merged, kv.first, std::vector<Ctxt *>(1, res));
    }

    delete tmp;
}

bool __mul(Ctxt *res, int64_t uid, const std::vector<UserEncData> &users) {
    if (!res || users.empty()) return false;

    const UserEncData *u = &(users.front());
    Ctxt *op = __uData_get(u, uid);
    if (!op) {
        L_DEBUG(global::_console, "Can not find {0}", uid);
        return false;
    }

    res->operator=(*op);
    for (size_t i = 1; i < users.size(); i++) {
        u = &(users.at(i));
        op = __uData_get(u, uid);
        if (!op) {
            L_DEBUG(global::_console, "Can not find {0}", uid);
            return false;
        }
        res->operator*=(*op);
    }
    res->reLinearize();
    return true;
}

Ctxt *__evaluate(std::vector<UserEncData> &users,
                 const EvalArgs args,
                 core::pk_ptr pk,
                 core::context_ptr context) {
    assert(args.selections.size() == users.size()
           &&
           "The number of users in data mismatched the number of users in selection");

    auto G = context->alMod.getFactorsOverZZ()[0];
    EncryptedArray ea(*context, G);
    std::vector<UserEncData> mergedUser(users.size());

    for (size_t i = 0; i < users.size(); i++) {
        __merge(&mergedUser.at(i), users.at(i),
                args.selections.at(i), pk, ea);
    }

    auto keys = __uData_key(&(users.front()));
    Ctxt *res = nullptr;
    Ctxt *multed = new Ctxt(*pk);
    for (auto k : keys) {
        if (!__mul(multed, k, mergedUser)) {
            L_WARN(global::_console, "Went wrong in __mul");
            return nullptr;
        }

        if (res)
            res->operator+=(*multed);
        else
            res = new Ctxt(*multed);
    }

    for (auto &uD : mergedUser)
        __uData_free(&uD);
    delete multed;
    return res;
}

/// @TODO(riku) to add EvalArgs as one of the parameter
bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              core::pk_ptr pk,
              core::context_ptr context) {
    std::vector<UserEncData> uDatas(inputDirs.size());
    UserEncDataLoader loader;
    for (size_t i = 0; i < inputDirs.size(); i++) {
        __uData_init(&uDatas.at(i));
        loader.loadCiphers(inputDirs.at(i), pk, uDatas[i]);
        L_INFO(global::_console, "Loaded {0} files from {1}",
               __uData_size(&uDatas[i]), inputDirs[i]);
    }
    /// tentativly set the selections
    EvalArgs args;
    Selection selection = {.AttrIdx = 0, .AttrVal = 0};
    args.selections.resize(inputDirs.size(),
                           std::vector<Selection>(1, selection));
    //args.selections[1].front().AttrIdx = 1;
    Ctxt *res = __evaluate(uDatas, args, pk, context);
    bool ok;
    if (res) {
        auto resultFile = util::concatenate(outputDir, "File_result");
        std::fstream ostream(resultFile, std::ios::binary | std::ios::out);
        if (!ostream.is_open()) {
            L_WARN(global::_console, "Can not open {0}", resultFile);
            ok = false;
        } else {
            ostream << *res;
            ostream.close();
            auto doneFile = util::concatenate(outputDir, global::_doneFileName);
            auto fd = util::createDoneFile(doneFile);
            if (fd) {
                fwrite("DONE\n", 1UL, 5UL, fd);
                ok = fclose(fd) == 0;
            } else {
                L_WARN(global::_console, "Can not create {0}", doneFile);
                ok = false;
            }
        }
        delete res;
    } else {
        L_DEBUG(global::_console, "Something went wrong in __evaluate");
        ok = false;
    }

    for (auto &uData : uDatas)
        __uData_free(&uData);
    return ok;
}

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk,
             core::context_ptr context) {
    auto G = context->alMod.getFactorsOverZZ()[0];
    EncryptedArray ea(*context, G);
    std::list<Ctxt> ciphers;
    core::loadCiphers(ciphers, pk, inputFilePath);
    assert(ciphers.size() == 1);
    std::vector<long> slots(ea.size());
    ea.decrypt(ciphers.front(), *sk, slots);
    printf("%ld\n", slots[0]);
    return true;
}

} // namespace contingency
} // namespace protocol
