#ifndef CORE_CORE_H
#define CORE_CORE_H
#include "coda.hpp"

#include <vector>
#include <list>
#include <memory>

namespace core {
context_ptr loadContext(bool *ok, const std::string &contextFile);

pk_ptr loadPK(bool *ok, const context_ptr &context, const std::string &pkFile);

sk_ptr loadSK(bool *ok, const context_ptr &context, const std::string &skFile);

bool loadCiphers(std::vector<Ctxt>& out, const pk_ptr &pk, const std::string &file);

bool loadCiphers(std::list<Ctxt>& out, const pk_ptr &pk, const std::string &file);

bool dumpCiphers(const std::list<Ctxt>& ciphers, const std::string &file);

bool dumpCiphers(const std::vector<Ctxt *>& ciphers, const std::string &file);

bool genKeypair(const std::string &metaFilePath);

bool encrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             const std::string &metaFilePath,
             bool local_compute);

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             const std::string &metaFilePath);

bool evaluate(const std::string &sessionDirPath,
              const std::string &outputDirPath,
              const std::string &metaFilePath,
              const std::vector<std::string> &params);
} // namespace core
#endif // CORE_CORE_H
