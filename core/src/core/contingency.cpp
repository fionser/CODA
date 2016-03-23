#include <spdlog/spdlog.h>
#include <core/global.hpp>
#include <core/protocol.hpp>
namespace protocol {
namespace contingency {
const core::FHEArg _fheArgs = {64, 1031, 2, 5};
bool encrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk) {
    L_INFO(global::_console, "Not implemented yet");
    return false;
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
             core::sk_ptr sk) {
    L_INFO(global::_console, "Not implemented yet");
    return false;
}
} // namespace contingency
} // namespace protocol
