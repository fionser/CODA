#ifndef core_PROTOCOL_PROTOCOL_H
#define core_PROTOCOL_PROTOCOL_H
#include "coda.hpp"
#include <vector>
#include <string>
#include <memory>
#include <map>
class Protocol {
public:
    Protocol(const std::string &description) : description(description) {}
    ~Protocol() {}
    std::string which() const { return description; }

    virtual bool encrypt(const std::string &inputFilePath,
                         const std::string &outputDirPath,
                         core::pk_ptr pk) = 0;

    virtual bool decrypt(const std::string &inputFilePath,
                         const std::string &outputDirPath,
                         core::pk_ptr pk,
                         core::sk_ptr sk) = 0;

    virtual bool evaluate(const std::vector<std::string> &inputDirs,
                          const std::string &outputDir,
                          core::pk_ptr pk,
                          core::context_ptr context) = 0;
private:
    std::string description;
};

namespace core {
class ProtocolBuilder;

class ProtocolFactory {
public:
    typedef std::shared_ptr<ProtocolBuilder> Builder;
    std::shared_ptr<Protocol> create(const std::string &type);

    void register_builder(const std::string &type,
                                 const Builder &builder);

    ProtocolFactory *instance();

private:
    ProtocolFactory() {}
    ProtocolFactory& operator=(const ProtocolFactory &) = delete;
    ProtocolFactory(const ProtocolFactory &) = delete;
    ProtocolFactory(ProtocolFactory &&) = delete;

    static ProtocolFactory *__instance;
    static std::map<std::string, Builder> builders;
};
} // namespace core

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
