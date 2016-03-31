#ifndef core_PROTOCOL_PROTOCOL_H
#define core_PROTOCOL_PROTOCOL_H
#include "coda.hpp"
#include <vector>
namespace protocol {
bool genKeypair(core::Protocol protocol,
                std::fstream &skStream,
                std::fstream &ctxtStream,
                std::fstream &pkStream);

namespace chi2 {
extern const core::FHEArg _fheArgs;
extern std::string _genotype_data;
extern std::string _phenotype_data;

bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             core::pk_ptr pk);

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk);

bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              core::pk_ptr pk);

} // namespace chi2

namespace contingency {
extern const core::FHEArg _fheArgs;

bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             core::pk_ptr pk,
             core::context_ptr context);

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk,
             core::context_ptr context);

bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              core::pk_ptr pk);
} // contingency
} // namespace protocol
#endif // core_PROTOCOL_PROTOCOL_H
