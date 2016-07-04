#include "core/protocol.hpp"
namespace rank {
bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             core::pk_ptr pk) {
    return false;
}

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk) {
    return false;
}

/// @param inputDirs The collection of users' data directory.
///                  @see bool encrypt(const std::string &, const std::string &, core::pk_ptr).
/// @param outputDir The directory to place the evaluation result.
/// @param pk The public encryption key.
bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              core::pk_ptr pk) {
    return false;
}

} // namespace rank