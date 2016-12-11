#ifndef core_PROTOCOL_PROTOCOL_H
#define core_PROTOCOL_PROTOCOL_H
#include "coda.hpp"
#include <vector>
#include <string>
#include <memory>
#include <map>
class Protocol {
public:
    Protocol(const std::string &description) : description_(description) {}

    ~Protocol() {}

    std::string which() const { return description_; }

    void setMeta(const std::string &metaPath) { metaPath_ = metaPath; }

    bool genKeypair() const;

    virtual bool encrypt(const std::string &inputFilePath,
                         const std::string &outputDirPath,
                         bool local_compute,
                         core::pk_ptr pk,
                         core::context_ptr context) = 0;

    virtual bool decrypt(const std::string &inputFilePath,
                         const std::string &outputDirPath,
                         core::pk_ptr pk,
                         core::sk_ptr sk,
                         core::context_ptr context) = 0;

    virtual bool evaluate(const std::vector<std::string> &inputDirs,
                          const std::string &outputDir,
                          const std::vector<std::string> &params,
                          core::pk_ptr pk,
                          core::context_ptr context) = 0;

protected:
    virtual core::FHEArg parameters() const = 0;

private:
    std::string description_;
    std::string metaPath_;
};

class CurrentProtocol {
public:
    static std::shared_ptr<Protocol> get() { return instance_; }
    static void set(std::shared_ptr<Protocol> in) { instance_ = in; }

private:
    CurrentProtocol() {}
    ~CurrentProtocol() {}
    CurrentProtocol(const CurrentProtocol &oth) = delete;
    CurrentProtocol(CurrentProtocol &&oth) = delete;
    CurrentProtocol& operator=(const CurrentProtocol &oth) = delete;

private:
    static std::shared_ptr<Protocol> instance_;
};

namespace protocol {
namespace chi2 {
extern const core::FHEArg _fheArgs;

/// @param inputFilePath The raw data file.
/// @param outputDirPath The directory to place the cipher file(s).
/// @param pk The public encryption key.
bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             bool local_compute,
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
bool evaluate(const std::vector <std::string> &inputDirs,
              const std::string &outputDir,
              const std::vector<std::string> &params,
              core::pk_ptr pk,
              core::context_ptr context);
} // namespace chi2

} // namespace protocol
#endif // core_PROTOCOL_PROTOCOL_H
