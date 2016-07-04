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

/// @param inputFilePath The raw data file.
/// @param outputDirPath The directory to place the cipher file(s).
/// @param pk The public encryption key.
bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             core::pk_ptr pk);

/// @param inputFilePath One cipher file.
/// @param outputFilePath The directory to place the result file.
/// @param pk The public encryption key.
/// @param sk The secret decryption key.
bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk);

/// @param inputDirs The collection of users' data directory.
///                  @see bool encrypt(const std::string &, const std::string &, core::pk_ptr).
/// @param outputDir The directory to place the evaluation result.
/// @param pk The public encryption key.
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
              core::pk_ptr pk,
              core::context_ptr context);
} // contingency

namespace mean {
extern const core::FHEArg _fheArgs;

/// @param inputFilePath The raw data file.
/// @param outputDirPath The directory to place the cipher file(s).
/// @param pk The public encryption key.
bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             core::pk_ptr pk,
             core::context_ptr context);

/// @param inputFilePath One cipher file.
/// @param outputFilePath The directory to place the result file.
/// @param pk The public encryption key.
/// @param sk The secret decryption key.
bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk,
             core::context_ptr context);

/// @param inputDirs The collection of users' data directory.
///                  @see bool encrypt(const std::string &, const std::string &, core::pk_ptr).
/// @param outputDir The directory to place the evaluation result.
/// @param pk The public encryption key.
bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              core::pk_ptr pk,
              core::context_ptr context);

} // namespace mean

namespace rank {
extern const core::FHEArg _fheArgs;

/// @param inputFilePath The raw data file.
/// @param outputD
/// @param pk The public encryption key.
bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             core::pk_ptr pk);

/// @param inputFilePath One cipher file.
/// @param outputFilePath The directory to place the result file.
/// @param pk The public encryption key.
/// @param sk The secret decryption key.
bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk);

/// @param inputDirs The collection of users' data directory.
///                  @see bool encrypt(const std::string &, const std::string &, core::pk_ptr).
/// @param outputDir The directory to place the evaluation result.
/// @param pk The public encryption key.
bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              core::pk_ptr pk);

} // namespace rank
} // namespace protocol
#endif // core_PROTOCOL_PROTOCOL_H
